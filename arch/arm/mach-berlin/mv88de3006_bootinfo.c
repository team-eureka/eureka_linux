/*
 * Process information shared by bootloader/EROM through bootinfo structure.
 *
 * Copyright (c) 2014 Google, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/string.h>

#include <asm/io.h>
#include <asm/setup.h>
#include <asm/system_info.h>

/* Location used by EROM to copy bootinfo */
#define BOOTINFO_ADDRESS 0x01010000
#define BOOTINFO_SIZE 0x10000

/* EROM copies non-secure fields from OTP to a well defined memory
 * location during boot process. Following structure defines the
 * contents of this memory. Structure defined in berlin_bootinfo.h
 * in bootloader.
 */
struct cdp_bootinfo {
	u32 otp_rkek_id[2]; /* rkek id from OTP */
	u32 otp_version;    /* otp version */
	u32 otp_market_id;  /* otp market id */
	u32 otp_ult[2];     /* otp ULT info */
	u32 chip_version;   /* bg2cdp chip version*/
	u32 speed_tag;      /* 0 - slow, 1 - full */
	u32 leakage_current;/* leakage current info */
	u32 temp_id;        /* Temp ID in deg C */
	u32 erom_id;        /* erom id*/
	u32 feature_bits;   /* Feature flags */
};

static __init struct cdp_bootinfo *get_bootinfo(void)
{
	void* bootinfo_v = NULL;
	bootinfo_v = ioremap(BOOTINFO_ADDRESS, BOOTINFO_SIZE);
	if (!bootinfo_v)
		early_print(KERN_ERR "Failed to map BOOTINFO\n");
	return (struct cdp_bootinfo *)bootinfo_v;
}

static __init void unmap_bootinfo(struct cdp_bootinfo *bootinfo_v)
{
	iounmap(bootinfo_v);
}

/* Get SOC revision information */
static __init unsigned char get_soc_rev(struct cdp_bootinfo *pinfo)
{
	return pinfo->chip_version & 0xFF;
}

static __init u64 get_rkek_id(struct cdp_bootinfo *pinfo)
{
	u64 rkek_id = pinfo->otp_rkek_id[1];
	rkek_id = (rkek_id << 32) | pinfo->otp_rkek_id[0];
	return rkek_id;
}

static __init u32 get_otp_ver(struct cdp_bootinfo *pinfo)
{
	return pinfo->otp_version;
}

static __init u32 get_otp_mktid(struct cdp_bootinfo *pinfo)
{
	return pinfo->otp_market_id;
}

static __init u32 get_speed_tag(struct cdp_bootinfo *pinfo)
{
	return pinfo->speed_tag;
}

static __init u32 get_otp_feature_bits(struct cdp_bootinfo *pinfo)
{
	return pinfo->feature_bits;
}

static __init u64 get_otp_ult(struct cdp_bootinfo *pinfo)
{
	u64 otp_ult = pinfo->otp_ult[1];
	otp_ult = (otp_ult << 32) | pinfo->otp_ult[0];
	return otp_ult;
}

void __init mv88de3006_proc_bootinfo(int board_rev)
{
	u64 rkek_id, ult;
	u32 chip_rev, ult_l, ult_h;
	struct cdp_bootinfo *pinfo = get_bootinfo();

	if (!pinfo)
		return;

	rkek_id = get_rkek_id(pinfo);
	chip_rev = get_soc_rev(pinfo);
	ult = get_otp_ult(pinfo);
	ult_l = ult & 0xFFFFFFFF;
	ult_h = ult >> 32;
	/* Since bootloader is not capable of generating DTB and device
	 * tree does not support setting of system_rev and serial, parse
	 * bootinfo to get and set this.
	 */
	system_rev = (board_rev << 8) | chip_rev;
	system_serial_low = __builtin_bswap32(rkek_id & 0xFFFFFFFF);
	system_serial_high = __builtin_bswap32(rkek_id >> 32);

	early_print(KERN_INFO "speed_tag: %d otp_ver: %d mkt_id: %d "
			" otp_ult: 0x%08X%08X feature_bits:0x%08X\n",
			get_speed_tag(pinfo),
			get_otp_ver(pinfo), get_otp_mktid(pinfo),
			ult_h, ult_l, get_otp_feature_bits(pinfo));
	unmap_bootinfo(pinfo);
	return;
}
