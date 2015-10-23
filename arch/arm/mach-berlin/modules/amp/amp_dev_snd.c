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
 ********************************************************************************/

/*******************************************************************************
  System head files
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <linux/proc_fs.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_irq.h>

#include <linux/mm.h>
#include <linux/kdev_t.h>
#include <asm/page.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>

/*******************************************************************************
  Local head files
*/
#include "galois_io.h"
#include "cinclude.h"
#include "zspWrapper.h"

#include "api_avio_dhub.h"
#include "shm_api.h"
#include "cc_msgq.h"
#include "cc_error.h"

#include "amp_driver.h"
#include "amp_memmap.h"

#include "amp_dev_snd.h"

/*******************************************************************************
  Macro Defined
  */

#define AOUT_IOCTL_START_CMD			0xbeef2001
#define AIP_IOCTL_START_CMD				0xbeef2002
#define AIP_IOCTL_STOP_CMD				0xbeef2003
#define APP_IOCTL_INIT_CMD				0xbeef3001
#define APP_IOCTL_START_CMD				0xbeef3002

#define ADSP_ZSPINT2Soc_IRQ0			0

#define AMP_DEVICE_TAG					"[amp_snd] "

#define snd_trace(...)   printk(KERN_WARNING AMP_DEVICE_TAG __VA_ARGS__)
#define snd_error(...)   printk(KERN_ERR AMP_DEVICE_TAG __VA_ARGS__)

extern int berlin_chip_revision;

static int snd_dev_open(struct inode *inode, struct file *filp);
static int snd_dev_release(struct inode *inode, struct file *filp);
static long snd_dev_ioctl_unlocked(struct file *filp, unsigned int cmd,
				   unsigned long arg);

/*******************************************************************************
  Module Variable
*/
typedef struct snd_cntx {
	int owner;
	unsigned int user_count;
};

static int snd_irqs[IRQ_AMP_MAX];

INT32 pri_audio_chanId_Z1[4] =
    { avioDhubChMap_ag_MA0_R_Z1, avioDhubChMap_ag_MA1_R_Z1, avioDhubChMap_ag_MA2_R_Z1,
	avioDhubChMap_ag_MA3_R_Z1
};

INT32 pri_audio_chanId_A0[4] =
    { avioDhubChMap_ag_MA0_R_A0, avioDhubChMap_ag_MA1_R_A0, avioDhubChMap_ag_MA2_R_A0,
	avioDhubChMap_ag_MA3_R_A0
};

static int enable_trace;

static AOUT_PATH_CMD_FIFO *p_ma_fifo;
static AOUT_PATH_CMD_FIFO *p_sa_fifo;
static AOUT_PATH_CMD_FIFO *p_spdif_fifo;
static AOUT_PATH_CMD_FIFO *p_hdmi_fifo;
static HWAPP_CMD_FIFO *p_app_fifo;
static AIP_DMA_CMD_FIFO *p_aip_fifo;

static int aip_i2s_pair;

static DEFINE_SPINLOCK(aout_spinlock);
static DEFINE_SPINLOCK(app_spinlock);
static DEFINE_SPINLOCK(aip_spinlock);

static void amp_ma_do_tasklet(unsigned long);
//static void amp_aip_do_tasklet(unsigned long);
static void amp_hdmi_do_tasklet(unsigned long);
static void amp_app_do_tasklet(unsigned long);
static void amp_zsp_do_tasklet(unsigned long);
#ifndef BERLIN_BG2_CD
static void amp_spdif_do_tasklet(unsigned long);
static void amp_sa_do_tasklet(unsigned long);
static void amp_pg_dhub_done_tasklet(unsigned long);
#endif
static void amp_rle_do_err_tasklet(unsigned long);
static void amp_rle_do_done_tasklet(unsigned long);

static void aout_resume_cmd(int path_id);
static void aip_resume_cmd(void);

static DECLARE_TASKLET_DISABLED(amp_ma_tasklet, amp_ma_do_tasklet, 0);
//static DECLARE_TASKLET_DISABLED(amp_aip_tasklet, amp_aip_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_hdmi_tasklet, amp_hdmi_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_app_tasklet, amp_app_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_zsp_tasklet, amp_zsp_do_tasklet, 0);
#ifndef BERLIN_BG2_CD
static DECLARE_TASKLET_DISABLED(amp_spdif_tasklet, amp_spdif_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_sa_tasklet, amp_sa_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_pg_done_tasklet,
				amp_pg_dhub_done_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_rle_err_tasklet, amp_rle_do_err_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_rle_done_tasklet,
				amp_rle_do_done_tasklet, 0);
#endif

static void amp_ma_do_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0, };

	msg.m_MsgID = 1 << avioDhubChMap_ag_MA0_R_auto;
	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_AUD, &msg);

}

#if (BERLIN_CHIP_VERSION != BERLIN_BG2_CD)
static void amp_sa_do_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0, };

