/*
 *  MARVELL BERLIN OPP related functions
 *
 *  Author:	Jisheng Zhang <jszhang@marvell.com>
 *  Copyright (c) 2014 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/opp.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/export.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

#define VARIANT_BERLIN2Q	0
#define VARIANT_BERLIN2CDP	1

struct leakage_volt {
	int leakage;
	unsigned long vh_volt;
	unsigned long vl_volt;
};

static struct leakage_volt berlin2q_lv[] = {
	{1729, 1150000, 1125000},
	{1513, 1175000, 1150000},
	{1321, 1200000, 1175000},
	{1129, 1225000, 1200000},
	{913, 1250000, 1225000},
	{601, 1275000, 1250000},
	{313, 1300000, 1275000},
	{0, 1325000, 1300000},
};

static struct leakage_volt berlin2cdp_lv[] = {
	{353, 1025000, 975000},
	{305, 1050000, 1000000},
	{257, 1075000, 1025000},
	{234, 1100000, 1050000},
	{193, 1125000, 1075000},
	{0, 1150000, 1100000},
};

static unsigned long berlin2q_freq[] = {600000000, 800000000, 1000000000, 1200000000};
static unsigned long berlin2cdp_freq[] = {600000000, 800000000, 1000000000, 1300000000};

static int berlin_init_opp_table(unsigned long *freq_table, unsigned long vh_volt,
				unsigned long vl_volt)
{
	int i, ret = 0;
	struct device_node *np;
	struct device *cpu_dev;

	np = of_find_node_by_path("/cpus/cpu@0");
	if (!np) {
		pr_err("failed to find cpu0 node\n");
		return -ENODEV;
	}

	cpu_dev = get_cpu_device(0);
	if (!cpu_dev) {
		pr_err("failed to get cpu0 device\n");
		ret = -ENODEV;
		goto out_put_node;
	}
	cpu_dev->of_node = np;

	for (i = 0; i < ARRAY_SIZE(berlin2q_freq); i++) {
		if (freq_table[i] > 1000000000)
			opp_add(cpu_dev, freq_table[i], vh_volt);
		else
			opp_add(cpu_dev, freq_table[i], vl_volt);
	}

	platform_device_register_simple("cpufreq-cpu0", -1, NULL, 0);

out_put_node:
	of_node_put(np);
	return ret;
}

static int get_volt(const struct of_device_id *of_id, int leakage,
		    unsigned long *vh_volt, unsigned long *vl_volt)
{
	int variant, i;

	if (!of_id)
		return -EINVAL;

	variant = (int)of_id->data;
	switch (variant) {
	case VARIANT_BERLIN2Q:
		leakage = (leakage >> 8) & 0xFF;
		leakage <<= 4;
		for (i = 0; i < ARRAY_SIZE(berlin2q_lv); i++) {
			if (berlin2q_lv[i].leakage <= leakage)
				break;
		}
		if (i < ARRAY_SIZE(berlin2q_lv)) {
			*vh_volt = berlin2q_lv[i].vh_volt;
			*vl_volt = berlin2q_lv[i].vl_volt;
		}
		break;
	case VARIANT_BERLIN2CDP:
		for (i = 0; i < ARRAY_SIZE(berlin2cdp_lv); i++) {
			if (berlin2cdp_lv[i].leakage <= leakage)
				break;
		}
		if (i < ARRAY_SIZE(berlin2cdp_lv)) {
			*vh_volt = berlin2cdp_lv[i].vh_volt;
			*vl_volt = berlin2cdp_lv[i].vl_volt;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int get_ft(const struct of_device_id *of_id, unsigned long **freq_table)
{
	int variant;

	if (!of_id)
		return -EINVAL;
	variant = (int)of_id->data;
	switch (variant) {
	case VARIANT_BERLIN2Q:
		*freq_table = berlin2q_freq;
		break;
	case VARIANT_BERLIN2CDP:
		*freq_table = berlin2cdp_freq;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct of_device_id berlin_opp_match[] = {
	{
		.compatible = "marvell,berlin2q-opp",
		.data = (void *)VARIANT_BERLIN2Q
	},
	{
		.compatible = "marvell,berlin2cdp-opp",
		.data = (void *)VARIANT_BERLIN2CDP
	},
	{},
};

static int berlin_opp_probe(struct platform_device *pdev)
{
	void __iomem *otp_base;
	int leakage;
	unsigned long *freq_table;
	unsigned long vh_volt = 1150000;
	unsigned long vl_volt = 1150000;
	const struct of_device_id *of_id;
	struct device_node *np = pdev->dev.of_node;

	of_id = of_match_device(berlin_opp_match, &pdev->dev);

	otp_base = of_iomap(np, 0);
	if (!otp_base)
		goto out;

	leakage = readl(otp_base);
	iounmap(otp_base);

	if (get_volt(of_id, leakage, &vh_volt, &vl_volt))
		return -EINVAL;
out:
	if (get_ft(of_id, &freq_table))
		return -EINVAL;
	printk("leakage:%d,vh:%d,vl:%d\n", leakage,vh_volt,vl_volt);
	return berlin_init_opp_table(freq_table, vh_volt, vl_volt);
}

static struct platform_driver berlin_opp_driver = {
	.probe		= berlin_opp_probe,
	.driver = {
		.name	= "berlin_opp",
		.owner	= THIS_MODULE,
		.of_match_table = berlin_opp_match,
	},
};

static int __init berlin_init_opp(void)
{
	return platform_driver_register(&berlin_opp_driver);
}
late_initcall(berlin_init_opp);
