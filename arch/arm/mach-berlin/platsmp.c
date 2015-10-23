/*
 *  linux/arch/arm/mach-berlin/platsmp.c
 *
 *  Author:	Jisheng Zhang <jszhang@marvell.com>
 *  Copyright (c) 2013 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * Cloned from linux/arch/arm/plat-vexpress/platsmp.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <asm/hardware/gic.h>
#include <asm/smp_scu.h>
#include "common.h"

static void __iomem *jump_addr;

extern void berlin_secondary_startup(void);

/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
static void __cpuinit write_pen_release(int val)
{
	pen_release = val;
	smp_wmb();
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));
}

static DEFINE_SPINLOCK(boot_lock);

static void __cpuinit berlin_secondary_init(unsigned int cpu)
{
	/*
	 * if any interrupts are already enabled for the primary
	 * core (e.g. timer irq), then they will not have been enabled
	 * for us: do so
	 */
	gic_secondary_init(0);

	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	write_pen_release(-1);

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

static int __cpuinit berlin_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * This is really belt and braces; we hold unintended secondary
	 * CPUs in the holding pen until we're ready for them.  However,
	 * since we haven't sent them a soft interrupt, they shouldn't
	 * be there.
	 */
	write_pen_release(cpu_logical_map(cpu));
	/*
	 * Send the secondary CPU a soft interrupt, thereby causing
	 * the boot monitor to read the system wide flags register,
	 * and branch to the address found there.
	 */
	gic_raise_softirq(cpumask_of(cpu), 0);

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (pen_release == -1)
			break;
		udelay(10);
	}
	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);
	return pen_release != -1 ? -ENOSYS : 0;
}


/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
static void __init berlin_smp_init_cpus(void)
{
	set_smp_cross_call(gic_raise_softirq);
}

static inline void mv_scu_enable(void)
{
	u32 scu_ctrl;
       void __iomem *scu_base;

       if (scu_a9_has_base()) {
               scu_base = IOMEM(scu_a9_get_base());
               /* Enabled SCU speculative linefill and IC, SCU standby */
               scu_ctrl = __raw_readl(scu_base);
               if (!(scu_ctrl & 1)) {
                       scu_ctrl |= (1 << 3) | (3 << 5);
                       __raw_writel(scu_ctrl, scu_base);
               }
               scu_enable(scu_base);
}
}

static void __init berlin_smp_prepare_cpus(unsigned int max_cpus)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "marvell,berlin-sw_generic1");
	if (!np) {
		pr_err("No sw_generic1 node, SMP may not work!\n");
		return;
	}

	jump_addr = of_iomap(np, 0);
	if (!jump_addr) {
		pr_err("Can't map sw_generic1, SMP may not work!\n");
		return;
	}

	of_node_put(np);

	/*
	 * Initialise the present map, which describes the set of CPUs
	 * actually populated at the present time.
	 */
	mv_scu_enable();

	/*
	 * Write the address of secondary startup into the
	 * system-wide flags register. The boot monitor waits
	 * until it receives a soft interrupt, and then the
	 * secondary CPU branches to this address.
	 */
	writel(virt_to_phys(berlin_secondary_startup), jump_addr);
}

struct smp_operations __initdata berlin_smp_ops = {
	.smp_init_cpus		= berlin_smp_init_cpus,
	.smp_prepare_cpus	= berlin_smp_prepare_cpus,
	.smp_secondary_init	= berlin_secondary_init,
	.smp_boot_secondary	= berlin_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= berlin_cpu_die,
#endif
};