#if (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)
	msg.m_MsgID = 1 << avioDhubChMap_ag_SA_R;
	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_AUD, &msg);
#endif
}

static void amp_spdif_do_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0, };

#if (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)
	msg.m_MsgID = 1 << avioDhubChMap_ag_SPDIF_R;
	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_AUD, &msg);
#endif
}

static void amp_aip_do_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0, };
#if (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)
	msg.m_MsgID = 1 << avioDhubChMap_vip_MIC0_W;

	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_AIP, &msg);
#endif
}
#endif

static void amp_hdmi_do_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0, };

	msg.m_MsgID = 1 << avioDhubChMap_vpp_HDMI_R_auto;
	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_AUD, &msg);
}

static void amp_app_do_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0, };

	msg.m_MsgID = 1 << avioDhubSemMap_ag_app_intr2;
	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_HWAPP, &msg);
}

static void amp_zsp_do_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0, };

	msg.m_MsgID = 1 << ADSP_ZSPINT2Soc_IRQ0;
	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_ZSP, &msg);
}

#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
static void amp_pg_dhub_done_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0, };

	msg.m_MsgID = 1 << avioDhubChMap_ag_PG_ENG_W;
	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_RLE, &msg);
}

static void amp_rle_do_err_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0, };

	msg.m_MsgID = 1 << avioDhubSemMap_ag_spu_intr0;
	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_RLE, &msg);
}

static void amp_rle_do_done_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0, };

	msg.m_MsgID = 1 << avioDhubSemMap_ag_spu_intr1;
	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_RLE, &msg);
}
#endif

//static DEFINE_SPINLOCK(vmeta_spinlock);

static void *AoutFifoGetKernelRdDMAInfo(AOUT_PATH_CMD_FIFO * p_aout_cmd_fifo,
					int pair)
{
	void *pHandle;
	pHandle =
	    &(p_aout_cmd_fifo->aout_dma_info[pair]
	      [p_aout_cmd_fifo->kernel_rd_offset]);
	return pHandle;
}

static void AoutFifoKernelRdUpdate(AOUT_PATH_CMD_FIFO * p_aout_cmd_fifo,
				   int adv)
{
	p_aout_cmd_fifo->kernel_rd_offset += adv;
	if (p_aout_cmd_fifo->kernel_rd_offset >= p_aout_cmd_fifo->size)
		p_aout_cmd_fifo->kernel_rd_offset -= p_aout_cmd_fifo->size;
}

static int AoutFifoCheckKernelFullness(AOUT_PATH_CMD_FIFO * p_aout_cmd_fifo)
{
	int full;
	full = p_aout_cmd_fifo->wr_offset - p_aout_cmd_fifo->kernel_rd_offset;
	if (full < 0)
		full += p_aout_cmd_fifo->size;
	return full;
}

static void aout_start_cmd(int *aout_info)
{
	int *p = aout_info;
	int i, chanId;
	AOUT_DMA_INFO *p_dma_info;

	if (*p == MULTI_PATH) {
		p_ma_fifo =
		    (AOUT_PATH_CMD_FIFO *) MV_SHM_GetNonCacheVirtAddr(*(p + 1));
		for (i = 0; i < 4; i++) {
			p_dma_info = (AOUT_DMA_INFO *)
			    AoutFifoGetKernelRdDMAInfo(p_ma_fifo, i);
			if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
				chanId = pri_audio_chanId_Z1[i];
			else
				chanId = pri_audio_chanId_A0[i];
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
					       p_dma_info->addr0,
					       p_dma_info->size0, 0, 0, 0,
					       i == 0 ? 1 : 0, 0, 0);
		}
	} else if (*p == LoRo_PATH) {
#if !defined (BERLIN_BG2_CD)
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
		if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
		 {
			p_sa_fifo =
			 (AOUT_PATH_CMD_FIFO *) MV_SHM_GetNonCacheVirtAddr(*(p + 1));
			p_dma_info =
			(AOUT_DMA_INFO *) AoutFifoGetKernelRdDMAInfo(p_sa_fifo, 0);
			chanId = avioDhubChMap_ag_SA_R;
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
					       p_dma_info->addr0, p_dma_info->size0, 0,
					       0, 0, 1, 0, 0);
		}

#endif
	} else if (*p == SPDIF_PATH) {
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
		if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT) /* Z1/Z2 */
		{
			p_spdif_fifo =
			(AOUT_PATH_CMD_FIFO *) MV_SHM_GetNonCacheVirtAddr(*(p + 1));
			p_dma_info =
			(AOUT_DMA_INFO *) AoutFifoGetKernelRdDMAInfo(p_spdif_fifo,
									 0);
			chanId = avioDhubChMap_ag_SPDIF_R_Z1;
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
					       p_dma_info->addr0, p_dma_info->size0, 0,
					       0, 0, 1, 0, 0);
		}
