/*********************************************************************************
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
***********************************************************************************/

#ifndef _BERLIN_MEMMAP_H_
#define _BERLIN_MEMMAP_H_

#define        MEMMAP_VIP_DHUB_REG_BASE                    0xF7AE0000
#define        MEMMAP_VIP_DHUB_REG_SIZE                    0x20000
#define        MEMMAP_VIP_DHUB_REG_DEC_BIT                 0x11

#define        MEMMAP_AVIF_REG_BASE                        0xF7980000
#define        MEMMAP_AVIF_REG_SIZE                        0x80000
#define        MEMMAP_AVIF_REG_DEC_BIT                     0x13

#define        MEMMAP_ZSP_REG_BASE                         0xF7C00000
#define        MEMMAP_ZSP_REG_SIZE                         0x80000
#define        MEMMAP_ZSP_REG_DEC_BIT                      0x13

#define        MEMMAP_AG_DHUB_REG_BASE                     0xF7D00000
#define        MEMMAP_AG_DHUB_REG_SIZE                     0x20000
#define        MEMMAP_AG_DHUB_REG_DEC_BIT                  0x11

#define        MEMMAP_VPP_DHUB_REG_BASE                    0xF7F40000
#define        MEMMAP_VPP_DHUB_REG_SIZE                    0x20000
#define        MEMMAP_VPP_DHUB_REG_DEC_BIT                 0x11

#define        MEMMAP_I2S_REG_BASE                         0xF7E70000
#define        MEMMAP_I2S_REG_SIZE                         0x10000
#define        MEMMAP_I2S_REG_DEC_BIT                      0x10

#define        MEMMAP_AVIO_GBL_BASE                        0xF7E20000
#define        MEMMAP_AVIO_GBL_SIZE                        0x10000
#define        MEMMAP_AVIO_GBL_DEC_BIT                     0x10

#endif	//_BERLIN_MEMMAP_H_
