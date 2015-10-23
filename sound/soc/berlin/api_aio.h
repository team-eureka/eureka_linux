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
#ifndef __AIO_API_H__
#define __AIO_API_H__

#include "aio.h"

#define AIO_P71         0 // primary audio
#define AIO_SEC         1 // secondary audio
#define AIO_MIC         2 // mic input
#define AIO_SPDIF       3
#define AIO_HDMI        4 // HDMI audio

#define AIO_TSD0        0
#define AIO_TSD1        1
#define AIO_TSD2        2
#define AIO_TSD3        3

#define AIO_DIV1        PRIAUD_CLKDIV_SETTING_DIV1
#define AIO_DIV2        PRIAUD_CLKDIV_SETTING_DIV2
#define AIO_DIV4        PRIAUD_CLKDIV_SETTING_DIV4
#define AIO_DIV8        PRIAUD_CLKDIV_SETTING_DIV8
#define AIO_DIV16       PRIAUD_CLKDIV_SETTING_DIV16
#define AIO_DIV32       PRIAUD_CLKDIV_SETTING_DIV32
#define AIO_DIV64       PRIAUD_CLKDIV_SETTING_DIV64
#define AIO_DIV128      PRIAUD_CLKDIV_SETTING_DIV128

#define AIO_JISTIFIED_MODE   PRIAUD_CTRL_TFM_JUSTIFIED
#define AIO_I2S_MODE    PRIAUD_CTRL_TFM_I2S

#define AIO_16DFM       PRIAUD_CTRL_TDM_16DFM
#define AIO_18DFM       PRIAUD_CTRL_TDM_18DFM
#define AIO_20DFM       PRIAUD_CTRL_TDM_20DFM
#define AIO_24DFM       PRIAUD_CTRL_TDM_24DFM
#define AIO_32DFM       PRIAUD_CTRL_TDM_32DFM

#define AIO_16CFM       PRIAUD_CTRL_TCF_16CFM
#define AIO_24CFM       PRIAUD_CTRL_TCF_24CFM
#define AIO_32CFM       PRIAUD_CTRL_TCF_32CFM

/*******************************************************************************
* Function:    AIO_SetCtl(UNSG32 id, UNSG32 data_fmt, UNSG32 width_word, UNSG32 width_sample)
* Description: Configure Audio output format
* Inputs:      id           -- audio port
*              data_fmt     -- output data format
*              width_word   -- clock cycles
*              width_sample -- data cycles
* Outputs:     none
* Return:      none
*******************************************************************************/
void AIO_SetCtl(UNSG32 id, UNSG32 data_fmt, UNSG32 width_word, UNSG32 width_sample);

/*******************************************************************************
* Function:    AIO_SetClkDiv(UNSG32 id, UNSG32 div)
* Description: Config Audio output clock
* Inputs:      id           -- audio port
*              div          -- audio clock divider
* Outputs:     none
* Return:      none
*******************************************************************************/
void AIO_SetClkDiv(UNSG32 id, UNSG32 div);

/*******************************************************************************
* Function:    AIO_SetAudChEn(UNSG32 id, UNSG32 tsd, UNSG32 enable)
* Description: Enable/Disable Audio Output Channels
* Inputs:      id           -- audio port
*              tsd          -- audio channel pair
*              enable       -- enable/disable TSD group
* Outputs:     none
* Return:      none
*******************************************************************************/
void AIO_SetAudChEn(UNSG32 id, UNSG32 tsd, UNSG32 enable);

/*******************************************************************************
* Function:    AIO_SetAudChMute(UNSG32 id, UNSG32 tsd, UNSG32 mute)
* Description: Mute/Unmute Audio Output Channels
* Inputs:      id           -- audio port
*              tsd          -- audio channel pair
*              mute         -- mute/unmute TSD group.
* Outputs:     none
* Return:      none
*******************************************************************************/
void AIO_SetAudChMute(UNSG32 id, UNSG32 tsd, UNSG32 mute);

#endif