#endif
	} else if (*p == HDMI_PATH) {
		p_hdmi_fifo =
		    (AOUT_PATH_CMD_FIFO *) MV_SHM_GetNonCacheVirtAddr(*(p + 1));
		p_dma_info =
		    (AOUT_DMA_INFO *) AoutFifoGetKernelRdDMAInfo(p_hdmi_fifo,
								 0);
		chanId = avioDhubChMap_vpp_HDMI_R_auto;
		dhub_channel_write_cmd(&VPP_dhubHandle.dhub, chanId,
				       p_dma_info->addr0, p_dma_info->size0, 0,
				       0, 0, 1, 0, 0);
	}
}

static void aout_resume_cmd(int path_id)
{
	AOUT_DMA_INFO *p_dma_info;
	unsigned int i, chanId;

	if (path_id == MULTI_PATH) {
		if (!p_ma_fifo->fifo_underflow)
			AoutFifoKernelRdUpdate(p_ma_fifo, 1);

		if (AoutFifoCheckKernelFullness(p_ma_fifo)) {
			p_ma_fifo->fifo_underflow = 0;
			for (i = 0; i < 4; i++) {
				p_dma_info = (AOUT_DMA_INFO *)
				    AoutFifoGetKernelRdDMAInfo(p_ma_fifo, i);
				if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
					chanId = pri_audio_chanId_Z1[i];
				else
					chanId = pri_audio_chanId_A0[i];
				dhub_channel_write_cmd(&AG_dhubHandle.dhub,
						       chanId,
						       p_dma_info->addr0,
						       p_dma_info->size0, 0, 0,
						       0,
						       (p_dma_info->size1 == 0
							&& i == 0) ? 1 : 0, 0,
						       0);
				if (p_dma_info->size1)
					dhub_channel_write_cmd(&AG_dhubHandle.
							       dhub, chanId,
							       p_dma_info->
							       addr1,
							       p_dma_info->
							       size1, 0, 0, 0,
							       (i == 0) ? 1 : 0,
							       0, 0);
			}
		} else {
			p_ma_fifo->fifo_underflow = 1;
			for (i = 0; i < 4; i++) {
				if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
					chanId = pri_audio_chanId_Z1[i];
				else
					chanId = pri_audio_chanId_A0[i];
				dhub_channel_write_cmd(&AG_dhubHandle.dhub,
						       chanId,
						       p_ma_fifo->zero_buffer,
						       p_ma_fifo->
						       zero_buffer_size, 0, 0,
						       0, i == 0 ? 1 : 0, 0, 0);
			}
		}
	} else if (path_id == LoRo_PATH) {
#ifndef BERLIN_BG2_CD
		if (!p_sa_fifo->fifo_underflow)
			AoutFifoKernelRdUpdate(p_sa_fifo, 1);

		if (AoutFifoCheckKernelFullness(p_sa_fifo)) {
			p_sa_fifo->fifo_underflow = 0;
			p_dma_info = (AOUT_DMA_INFO *)
			    AoutFifoGetKernelRdDMAInfo(p_sa_fifo, 0);
			chanId = avioDhubChMap_ag_SA_R;
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
					       p_dma_info->addr0,
					       p_dma_info->size0, 0, 0, 0,
					       p_dma_info->size1 ? 0 : 1, 0, 0);
			if (p_dma_info->size1)
				dhub_channel_write_cmd(&AG_dhubHandle.dhub,
						       chanId,
						       p_dma_info->addr1,
						       p_dma_info->size1, 0, 0,
						       0, 1, 0, 0);
		} else {
			p_sa_fifo->fifo_underflow = 1;
			chanId = avioDhubChMap_ag_SA_R;
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
					       p_sa_fifo->zero_buffer,
					       p_sa_fifo->zero_buffer_size, 0,
					       0, 0, 1, 0, 0);
		}
#endif
	} else if (path_id == SPDIF_PATH) {
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
		if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT) /* Z1/Z2 */
		{
			if (!p_spdif_fifo->fifo_underflow)
				AoutFifoKernelRdUpdate(p_spdif_fifo, 1);

			if (AoutFifoCheckKernelFullness(p_spdif_fifo)) {
				p_spdif_fifo->fifo_underflow = 0;
				p_dma_info = (AOUT_DMA_INFO *)
				    AoutFifoGetKernelRdDMAInfo(p_spdif_fifo, 0);
				chanId = avioDhubChMap_ag_SPDIF_R_Z1;
				dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
						       p_dma_info->addr0,
						       p_dma_info->size0, 0, 0, 0,
						 p_dma_info->size1 ? 0 : 1, 0, 0);
				if (p_dma_info->size1)
					dhub_channel_write_cmd(&AG_dhubHandle.dhub,
							       chanId,
							       p_dma_info->addr1,
							       p_dma_info->size1, 0, 0,
							       0, 1, 0, 0);
			} else {
				p_spdif_fifo->fifo_underflow = 1;
				chanId = avioDhubChMap_ag_SPDIF_R_Z1;
				dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
						       p_spdif_fifo->zero_buffer,
						       p_spdif_fifo->zero_buffer_size,
						       0, 0, 0, 1, 0, 0);
			}
		}
