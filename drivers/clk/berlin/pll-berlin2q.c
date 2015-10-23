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
#define AVPLLCH			32
#define AVPLLCH_SIZE		16
#define AVPLLCH_CTRL0		0
#define AVPLLCH_CTRL1		4
#define AVPLLCH_CTRL2		8
#define AVPLLCH_CTRL3		12

static const u8 vcodiv_berlin2q[] = {1, 0, 2, 0, 3, 4, 0, 6, 8};

static unsigned long berlin2q_pll_recalc_rate(struct clk_hw *hw,
					unsigned long parent_rate)
{
	u32 val, fbdiv, rfdiv, vcodivsel;
	struct berlin_pll *pll = to_berlin_pll(hw);

	val = readl_relaxed(pll->ctrl + PLL_CTRL0);
	fbdiv = (val >> 7) & 0x1FF;
	rfdiv = (val >> 2) & 0x1F;
	val = readl_relaxed(pll->ctrl + PLL_CTRL1);
	vcodivsel = (val >> 9) & 0xF;
	return parent_rate * fbdiv / rfdiv / vcodiv_berlin2q[vcodivsel];
}

static const struct clk_ops berlin2q_pll_ops = {
	.recalc_rate	= berlin2q_pll_recalc_rate,
};

void __init berlin2q_pll_setup(struct device_node *np)
{
	berlin_pll_setup(np, &berlin2q_pll_ops);
}

static u8 hdmidiv[] = { 1, 2, 4, 6};

static unsigned long berlin2q_avpllch_recalc_rate(struct clk_hw *hw,
						unsigned long parent_rate)
{
	u32 val;
	void __iomem *addr;
	unsigned long pll;
	int endpll, sync1, sync2, divhdmi, divav1, divav2, divav3;
	struct berlin_avpllch *avpllch = to_berlin_avpllch(hw);

	val = readl_relaxed(avpllch->base + AVPLL_CTRL3);
	val &= 0xff;
	endpll = val & (1 << avpllch->which);

	addr = avpllch->base + AVPLLCH + avpllch->which * AVPLLCH_SIZE;
	sync1 = readl_relaxed(addr + AVPLLCH_CTRL1) & 0x1FFFF;
	sync2 = readl_relaxed(addr + AVPLLCH_CTRL2) & 0x1FFFF;
	val = readl_relaxed(addr + AVPLLCH_CTRL3);
	divhdmi = val & 0x7;
	divav1 = (val >> 3) & 0x7;
	divav2 = (val >> 6) & 0x3F;
	divav3 = (val >> 13) & 0xF;

	parent_rate /= 1000000;

	if (endpll)
		pll = parent_rate * sync2 / sync1;
	else
		pll = parent_rate;

	if (divhdmi) {
		divhdmi &= 0x3;
		pll /= hdmidiv[divhdmi];
	}

	if (divav2) {
		pll /= divav2;
		if (divav3)
			pll *= 2;
	}

	return pll * 1000000;
}

static const struct clk_ops berlin2q_avpllch_ops = {
	.recalc_rate	= berlin2q_avpllch_recalc_rate,
};

static struct berlin_avplldata avpll_data = {
	.offset = 144,
	.offset_mask = 0x7FFFF,
	.offset_shift = 0,
	.fbdiv = 4,
	.fbdiv_mask = 0xFF,
	.fbdiv_shift = 6,
	.ops = &berlin2q_avpllch_ops,
};

void __init berlin2q_avpll_setup(struct device_node *np)
{
	berlin_avpll_setup(np, &avpll_data);
}
