/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * (C) Copyright 2021 Rockchip Electronics Co., Ltd
 *
 */

#ifndef __CONFIG_RK3588_COMMON_H
#define __CONFIG_RK3588_COMMON_H

#include "rockchip-common.h"

#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_TEXT_BASE		0x00000000
#define CONFIG_SPL_MAX_SIZE		0x00040000
#define CONFIG_SPL_BSS_START_ADDR	0x03fe0000
#define CONFIG_SPL_BSS_MAX_SIZE		0x00010000
#define CONFIG_SPL_STACK		0x03fe0000
#ifdef CONFIG_SPL_LOAD_FIT_ADDRESS
#undef CONFIG_SPL_LOAD_FIT_ADDRESS
#endif
#define CONFIG_SPL_LOAD_FIT_ADDRESS	0x10000000

#define CONFIG_SYS_MALLOC_LEN		(32 << 20)
#define CONFIG_SYS_CBSIZE		1024
#define CONFIG_SKIP_LOWLEVEL_INIT

#ifdef CONFIG_SUPPORT_USBPLUG
#define CONFIG_SYS_TEXT_BASE		0x00000000
#else
#define CONFIG_SYS_TEXT_BASE		0x00200000
#endif

#define CONFIG_SYS_INIT_SP_ADDR		0x00600000
#define CONFIG_SYS_LOAD_ADDR		0x00600800
#define CONFIG_SYS_BOOTM_LEN		(64 << 20)	/* 64M */
#define COUNTER_FREQUENCY		24000000

#define GICD_BASE			0xfe600000
#define GICR_BASE			0xfe680000
#define GICC_BASE			0xfe600000

/* secure otp */
#define OTP_UBOOT_ROLLBACK_OFFSET	0x150
#define OTP_UBOOT_ROLLBACK_WORDS	2	/* 64 bits, 2 words */
#define OTP_ALL_ONES_NUM_BITS		32
#define OTP_SECURE_BOOT_ENABLE_ADDR	0x20
#define OTP_SECURE_BOOT_ENABLE_SIZE	1
#define OTP_RSA_HASH_ADDR		0x9c0
#define OTP_RSA_HASH_SIZE		32

/* MMC/SD IP block */
#define CONFIG_BOUNCE_BUFFER

#define CONFIG_SYS_SDRAM_BASE		0
#define SDRAM_MAX_SIZE			0xf0000000
#define CONFIG_SYS_NONCACHED_MEMORY	(1 << 20)	/* 1 MiB */

#ifndef CONFIG_SPL_BUILD
#ifdef CONFIG_CMD_ROCKUSB
/* usb mass storage */
#define CONFIG_USB_FUNCTION_MASS_STORAGE
#define CONFIG_ROCKUSB_G_DNL_PID	0x350b
#define ROCKUSB_FSG_BUFLEN		0x400000
#endif

#define CONFIG_MISC_INIT_R

/*
 * decompressed kernel:  4M ~ 84M
 *	Why not start from 2M ? if kernel < 5.10 in Android image,
 *	the image header will use the 0x180000~0x200000, which is
 *	overlap with share memory region 0x100000~0x200000.
 *
 * compressed kernel:   84M ~ 130M
 */
#define ENV_MEM_LAYOUT_SETTINGS \
	"scriptaddr=0x00500000\0" \
	"pxefile_addr_r=0x00600000\0" \
	"fdtoverlay_addr_r=0x08200000\0" \
	"fdt_addr_r=0x08300000\0" \
	"kernel_addr_r=0x00400000\0" \
	"kernel_addr_c=0x05480000\0" \
	"ramdisk_addr_r=0x0a200000\0"

#include <config_distro_bootcmd.h>

#define CONFIG_EXTRA_ENV_SETTINGS \
	BOOTENV_SHARED_MTD	\
	ENV_MEM_LAYOUT_SETTINGS \
	"fdtfile=" FDTFILE \
	"partitions=" PARTS_RKIMG \
	ROCKCHIP_DEVICE_SETTINGS \
	RKIMG_DET_BOOTDEV \
	BOOTENV
#endif

/* rockchip ohci host driver */
#define CONFIG_USB_OHCI_NEW
#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS	1

#define CONFIG_PREBOOT
#define CONFIG_LIB_HW_RAND

