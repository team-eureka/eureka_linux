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
#include "api_avio_dhub.h"

HDL_dhub2d AG_dhubHandle;

DHUB_channel_config  AG_config[AG_NUM_OF_CHANNELS_A0] = {
#if 1
        // Bank0
        { avioDhubChMap_ag_MA0_R_A0, AG_DHUB_BANK0_START_ADDR, AG_DHUB_BANK0_START_ADDR+32, 32, (512-32), dHubChannel_CFG_MTU_128byte, 0, 0, 1},
        { avioDhubChMap_ag_MA1_R_A0, AG_DHUB_BANK0_START_ADDR+512,AG_DHUB_BANK0_START_ADDR+512+32, 32, (512-32), dHubChannel_CFG_MTU_128byte, 0, 0, 1},
        { avioDhubChMap_ag_MA2_R_A0, AG_DHUB_BANK0_START_ADDR+512*2,AG_DHUB_BANK0_START_ADDR+512*2+32, 32, (512-32), dHubChannel_CFG_MTU_128byte, 0, 0, 1},
        { avioDhubChMap_ag_MA3_R_A0, AG_DHUB_BANK0_START_ADDR+512*3,AG_DHUB_BANK0_START_ADDR+512*3+32, 32, (512-32), dHubChannel_CFG_MTU_128byte, 0, 0, 1},
        { avioDhubChMap_ag_SA0_R_A0, AG_DHUB_BANK0_START_ADDR+512*4,AG_DHUB_BANK0_START_ADDR+512*4+32, 32, (1024-32), dHubChannel_CFG_MTU_128byte, 0, 0, 1},
        { avioDhubChMap_ag_MIC0_W_A0, AG_DHUB_BANK0_START_ADDR+1024*3, AG_DHUB_BANK0_START_ADDR+1024*3+32, 32, (512-32), dHubChannel_CFG_MTU_128byte, 0, 0, 1},
        { avioDhubChMap_ag_MIC1_W_A0, AG_DHUB_BANK0_START_ADDR+512+1024*3, AG_DHUB_BANK0_START_ADDR+512+1024*3+32, 32, (512-32), dHubChannel_CFG_MTU_128byte, 0, 0, 1},
        // Bank1
        { avioDhubChMap_ag_CSR_R_A0, AG_DHUB_BANK1_START_ADDR, AG_DHUB_BANK1_START_ADDR+32, 32, (8192-32), dHubChannel_CFG_MTU_128byte, 1, 0, 1},
        // Bank2
        { avioDhubChMap_ag_GFX_R_A0, AG_DHUB_BANK2_START_ADDR, AG_DHUB_BANK2_START_ADDR+32, 32, (8192-32), dHubChannel_CFG_MTU_128byte, 1, 0, 1},
#else
	// Bank0
        { avioDhubChMap_ag_SA0_R_A0, AG_DHUB_BANK0_START_ADDR, AG_DHUB_BANK0_START_ADDR+32, 32, (512-32), dHubChannel_CFG_MTU_128byte, 0, 0, 1},
#endif
};


/**************************************************************
 *	Function: DhubInitialization
 *	Description: Initialize DHUB .
 *	Parameter : cpuId   ----- cpu ID
 *             dHubBaseAddr ----- dHub Base address.
 *             hboSramAddr  ----- Sram Address for HBO.
 *             pdhubHandle  ----- pointer to 2D dhubHandle
 *             dhub_config  ----- configuration of AG
 *             numOfChans   ----- number of channels
 *	Return:		void
**************************************************************/
void DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr,
			UNSG32 hboSramAddr, HDL_dhub2d *pdhubHandle,
			DHUB_channel_config *dhub_config,
			SIGN32 numOfChans)
{
	HDL_semaphore *pSemHandle;
	SIGN32 i;
	SIGN32 chanId;

	//Initialize HDL_dhub with a $dHub BIU instance.
	dhub2d_hdl(hboSramAddr,			/*!	Base address of dHub.HBO SRAM !*/
		dHubBaseAddr,			/*!	Base address of a BIU instance of $dHub !*/
		pdhubHandle			/*!	Handle to HDL_dhub2d !*/
		);
	//set up semaphore to trigger cmd done interrupt
	//note that this set of semaphores are different from the HBO semaphores
	//the ID must match the dhub ID because they are hardwired.
	pSemHandle = dhub_semaphore(&pdhubHandle->dhub);

	for (i = 0; i< numOfChans; i++) {
		//Configurate a dHub channel
		//note that in this function, it also configured right HBO channels(cmdQ and dataQ) and semaphores
		chanId = dhub_config[i].chanId;
		{
			dhub_channel_cfg(
				&pdhubHandle->dhub,			/*!	Handle to HDL_dhub !*/
				chanId,					/*!	Channel ID in $dHubReg !*/
				dhub_config[chanId].chanCmdBase,	//UNSG32 baseCmd,	/*!	Channel FIFO base address (byte address) for cmdQ !*/
				dhub_config[chanId].chanDataBase,	//UNSG32 baseData,	/*!	Channel FIFO base address (byte address) for dataQ !*/
				dhub_config[chanId].chanCmdSize/8,	//SIGN32		depthCmd,			/*!	Channel FIFO depth for cmdQ, in 64b word !*/
				dhub_config[chanId].chanDataSize/8,	//SIGN32		depthData,			/*!	Channel FIFO depth for dataQ, in 64b word !*/
				dhub_config[chanId].chanMtuSize,	/*!	See 'dHubChannel.CFG.MTU', 0/1/2 for 8/32/128 bytes !*/
				dhub_config[chanId].chanQos,		/*!	See 'dHubChannel.CFG.QoS' !*/
				dhub_config[chanId].chanSelfLoop,	/*!	See 'dHubChannel.CFG.selfLoop' !*/
				dhub_config[chanId].chanEnable,		/*!	0 to disable, 1 to enable !*/
				0					/*!	Pass NULL to directly init dHub, or
										Pass non-zero to receive programming sequence
										in (adr,data) pairs
									!*/
				);
#if 0
			// setup interrupt for channel chanId
			// configure the semaphore depth to be 1
			semaphore_cfg(pSemHandle, chanId, 1, 0);

			// enable interrupt from this semaphore
			semaphore_intr_enable (
				pSemHandle, // semaphore handler
				chanId,
				0,       // empty
				1,       // full
				0,       // almost_empty
				0,       // almost_full
				cpuId    // 0~2, depending on which CPU the interrupt is enabled for.
			);
#endif
		}
	}
}

/************************************************************
 *      Function: DhubEnableIntr
 *      Description: Initialize interrupt for DHUB channel.
 *      Parameter :
 *             cpuId       ----- cpu ID
 *             pdhubHandle ----- pointer to 2D dhubHandle
 *             chanId      ----- channel ID
 *             enable      ----- 1=enable;0=disable
 *      Return:         void
************************************************************/
void DhubEnableIntr(SIGN32 cpuId, HDL_dhub2d *pdhubHandle,
			SIGN32 chanId, SIGN32 enable)
{
        HDL_semaphore *pSemHandle;

	pSemHandle = dhub_semaphore(&pdhubHandle->dhub);

	// setup interrupt for channel chanId
	// configure the semaphore depth to be 1
	semaphore_cfg(pSemHandle, chanId, 1, 0);

	// enable interrupt from this semaphore
	semaphore_intr_enable (
		pSemHandle,	// semaphore handler
		chanId,
		0,       	// empty
		enable ? 1:0,   // full
		0,       	// almost_empty
		0,       	// almost_full
		cpuId    	// 0~2, depending on which CPU the interrupt is enabled for.
		);
}
