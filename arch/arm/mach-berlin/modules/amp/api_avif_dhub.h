/********************************************************************************
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
 ******************************************************************************/

#ifndef __API_AVIF_DHUB_H__
#define __API_AVIF_DHUB_H__
#include "dHub.h"
#include "api_dhub.h"
#include "avif_dhub_config.h"
#include "api_avio_dhub.h"
//#include "avif_port.h"

#define MEMMAP_AVIF_DHUB_REG_BASE (HIF_CPU_DHUB_BASE_OFF)

#define AVIF_DHUB_BASE          (MEMMAP_AVIF_DHUB_REG_BASE + RA_avif_dhub_config_dHub0)
#define AVIF_HBO_SRAM_BASE      (MEMMAP_AVIF_DHUB_REG_BASE + RA_avif_dhub_config_tcm0)
#define AVIF_NUM_OF_CHANNELS    (avif_dhub_config_ChMap_avif_AUD_RD0+1)


#define AVIF_DHUB_BANK0_START_ADDR        (0)
#define AVIF_DHUB_BANK1_START_ADDR        (1024*8)
#define AVIF_DHUB_BANK2_START_ADDR        (1024*20)
#define AVIF_DHUB_BANK3_START_ADDR        (1024*32)

#if 0
#define DHUB_BANK0_START_ADDR       (8192*0)
#define DHUB_BANK1_START_ADDR       (8192*1)
#define DHUB_BANK2_START_ADDR       (8192*2)
#define DHUB_BANK3_START_ADDR       (8192*3)
#define DHUB_BANK4_START_ADDR       (8192*4)
#define DHUB_BANK5_START_ADDR       (8192*5)
#endif


#if 0//def AVIF_BOOTCODE
#define HIF_CPU_MADDR_BASE_OFF  0x10000000
#define HIF_CPU_ITCM_BASE_OFF   0x00000000
#define HIF_CPU_DHUB_BASE_OFF   0x30000000
#else
#define HIF_CPU_MADDR_BASE_OFF  0xF7980000
#define HIF_CPU_ITCM_BASE_OFF   0xF7990000
#define HIF_CPU_DHUB_BASE_OFF   0xF79A0000
#endif
#if 0
typedef struct _DHUB_channel_config {
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
#endif
extern HDL_dhub2d AVIF_dhubHandle;
extern DHUB_channel_config  AVIF_config[];

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
//void DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr, UNSG32 hboSramAddr, HDL_dhub2d *pdhubHandle, DHUB_channel_config *dhub_config,SIGN32 numOfChans);

void DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[]);

#endif //__API_AVIF_DHUB_H__
