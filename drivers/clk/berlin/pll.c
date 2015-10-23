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

void __init berlin_pll_setup(struct device_node *np,
			const struct clk_ops *ops)
{
	struct clk_init_data init;
	struct berlin_pll *pll;
	const char *parent_name;
	struct clk *clk;
	int ret;

	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (WARN_ON(!pll))
		return;

	pll->ctrl = of_iomap(np, 0);
	pll->bypass = of_iomap(np, 1);
	ret = of_property_read_u8(np, "bypass-shift", &pll->bypass_shift);
	if (WARN_ON(!pll->ctrl || !pll->bypass || ret))
		return;

	init.name = np->name;
	init.ops = ops;
	parent_name = of_clk_get_parent_name(np, 0);
	init.parent_names = &parent_name;
	init.num_parents = 1;

	pll->hw.init = &init;

	clk = clk_register(NULL, &pll->hw);
	if (WARN_ON(IS_ERR(clk)))
		return;

	ret = of_clk_add_provider(np, of_clk_src_simple_get, clk);
	if (WARN_ON(ret))
		return;
}

struct berlin_avpll {
	struct clk_hw		hw;
	struct clk_onecell_data	onecell_data;
	struct clk		*chs[7];
	void __iomem		*base;
	void			*data;
};

#define to_berlin_avpll(hw)	container_of(hw, struct berlin_avpll, hw)

static unsigned long berlin_avpll_recalc_rate(struct clk_hw *hw,
					unsigned long parent_rate)
{
	int offset, fbdiv;
	long rate;
	u32 val;
	struct berlin_avpll *avpll = to_berlin_avpll(hw);
	struct berlin_avplldata *data = avpll->data;

	rate = parent_rate / 1000000;
	val = readl_relaxed(avpll->base + data->offset);
	offset = (val >> data->offset_shift) & data->offset_mask;
	val = readl_relaxed(avpll->base + data->fbdiv);
	fbdiv = (val >> data->fbdiv_shift) & data->fbdiv_mask;
	if (offset & (1 << 18))
		offset = -(offset & ((1 << 18) - 1));
	rate = rate * fbdiv + rate * fbdiv * offset / 4194304;

	return rate * 1000000;
}

static const struct clk_ops berlin_avpll_ops = {
	.recalc_rate	= berlin_avpll_recalc_rate,
};

static struct clk * __init berlin_avpllch_setup(struct device_node *np,
						u8 which, void __iomem *base,
						const struct clk_ops *ops)
{
	struct berlin_avpllch *avpllch;
	struct clk_init_data init;
	struct clk *clk;
	int err;

	err = of_property_read_string_index(np, "clock-output-names",
					    which, &init.name);
	if (WARN_ON(err))
		goto err_read_output_name;

	avpllch = kzalloc(sizeof(*avpllch), GFP_KERNEL);
	if (!avpllch)
		goto err_read_output_name;

	avpllch->base = base;
	avpllch->which = which;

	init.ops = ops;
	init.parent_names = &np->name;
	init.num_parents = 1;

	avpllch->hw.init = &init;

	clk = clk_register(NULL, &avpllch->hw);
	if (WARN_ON(IS_ERR(clk)))
		goto err_clk_register;

	return clk;

err_clk_register:
	kfree(avpllch);
err_read_output_name:
	return ERR_PTR(-EINVAL);
}

void __init berlin_avpll_setup(struct device_node *np,
			struct berlin_avplldata *data)
{
	struct clk_init_data init;
	struct berlin_avpll *avpll;
	const char *parent_name;
	struct clk *clk;
	int i, ret;

	avpll = kzalloc(sizeof(*avpll), GFP_KERNEL);
	if (WARN_ON(!avpll))
		return;

	avpll->base = of_iomap(np, 0);
	if (WARN_ON(!avpll->base))
		return;

	avpll->data = data;
	init.name = np->name;
	init.ops = &berlin_avpll_ops;
	parent_name = of_clk_get_parent_name(np, 0);
	init.parent_names = &parent_name;
	init.num_parents = 1;

	avpll->hw.init = &init;

	clk = clk_register(NULL, &avpll->hw);
	if (WARN_ON(IS_ERR(clk)))
		return;

	ret = of_clk_add_provider(np, of_clk_src_simple_get, clk);
	if (WARN_ON(ret))
		return;

	for (i = 0; i < 7; i++) {
		avpll->chs[i] = berlin_avpllch_setup(np, i, avpll->base,
						data->ops);
		if (WARN_ON(IS_ERR(avpll->chs[i])))
			return;
	}

	avpll->onecell_data.clks = avpll->chs;
	avpll->onecell_data.clk_num = i;

	ret = of_clk_add_provider(np, of_clk_src_onecell_get,
				  &avpll->onecell_data);
	if (WARN_ON(ret))
		return;
}
