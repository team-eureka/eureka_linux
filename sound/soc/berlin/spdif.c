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
**************************************************************************************/
#include <linux/module.h>
#include "api_spdif.h"

enum {
	SPDIF_FREQ_11K = 0x1,
	SPDIF_FREQ_12K = 0x1,
	SPDIF_FREQ_22K = 0x4,
	SPDIF_FREQ_24K = 0x6,
	SPDIF_FREQ_32K = 0x3,
	SPDIF_FREQ_44K = 0x0,
	SPDIF_FREQ_48K = 0x2,
	SPDIF_FREQ_88K = 0x8,
	SPDIF_FREQ_96K = 0xA,
	SPDIF_FREQ_176K = 0xC,
	SPDIF_FREQ_192K = 0xE,
};

unsigned int spdif_init_channel_status(struct spdif_channel_status *chnsts, unsigned int sample_rate)
{
	if(chnsts == NULL)
		return -1;

	memset(chnsts, 0, sizeof(struct spdif_channel_status));
	chnsts->consumer = 0;
	chnsts->digital_data = 0;
	chnsts->copyright = 0;
	chnsts->nonlinear = 0;
	chnsts->mode = 0;
	chnsts->category = 0b0001001;
	chnsts->lbit = 0;
	chnsts->source_num = 0;
	chnsts->channel_num = 0;

	switch(sample_rate) {
	case 32000 :
		chnsts->sample_rate = SPDIF_FREQ_32K;
		break;
	case 44100 :
		chnsts->sample_rate = SPDIF_FREQ_44K;
		break;
	case 48000 :
		chnsts->sample_rate = SPDIF_FREQ_48K;
		break;
	case 88200 :
		chnsts->sample_rate = SPDIF_FREQ_88K;
		break;
	case 96000 :
		chnsts->sample_rate = SPDIF_FREQ_96K;
		break;
	default :
		break;
	}

	chnsts->word_length = (chnsts->digital_data == 1 ? 0x2 : 0xb);
	chnsts->CGMSA = 0b11;

	return 0;
}

unsigned char spdif_get_channel_status(unsigned char *chnsts, unsigned int spdif_frames)
{
	unsigned int bytes, bits;

	bytes = spdif_frames >> 3;
	bits = spdif_frames - (bytes << 3);
	return ((chnsts[bytes] >> bits) & 0x01);
}
