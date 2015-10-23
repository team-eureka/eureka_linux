/*******************************************************************************
* Copyright (C) Marvell International Ltd. and its affiliates
*
* Marvell GPL License Option
*
* If you received this File from Marvell, you may opt to use, redistribute and/or
* modify this File in accordance with the terms and conditions of the General
* Public License Version 2, June 1991 (the "GPL License"), a copy of which is
* available along with the File in the license.txt file or by writing to the Free
* Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
* on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
*
* THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
* WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
* DISCLAIMED.  The GPL License provides additional details about this warranty
* disclaimer.
********************************************************************************/



#ifndef _FASTLOGO_H_
#define _FASTLOGO_H_

#include "Galois_memmap.h"

#include "thinvpp_module.h"
#include "thinvpp_api.h"
#include "thinvpp_apifuncs.h"
#include "thinvpp_isr.h"

#include "vpp.h"
#include "maddr.h"

#include <mach/api_dhub.h>

typedef struct
{
	//HDL_dhub2d AG_dhubHandle;
	//HDL_dhub2d VPP_dhubHandle;
	//HDL_dhub2d VIP_dhubHandle;

    	unsigned VPP_NUM_OF_CHANNELS;
    	unsigned AG_NUM_OF_CHANNELS;

	//DHUB_channel_config  *VIP_config;
	DHUB_channel_config  *VPP_config;
	DHUB_channel_config  *AG_config;

        unsigned avioDhubChMap_vpp_BCM_R;
        unsigned RA_Vpp_VP_TG;
	unsigned RA_Vpp_vpIn_pix;
	unsigned RA_Vpp_vpOut_pix;
	unsigned RA_Vpp_diW_pix;
	unsigned RA_Vpp_diR_word;
} logo_soc_specific;

typedef struct
{
	unsigned planes;
	int vres;
	VPP_WIN win;
	unsigned *logoBuf;
	unsigned *logoBuf_2;
	unsigned *mapaddr;
	long length;
	unsigned count;

	const unsigned *bcm_cmd_0;
	unsigned bcm_cmd_0_len;
	const unsigned *bcm_cmd_a;
	unsigned bcm_cmd_a_len;
	const unsigned *bcm_cmd_n;
	unsigned bcm_cmd_n_len;
	const unsigned *bcm_cmd_z;
	unsigned bcm_cmd_z_len;

	unsigned *logo_frame_dma_cmd;
	unsigned logo_dma_cmd_len;

	unsigned bcmQ_len;
	unsigned dmaQ_len;
	unsigned cfgQ_len;

#if LOGO_USE_SHM
	size_t mSHMOffset;
	unsigned mSHMSize;
	char * bcmQ;
	unsigned bcmQ_phys;
	char * dmaQ;
	unsigned dmaQ_phys;
	char * cfgQ;
	unsigned cfgQ_phys;
#endif

	unsigned berlin_chip_revision;
} logo_device_t;


extern logo_device_t fastlogo_ctx;
extern logo_soc_specific *soc;

void fastlogo_soc_select(unsigned chip_revision);

#endif
