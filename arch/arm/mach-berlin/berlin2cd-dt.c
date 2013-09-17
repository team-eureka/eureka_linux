/*
 *  MARVELL BERLIN2CD Flattened Device Tree enabled machine
 *
 *  Author:	Jisheng Zhang <jszhang@marvell.com>
 *  Copyright:	Marvell International Ltd.
 *		http://www.marvell.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/of_platform.h>

#include <asm/mach/arch.h>
#include <asm/hardware/gic.h>
#include <asm/setup.h>

#include "common.h"

static void __init berlin2cd_init(void)
{
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

static char const *berlin2cd_dt_compat[] __initdata = {
	"marvell,berlin2cd",
	NULL
};

void __init mv88de31xx_android_fixup(char **from);

static void __init mv88de3100_fixup(struct tag *not_used,
			char **cmdline, struct meminfo *mi)
{
	/* Invoke android specific fixup handler */
	early_print("mv88de3100_fixup\n");
	early_print("cmdline = %x\n", cmdline);
	mv88de31xx_android_fixup( cmdline);
}

DT_MACHINE_START(BERLIN2CD_DT, "MV88DE3108")
	/* Maintainer: Jisheng Zhang<jszhang@marvell.com> */
	.map_io		= berlin_map_io,
	.fixup		= mv88de3100_fixup,
	.init_early	= berlin2cd_reset,
	.init_irq	= berlin_init_irq,
	.handle_irq	= gic_handle_irq,
	.timer		= &apb_timer,
	.init_machine	= berlin2cd_init,
	.dt_compat	= berlin2cd_dt_compat,
MACHINE_END
