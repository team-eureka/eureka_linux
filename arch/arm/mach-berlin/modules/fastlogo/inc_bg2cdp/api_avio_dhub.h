/*******************************************************************************
* Copyright (C) Marvell International Ltd. and its affiliates
*
* Marvell GPL License Option
*
* If you received this File from Marvell, you may opt to use, redistribute and/or
* modify this File in accordance with the terms and conditions of the General
* Public License Version 2, June 1991 (the GPL License), a copy of which is
* available along with the File in the license.txt file or by writing to the Free
* Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
* on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
*
* THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
* WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
* DISCLAIMED.  The GPL License provides additional details about this warranty
* disclaimer.
********************************************************************************/


#ifndef __API_AVIO_DHUB_H__
#define __API_AVIO_DHUB_H__
#include "dHub.h"
#include "avioDhub.h"
#include "api_dhub.h"


#define VPP_DHUB_BASE			(MEMMAP_VPP_DHUB_REG_BASE + RA_vppDhub_dHub0)
#define VPP_HBO_SRAM_BASE		(MEMMAP_VPP_DHUB_REG_BASE + RA_vppDhub_tcm0)

//dhub channels are changed in CDp A0
#define VPP_NUM_OF_CHANNELS_Z1		(avioDhubChMap_vpp_HDMI_R_Z1+1)
#define VPP_NUM_OF_CHANNELS_A0		(avioDhubChMap_vpp_HDMI_R_A0+1)

#define AG_DHUB_BASE			(MEMMAP_AG_DHUB_REG_BASE + RA_agDhub_dHub0)
#define AG_HBO_SRAM_BASE		(MEMMAP_AG_DHUB_REG_BASE + RA_agDhub_tcm0)
#define AG_NUM_OF_CHANNELS_Z1		(avioDhubChMap_ag_GFX_R_Z1+1)
#define AG_NUM_OF_CHANNELS_A0		(avioDhubChMap_ag_GFX_R_A0+1)

// add by yuxia for compilation
#define VPP_DHUB_BANK0_START_ADDR       (8192*0)
#define VPP_DHUB_BANK1_START_ADDR       (8192*1)
#define VPP_DHUB_BANK2_START_ADDR       (8192*2)
#define VPP_DHUB_BANK3_START_ADDR       (8192*3)
#define VPP_DHUB_BANK4_START_ADDR       (8192*4)


#define DHUB_BANK0_START_ADDR		(0)
#define DHUB_BANK1_START_ADDR		(8192*1)

#define AG_DHUB_BANK0_START_ADDR		(0)
#define AG_DHUB_BANK1_START_ADDR		(4096*1)
#define AG_DHUB_BANK2_START_ADDR		(4096*3)

//add by yuxia for compilation
#define DHUB_BANK5_START_ADDR       (8192*4)

typedef struct DHUB_channel_config {
	SIGN32 chanId;
	UNSG32 chanCmdBase;
	UNSG32 chanDataBase;
	SIGN32 chanCmdSize;
	SIGN32 chanDataSize;
	SIGN32 chanMtuSize;
	SIGN32 chanQos;
	SIGN32 chanSelfLoop;
	SIGN32 chanEnable;
} DHUB_channel_config;

extern HDL_dhub2d AG_dhubHandle;
extern HDL_dhub2d VPP_dhubHandle;
#if (BERLIN_CHIP_VERSION >= BERLIN_BG2)
extern HDL_dhub2d VIP_dhubHandle;
#endif
extern DHUB_channel_config  AG_config_a0[];
extern DHUB_channel_config  VPP_config_a0[];

extern DHUB_channel_config  AG_config[];
extern DHUB_channel_config  VPP_config[];
#if (BERLIN_CHIP_VERSION >= BERLIN_BG2)
extern DHUB_channel_config  VIP_config[];
#endif

/******************************************************************************************************************
 *      Function: GetChannelInfo
 *      Description: Get the Dhub configuration of requested channel.
 *      Parameter : pdhubHandle ----- pointer to 2D dhubHandle
 *             IChannel		----- Channel of the dhub
 *             cfg		----- Configuration need to be updated here.
 *      Return:  0 	----	Success

 ******************************************************************************************************************
 *	Function: DhubInitialization
 *	Description: Initialize DHUB .
 *	Parameter : cpuId ------------- cpu ID
 *             dHubBaseAddr -------------  dHub Base address.
 *             hboSramAddr ----- Sram Address for HBO.
 *             pdhubHandle ----- pointer to 2D dhubHandle
 *             dhub_config ----- configuration of AG
 *             numOfChans	 ----- number of channels
 *	Return:		void
******************************************************************************************************************/
void DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr, UNSG32 hboSramAddr, HDL_dhub2d *pdhubHandle, DHUB_channel_config *dhub_config,SIGN32 numOfChans);

void DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[]);
#endif //__API_AVIO_DHUB_H__
