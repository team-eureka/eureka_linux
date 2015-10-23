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
*
********************************************************************************/
#ifndef __API_AVIO_DHUB_H__
#define __API_AVIO_DHUB_H__

#include "dHub.h"
#include "avioDhub.h"
#include "api_dhub.h"

#define AG_DHUB_BASE			(MEMMAP_AG_DHUB_REG_BASE + RA_agDhub_dHub0)
#define AG_HBO_SRAM_BASE		(MEMMAP_AG_DHUB_REG_BASE + RA_agDhub_tcm0)
#define AG_NUM_OF_CHANNELS_A0		(avioDhubChMap_ag_GFX_R_A0+1)
//#define AG_NUM_OF_CHANNELS_A0           (1)


#define AG_DHUB_BANK0_START_ADDR	(0)
#define AG_DHUB_BANK1_START_ADDR	(4096*1)
#define AG_DHUB_BANK2_START_ADDR	(4096*3)

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

extern DHUB_channel_config  AG_config[];

/******************************************************************************************************************
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
void DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr, UNSG32 hboSramAddr,
			HDL_dhub2d *pdhubHandle, DHUB_channel_config *dhub_config,
			SIGN32 numOfChans);

/******************************************************************************************************************
 *      Function: DhubEnableIntr
 *      Description: Initialize interrupt for DHUB channel.
 *      Parameter :
 *             cpuId       ----- cpu ID
 *             pdhubHandle ----- pointer to 2D dhubHandle
 *             chanId      ----- channel ID
 *             enable      ----- 1=enable;0=disable
 *      Return:         void
******************************************************************************************************************/
void DhubEnableIntr(SIGN32 cpuId, HDL_dhub2d *pdhubHandle,
			SIGN32 chanId, SIGN32 enable);
#endif //__API_AVIO_DHUB_H__
