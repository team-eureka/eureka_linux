/*
 * Copyright (c) 2013 Marvell Technology Group Ltd.
 *
 * Author: Jisheng Zhang <jszhang@marvell.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/clk-provider.h>

#include "clk.h"

#define CLKEN		(1 << 0)
#define CLKPLLSEL_MASK	7
#define CLKPLLSEL_SHIFT	1
#define CLKPLLSWITCH	(1 << 4)
#define CLKSWITCH	(1 << 5)
#define CLKD3SWITCH	(1 << 6)
#define CLKSEL_MASK	7
#define CLKSEL_SHIFT	7

struct berlin_clk {
	struct clk_hw hw;
	void __iomem *base;
};

#define to_berlin_clk(hw)	container_of(hw, struct berlin_clk, hw)

static u8 clk_div[] = {1, 2, 4, 6, 8, 12, 1, 1};

static unsigned long berlin_clk_recalc_rate(struct clk_hw *hw,
					unsigned long parent_rate)
{
	u32 val, divider;
	struct berlin_clk *clk = to_berlin_clk(hw);

	val = readl_relaxed(clk->base);
	if (val & CLKD3SWITCH)
		divider = 3;
	else {
		if (val & CLKSWITCH) {
			val >>= CLKSEL_SHIFT;
			val &= CLKSEL_MASK;
			divider = clk_div[val];
		} else
			divider = 1;
	}

	return parent_rate / divider;
}

static u8 berlin_clk_get_parent(struct clk_hw *hw)
{
	u32 val;
	struct berlin_clk *clk = to_berlin_clk(hw);

	val = readl_relaxed(clk->base);
	if (val & CLKPLLSWITCH) {
		val >>= CLKPLLSEL_SHIFT;
		val &= CLKPLLSEL_MASK;
		return val;
	}

	return 0;
}

static const struct clk_ops berlin_clk_ops = {
	.recalc_rate	= berlin_clk_recalc_rate,
	.get_parent	= berlin_clk_get_parent,
};

static void __init berlin_clk_setup(struct device_node *np)
{
	struct clk_init_data init;
	struct berlin_clk *bclk;
	struct clk *clk;
	const char *parent_names[5];
	int i, ret;

	bclk = kzalloc(sizeof(*bclk), GFP_KERNEL);
	if (WARN_ON(!bclk))
		return;

	bclk->base = of_iomap(np, 0);
	if (WARN_ON(!bclk->base))
		return;

	init.name = np->name;
	init.ops = &berlin_clk_ops;
	for (i = 0; i < ARRAY_SIZE(parent_names); i++) {
		parent_names[i] = of_clk_get_parent_name(np, i);
		if (!parent_names[i])
			break;
	}
	init.parent_names = parent_names;
	init.num_parents = i;

	bclk->hw.init = &init;

	clk = clk_register(NULL, &bclk->hw);
	if (WARN_ON(IS_ERR(clk)))
		return;

	ret = of_clk_add_provider(np, of_clk_src_simple_get, clk);
	if (WARN_ON(ret))
		return;
}

struct berlin_grpctl {
	struct clk **clks;
	void __iomem *switch_base;
	void __iomem *select_base;
	struct clk_onecell_data	onecell_data;
};

struct berlin_grpclk {
	struct clk_hw hw;
	struct berlin_grpctl *ctl;
	int switch_shift;
	int select_shift;
};

#define to_berlin_grpclk(hw)	container_of(hw, struct berlin_grpclk, hw)

static u8 berlin_grpclk_get_parent(struct clk_hw *hw)
{
	u32 val;
	struct berlin_grpclk *clk = to_berlin_grpclk(hw);
	struct berlin_grpctl *ctl = clk->ctl;
	void __iomem *base = ctl->switch_base;

	base += clk->switch_shift / 32 * 4;
	val = readl_relaxed(base);
	if (val & (1 << clk->switch_shift)) {
		base = ctl->select_base;
		base += clk->select_shift / 32 * 4;
		val = readl_relaxed(base);
		val >>= clk->select_shift;
		val &= CLKPLLSEL_MASK;
		return val;
	}

	return 0;
}

static unsigned long berlin_grpclk_recalc_rate(struct clk_hw *hw,
					unsigned long parent_rate)
{
	u32 val, divider;
	struct berlin_grpclk *clk = to_berlin_grpclk(hw);
	struct berlin_grpctl *ctl = clk->ctl;
	void __iomem *base = ctl->switch_base;
	int shift;

	shift = clk->switch_shift + 2;
	base += shift / 32 * 4;
	shift = shift - shift / 32 * 32;
	val = readl_relaxed(base);
	if (val & (1 << shift))
		divider = 3;
	else {
		if (unlikely(base != ctl->switch_base))
			val = readl_relaxed(ctl->switch_base);

		shift = clk->switch_shift + 1;
		shift = shift - shift / 32 * 32;
		if (val & (1 << shift)) {
			base = ctl->select_base;
			shift = clk->select_shift + 3;
			base += shift / 32 * 4;
			shift = shift - shift / 32 * 32;
			val = readl_relaxed(base);
			val >>= shift;
			val &= CLKSEL_MASK;
			divider = clk_div[val];
		} else
			divider = 1;
	}

	return parent_rate / divider;
}

static const struct clk_ops berlin_grpclk_ops = {
	.recalc_rate	= berlin_grpclk_recalc_rate,
	.get_parent	= berlin_grpclk_get_parent,
};

static struct clk * __init berlin_grpclk_register(const char *name,
		const char **parent_names, int num_parents,
		int switch_shift, int select_shift,
		struct berlin_grpctl *ctl)
{
	struct berlin_grpclk *grpclk;
	struct clk *clk;
	struct clk_init_data init;

	/* allocate the gate */
	grpclk = kzalloc(sizeof(*grpclk), GFP_KERNEL);
	if (!grpclk) {
		pr_err("%s: could not allocate berlin grpclk\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	init.name = name;
	init.ops = &berlin_grpclk_ops;
	init.parent_names = parent_names;
	init.num_parents = num_parents;

	grpclk->ctl = ctl;
	grpclk->switch_shift = switch_shift;
	grpclk->select_shift = select_shift;
	grpclk->hw.init = &init;

	clk = clk_register(NULL, &grpclk->hw);

	if (IS_ERR(clk))
		kfree(grpclk);

	return clk;
}

void __init berlin_grpclk_setup(struct device_node *np,
				const struct grpclk_desc *desc)
{
	struct berlin_grpctl *ctl;
	const char *parent_names[5];
	int i, n, ret;

	ctl = kzalloc(sizeof(*ctl), GFP_KERNEL);
	if (WARN_ON(!ctl))
		return;

	ctl->switch_base = of_iomap(np, 0);
	if (WARN_ON(!ctl->switch_base))
		return;

	ctl->select_base = of_iomap(np, 1);
	if (WARN_ON(!ctl->select_base))
		return;

	/* Count, allocate, and register clock gates */
	for (n = 0; desc[n].name;)
		n++;

	ctl->clks = kzalloc(n * sizeof(struct clk *), GFP_KERNEL);
	if (WARN_ON(!ctl->clks))
		return;

	for (i = 0; i < ARRAY_SIZE(parent_names); i++)
		parent_names[i] = of_clk_get_parent_name(np, i);

	for (i = 0; i < n; i++) {
		ctl->clks[i] = berlin_grpclk_register(desc[i].name,
				parent_names, ARRAY_SIZE(parent_names),
				desc[i].switch_shift, desc[i].select_shift,
				ctl);
		WARN_ON(IS_ERR(ctl->clks[i]));
	}

	ctl->onecell_data.clks = ctl->clks;
	ctl->onecell_data.clk_num = i;

	ret = of_clk_add_provider(np, of_clk_src_onecell_get,
				  &ctl->onecell_data);
	if (WARN_ON(ret))
		return;
}

static const __initconst struct of_device_id clk_match[] = {
	{ .compatible = "fixed-clock", .data = of_fixed_clk_setup, },
	{ .compatible = "fixed-factor-clock", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "marvell,berlin2q-pll", .data = berlin2q_pll_setup, },
	{ .compatible = "marvell,berlin2q-avpll", .data = berlin2q_avpll_setup, },
	{ .compatible = "marvell,berlin2cdp-pll", .data = berlin2cdp_pll_setup, },
	{ .compatible = "marvell,berlin2cdp-avpll", .data = berlin2cdp_avpll_setup, },
	{ .compatible = "marvell,berlin-clk", .data = berlin_clk_setup, },
	{ .compatible = "marvell,berlin2q-grpclk", .data = berlin2q_grpclk_setup, },
	{}
};

void __init berlin_clk_init(void)
{
	of_clk_init(clk_match);
}
