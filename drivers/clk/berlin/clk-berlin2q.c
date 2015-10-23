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

static const struct grpclk_desc berlin2q_grpclk_desc[] __initconst = {
	{ "cfgclk", 9, 12 },
	{ "nfceccclk", 27, 50 },
	{},
};

void __init berlin2q_grpclk_setup(struct device_node *np)
{
	berlin_grpclk_setup(np, berlin2q_grpclk_desc);
}
