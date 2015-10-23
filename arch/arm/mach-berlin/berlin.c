/*
 *  MARVELL BERLIN Flattened Device Tree enabled machine
 *
 *  Author:	Jisheng Zhang <jszhang@marvell.com>
 *  Copyright (c) 2013 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/of_platform.h>

#include <asm/mach/map.h>
#include <asm/mach/arch.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/hardware/gic.h>

#include "common.h"

extern int board_rev;

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

void __init mv88de3006_proc_bootinfo(int board_rev);
void __init mv88de31xx_android_fixup(char **from);
static void __init berlin_fixup(struct tag *not_used,
               char **cmdline, struct meminfo *mi)
{
       /* Invoke android specific fixup handler */
       mv88de31xx_android_fixup( cmdline);
}

static void __init berlin_map_io(void)
{
	iotable_init(berlin_io_desc, ARRAY_SIZE(berlin_io_desc));
}

static void __init berlin_init_early(void)
{
	l2x0_of_init(0x70c00000, 0xfeffffff);
}

static void __init berlin_init(void)
{
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

static void __init berlin_init_late(void)
{
#ifdef CONFIG_BERLIN2CD
	void __iomem *phy_base = ioremap(0xf7cc07d4, SZ_4);
	if (!phy_base) {
		printk(KERN_WARNING, "can't map 0xf7cc07d4\n");
		return;
	}
	int chip_rev = __raw_readl(IOMEM(phy_base)) >> 29;
	iounmap(phy_base);
	system_rev = (board_rev << 8) | chip_rev;
#elif CONFIG_BERLIN2CDP
	mv88de3006_proc_bootinfo(board_rev);
#endif /* CONFIG_BERLIN2CDP */
}

static char const *berlin_dt_compat[] __initdata = {
	"marvell,berlin",
	NULL
};

DT_MACHINE_START(BERLIN_DT, "Marvell Berlin SoC (Flattened Device Tree)")
	/* Maintainer: Jisheng Zhang<jszhang@marvell.com> */
	.map_io		= berlin_map_io,
	.smp		= smp_ops(berlin_smp_ops),
        .fixup          = berlin_fixup,
	.init_early	= berlin_init_early,
	.init_irq	= berlin_init_irq,
	.handle_irq	= gic_handle_irq,
	.timer		= &apb_timer,
	.init_machine	= berlin_init,
	.init_late	= berlin_init_late,
	.dt_compat	= berlin_dt_compat,
MACHINE_END
