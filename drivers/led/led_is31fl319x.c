/*
 * Copyright 2017 SolidRun ltd.
 *
 * Author: Rabeeh Khoury <rabeeh@solid-run.com>
 * Based on Linux kernel driver work from Nokolaus Schaller <hns@goldelico.com>
 *
 * Adapted to newer u-boot and simplified for internal use by Filipe Coelho <falktx@darkglass.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 * This file is subject to the terms and conditions of version 2 of
 * the GNU General Public License.  See the file COPYING in the main
 * directory of this archive for more details.
 *
 * LED driver for the IS31FL319{0,1,3,6,9} to drive 1, 3, 6 or 9 light
 * effect LEDs.
 *
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <led.h>
#include <i2c.h>
#include <asm/gpio.h>
#include <dm/lists.h>

DECLARE_GLOBAL_DATA_PTR;

/* register numbers */
#define IS31FL319X_SHUTDOWN		0x00
#define IS31FL319X_CTRL1		0x01
#define IS31FL319X_CTRL2		0x02
#define IS31FL319X_CONFIG1		0x03
#define IS31FL319X_CONFIG2		0x04
#define IS31FL319X_RAMP_MODE		0x05
#define IS31FL319X_BREATH_MASK		0x06
#define IS31FL319X_PWM(channel)		(0x07 + channel)
#define IS31FL319X_DATA_UPDATE		0x10
#define IS31FL319X_T0(channel)		(0x11 + channel)
#define IS31FL319X_T123_1		0x1a
#define IS31FL319X_T123_2		0x1b
#define IS31FL319X_T123_3		0x1c
#define IS31FL319X_T4(channel)		(0x1d + channel)
#define IS31FL319X_TIME_UPDATE		0x26
#define IS31FL319X_RESET		0xff

#define IS31FL319X_REG_CNT		(IS31FL319X_RESET + 1)

#define IS31FL319X_MAX_LEDS		9

/* CS (Current Setting) in CONFIG2 register */
#define IS31FL319X_CONFIG2_CS_SHIFT	4
#define IS31FL319X_CONFIG2_CS_MASK	0x7
#define IS31FL319X_CONFIG2_CS_STEP_REF	12

#define IS31FL319X_CURRENT_MIN		((u32)5000)
#define IS31FL319X_CURRENT_MAX		((u32)40000)
#define IS31FL319X_CURRENT_STEP		((u32)5000)
#define IS31FL319X_CURRENT_DEFAULT	((u32)20000)

/* Audio gain in CONFIG2 register */
#define IS31FL319X_AUDIO_GAIN_DB_MAX	((u32)21)
#define IS31FL319X_AUDIO_GAIN_DB_STEP	((u32)3)

/*
 * regmap is used as a cache of chip's register space,
 * to avoid reading back brightness values from chip,
 * which is known to hang.
 */
struct led_is31fl319x_priv {
	int num_leds;
	u8 regmap[256]; /* Caching of 256 registers */
	bool configured;

	struct is31fl319x_led {
		struct udevice *dev;
		bool            configured;
		bool            on;
	} leds[IS31FL319X_MAX_LEDS];
};

/* is31fl319x family are write only devices. reads must be cached */
int dm_i2c_reg_read_cached(struct udevice *dev, uint offset)
{
	struct led_is31fl319x_priv *priv = dev_get_priv(dev);
	return priv->regmap[offset];
}

int dm_i2c_reg_write_cached(struct udevice *dev, uint offset, uint value)
{
	struct led_is31fl319x_priv *priv = dev_get_priv(dev);
	priv->regmap[offset] = value;
	return dm_i2c_reg_write(dev, offset, value);
}

static enum led_state_t is31fl319x_led_get_state(struct udevice *dev)
{
	struct led_uc_plat *uc_plat = dev_get_uclass_platdata(dev);
	struct udevice *parent;
	struct led_is31fl319x_priv *priv;
	int chan;

	if (!uc_plat->label) {
		return 0; /* Parent device. */
	}

	parent = dev_get_parent(dev);
	priv = dev_get_priv(parent);

	for (chan = 0 ; chan < priv->num_leds; chan++) {
		if (dev == priv->leds[chan].dev)
			break; /* Found a match */
	}
	if (chan == priv->num_leds) {
		return -EINVAL;
	}

	return priv->leds[chan].on ? LEDST_ON : LEDST_OFF;
}