#endif
	} else if (path_id == HDMI_PATH) {
		if (!p_hdmi_fifo->fifo_underflow)
			AoutFifoKernelRdUpdate(p_hdmi_fifo, 1);

		if (AoutFifoCheckKernelFullness(p_hdmi_fifo)) {
			p_hdmi_fifo->fifo_underflow = 0;
			p_dma_info = (AOUT_DMA_INFO *)
			    AoutFifoGetKernelRdDMAInfo(p_hdmi_fifo, 0);
			chanId = avioDhubChMap_vpp_HDMI_R_auto;
			dhub_channel_write_cmd(&VPP_dhubHandle.dhub, chanId,
					       p_dma_info->addr0,
					       p_dma_info->size0, 0, 0, 0,
					       p_dma_info->size1 ? 0 : 1, 0, 0);
			if (p_dma_info->size1)
				dhub_channel_write_cmd(&VPP_dhubHandle.dhub,
						       chanId,
						       p_dma_info->addr1,
						       p_dma_info->size1, 0, 0,
						       0, 1, 0, 0);
		} else {
			p_hdmi_fifo->fifo_underflow = 1;
			chanId = avioDhubChMap_vpp_HDMI_R_auto;
			dhub_channel_write_cmd(&VPP_dhubHandle.dhub, chanId,
					       p_hdmi_fifo->zero_buffer,
					       p_hdmi_fifo->zero_buffer_size, 0,
					       0, 0, 1, 0, 0);
		}
	}
}

static void *AIPFifoGetKernelPreRdDMAInfo(AIP_DMA_CMD_FIFO * p_aip_cmd_fifo,
					  int pair)
{
	void *pHandle;
	int rd_offset = p_aip_cmd_fifo->kernel_pre_rd_offset;
	pHandle = &(p_aip_cmd_fifo->aip_dma_cmd[pair][rd_offset]);
	return pHandle;
}

static void AIPFifoKernelPreRdUpdate(AIP_DMA_CMD_FIFO * p_aip_cmd_fifo, int adv)
{
	int tmp;
	tmp = p_aip_cmd_fifo->kernel_pre_rd_offset + adv;
	p_aip_cmd_fifo->kernel_pre_rd_offset = tmp >= p_aip_cmd_fifo->size ?
	    tmp - p_aip_cmd_fifo->size : tmp;
}

static void AIPFifoKernelRdUpdate(AIP_DMA_CMD_FIFO * p_aip_cmd_fifo, int adv)
{
	int tmp;
	tmp = p_aip_cmd_fifo->kernel_rd_offset + adv;
	p_aip_cmd_fifo->kernel_rd_offset = tmp >= p_aip_cmd_fifo->size ?
	    tmp - p_aip_cmd_fifo->size : tmp;
}

static int AIPFifoCheckKernelFullness(AIP_DMA_CMD_FIFO * p_aip_cmd_fifo)
{
	int full;
	full = p_aip_cmd_fifo->wr_offset - p_aip_cmd_fifo->kernel_pre_rd_offset;
	if (full < 0)
		full += p_aip_cmd_fifo->size;
	return full;
}

static void aip_start_cmd(int *aip_info)
{
	int *p = aip_info;
	int chanId, pair;
	AIP_DMA_CMD *p_dma_cmd;

#if (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)
	if (*p == 1) {
		aip_i2s_pair = 1;
		p_aip_fifo =
		    (AIP_DMA_CMD_FIFO *) MV_SHM_GetNonCacheVirtAddr(*(p + 1));
		p_dma_cmd =
		    (AIP_DMA_CMD *) AIPFifoGetKernelPreRdDMAInfo(p_aip_fifo, 0);

		chanId = avioDhubChMap_vip_MIC0_W;
		dhub_channel_write_cmd(&VIP_dhubHandle.dhub, chanId,
				       p_dma_cmd->addr0, p_dma_cmd->size0, 0, 0,
				       0, 1, 0, 0);
		AIPFifoKernelPreRdUpdate(p_aip_fifo, 1);

		// push 2nd dHub command
		p_dma_cmd =
		    (AIP_DMA_CMD *) AIPFifoGetKernelPreRdDMAInfo(p_aip_fifo, 0);
		chanId = avioDhubChMap_vip_MIC0_W;
		dhub_channel_write_cmd(&VIP_dhubHandle.dhub, chanId,
				       p_dma_cmd->addr0, p_dma_cmd->size0, 0, 0,
				       0, 1, 0, 0);
		AIPFifoKernelPreRdUpdate(p_aip_fifo, 1);
	} else if (*p == 4) {
		/* 4 I2S will be introduced since BG2 A0 */
		aip_i2s_pair = 4;
		p_aip_fifo =
		    (AIP_DMA_CMD_FIFO *) MV_SHM_GetNonCacheVirtAddr(*(p + 1));
		for (pair = 0; pair < 4; pair++) {
			p_dma_cmd =
			    (AIP_DMA_CMD *)
			    AIPFifoGetKernelPreRdDMAInfo(p_aip_fifo, pair);
			chanId = avioDhubChMap_vip_MIC0_W + pair;
			dhub_channel_write_cmd(&VIP_dhubHandle.dhub, chanId,
					       p_dma_cmd->addr0,
					       p_dma_cmd->size0, 0, 0, 0, 1, 0,
					       0);
		}
		AIPFifoKernelPreRdUpdate(p_aip_fifo, 1);

		// push 2nd dHub command
		for (pair = 0; pair < 4; pair++) {
			p_dma_cmd = (AIP_DMA_CMD *)
			    AIPFifoGetKernelPreRdDMAInfo(p_aip_fifo, pair);
			chanId = avioDhubChMap_vip_MIC0_W + pair;
			dhub_channel_write_cmd(&VIP_dhubHandle.dhub, chanId,
					       p_dma_cmd->addr0,
					       p_dma_cmd->size0, 0, 0, 0, 1, 0,
					       0);
		}
		AIPFifoKernelPreRdUpdate(p_aip_fifo, 1);
	}
#endif
}

