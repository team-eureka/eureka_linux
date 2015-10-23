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
#include <linux/kernel.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>

#include "clk.h"

#define PLL_CTRL0	0x0
#define PLL_CTRL1	0x4
#define PLL_CTRL2	0x8
#define PLL_CTRL3	0xC
#define PLL_CTRL4	0x10
#define PLL_STATUS	0x14

#define AVPLL_CTRL0		0
#define AVPLL_CTRL1		4
#define AVPLL_CTRL2		8
#define AVPLL_CTRL3		12
#define AVPLLCH			28
#define AVPLLCH_SIZE		16
#define AVPLLCH_CTRL0		0
#define AVPLLCH_CTRL1		4
#define AVPLLCH_CTRL2		8
#define AVPLLCH_CTRL3		12

static u8 vcodiv_berlin2cdp[] = {1, 2, 4, 8, 16, 32, 64, 128};

static unsigned long berlin2cdp_pll_recalc_rate(struct clk_hw *hw,
					unsigned long parent_rate)
{
	u32 val, fbdiv, rfdiv, vcodivsel;
	struct berlin_pll *pll = to_berlin_pll(hw);

	val = readl_relaxed(pll->ctrl + PLL_CTRL0);
	fbdiv = (val >> 12) & 0x1FF;
	rfdiv = (val >> 3) & 0x1FF;
	val = readl_relaxed(pll->ctrl + PLL_CTRL1);
	vcodivsel = (val >> 9) & 0x7;
	return parent_rate * fbdiv * 4 / rfdiv /
		vcodiv_berlin2cdp[vcodivsel];
}

static long berlin2cdp_pll_round_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long *parent_rate)
{
	int vcodivsel, pllintpi, kvco;
	u32 vco, rfdiv, fbdiv, ctrl0;
	struct berlin_pll *pll = to_berlin_pll(hw);
	long ret;

	rate /= 1000000;

	if (rate <= 3000 && rate >= 1200)
		vcodivsel = 0;
	else if (rate < 1200 && rate >= 600)
		vcodivsel = 1;
	else if (rate < 600 && rate >= 300)
		vcodivsel = 2;
	else if (rate < 300 && rate >= 150)
		vcodivsel = 3;
	else if (rate < 150 && rate >= 75)
		vcodivsel = 4;
	else if (rate < 75 && rate >= 37)
		vcodivsel = 5;
	else
		return berlin2cdp_pll_recalc_rate(hw, *parent_rate);

	vco = rate * vcodiv_berlin2cdp[vcodivsel];
	if (vco >= 1200 && vco <= 1350)
		kvco = 8;
	else if (vco <= 1500)
		kvco = 9;
	else if (vco <= 1750)
		kvco = 0xA;
	else if (vco <= 2000)
		 kvco = 0xB;
	else if (vco <= 2200)
		kvco = 0xC;
	else if (vco <= 2400)
		kvco = 0xD;
	else if (vco <= 2600)
		kvco = 0xE;
	else if (vco <= 3000)
		kvco = 0xF;
	else
		return berlin2cdp_pll_recalc_rate(hw, *parent_rate);

	if (vco>= 1500 && vco <= 2000)
		pllintpi = 5;
	else if (vco <= 2500)
		pllintpi = 6;
	else if (vco <= 3000)
		pllintpi = 8;
	else
		return berlin2cdp_pll_recalc_rate(hw, *parent_rate);

	ctrl0 = readl_relaxed(pll->ctrl);
	rfdiv = (ctrl0 >> 3) & 0x1FF;
	fbdiv = (vco * rfdiv) / (4 * *parent_rate / 1000000);
	fbdiv &= 0x1FF;
	ret = *parent_rate * fbdiv * 4 / rfdiv /
		vcodiv_berlin2cdp[vcodivsel];
	return ret;
}