/* Set the brightness of an output */
static int is31fl319x_led_set_state(struct udevice *dev, enum led_state_t state)
{
	struct led_uc_plat *uc_plat = dev_get_uclass_platdata(dev);
	struct udevice *parent;
	struct led_is31fl319x_priv *priv;
	int chan, i, ret;

	if (!uc_plat->label) {
		return 0; /* Parent device. */
	}

	parent = dev_get_parent(dev);
	priv = dev_get_priv(parent);

	for (chan = 0 ; chan < priv->num_leds; chan++) {
		if (dev == priv->leds[chan].dev)
			break; /* Found a match */
	}
	if (chan == priv->num_leds) {
		return -EINVAL;
	}

	u8 ctrl1 = 0, ctrl2 = 0;

	switch (state) {
	case LEDST_OFF:
		priv->leds[chan].on = false;
		break;
	case LEDST_ON:
		priv->leds[chan].on = true;
		break;
	case LEDST_TOGGLE:
		priv->leds[chan].on = !priv->leds[chan].on;
		break;
	default:
		return -ENOSYS;
	}

	/* update PWM register */
	dm_i2c_reg_write_cached(parent, IS31FL319X_PWM(chan), priv->leds[chan].on ? 20 : 0);

	/* read current brightness of all PWM channels */
	for (i = 0; i < priv->num_leds; i++) {
		unsigned int pwm_value;
		bool on;

		/*
		 * since neither cdev nor the chip can provide
		 * the current setting, we read from the regmap cache
		 */

		pwm_value = dm_i2c_reg_read_cached(parent, IS31FL319X_PWM(i));
		on = (pwm_value > 0) ? 1 : 0;

		if (i < 3)
			ctrl1 |= on << i;       /* 0..2 => bit 0..2 */
		else if (i < 6)
			ctrl1 |= on << (i + 1); /* 3..5 => bit 4..6 */
		else
			ctrl2 |= on << (i - 6); /* 6..8 => bit 0..2 */
	}

	if (ctrl1 > 0 || ctrl2 > 0) {
		dm_i2c_reg_write_cached(parent, IS31FL319X_CTRL1, ctrl1);
		dm_i2c_reg_write_cached(parent, IS31FL319X_CTRL2, ctrl2);
		/* update PWMs */
		dm_i2c_reg_write_cached(parent, IS31FL319X_DATA_UPDATE, 0x00);
		/* enable chip from shut down */
		ret = dm_i2c_reg_write_cached(parent, IS31FL319X_SHUTDOWN, 0x01);
	} else {
		/* shut down (no need to clear CTRL1/2) */
		ret = dm_i2c_reg_write_cached(parent, IS31FL319X_SHUTDOWN, 0x00);
	}

	return ret;
}

static int led_is31fl319x_probe(struct udevice *dev)
{
	struct udevice *parent;
	struct led_uc_plat *uc_plat = dev_get_uclass_platdata(dev);
	struct led_is31fl319x_priv *priv;
	int val;
	fdt_addr_t addr;
	ulong driver_data;

	if (!uc_plat->label) { /* Parent device is the actual controller */
		parent = dev;
		priv = dev_get_priv(parent);
		addr = dev_read_addr(dev);

		if (addr == FDT_ADDR_T_NONE)
			return -EINVAL;

		driver_data = dev_get_driver_data(parent);

		if ((driver_data <= 0) || (driver_data > IS31FL319X_MAX_LEDS))
			return -EINVAL;

		priv->num_leds = driver_data;
		priv->configured = true;

		/* Reset */
		dm_i2c_reg_write_cached(parent, IS31FL319X_RESET, 0x00);

		/* Initial setup, low brightness */
		val = dm_i2c_reg_write_cached(parent, IS31FL319X_CONFIG2, 48);

		return val;
	} else
	{ /* Child is the LED */
		parent = dev_get_parent(dev);
		priv = dev_get_priv(parent);
		val = dev_read_addr(dev);
		val -= 1; /* reg starts from 1, where val from 0 */
		if ((val < 0) || (val >= priv->num_leds)) {
			printf("Error in FDT (LED = %d)\n", val);
			return -EINVAL;
		}
		priv->leds[val].configured = true;
		priv->leds[val].dev = dev;

		return 0;
	}
}

static int led_is31fl319x_remove(struct udevice *dev)
{
	return 0;
}

static int led_is31fl319x_bind(struct udevice *parent)
{
	struct udevice *dev;
	ofnode node;
	int ret;

	dev_for_each_subnode(node, parent) {
		struct led_uc_plat *uc_plat;
		const char *label;

		label = ofnode_read_string(node, "label");
		if (!label) {
			printf("%s: node %s has no label\n", __func__,
			      ofnode_get_name(node));
			return -EINVAL;
		}
		/* Notice recursive call */
		ret = device_bind_driver_to_node(parent,
			"is31fl319x_led_driver",
			ofnode_get_name(node),
			node, &dev);
		if (ret) {
			return ret;
		}

		uc_plat = dev_get_uclass_platdata(dev);
		uc_plat->label = label;
	}

	return 0;
}

static const struct led_ops is31fl319x_led_ops = {
	.get_state = is31fl319x_led_get_state,
	.set_state = is31fl319x_led_set_state,
};

static const struct udevice_id led_is31fl319x_ids[] = {
	{ .compatible = "issi,is31fl3190", .data = 0, },
	{ .compatible = "issi,is31fl3191", .data = 1, },
	{ .compatible = "issi,is31fl3193", .data = 3, },
	{ .compatible = "issi,is31fl3196", .data = 6, },
	{ .compatible = "issi,is31fl3199", .data = 9, },
	{ .compatible = "si-en,sn3199",    .data = 9, },
	{ }
};

U_BOOT_DRIVER(led_is31fl319x) = {
	.name	= "is31fl319x_led_driver",
	.id	= UCLASS_LED,
	.of_match = led_is31fl319x_ids,
	.ops	= &is31fl319x_led_ops,
	.priv_auto_alloc_size = sizeof(struct led_is31fl319x_priv),
	.bind	= led_is31fl319x_bind,
	.probe	= led_is31fl319x_probe,
	.remove	= led_is31fl319x_remove,
};
