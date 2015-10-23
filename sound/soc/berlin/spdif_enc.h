/*************************************************************************************
*       Copyright (C) 2007-2011
*       Copyright ? 2007 Marvell International Ltd.
*
*       This program is free software; you can redistribute it and/or
*       modify it under the terms of the GNU General Public License
*       as published by the Free Software Foundation; either version 2
*       of the License, or (at your option) any later version.
*
*       This program is distributed in the hope that it will be useful,
*       but WITHOUT ANY WARRANTY; without even the implied warranty of
*       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*       GNU General Public License for more details.
*
*       You should have received a copy of the GNU General Public License
*       along with this program; if not, write to the Free Software
*       Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
***************************************************************************************/

#ifndef __SPDIF_ENC_H__
#define	__SPDIF_ENC_H__

#ifndef __KERNEL__
#include <stdint.h>
#endif

#include "bmc_enc_tbl.h"

enum subframe_type
{
	TYPE_B = 0,
	TYPE_M = 1,
	TYPE_W = 2,
};

static uint8_t __sync_preamble[3] =
{
	0xE8,	/* TYPE_B: 11101000 */
	0xE2,	/* TYPE_M: 11100010 */
	0xE4,	/* TYPE_W: 11100100 */
};

static inline void spdif_enc_subframe(uint32_t *subframe,
			uint32_t data, uint32_t sync_type, uint8_t v, uint8_t u, uint8_t c)
{
	uint32_t val, pattern, parity;
	uint32_t last_phase;

	parity = 0;
	last_phase = 0;

	/* Only data[31:8] are effective */
	data >>= 8;

	/* Fill preamble*/
	val  = __sync_preamble[sync_type % 3] << 16;

	/* Generate pattern for data[8:15] */
	pattern = __bmc_enc_tbl_8bit[data & 0xFF]; data >>= 8;

	/* Fill data */
	val |= pattern; parity += pattern;
	last_phase = pattern & 1;

	/* Generate pattern for data[16:23] */
	pattern = __bmc_enc_tbl_8bit[data & 0xFF]; data >>= 8;
	parity += pattern;

	/* Handle bi-phase */
	pattern = last_phase ? (pattern ^ 0xFFFF) : pattern;
	last_phase = pattern & 1;

	/* Fill data - data[16:19] in the first 32-bit */
	subframe[0] = (val << 8) | (pattern >> 8);

	/* Fill data - data[20:23] in the second 32-bit */
	val = pattern & 0xFF;

	/* Generate pattern for data[24:31] */
	pattern = __bmc_enc_tbl_8bit[data & 0xFF];
	parity += pattern;

	/* Handle bi-phase */
	pattern = last_phase ? (pattern ^ 0xFFFF) : pattern;
	last_phase = pattern & 1;

	/* Fill data */
	val = (val << 16) | pattern;

	/* Calculate VUCP */
	v &= 1; u &= 1; c &= 1;
	parity += v + u + c;
	parity &= 1;

	/* Generate pattern */
	pattern = __bmc_enc_tbl_8bit[(parity<<3)+(c<<2)+(u<<1)+v] >> 8;

	/* Handle bi-phase */
	pattern = last_phase ? (pattern ^ 0xFF) : pattern;

	/* Fill data */
	subframe[1] = (val<<8) | pattern;
}

#endif /* __SPDIF_ENC_H__ */
