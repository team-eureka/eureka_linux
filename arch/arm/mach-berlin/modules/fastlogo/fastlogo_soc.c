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



#include "fastlogo.h"
#include "maddr.h"

extern logo_device_t fastlogo_ctx;
extern logo_soc_specific *soc;
static logo_soc_specific soc_a0 =
{
    	VPP_NUM_OF_CHANNELS_A0,
    	AG_NUM_OF_CHANNELS_A0,

        VPP_config_a0,
        AG_config_a0,

	avioDhubChMap_vpp_BCM_R_A0,
        RA_Vpp_VP_TG_A0,
	RA_Vpp_vpIn_pix_A0,
	RA_Vpp_vpOut_pix_A0,
	RA_Vpp_diW_pix_A0,
	RA_Vpp_diR_word_A0,
};

static logo_soc_specific soc_z1 =
{
    	VPP_NUM_OF_CHANNELS_Z1,
    	AG_NUM_OF_CHANNELS_Z1,

        VPP_config,
        AG_config,

	avioDhubChMap_vpp_BCM_R_Z1,
        RA_Vpp_VP_TG_Z1,
	RA_Vpp_vpIn_pix_Z1,
	RA_Vpp_vpOut_pix_Z1,
	RA_Vpp_diW_pix_Z1,
	RA_Vpp_diR_word_Z1,
};


void fastlogo_soc_select(unsigned chip_revision)
{
    if (fastlogo_ctx.berlin_chip_revision >= 0xA0) /* CDP A0 */
    	soc = &soc_a0;
    else
    	soc = &soc_z1;
}