static void aip_stop_cmd(void)
{
	return;
}

static void aip_resume_cmd()
{
	AIP_DMA_CMD *p_dma_cmd;
	unsigned int chanId;
	int pair;

#if (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)
	if (!p_aip_fifo->fifo_overflow)
		AIPFifoKernelRdUpdate(p_aip_fifo, 1);

	if (AIPFifoCheckKernelFullness(p_aip_fifo)) {
		p_aip_fifo->fifo_overflow = 0;
		for (pair = 0; pair < aip_i2s_pair; pair++) {
			p_dma_cmd =
			    (AIP_DMA_CMD *)
			    AIPFifoGetKernelPreRdDMAInfo(p_aip_fifo, pair);
			chanId = avioDhubChMap_vip_MIC0_W + pair;
			dhub_channel_write_cmd(&VIP_dhubHandle.dhub, chanId,
					       p_dma_cmd->addr0,
					       p_dma_cmd->size0, 0, 0, 0,
					       p_dma_cmd->addr1 ? 0 : 1, 0, 0);
			if (p_dma_cmd->addr1) {
				dhub_channel_write_cmd(&VIP_dhubHandle.dhub,
						       chanId, p_dma_cmd->addr1,
						       p_dma_cmd->size1, 0, 0,
						       0, 1, 0, 0);
			}
		}
		AIPFifoKernelPreRdUpdate(p_aip_fifo, 1);
	} else {
		p_aip_fifo->fifo_overflow = 1;
		p_aip_fifo->fifo_overflow_cnt++;
		for (pair = 0; pair < aip_i2s_pair; pair++) {
			/* FIXME:
			 *chanid should be changed if 4 pair is supported
			 */
			chanId = avioDhubChMap_vip_MIC0_W + pair;
			dhub_channel_write_cmd(&VIP_dhubHandle.dhub, chanId,
					       p_aip_fifo->overflow_buffer,
					       p_aip_fifo->overflow_buffer_size,
					       0, 0, 0, 1, 0, 0);
		}
	}
#endif
}

static int HwAPPFifoCheckKernelFullness(HWAPP_CMD_FIFO * p_app_cmd_fifo)
{
	int full;
	full = p_app_cmd_fifo->wr_offset - p_app_cmd_fifo->kernel_rd_offset;
	if (full < 0)
		full += p_app_cmd_fifo->size;
	return full;
}

static void *HwAPPFifoGetKernelInCoefRdCmdBuf(HWAPP_CMD_FIFO * p_app_cmd_fifo)
{
	void *pHandle;
	pHandle =
	    &(p_app_cmd_fifo->in_coef_cmd[p_app_cmd_fifo->kernel_rd_offset]);
	return pHandle;
}

static void *HwAPPFifoGetKernelOutCoefRdCmdBuf(HWAPP_CMD_FIFO * p_app_cmd_fifo)
{
	void *pHandle;
	pHandle =
	    &(p_app_cmd_fifo->out_coef_cmd[p_app_cmd_fifo->kernel_rd_offset]);
	return pHandle;
}

static void *HwAPPFifoGetKernelInDataRdCmdBuf(HWAPP_CMD_FIFO * p_app_cmd_fifo)
{
	void *pHandle;
	pHandle =
	    &(p_app_cmd_fifo->in_data_cmd[p_app_cmd_fifo->kernel_rd_offset]);
	return pHandle;
}