static int berlin2cdp_pll_set_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long parent_rate)
{
	int vcodivsel, pllintpi, kvco;
	u32 vco, rfdiv, fbdiv, ctrl0, ctrl1, bypass;
	struct berlin_pll *pll = to_berlin_pll(hw);

	rate /= 1000000;

	if (rate <= 3000 && rate >= 1200)
		vcodivsel = 0;
	else if (rate < 1200 && rate >= 600)
		vcodivsel = 1;
	else if (rate < 600 && rate >= 300)
		vcodivsel = 2;
	else if (rate < 300 && rate >= 150)
		vcodivsel = 3;
	else if (rate < 150 && rate >= 75)
		vcodivsel = 4;
	else if (rate < 75 && rate >= 37)
		vcodivsel = 5;
	else
		return -EPERM;

	vco = rate * vcodiv_berlin2cdp[vcodivsel];
	if (vco >= 1200 && vco <= 1350)
		kvco = 8;
	else if (vco <= 1500)
		kvco = 9;
	else if (vco <= 1750)
		kvco = 0xA;
	else if (vco <= 2000)
		 kvco = 0xB;
	else if (vco <= 2200)
		kvco = 0xC;
	else if (vco <= 2400)
		kvco = 0xD;
	else if (vco <= 2600)
		kvco = 0xE;
	else if (vco <= 3000)
		kvco = 0xF;
	else
		return -EPERM;

	if (vco>= 1500 && vco <= 2000)
		pllintpi = 5;
	else if (vco <= 2500)
		pllintpi = 6;
	else if (vco <= 3000)
		pllintpi = 8;
	else
		return -EPERM;

	ctrl0 = readl_relaxed(pll->ctrl + PLL_CTRL0);
	ctrl1 = readl_relaxed(pll->ctrl + PLL_CTRL1);
	rfdiv = (ctrl0 >> 3) & 0x1FF;
	fbdiv = (vco * rfdiv) / (4 * parent_rate / 1000000);
	fbdiv &= 0x1FF;
	ctrl0 &= ~(0x1FF << 12);
	ctrl0 |= (fbdiv << 12);
	ctrl1 &= ~(0xf << 0);
	ctrl1 |= (kvco << 0);
	ctrl1 &= ~(0x7 << 6);
	ctrl1 |= (vcodivsel << 6);
	ctrl1 &= ~(0x7 << 9);
	ctrl1 |= (vcodivsel << 9);
	ctrl1 &= ~(0xf << 26);
	ctrl1 |= (pllintpi << 26);

	/* Pll bypass enable */
	bypass = readl_relaxed(pll->bypass);
	bypass |= (1 << pll->bypass_shift);
	writel_relaxed(bypass, pll->bypass);
	ctrl1 |= (1 << 14);
	writel_relaxed(ctrl1, pll->ctrl + PLL_CTRL1);

	/* reset on */
	ctrl0 |= (1 << 1);
	writel_relaxed(ctrl0, pll->ctrl + PLL_CTRL0);

	/* make sure RESET is high for at least 2us */
	udelay(2);

	/* clear reset */
	ctrl0 &= ~(1 << 1);
	writel_relaxed(ctrl0, pll->ctrl + PLL_CTRL0);

	/* wait 50us */
	udelay(50);

	/* make sure pll locked */
	while (!(readl_relaxed(pll->ctrl + PLL_STATUS) & (1 << 0)))
		cpu_relax();

	/* pll bypass disable */
	ctrl1 &= ~(1 << 14);
	writel_relaxed(ctrl1, pll->ctrl + PLL_CTRL1);
	bypass &= ~(1 << pll->bypass_shift);
	writel_relaxed(bypass, pll->bypass);

	return 0;
}

static const struct clk_ops berlin2cdp_pll_ops = {
	.recalc_rate	= berlin2cdp_pll_recalc_rate,
	.round_rate	= berlin2cdp_pll_round_rate,
	.set_rate	= berlin2cdp_pll_set_rate,
};

void __init berlin2cdp_pll_setup(struct device_node *np)
{
	berlin_pll_setup(np, &berlin2cdp_pll_ops);
}

static unsigned long berlin2cdp_avpllch_recalc_rate(struct clk_hw *hw,
						unsigned long parent_rate)
{
	u32 val;
	void __iomem *addr;
	unsigned long pll;
	int endpll, sync1, sync2, post_div, post_0p5_div;
	struct berlin_avpllch *avpllch = to_berlin_avpllch(hw);


	addr = avpllch->base + AVPLLCH + avpllch->which * AVPLLCH_SIZE;
	val = readl_relaxed(addr + AVPLLCH_CTRL0);
	endpll = val & (1 << 14);
	post_div = val & 0x1FFF;
	post_0p5_div = (val >> 13) & 1;
	sync1 = readl_relaxed(addr + AVPLLCH_CTRL2) & 0xFFFFF;
	sync2 = readl_relaxed(addr + AVPLLCH_CTRL3) & 0xFFFFF;

	parent_rate /= 1000000;

	if (endpll)
		pll = parent_rate * sync2 / sync1;
	else
		pll = parent_rate;

	return 1000000 * 2 * pll / (2 * post_div + post_0p5_div);
}

static const struct clk_ops berlin2cdp_avpllch_ops = {
	.recalc_rate	= berlin2cdp_avpllch_recalc_rate,
};

static struct berlin_avplldata avpll_data = {
	.offset = 140,
	.offset_mask = 0x7FFFF,
	.offset_shift = 0,
	.fbdiv = 4,
	.fbdiv_mask = 0x1FF,
	.fbdiv_shift = 17,
	.ops = &berlin2cdp_avpllch_ops,
};

void __init berlin2cdp_avpll_setup(struct device_node *np)
{
	berlin_avpll_setup(np, &avpll_data);
}
