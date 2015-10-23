/************************************************************************************
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
*************************************************************************************/
#ifndef __API_SPDIF_H__
#define __API_SPDIF_H__

struct spdif_channel_status {
	unsigned int consumer     : 1;
	unsigned int digital_data : 1;
	unsigned int copyright    : 1;
	unsigned int nonlinear    : 3;
	unsigned int mode         : 2;
	unsigned int category     : 7;
	unsigned int lbit         : 1;
	unsigned int source_num   : 4;
	unsigned int channel_num  : 4;
	unsigned int sample_rate  : 4;
	unsigned int accuray      : 4;
	unsigned int word_length  : 4;
	unsigned int orig_sample_rate : 4;
	unsigned int CGMSA        : 2;
	unsigned int reseverd_bits: 22;
	unsigned int reseverd[4];
};

#define SPDIF_BLOCK_SIZE  192

unsigned int spdif_init_channel_status(struct spdif_channel_status *chnsts, unsigned int sample_rate);

unsigned char spdif_get_channel_status(unsigned char *chnsts, unsigned int spdif_frames);

#endif
