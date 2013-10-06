/*
 *  MARVELL BERLIN common setup
 *
 *  Author:	Jisheng Zhang <jszhang@marvell.com>
 *  Copyright:	Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>

#include <asm/mach/map.h>
#include <mach/system.h>

#include "common.h"
#include "clock.h"

static struct map_desc berlin_io_desc[] __initdata = {
	{
		.virtual = 0xF7000000,
		.pfn = __phys_to_pfn(0xF7000000),
		.length = 0x00CC0000,
		.type = MT_DEVICE
	},
	{
		.virtual = 0xF7CD0000,
		.pfn = __phys_to_pfn(0xF7CD0000),
		.length = 0x00330000,
		.type = MT_DEVICE
	},
};

void __init berlin_map_io(void)
{
	iotable_init(berlin_io_desc, ARRAY_SIZE(berlin_io_desc));
	berlin_clk_init();
}
static void arch_reset(char mode, const char *cmd)
{
	galois_arch_reset(mode, cmd);
}

static void __init berlin_wdt_restart(void)
{
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CD_A0)
	arm_pm_restart = arch_reset;
#endif
}

void __init berlin2cd_reset(void)
{
	berlin_wdt_restart();
}