static void *HwAPPFifoGetKernelOutDataRdCmdBuf(HWAPP_CMD_FIFO * p_app_cmd_fifo)
{
	void *pHandle;
	pHandle =
	    &(p_app_cmd_fifo->out_data_cmd[p_app_cmd_fifo->kernel_rd_offset]);
	return pHandle;
}

static void HwAPPFifoUpdateIdleFlag(HWAPP_CMD_FIFO * p_app_cmd_fifo, int flag)
{
	p_app_cmd_fifo->kernel_idle = flag;
}

static void HwAPPFifoKernelRdUpdate(HWAPP_CMD_FIFO * p_app_cmd_fifo, int adv)
{
	p_app_cmd_fifo->kernel_rd_offset += adv;
	if (p_app_cmd_fifo->kernel_rd_offset >= p_app_cmd_fifo->size)
		p_app_cmd_fifo->kernel_rd_offset -= p_app_cmd_fifo->size;
}

static void app_start_cmd(HWAPP_CMD_FIFO * p_app_cmd_fifo)
{
	APP_CMD_BUFFER *p_in_coef_cmd, *p_out_coef_cmd;
	APP_CMD_BUFFER *p_in_data_cmd, *p_out_data_cmd;
	unsigned int chanId, PA, cmdSize;

#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
  if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT) /* Z1/Z2 */
  {
	if (HwAPPFifoCheckKernelFullness(p_app_cmd_fifo)) {
		HwAPPFifoUpdateIdleFlag(p_app_cmd_fifo, 0);
		p_in_coef_cmd = (APP_CMD_BUFFER *)
		    HwAPPFifoGetKernelInCoefRdCmdBuf(p_app_cmd_fifo);
		chanId = avioDhubChMap_ag_APPCMD_R_Z1;
		if (p_in_coef_cmd->cmd_len) {
			PA = p_in_coef_cmd->cmd_buffer_hw_base;
			cmdSize = p_in_coef_cmd->cmd_len * sizeof(int);
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId, PA,
					       cmdSize, 0, 0, 0, 0, 0, 0);
		}
		p_in_data_cmd = (APP_CMD_BUFFER *)
		    HwAPPFifoGetKernelInDataRdCmdBuf(p_app_cmd_fifo);
		if (p_in_data_cmd->cmd_len) {
			PA = p_in_data_cmd->cmd_buffer_hw_base;
			cmdSize = p_in_data_cmd->cmd_len * sizeof(int);
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId, PA,
					       cmdSize, 0, 0, 0, 0, 0, 0);
		}
		p_out_coef_cmd = (APP_CMD_BUFFER *)
		    HwAPPFifoGetKernelOutCoefRdCmdBuf(p_app_cmd_fifo);
		chanId = avioDhubChMap_ag_APPCMD_R_Z1;
		if (p_out_coef_cmd->cmd_len) {
			PA = p_out_coef_cmd->cmd_buffer_hw_base;
			cmdSize = p_out_coef_cmd->cmd_len * sizeof(int);
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId, PA,
					       cmdSize, 0, 0, 0, 0, 0, 0);
		}
		p_out_data_cmd = (APP_CMD_BUFFER *)
		    HwAPPFifoGetKernelOutDataRdCmdBuf(p_app_cmd_fifo);
		if (p_out_data_cmd->cmd_len) {
			PA = p_out_data_cmd->cmd_buffer_hw_base;
			cmdSize = p_out_data_cmd->cmd_len * sizeof(int);
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId, PA,
					       cmdSize, 0, 0, 0, 0, 0, 0);
		}
	} else {
		HwAPPFifoUpdateIdleFlag(p_app_cmd_fifo, 1);
	}
  }
#endif
}

