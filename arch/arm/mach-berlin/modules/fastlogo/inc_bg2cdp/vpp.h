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


#ifndef vpp_h
#define vpp_h (){}

    #define     RA_LDR_BG                                      0x0000
    #define     RA_LDR_MAIN                                    0x0014
    #define     RA_LDR_PIP                                     0x0028
    #define     RA_LDR_PG                                      0x0400
    #define     RA_LDR_IG                                      0x0C00
    #define     RA_LDR_CURSOR                                  0x1400
    #define     RA_LDR_MOSD                                    0x2000

    #define     RA_Vpp_VP_DMX_CTRL                             0x12C04
    #define     RA_Vpp_VP_DMX_HRES                             0x12C08
    #define     RA_Vpp_VP_DMX_HT                               0x12C0C
    #define     RA_Vpp_VP_DMX_VRES                             0x12C10
    #define     RA_Vpp_VP_DMX_VT                               0x12C14
    #define     RA_Vpp_VP_DMX_IVT                              0x12C18

    #define     RA_Vpp_LDR                                     0x10400
    #define     RA_PLANE_SIZE                                  0x0000
    #define     RA_PLANE_CROP                                  0x0004

    #define     RA_TG_SIZE                                     0x0004
    #define     RA_TG_HB                                       0x000C
    #define     RA_TG_VB0                                      0x0018

    #define     RA_Vpp_vpIn_pix_A0                                0x10098
    #define     RA_Vpp_vpIn_pix_Z1                                0x10094
    #define     RA_Vpp_vpIn_pix_CD_A0                                0x100EC

    #define     RA_Vpp_vpOut_pix_A0                               0x1009C
    #define     RA_Vpp_vpOut_pix_Z1                               0x10098
    #define     RA_Vpp_vpOut_pix_CD_A0                               0x100F0

    #define     RA_Vpp_diW_pix_A0                                 0x100AC
    #define     RA_Vpp_diW_pix_Z1                                 0x100A8
    #define     RA_Vpp_diW_pix_CD_A0                                 0x10100

    #define     RA_Vpp_diR_word_A0                                0x100B0
    #define     RA_Vpp_diR_word_Z1                                0x100AC
    #define     RA_Vpp_diR_word_CD_A0                                0x10104

    #define     RA_Vpp_VP_TG_A0                                   0x100DC
    #define     RA_Vpp_VP_TG_Z1                                   0x100D8
    #define     RA_Vpp_VP_TG_CD_A0                                  0x10130

#endif