#undef CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS \
	"devtype=mmc\0" \
	"devnum=0\0" \
	"scriptaddr=0x00500000\0" \
	"fdt_addr_r=0x08300000\0" \
	"kernel_addr_r=0x00400000\0" \
	"bootdelay=0\0" \
	"bootcmd=if gpio input 35 || test -e mmc 0:7 /boot-restore; then run boot_restore; run restore_fail; else run boot_main; run boot_restore; run boot_usb; run usb_fail; fi\0" \
	"console=ttyS2,115200n8\0" \
	"loglevel=0\0" \
	"bootenv=/uEnv-pablito.txt\0" \
	"kernel=/Image-pablito\0" \
	"fdtbin=/pablito-rk3588.dtb\0" \
	"fdtprefix=\0" \
	"boot_image=booti ${kernel_addr_r} - ${fdt_addr_r}\0" \
	"setbootargs=setenv bootargs console=${console} init=/sbin/init root=${root} loglevel=${loglevel} pwm_bl.off ${extraargs}\0" \
	"main_bootargs=setenv root \"/dev/mmcblk0p5\"\0" \
	"main_loadbootenv=ext4load mmc 0:5 ${scriptaddr} /boot${bootenv} && env import ${scriptaddr} ${filesize}\0" \
	"main_script=ext4load mmc 0:5 ${fdt_addr_r} /boot${fdtprefix}${fdtbin}\0" \
	"main_kernel=ext4load mmc 0:5 ${kernel_addr_r} /boot${kernel}\0" \
	"boot_main=run main_bootargs; run main_loadbootenv && run main_script && run main_kernel && run setbootargs boot_image\0" \
	"restore_bootargs=setenv root \"\"\0" \
	"restore_check=setenv restore_ok 0\0" \
	"restore_loadbootenv=fatload mmc 0:3 ${scriptaddr} ${bootenv} && env import ${scriptaddr} ${filesize}\0" \
	"restore_script=fatload mmc 0:3 ${fdt_addr_r} ${fdtprefix}${fdtbin}\0" \
	"restore_kernel=fatload mmc 0:3 ${kernel_addr_r} ${kernel}\0" \
	"restore_loadbootenv2=fatload mmc 0:4 ${scriptaddr} ${bootenv} && env import ${scriptaddr} ${filesize}\0" \
	"restore_script2=fatload mmc 0:4 ${fdt_addr_r} ${fdtprefix}${fdtbin}\0" \
	"restore_kernel2=fatload mmc 0:4 ${kernel_addr_r} ${kernel}\0" \
	"restore_led_on=led 1:blue on\0" \
	"restore_led_off=led 1:blue off\0" \
	"restore_fail=while true; do led 1:blue on; led 2:blue on; led 3:blue on; led status:blue on; sleep 0.15; led 1:blue off; led 2:blue off; led 3:blue off; led status:blue off; sleep 0.15; led 1:blue on; led 2:blue on; led 3:blue on; led status:blue on; sleep 0.15; led 1:blue off; led 2:blue off; led 3:blue off; led status:blue off; sleep 0.15; led 1:blue on; led 2:blue on; led 3:blue on; led status:blue on; sleep 0.15; led 1:blue off; led 2:blue off; led 3:blue off; led status:blue off; sleep 0.5; done\0" \
	"boot_restore=run restore_led_on restore_bootargs; if run restore_loadbootenv && run restore_script && run restore_kernel; then setenv restore_ok 1; elif run restore_loadbootenv2 && run restore_script2 && run restore_kernel2; then setenv restore_ok 1; else setenv restore_ok 0; fi; if test \"${restore_ok}\" -eq 1; then run setbootargs boot_image; fi; run restore_led_off\0" \
	"usb_script=fatload usb 0 ${fdt_addr_r} /ANAGRAM${fdtprefix}${fdtbin} || fatload usb 1 ${fdt_addr_r} /ANAGRAM${fdtprefix}${fdtbin}\0" \
	"usb_kernel=fatload usb 0 ${kernel_addr_r} /ANAGRAM${kernel} || fatload usb 1 ${kernel_addr_r} /ANAGRAM${kernel}\0" \
	"usb_led_on=led 1:red on; led 1:blue on\0" \
	"usb_led_off=led 1:red off; led 1:blue off\0" \
	"usb_fail=while true; do led 1:red on; led 2:red on; led 3:red on; led 1:blue on; led 2:blue on; led 3:blue on; led status:blue on; sleep 0.15; led 1:red off; led 2:red off; led 3:red off; led 1:blue off; led 2:blue off; led 3:blue off; led status:blue off; sleep 0.15; led 1:red on; led 2:red on; led 3:red on; led 1:blue on; led 2:blue on; led 3:blue on; led status:blue on; sleep 0.15; led 1:red off; led 2:red off; led 3:red off; led 1:blue off; led 2:blue off; led 3:blue off; led status:blue off; sleep 0.15; led 1:red on; led 2:red on; led 3:red on; led 1:blue on; led 2:blue on; led 3:blue on; led status:blue on; sleep 0.15; led 1:red off; led 2:red off; led 3:red off; led 1:blue off; led 2:blue off; led 3:blue off; led status:blue off; sleep 0.5; done\0" \
	"boot_usb=run usb_led_on; usb start; run restore_bootargs; run usb_script && run usb_kernel && run setbootargs boot_image; run usb_led_off\0" \
	"loadbootenv=echo\0"

#endif