static void app_resume_cmd(HWAPP_CMD_FIFO * p_app_cmd_fifo)
{
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
	if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
	{
		HwAPPFifoKernelRdUpdate(p_app_cmd_fifo, 1);
		app_start_cmd(p_app_cmd_fifo);
	}
}
static irqreturn_t amp_devices_aout_isr(int irq, void *dev_id)
{
	int instat;
	UNSG32 chanId;
	HDL_semaphore *pSemHandle;

	pSemHandle = dhub_semaphore(&AG_dhubHandle.dhub);
	instat = semaphore_chk_full(pSemHandle, -1);

	for (chanId = avioDhubChMap_ag_MA0_R_auto; chanId <= avioDhubChMap_ag_MA3_R_auto;
	     chanId++) {
		if (bTST(instat, chanId)) {
			semaphore_pop(pSemHandle, chanId, 1);
			semaphore_clr_full(pSemHandle, chanId);
			if (chanId == avioDhubChMap_ag_MA0_R_auto) {
				aout_resume_cmd(MULTI_PATH);
				tasklet_hi_schedule(&amp_ma_tasklet);
			}
		}
	}

#if !defined (BERLIN_BG2_CD)
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
#if (BERLIN_CHIP_VERSION_EXT < BERLIN_BG2CDP_A0_EXT) /* CDP Z1/Z2 */
	{
		chanId = avioDhubChMap_ag_SA_R;
		if (bTST(instat, chanId)) {
			semaphore_pop(pSemHandle, chanId, 1);
			semaphore_clr_full(pSemHandle, chanId);
			aout_resume_cmd(LoRo_PATH);
			tasklet_hi_schedule(&amp_sa_tasklet);
		}

		chanId = avioDhubChMap_ag_SPDIF_R;
		if (bTST(instat, chanId)) {
			semaphore_pop(pSemHandle, chanId, 1);
			semaphore_clr_full(pSemHandle, chanId);
			aout_resume_cmd(SPDIF_PATH);
			tasklet_hi_schedule(&amp_spdif_tasklet);
		}
	}
#endif
#endif
#endif
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
	if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
	{
		chanId = avioDhubSemMap_ag_app_intr2;
		if (bTST(instat, chanId)) {
			semaphore_pop(pSemHandle, chanId, 1);
			semaphore_clr_full(pSemHandle, chanId);
			app_resume_cmd(p_app_fifo);
			tasklet_hi_schedule(&amp_app_tasklet);
		}
	}

#if (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)
#ifndef BERLIN_BG2_CD
	chanId = avioDhubChMap_ag_PG_ENG_W;
	if (bTST(instat, chanId)) {
		semaphore_pop(pSemHandle, chanId, 1);
		semaphore_clr_full(pSemHandle, chanId);
		tasklet_hi_schedule(&amp_pg_done_tasklet);
	}

	chanId = avioDhubSemMap_ag_spu_intr0;
	if (bTST(instat, chanId)) {
		semaphore_pop(pSemHandle, chanId, 1);
		semaphore_clr_full(pSemHandle, chanId);
		tasklet_hi_schedule(&amp_rle_err_tasklet);
	}

	chanId = avioDhubSemMap_ag_spu_intr1;
	if (bTST(instat, chanId)) {
		semaphore_pop(pSemHandle, chanId, 1);
		semaphore_clr_full(pSemHandle, chanId);
		tasklet_hi_schedule(&amp_rle_done_tasklet);
	}
#endif
#endif
	return IRQ_HANDLED;
}

static irqreturn_t amp_devices_zsp_isr(int irq, void *dev_id)
{
	UNSG32 addr, v_id;
	T32ZspInt2Soc_status reg;

	addr = MEMMAP_ZSP_REG_BASE + RA_ZspRegs_Int2Soc + RA_ZspInt2Soc_status;
	GA_REG_WORD32_READ(addr, &(reg.u32));

	addr = MEMMAP_ZSP_REG_BASE + RA_ZspRegs_Int2Soc + RA_ZspInt2Soc_clear;
	v_id = ADSP_ZSPINT2Soc_IRQ0;
	if ((reg.u32) & (1 << v_id)) {
		GA_REG_WORD32_WRITE(addr, v_id);
	}

	tasklet_hi_schedule(&amp_zsp_tasklet);

	return IRQ_HANDLED;
}

/*******************************************************************************
  Module API
  */
int snd_dev_open(struct inode *inode, struct file *filp)
{
	unsigned int vec_num;
	void *pHandle = &snd_dev.cdev;
	int err = 0;

	/* initialize dhub */
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
	if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT) /* Z1/Z2 */
	{
		DhubInitialization(CPUINDEX, VPP_DHUB_BASE, VPP_HBO_SRAM_BASE,
				   &VPP_dhubHandle, VPP_config, VPP_NUM_OF_CHANNELS_Z1);
		DhubInitialization(CPUINDEX, AG_DHUB_BASE, AG_HBO_SRAM_BASE,
				   &AG_dhubHandle, AG_config, AG_NUM_OF_CHANNELS_Z1);
	}
	else /* A0 */
	{
		DhubInitialization(CPUINDEX, VPP_DHUB_BASE, VPP_HBO_SRAM_BASE,
				   &VPP_dhubHandle, VPP_config_a0, VPP_NUM_OF_CHANNELS_A0);
		DhubInitialization(CPUINDEX, AG_DHUB_BASE, AG_HBO_SRAM_BASE,
				   &AG_dhubHandle, AG_config_a0, AG_NUM_OF_CHANNELS_A0);
	}
#else
	{
		DhubInitialization(CPUINDEX, VPP_DHUB_BASE, VPP_HBO_SRAM_BASE,
				   &VPP_dhubHandle, VPP_config, VPP_NUM_OF_CHANNELS);
		DhubInitialization(CPUINDEX, AG_DHUB_BASE, AG_HBO_SRAM_BASE,
				   &AG_dhubHandle, AG_config, AG_NUM_OF_CHANNELS);
	}
#endif
#if (BERLIN_CHIP_VERSION < BERLIN_BG2_DTV)
#if (BERLIN_CHIP_VERSION != BERLIN_BG2_CD) && (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)

	DhubInitialization(CPUINDEX, VIP_DHUB_BASE, VIP_HBO_SRAM_BASE,
			   &VIP_dhubHandle, VIP_config, VIP_NUM_OF_CHANNELS);
#endif
#endif
	/* register and enable audio out ISR */
	vec_num = snd_irqs[IRQ_DHUBINTRAVIO1];
	err =
	    request_irq(vec_num, amp_devices_aout_isr, IRQF_DISABLED,
			"amp_module_aout", pHandle);
	if (unlikely(err < 0)) {
		snd_error("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}

	snd_trace("snd_dev_open ok\n");

	return 0;
}

int snd_dev_release(struct inode *inode, struct file *filp)
{
	void *pHandle = &snd_dev.cdev;

	/* unregister audio out interrupt */
	free_irq(amp_irqs[IRQ_DHUBINTRAVIO1], pHandle);

	snd_trace("snd_dev_release ok\n");

	return 0;
}

long snd_dev_ioctl_unlocked(struct file *filp, unsigned int cmd,
			    unsigned long arg)
{
	int aout_info[2];
	int app_info[2];
	int aip_info[2];
	unsigned long aoutirq, appirq, aipirq;

	switch (cmd) {

	case AOUT_IOCTL_START_CMD:
		if (copy_from_user
		    (aout_info, (void __user *)arg, 2 * sizeof(int)))
			return -EFAULT;
		spin_lock_irqsave(&aout_spinlock, aoutirq);
		aout_start_cmd(aout_info);
		spin_unlock_irqrestore(&aout_spinlock, aoutirq);
		break;

	case AIP_IOCTL_START_CMD:
		if (copy_from_user
		    (aip_info, (void __user *)arg, 2 * sizeof(int))) {
			return -EFAULT;
		}
		spin_lock_irqsave(&aip_spinlock, aipirq);
		aip_start_cmd(aip_info);
		spin_unlock_irqrestore(&aip_spinlock, aipirq);
		break;

	case AIP_IOCTL_STOP_CMD:
		aip_stop_cmd();
		break;

	case APP_IOCTL_INIT_CMD:
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
		if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
		{
			if (copy_from_user(app_info, (void __user *)arg, sizeof(int)))
				return -EFAULT;
			p_app_fifo =
			(HWAPP_CMD_FIFO *) MV_SHM_GetCacheVirtAddr(*app_info);
		}
			break;

	case APP_IOCTL_START_CMD:
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
		if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
		{
			spin_lock_irqsave(&app_spinlock, appirq);
			app_start_cmd(p_app_fifo);
			spin_unlock_irqrestore(&app_spinlock, appirq);
		}
		break;
	default:
		break;
	}

	return 0;
}

/*******************************************************************************
  Module Register API
 */

int snd_dev_init(struct amp_device_t *amp_dev, unsigned int user)
{
	struct device_node *np;
	struct resource r;

#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
    np = of_find_compatible_node(NULL, NULL, "marvell,berlin-amp");
#else //CD is using old driver
    np = of_find_compatible_node(NULL, NULL, "mrvl,berlin-amp");
#endif
	if (!np)
		return -ENODEV;
	of_irq_to_resource(np, IRQ_DHUBINTRAVIO1, &r);
	snd_irqs[IRQ_DHUBINTRAVIO1] = r.start;
	of_node_put(np);

	tasklet_enable(&amp_ma_tasklet);
#if !defined (BERLIN_BG2_CD)
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
	if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
	{
		tasklet_enable(&amp_sa_tasklet);
		tasklet_enable(&amp_spdif_tasklet);
	}
#endif
	tasklet_enable(&amp_hdmi_tasklet);
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
	if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
	{
		tasklet_enable(&amp_app_tasklet);
	}
//	tasklet_enable(&amp_aip_tasklet);

	return 0;
}

int snd_dev_exit(struct amp_device_t *amp_dev, unsigned int user)
{
	tasklet_disable(&amp_ma_tasklet);
#if !defined (BERLIN_BG2_CD)
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
	if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
	{
		tasklet_disable(&amp_sa_tasklet);
		tasklet_disable(&amp_spdif_tasklet);
	}
#endif
	tasklet_disable(&amp_hdmi_tasklet);
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
	if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
	{
		tasklet_disable(&amp_app_tasklet);
	}
//	tasklet_disable(&amp_aip_tasklet);

	return 0;
}

static struct file_operations snd_ops = {
	.open = snd_dev_open,
	.release = snd_dev_release,
	.unlocked_ioctl = snd_dev_ioctl_unlocked,
	.owner = THIS_MODULE,
};

struct amp_device_t snd_dev = {
	.dev_name = "amp_snd",
	.minor = AMP_SND_MINOR,
	.dev_init = snd_dev_init,
	.dev_exit = snd_dev_exit,
	.fops = &snd_ops,
};
