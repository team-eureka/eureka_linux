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
#include "api_avif_dhub.h"
#include "avif_dhub_config.h"
#include "shm_api.h"
#include "cc_msgq.h"
#include "cc_error.h"

#include "amp_driver.h"
#include "amp_memmap.h"
#include "vmeta_sched_driver.h"
#include "amp_dev_snd.h"

#ifdef CONFIG_BERLIN_FASTLOGO
#include "../fastlogo/fastlogo_driver.h"
#endif
/*******************************************************************************
  Module API defined
  */
// enable hrx intr hanlding
#define CONFIG_HRX_IOCTL_MSG			1
// set 1 to enable message by ioctl
// set 0 to enable ICC message queue
#define CONFIG_VPP_IOCTL_MSG			1
// when CONFIG_VPP_IOCTL_MSG is 1
// set 1 to use internal message queue between isr and ioctl function
// set 0 to use no queue
#define CONFIG_VPP_ISR_MSGQ				1

// only enable when VPP use ioctl
#if CONFIG_VPP_IOCTL_MSG
// set 1 to enable message by ioctl
// set 0 to enable ICC message queue
#define CONFIG_VIP_IOCTL_MSG			1
// when CONFIG_VIP_IOCTL_MSG is 1
// set 1 to use internal message queue between isr and ioctl function
// set 0 to use no queue
#define CONFIG_VIP_ISR_MSGQ				1
#endif

#define CONFIG_VDEC_IRQ_CHECKER	        1

#define AMP_DEVICE_NAME					"ampcore"
#define AMP_DEVICE_TAG						"[amp_driver] "
#define AMP_DEVICE_PATH					("/dev/" AMP_DEVICE_NAME)

#define AMP_DEVICE_PROCFILE_STATUS		"status"
#define AMP_DEVICE_PROCFILE_DETAIL		"detail"

#define VPP_CC_MSG_TYPE_VPP				0x00
#define VPP_CC_MSG_TYPE_CEC				0x01

#define CONFIG_APP_IOCTL_MSG				1

#define ADSP_ZSPINT2Soc_IRQ0				0

#define VPP_IOCTL_VBI_DMA_CFGQ			0xbeef0001
#define VPP_IOCTL_VBI_BCM_CFGQ			0xbeef0002
#define VPP_IOCTL_VDE_BCM_CFGQ			0xbeef0003
#define VPP_IOCTL_GET_MSG					0xbeef0004
#define VPP_IOCTL_START_BCM_TRANSACTION	0xbeef0005
#define VPP_IOCTL_BCM_SCHE_CMD			0xbeef0006
#define VPP_IOCTL_INTR_MSG				0xbeef0007
#define CEC_IOCTL_RX_MSG_BUF_MSG			0xbeef0008

#define VDEC_IOCTL_ENABLE_INT				0xbeef1001
#define AOUT_IOCTL_START_CMD				0xbeef2001
#define AIP_IOCTL_START_CMD				0xbeef2002
#define AIP_IOCTL_STOP_CMD				0xbeef2003
#define AIP_AVIF_IOCTL_START_CMD			0xbeef2004
#define AIP_AVIF_IOCTL_STOP_CMD			0xbeef2005
#define AIP_AVIF_IOCTL_SET_MODE			0xbeef2006
#define APP_IOCTL_INIT_CMD				0xbeef3001
#define APP_IOCTL_START_CMD				0xbeef3002
#define APP_IOCTL_GET_MSG_CMD				0xbeef3003

#define VIP_IOCTL_GET_MSG					0xbeef4001
#define VIP_IOCTL_VBI_BCM_CFGQ			0xbeef4002
#define VIP_IOCTL_SD_WRE_CFGQ				0xbeef4003
#define VIP_IOCTL_SD_RDE_CFGQ				0xbeef4004
#define VIP_IOCTL_SEND_MSG				0xbeef4005
#define VIP_IOCTL_VDE_BCM_CFGQ			0xbeef4006
#define VIP_IOCTL_INTR_MSG				0xbeef4007

#define AVIF_IOCTL_GET_MSG				0xbeef6001
#define AVIF_IOCTL_VBI_CFGQ				0xbeef6002
#define AVIF_IOCTL_SD_WRE_CFGQ			0xbeef6003
#define AVIF_IOCTL_SD_RDE_CFGQ			0xbeef6004
#define AVIF_IOCTL_SEND_MSG				0xbeef6005
#define AVIF_IOCTL_VDE_CFGQ				0xbeef6006
#define AVIF_IOCTL_INTR_MSG				0xbeef6007

#define AVIF_HRX_IOCTL_GET_MSG			0xbeef6050
#define AVIF_HRX_IOCTL_SEND_MSG			0xbeef6051
#define AVIF_HRX_DESTROY_ISR_TASK		1

#define HDMIRX_IOCTL_GET_MSG				0xbeef5001
#define HDMIRX_IOCTL_SEND_MSG				0xbeef5002

#define HRX_MSG_DESTROY_ISR_TASK			1
//CEC MACRO
#define SM_APB_ICTL1_BASE					0xf7fce000
#define SM_APB_GPIO0_BASE					0xf7fcc000
#define SM_APB_GPIO1_BASE					0xf7fc5000
#define APB_GPIO_PORTA_EOI				0x4c
#define APB_GPIO_INTSTATUS				0x40
#define APB_ICTL_IRQ_STATUS_L				0x20
#define BE_CEC_INTR_TX_SFT_FAIL			0x2008	// puneet
#define BE_CEC_INTR_TX_FAIL				0x200F
#define BE_CEC_INTR_TX_COMPLETE			0x0010
#define BE_CEC_INTR_RX_COMPLETE			0x0020
#define BE_CEC_INTR_RX_FAIL				0x00C0
#define SOC_SM_CEC_BASE					0xF7FE1000
#define CEC_INTR_STATUS0_REG_ADDR		0x0058
#define CEC_INTR_STATUS1_REG_ADDR		0x0059
#define CEC_INTR_ENABLE0_REG_ADDR		0x0048
#define CEC_INTR_ENABLE1_REG_ADDR		0x0049
#define CEC_RDY_ADDR						0x0008
#define CEC_RX_BUF_READ_REG_ADDR			0x0068
#define CEC_RX_EOM_READ_REG_ADDR			0x0069
#define CEC_TOGGLE_FOR_READ_REG_ADDR		0x0004
#define CEC_RX_RDY_ADDR					0x000c	//01
#define CEC_RX_FIFO_DPTR					0x0087

// HDMI RX Macros
#define SOC_VPP_BASE					0xf7f60000

#define HDMI_RX_BASE					0x3900
#define HDMIRX_INTR_EN_0				HDMI_RX_BASE + 0x0160
#define HDMIRX_INTR_EN_1				HDMI_RX_BASE + 0x0161
#define HDMIRX_INTR_EN_2				HDMI_RX_BASE + 0x0162
#define HDMIRX_INTR_EN_3				HDMI_RX_BASE + 0x0163
#define HDMIRX_INTR_STATUS_0			HDMI_RX_BASE + 0x0164
#define HDMIRX_INTR_STATUS_1			HDMI_RX_BASE + 0x0165
#define HDMIRX_INTR_STATUS_2			HDMI_RX_BASE + 0x0166
#define HDMIRX_INTR_STATUS_3			HDMI_RX_BASE + 0x0167
#define HDMIRX_INTR_CLR_0				HDMI_RX_BASE + 0x0168
#define HDMIRX_INTR_CLR_1				HDMI_RX_BASE + 0x0169
#define HDMIRX_INTR_CLR_2				HDMI_RX_BASE + 0x016A
#define HDMIRX_INTR_CLR_3				HDMI_RX_BASE + 0x016B
#define HDMIRX_PHY_CAL_TRIG			HDMI_RX_BASE + 0x0461	//new register
#define HDMIRX_PHY_PM_TRIG			HDMI_RX_BASE + 0x0462	//new register

#define HDMI_RX_INT_ACR_N				0x01
#define HDMI_RX_INT_ACR_CTS			0x02
#define HDMI_RX_INT_GCP_AVMUTE		0x04
#define HDMI_RX_INT_GCP_COLOR_DEPTH	0x08
#define HDMI_RX_INT_GCP_PHASE			0x10
#define HDMI_RX_INT_ACP_PKT			0x20
#define HDMI_RX_INT_ISRC1_PKT			0x40
#define HDMI_RX_INT_ISRC2_PKT			0x80
#define HDMI_RX_INT_ALL_INTR0			0xFF

#define HDMI_RX_INT_GAMUT_PKT			0x01
#define HDMI_RX_INT_VENDOR_PKT		0x02
#define HDMI_RX_INT_AVI_INFO			0x04
#define HDMI_RX_INT_SPD_INFO			0x08
#define HDMI_RX_INT_AUD_INFO			0x10
#define HDMI_RX_INT_MPEG_INFO			0x20
#define HDMI_RX_INT_CHNL_STATUS		0x40
#define HDMI_RX_INT_TMDS_MODE			0x80
#define HDMI_RX_INT_ALL_INTR1			0xFF

#define HDMI_RX_INT_AUTH_STARTED		0x01
#define HDMI_RX_INT_AUTH_COMPLETE	0x02
#define HDMI_RX_INT_VSYNC				0x04	// new
#define HDMI_RX_INT_VSI_STOP			0x10	// new
#define HDMI_RX_INT_SYNC_DET			0x20
#define HDMI_RX_INT_VRES_CHG			0x40
#define HDMI_RX_INT_HRES_CHG			0x80
#define HDMI_RX_INT_ALL_INTR2			0xFF

#define HDMI_RX_INT_5V_PWR			0x01
#define HDMI_RX_INT_CLOCK_CHANGE		0x02
#define HDMI_RX_INT_EDID				0x04
#define HDMI_RX_INT_ALL_INTR3			0xFF

#define HDMIRX_INTR_SYNC				0x01
#define HDMIRX_INTR_HDCP				0x02
#define HDMIRX_INTR_EDID				0x03
#define HDMIRX_INTR_PKT				0x04
#define HDMIRX_INTR_AVMUTE			0x05
#define HDMIRX_INTR_TMDS				0x06
#define HDMIRX_INTR_CHNL_STS			0x07
#define HDMIRX_INTR_CLR_DEPTH			0x08
#define HDMIRX_INTR_VSI_STOP			0x09
#define HDMIRX_INTR_GMD_PKT			0x0A

#ifdef BERLIN_BOOTLOGO
#define MEMMAP_AVIO_BCM_REG_BASE			0xF7B50000
#define RA_AVIO_BCM_AUTOPUSH				0x0198
#endif


#define SOC_AVIF_BASE						0xF7980000
#define AVIF_HRX_BASE_OFFSET				0x800
#define CPU_SS_MBASE_ADDR					0x960

#define CPU_INTR_MASK0_EXTREG_ADDR		SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x003D)<<2)
#define CPU_INTR_MASK1_EXTREG_ADDR		SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x0020)<<2)
#define CPU_INTR_MASK2_EXTREG_ADDR		SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x0021)<<2)
#define CPU_INTR_MASK3_EXTREG_ADDR		SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x0022)<<2)
#define CPU_INTR_MASK4_EXTREG_ADDR		SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x0023)<<2)

#define CPU_INTR_MASK0_STATUS_EXTREG_ADDR SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x003E)<<2)
#define CPU_INTR_MASK1_STATUS_EXTREG_ADDR SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x0028)<<2)
#define CPU_INTR_MASK2_STATUS_EXTREG_ADDR SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x0029)<<2)
#define CPU_INTR_MASK3_STATUS_EXTREG_ADDR SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x002A)<<2)
#define CPU_INTR_MASK4_STATUS_EXTREG_ADDR SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x002B)<<2)

#define CPU_INTR_CLR0_EXTREG_ADDR		SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x004D)<<2)
#define CPU_INTR_CLR1_EXTREG_ADDR		SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x002C)<<2)
#define CPU_INTR_CLR2_EXTREG_ADDR		SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x002D)<<2)
#define CPU_INTR_CLR3_EXTREG_ADDR		SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x002E)<<2)
#define CPU_INTR_CLR4_EXTREG_ADDR		SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x002F)<<2)

#define CPU_INTR_RAW_STATUS0_EXTREG_ADDR	SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x003C)<<2)
#define CPU_INTR_RAW_STATUS1_EXTREG_ADDR	SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x0024)<<2)
#define CPU_INTR_RAW_STATUS2_EXTREG_ADDR	SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x0025)<<2)
#define CPU_INTR_RAW_STATUS3_EXTREG_ADDR	SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x0026)<<2)
#define CPU_INTR_RAW_STATUS4_EXTREG_ADDR	SOC_AVIF_BASE+((CPU_SS_MBASE_ADDR + 0x0027)<<2)
#define CPU_INTCPU_2_EXTCPU_MASK0INTR_ADDR SOC_AVIF_BASE+(CPU_SS_MBASE_ADDR + 0x0048)*4
#define CPU_INTCPU_2_EXTCPU_INTR0_ADDR SOC_AVIF_BASE+(CPU_SS_MBASE_ADDR + 0x003F)*4
#define CPU_INTCPU_2_EXTCPU_INTR1_ADDR SOC_AVIF_BASE+(CPU_SS_MBASE_ADDR + 0x0040)*4
#define CPU_INTCPU_2_EXTCPU_INTR2_ADDR SOC_AVIF_BASE+(CPU_SS_MBASE_ADDR + 0x0041)*4
#define CPU_INTCPU_2_EXTCPU_INTR3_ADDR SOC_AVIF_BASE+(CPU_SS_MBASE_ADDR + 0x0042)*4
#define CPU_INTCPU_2_EXTCPU_INTR4_ADDR SOC_AVIF_BASE+(CPU_SS_MBASE_ADDR + 0x0043)*4
#define CPU_INTCPU_2_EXTCPU_INTR5_ADDR SOC_AVIF_BASE+(CPU_SS_MBASE_ADDR + 0x0044)*4
#define CPU_INTCPU_2_EXTCPU_INTR6_ADDR SOC_AVIF_BASE+(CPU_SS_MBASE_ADDR + 0x0045)*4
#define CPU_INTCPU_2_EXTCPU_INTR7_ADDR SOC_AVIF_BASE+(CPU_SS_MBASE_ADDR + 0x0046)*4

//intr0 reg
#define AVIF_INTR_TIMER1				 0x01
#define AVIF_INTR_TIMER0				0x02
#define AVIF_INTR_PIP_VDE				0x04
#define AVIF_INTR_MAIN_VDE			0x08
#define AVIF_INTR_VGA_SYNC_MON		0x10
#define AVIF_INTR_VGA_DI				0x20
#define AVIF_INTR_DELAYED_MAIN_VDE	0x40
//intr1 reg
#define AVIF_INTR_DELAYED_PIP_VDE	0x20

#define AVIF_HRX_BASE					0x0C00
#define AVIF_HRX_INTR_EN_0			SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x0160)<<2)
#define AVIF_HRX_INTR_EN_1			SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x0161)<<2)
#define AVIF_HRX_INTR_EN_2			SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x0162)<<2)
#define AVIF_HRX_INTR_EN_3			SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x0163)<<2)
#define AVIF_HRX_INTR_STATUS_0		SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x0164)<<2)
#define AVIF_HRX_INTR_STATUS_1		SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x0165)<<2)
#define AVIF_HRX_INTR_STATUS_2		SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x0166)<<2)
#define AVIF_HRX_INTR_STATUS_3		SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x0167)<<2)
#define AVIF_HRX_INTR_CLR_0			SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x0168)<<2)
#define AVIF_HRX_INTR_CLR_1			SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x0169)<<2)
#define AVIF_HRX_INTR_CLR_2			SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x016A)<<2)
#define AVIF_HRX_INTR_CLR_3			SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x016B)<<2)
#define AVIF_HRX_PHY_CAL_TRIG			SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x0461)<<2)   //new register
#define AVIF_HRX_PHY_PM_TRIG			SOC_AVIF_BASE+((AVIF_HRX_BASE + 0x0462)<<2)   //new register




static int amp_driver_open(struct inode *inode, struct file *filp);
static int amp_driver_release(struct inode *inode, struct file *filp);
static long amp_driver_ioctl_unlocked(struct file *filp, unsigned int cmd,
					unsigned long arg);

/*******************************************************************************
  Macro Defined
  */
#define ENABLE_DEBUG
#ifdef ENABLE_DEBUG
#define amp_debug(...)		printk(KERN_DEBUG AMP_DEVICE_TAG __VA_ARGS__)
#else
#define amp_debug(...)
#endif

#define amp_trace(...)		printk(KERN_WARNING AMP_DEVICE_TAG __VA_ARGS__)
#define amp_error(...)		printk(KERN_ERR AMP_DEVICE_TAG __VA_ARGS__)

//#define VPP_DBG

#define DEBUG_TIMER_VALUE				(0xFFFFFFFF)

/*******************************************************************************
  Module Variable
  */

#if CONFIG_VDEC_IRQ_CHECKER
static unsigned int vdec_int_cnt;
static unsigned int vdec_enable_int_cnt;
#endif

int amp_irqs[IRQ_AMP_MAX];

int avioDhub_channel_num = 0x7;
int berlin_chip_revision = 0;

int amp_major;
static struct cdev amp_dev;
static struct class *amp_dev_class;

static struct file_operations amp_ops = {
    .open = amp_driver_open,
    .release = amp_driver_release,
    .unlocked_ioctl = amp_driver_ioctl_unlocked,
    .owner = THIS_MODULE,
};

typedef struct VPP_DMA_INFO_T {
    UINT32 DmaAddr;
    UINT32 DmaLen;
    UINT32 cpcbID;
} VPP_DMA_INFO;

typedef struct VPP_CEC_RX_MSG_BUF_T {
    UINT8 buf[16];
    UINT8 len;
} VPP_CEC_RX_MSG_BUF;

static VPP_DMA_INFO dma_info[3];
static VPP_DMA_INFO vbi_bcm_info[3];
static VPP_DMA_INFO vde_bcm_info[3];
static unsigned int bcm_sche_cmd[6][2];
static VPP_CEC_RX_MSG_BUF rx_buf;

typedef struct VIP_DMA_INFO_T {
    UINT32 DmaAddr;
    UINT32 DmaLen;
} VIP_DMA_INFO;

static VIP_DMA_INFO vip_dma_info;
static VIP_DMA_INFO vip_vbi_info;
static VIP_DMA_INFO vip_sd_wr_info;
static VIP_DMA_INFO vip_sd_rd_info;

/////////////////////////////////////////NEW_ISR related
#define NEW_ISR
//This array stores all the VIP intrs enable/disable status
#define MAX_INTR_NUM 0x20
static UINT32 vip_intr_status[MAX_INTR_NUM];
static UINT32 vpp_intr_status[MAX_INTR_NUM];

typedef struct INTR_MSG_T {
    UINT32 DhubSemMap;
    UINT32 Enable;
} INTR_MSG;
/////////////////////////////////////////End of NEW_ISR

static void amp_vip_do_tasklet(unsigned long);
DECLARE_TASKLET_DISABLED(amp_vip_tasklet, amp_vip_do_tasklet, 0);

#define VIP_MSG_DESTROY_ISR_TASK   1


#define AVIF_MSG_DESTROY_ISR_TASK   1
typedef struct AVIF_DMA_INFO_T {
	UINT32 DmaAddr;
	UINT32 DmaLen;
} AVIF_DMA_INFO;

static int IsPIPAudio;
//This array stores all the AVIF intrs enable/disable status
static UINT32 avif_intr_status[MAX_INTR_NUM];
static void amp_avif_do_tasklet(unsigned long);
DECLARE_TASKLET_DISABLED(amp_avif_tasklet, amp_avif_do_tasklet, 0);

static AOUT_PATH_CMD_FIFO *p_ma_fifo;
static AOUT_PATH_CMD_FIFO *p_sa_fifo;
static AOUT_PATH_CMD_FIFO *p_spdif_fifo;
static AOUT_PATH_CMD_FIFO *p_hdmi_fifo;
static HWAPP_CMD_FIFO *p_app_fifo;
static AIP_DMA_CMD_FIFO *p_aip_fifo = NULL;

static int aip_i2s_pair;

static struct proc_dir_entry *amp_driver_procdir;
static int vpp_cpcb0_vbi_int_cnt;
//static int vpp_cpcb2_vbi_int_cnt = 0;

static DEFINE_SPINLOCK(bcm_spinlock);
static DEFINE_SPINLOCK(aout_spinlock);
static DEFINE_SPINLOCK(app_spinlock);
static DEFINE_SPINLOCK(aip_spinlock);
static DEFINE_SPINLOCK(msgQ_spinlock);

static int dma_proc[3000];

static int dma_cnt;

static void amp_vpp_do_tasklet(unsigned long);
static void amp_vpp_cec_do_tasklet(unsigned long);  // cec tasklet added
static void amp_vdec_do_tasklet(unsigned long);

static void amp_ma_do_tasklet(unsigned long);
//static void amp_aip_do_tasklet(unsigned long);
static void amp_hdmi_do_tasklet(unsigned long);
static void amp_app_do_tasklet(unsigned long);
static void amp_zsp_do_tasklet(unsigned long);
#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
static void amp_sa_do_tasklet(unsigned long);
static void amp_pg_dhub_done_tasklet(unsigned long);
static void amp_spdif_do_tasklet(unsigned long);
//static void amp_rle_do_err_tasklet(unsigned long);
//static void amp_rle_do_done_tasklet(unsigned long);
#endif
static void aout_resume_cmd(int path_id);
static void aip_resume_cmd(void);
static void amp_aip_avif_do_tasklet(unsigned long);
static void aip_avif_resume_cmd(void);

static DECLARE_TASKLET_DISABLED(amp_vpp_tasklet, amp_vpp_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_vpp_cec_tasklet, amp_vpp_cec_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_vdec_tasklet, amp_vdec_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_ma_tasklet, amp_ma_do_tasklet, 0);
//static DECLARE_TASKLET_DISABLED(amp_aip_tasklet, amp_aip_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_hdmi_tasklet, amp_hdmi_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_app_tasklet, amp_app_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_zsp_tasklet, amp_zsp_do_tasklet, 0);
#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
static DECLARE_TASKLET_DISABLED(amp_aip_avif_tasklet, amp_aip_avif_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_spdif_tasklet, amp_spdif_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_sa_tasklet, amp_sa_do_tasklet, 0);
static DECLARE_TASKLET_DISABLED(amp_pg_done_tasklet,
            amp_pg_dhub_done_tasklet, 0);
//static DECLARE_TASKLET_DISABLED(amp_rle_err_tasklet, amp_rle_do_err_tasklet, 0);
//static DECLARE_TASKLET_DISABLED(amp_rle_done_tasklet, amp_rle_do_done_tasklet, 0);
#endif

#if CONFIG_VPP_IOCTL_MSG
static UINT vpp_intr_timestamp;
static DEFINE_SEMAPHORE(vpp_sem);
#endif

#if CONFIG_VIP_IOCTL_MSG
static UINT vip_intr_timestamp;
static DEFINE_SEMAPHORE(vip_sem);
#endif

#if CONFIG_VIP_IOCTL_MSG
static UINT hrx_intr_timestamp;
static DEFINE_SEMAPHORE(hrx_sem);
#endif

static DEFINE_SEMAPHORE(avif_sem);
static DEFINE_SEMAPHORE(avif_hrx_sem);
#if defined (CONFIG_APP_IOCTL_MSG)
static UINT app_intr_timestamp;
static DEFINE_SEMAPHORE(app_sem);
#endif

#if defined (CONFIG_VPP_ISR_MSGQ) || defined (CONFIG_APP_IOCTL_MSG)
struct amp_message_queue;
typedef struct amp_message_queue {
	UINT q_length;
	UINT rd_number;
	UINT wr_number;
	MV_CC_MSG_t *pMsg;

	 HRESULT(*Add) (struct amp_message_queue * pMsgQ, MV_CC_MSG_t * pMsg);
	 HRESULT(*ReadTry) (struct amp_message_queue * pMsgQ,
			    MV_CC_MSG_t * pMsg);
	 HRESULT(*ReadFin) (struct amp_message_queue * pMsgQ);
	 HRESULT(*Destroy) (struct amp_message_queue * pMsgQ);

} PEMsgQ_t;

#if defined (CONFIG_VPP_ISR_MSGQ)
#define VPP_ISR_MSGQ_SIZE			8

static PEMsgQ_t hPEMsgQ;
#endif

#if defined (CONFIG_APP_IOCTL_MSG)
#define APP_ISR_MSGQ_SIZE			16

static PEMsgQ_t hAPPMsgQ;
#endif

static HRESULT PEMsgQ_Add(PEMsgQ_t * pMsgQ, MV_CC_MSG_t * pMsg)
{
	INT wr_offset;

	if (NULL == pMsgQ->pMsg || pMsg == NULL)
		return S_FALSE;

	wr_offset = pMsgQ->wr_number - pMsgQ->rd_number;

	if (wr_offset == -1 || wr_offset == (pMsgQ->q_length - 2)) {
		return S_FALSE;
	} else {
		memcpy((CHAR *) & pMsgQ->pMsg[pMsgQ->wr_number], (CHAR *) pMsg,
		       sizeof(MV_CC_MSG_t));
		pMsgQ->wr_number++;
		pMsgQ->wr_number %= pMsgQ->q_length;
	}

	return S_OK;
}

static HRESULT PEMsgQ_ReadTry(PEMsgQ_t * pMsgQ, MV_CC_MSG_t * pMsg)
{
	INT rd_offset;

	if (NULL == pMsgQ->pMsg || pMsg == NULL)
		return S_FALSE;

	rd_offset = pMsgQ->rd_number - pMsgQ->wr_number;

	if (rd_offset != 0) {
		memcpy((CHAR *) pMsg, (CHAR *) & pMsgQ->pMsg[pMsgQ->rd_number],
		       sizeof(MV_CC_MSG_t));
		return S_OK;
	} else {
	    amp_trace("VIP read message queue failed r: %d w: %d\n", pMsgQ->rd_number, pMsgQ->wr_number);
		return S_FALSE;
	}
}

static HRESULT PEMsgQ_ReadFinish(PEMsgQ_t * pMsgQ)
{
	INT rd_offset;

	rd_offset = pMsgQ->rd_number - pMsgQ->wr_number;

	if (rd_offset != 0) {
		pMsgQ->rd_number++;
		pMsgQ->rd_number %= pMsgQ->q_length;

		return S_OK;
	} else {
		return S_FALSE;
	}
}

#define	MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static int PEMsgQ_Dequeue(PEMsgQ_t * pMsgQ, int cnt)
{
	INT fullness;

	if (cnt <= 0)
		return -1;

	fullness = pMsgQ->wr_number - pMsgQ->rd_number;
	if (fullness < 0) {
		fullness += pMsgQ->q_length;
	}

	cnt = MIN(fullness, cnt);
	if (cnt) {
		pMsgQ->rd_number += cnt;
		if (pMsgQ->rd_number >= pMsgQ->q_length)
			pMsgQ->rd_number -= pMsgQ->q_length;
	}
	return cnt;
}

static int PEMsgQ_DequeueRead(PEMsgQ_t * pMsgQ, MV_CC_MSG_t * pMsg)
{
	INT fullness;

	if (NULL == pMsgQ->pMsg || pMsg == NULL)
		return S_FALSE;

	fullness = pMsgQ->wr_number - pMsgQ->rd_number;
	if (fullness < 0) {
		fullness += pMsgQ->q_length;
	}

	if (fullness) {
		if (pMsg)
			memcpy((void *)pMsg,
			       (void *)&pMsgQ->pMsg[pMsgQ->rd_number],
			       sizeof(MV_CC_MSG_t));

		pMsgQ->rd_number++;
		if (pMsgQ->rd_number >= pMsgQ->q_length)
			pMsgQ->rd_number -= pMsgQ->q_length;

		return 1;
	}

	return 0;
}

static int PEMsgQ_Fullness(PEMsgQ_t * pMsgQ)
{
	INT fullness;

	fullness = pMsgQ->wr_number - pMsgQ->rd_number;
	if (fullness < 0) {
		fullness += pMsgQ->q_length;
	}

	return fullness;
}

static HRESULT PEMsgQ_Destroy(PEMsgQ_t * pMsgQ)
{
	if (pMsgQ == NULL) {
		return E_FAIL;
	}

	if (pMsgQ->pMsg)
		kfree(pMsgQ->pMsg);

	return S_OK;
}

static HRESULT PEMsgQ_Init(PEMsgQ_t * pPEMsgQ, UINT q_length)
{
	pPEMsgQ->q_length = q_length;
	pPEMsgQ->rd_number = pPEMsgQ->wr_number = 0;
	pPEMsgQ->pMsg =
	    (MV_CC_MSG_t *) kmalloc(sizeof(MV_CC_MSG_t) * q_length, GFP_ATOMIC);

	if (pPEMsgQ->pMsg == NULL) {
		return E_OUTOFMEMORY;
	}

	pPEMsgQ->Destroy = PEMsgQ_Destroy;
	pPEMsgQ->Add = PEMsgQ_Add;
	pPEMsgQ->ReadTry = PEMsgQ_ReadTry;
	pPEMsgQ->ReadFin = PEMsgQ_ReadFinish;
//    pPEMsgQ->Fullness =   PEMsgQ_Fullness;

	return S_OK;
}

#endif

#if CONFIG_VIP_ISR_MSGQ
// MSGQ for VIP
#define VIP_ISR_MSGQ_SIZE           64
static PEMsgQ_t hPEVIPMsgQ;
#endif
#define HDMIRX_ISR_MSGQ_SIZE	64
static PEMsgQ_t hPEHRXMsgQ;

static void amp_vip_do_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0 };

	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_VIP, &msg);
}

// MSGQ for AVIF
#define AVIF_ISR_MSGQ_SIZE		64
#define AVIF_HRX_ISR_MSGQ_SIZE	64
static PEMsgQ_t hPEAVIFMsgQ;
static PEMsgQ_t hPEAVIFHRXMsgQ;
static void amp_avif_do_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0 };

	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_VIP, &msg);
	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_AVIF, &msg);
}

static void amp_vpp_do_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0 };
	UINT32 val;

	msg.m_MsgID = VPP_CC_MSG_TYPE_VPP;

	msg.m_Param1 = unused;

	GA_REG_WORD32_READ(0xf7e82C00 + 0x04 + 7 * 0x14, &val);
	msg.m_Param2 = DEBUG_TIMER_VALUE - val;

	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_VPP, &msg);
}

static void amp_vpp_cec_do_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0 };
	UINT32 val;

	msg.m_MsgID = VPP_CC_MSG_TYPE_CEC;

	msg.m_Param1 = unused;

	GA_REG_WORD32_READ(0xf7e82C00 + 0x04 + 7 * 0x14, &val);
	msg.m_Param2 = DEBUG_TIMER_VALUE - val;

	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_VPP, &msg);
}

static void amp_vdec_do_tasklet(unsigned long unused)
{
	MV_CC_MSG_t msg = { 0 };

#if CONFIG_VDEC_IRQ_CHECKER
	msg.m_Param1 = vdec_int_cnt;
#endif
	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_VDEC, &msg);
}

static void MV_VPP_action(struct softirq_action *h)
{
	MV_CC_MSG_t msg = { 0, };

	MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_VPP, &msg);
//    amp_trace("ISR conext irq:%d\n", vpp_cpcb0_vbi_int_cnt);
}

static int vbi_bcm_cmd_fullcnt;
static int vde_bcm_cmd_fullcnt;

static void start_vbi_bcm_transaction(int cpcbID)
{
    unsigned int bcm_sched_cmd[2];

    if (vbi_bcm_info[cpcbID].DmaLen) {
    dhub_channel_generate_cmd(&(VPP_dhubHandle.dhub),
                  avioDhubChMap_vpp_BCM_R_auto,
                  (INT) vbi_bcm_info[cpcbID].DmaAddr,
                  (INT) vbi_bcm_info[cpcbID].DmaLen * 8,
                  0, 0, 0, 1, bcm_sched_cmd);
    while (!BCM_SCHED_PushCmd(BCM_SCHED_Q12, bcm_sched_cmd, NULL)) {
        vbi_bcm_cmd_fullcnt++;
    }
    }
    /* invalid vbi_bcm_info */
    vbi_bcm_info[cpcbID].DmaLen = 0;
}

static void start_vde_bcm_transaction(int cpcbID)
{
    unsigned int bcm_sched_cmd[2];

    if (vde_bcm_info[cpcbID].DmaLen) {
    dhub_channel_generate_cmd(&(VPP_dhubHandle.dhub),
                  avioDhubChMap_vpp_BCM_R_auto,
                  (INT) vde_bcm_info[cpcbID].DmaAddr,
                  (INT) vde_bcm_info[cpcbID].DmaLen * 8,
                  0, 0, 0, 1, bcm_sched_cmd);
    while (!BCM_SCHED_PushCmd(BCM_SCHED_Q12, bcm_sched_cmd, NULL)) {
        vde_bcm_cmd_fullcnt++;
    }
    }
    /* invalid vde_bcm_info */
    vde_bcm_info[cpcbID].DmaLen = 0;
}

static void start_vbi_dma_transaction(int cpcbID)
{
    UINT32 cnt;
    UINT32 *ptr;

    ptr = (UINT32 *) dma_info[cpcbID].DmaAddr;
    for (cnt = 0; cnt < dma_info[cpcbID].DmaLen; cnt++) {
    *((volatile int *)*(ptr + 1)) = *ptr;
    ptr += 2;
    }
    /* invalid dma_info */
    dma_info[cpcbID].DmaLen = 0;
}

static void send_bcm_sche_cmd(int q_id)
{
    if ((bcm_sche_cmd[q_id][0] != 0) && (bcm_sche_cmd[q_id][1] != 0))
    BCM_SCHED_PushCmd(q_id, bcm_sche_cmd[q_id], NULL);
}

static void amp_ma_do_tasklet(unsigned long unused)
{
    MV_CC_MSG_t msg = { 0, };

    msg.m_MsgID = 1 << avioDhubChMap_ag_MA0_R_auto;
    MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_AUD, &msg);

}

static void amp_sa_do_tasklet(unsigned long unused)
{
    MV_CC_MSG_t msg = { 0, };
#if (BERLIN_CHIP_VERSION != BERLIN_BG2_CD) && (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)

    msg.m_MsgID = 1 << avioDhubChMap_ag_SA_R;
    MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_AUD, &msg);
#endif
}

static void amp_spdif_do_tasklet(unsigned long unused)
{
    MV_CC_MSG_t msg = { 0, };
#if (BERLIN_CHIP_VERSION != BERLIN_BG2_CD) && (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)

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

static void amp_aip_avif_do_tasklet(unsigned long unused)
{
    MV_CC_MSG_t msg = { 0, };
    if(IsPIPAudio)
        msg.m_MsgID = 1 << avif_dhub_config_ChMap_avif_AUD_WR0_PIP;
    else
        msg.m_MsgID = 1 << avif_dhub_config_ChMap_avif_AUD_WR0_MAIN;
    MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_AIP, &msg);
}

static void amp_hdmi_do_tasklet(unsigned long unused)
{
    MV_CC_MSG_t msg = { 0, };

    msg.m_MsgID = 1 << avioDhubChMap_vpp_HDMI_R_auto;
    MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_AUD, &msg);
}

static void amp_app_do_tasklet(unsigned long unused)
{
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
   if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
   {
       MV_CC_MSG_t msg = { 0, };

       msg.m_MsgID = 1 << avioDhubSemMap_ag_app_intr2;
       MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_HWAPP, &msg);
    }
}

static void amp_zsp_do_tasklet(unsigned long unused)
{
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
   if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
   {
        MV_CC_MSG_t msg = { 0, };

        msg.m_MsgID = 1 << ADSP_ZSPINT2Soc_IRQ0;
        MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_ZSP, &msg);
   }
}

#if (BERLIN_CHIP_VERSION != BERLIN_BG2_CD) && (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)
static void amp_pg_dhub_done_tasklet(unsigned long unused)
{
    MV_CC_MSG_t msg = { 0, };

#if (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)
    msg.m_MsgID = 1 << avioDhubChMap_ag_PG_ENG_W;
    MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_RLE, &msg);
#endif
}

static void amp_rle_do_err_tasklet(unsigned long unused)
{
    MV_CC_MSG_t msg = { 0, };

#if (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)
    msg.m_MsgID = 1 << avioDhubSemMap_ag_spu_intr0;
    MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_RLE, &msg);
#endif
}

static void amp_rle_do_done_tasklet(unsigned long unused)
{
    MV_CC_MSG_t msg = { 0, };

#if (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)
    msg.m_MsgID = 1 << avioDhubSemMap_ag_spu_intr1;
    MV_CC_MsgQ_PostMsgByID(AMP_MODULE_MSG_ID_RLE, &msg);
#endif
}
#endif

static irqreturn_t amp_devices_vpp_cec_isr(int irq, void *dev_id)
{
    UINT16 value = 0;
    UINT16 reg = 0;
    INT intr;
    INT i;
    INT dptr_len = 0;
    HRESULT ret = S_OK;
#if CONFIG_VPP_IOCTL_MSG
    GA_REG_WORD32_READ(0xf7e82C00 + 0x04 + 7 * 0x14, &vpp_intr_timestamp);
    vpp_intr_timestamp = DEBUG_TIMER_VALUE - vpp_intr_timestamp;
#endif

    // Read CEC status register
    GA_REG_BYTE_READ(SOC_SM_CEC_BASE +
         (CEC_INTR_STATUS0_REG_ADDR << 2), &value);
    reg = (UINT16) value;
    GA_REG_BYTE_READ(SOC_SM_CEC_BASE +
         (CEC_INTR_STATUS1_REG_ADDR << 2), &value);
    reg |= ((UINT16) value << 8);
    // Clear CEC interrupt
    if (reg & BE_CEC_INTR_TX_FAIL) {
    intr = BE_CEC_INTR_TX_FAIL;
    GA_REG_BYTE_WRITE(SOC_SM_CEC_BASE + (CEC_RDY_ADDR << 2), 0x0);
    GA_REG_BYTE_READ(SOC_SM_CEC_BASE +
             (CEC_INTR_ENABLE0_REG_ADDR << 2), &value);
    value &= ~(intr & 0x00ff);
    GA_REG_BYTE_WRITE(SOC_SM_CEC_BASE +
              (CEC_INTR_ENABLE0_REG_ADDR << 2), value);
    }
    if (reg & BE_CEC_INTR_TX_COMPLETE) {
    intr = BE_CEC_INTR_TX_COMPLETE;
    GA_REG_BYTE_READ(SOC_SM_CEC_BASE +
             (CEC_INTR_ENABLE0_REG_ADDR << 2), &value);
    value &= ~(intr & 0x00ff);
    GA_REG_BYTE_WRITE(SOC_SM_CEC_BASE +
              (CEC_INTR_ENABLE0_REG_ADDR << 2), value);
    }
    if (reg & BE_CEC_INTR_RX_FAIL) {
    intr = BE_CEC_INTR_RX_FAIL;
    GA_REG_BYTE_READ(SOC_SM_CEC_BASE +
             (CEC_INTR_ENABLE0_REG_ADDR << 2), &value);
    value &= ~(intr & 0x00ff);
    GA_REG_BYTE_WRITE(SOC_SM_CEC_BASE +
              (CEC_INTR_ENABLE0_REG_ADDR << 2), value);
    }
    if (reg & BE_CEC_INTR_RX_COMPLETE) {
    intr = BE_CEC_INTR_RX_COMPLETE;
    GA_REG_BYTE_READ(SOC_SM_CEC_BASE +
             (CEC_INTR_ENABLE0_REG_ADDR << 2), &value);
    value &= ~(intr & 0x00ff);
    GA_REG_BYTE_WRITE(SOC_SM_CEC_BASE +
              (CEC_INTR_ENABLE0_REG_ADDR << 2), value);
    // read cec mesg from rx buffer
    GA_REG_BYTE_READ(SOC_SM_CEC_BASE + (CEC_RX_FIFO_DPTR << 2),
             &dptr_len);
    rx_buf.len = dptr_len;
    for (i = 0; i < dptr_len; i++) {
        GA_REG_BYTE_READ(SOC_SM_CEC_BASE +
                 (CEC_RX_BUF_READ_REG_ADDR << 2),
                 &rx_buf.buf[i]);
        GA_REG_BYTE_WRITE(SOC_SM_CEC_BASE +
                  (CEC_TOGGLE_FOR_READ_REG_ADDR << 2),
                  0x01);
    }
    GA_REG_BYTE_WRITE(SOC_SM_CEC_BASE + (CEC_RX_RDY_ADDR << 2),
              0x00);
    GA_REG_BYTE_WRITE(SOC_SM_CEC_BASE + (CEC_RX_RDY_ADDR << 2),
              0x01);
    GA_REG_BYTE_READ(SOC_SM_CEC_BASE +
             (CEC_INTR_ENABLE0_REG_ADDR << 2), &value);
    value |= (intr & 0x00ff);
    GA_REG_BYTE_WRITE(SOC_SM_CEC_BASE +
              (CEC_INTR_ENABLE0_REG_ADDR << 2), value);
    }
    // schedule tasklet with intr status as param
#if CONFIG_VPP_IOCTL_MSG
#if CONFIG_VPP_ISR_MSGQ
    {
    MV_CC_MSG_t msg =
        { VPP_CC_MSG_TYPE_CEC, reg, vpp_intr_timestamp };
    spin_lock(&msgQ_spinlock);
    ret = PEMsgQ_Add(&hPEMsgQ, &msg);
    spin_unlock(&msgQ_spinlock);
    if (ret != S_OK) {
        return IRQ_HANDLED;
    }

    }
#else
    vpp_instat = reg;
#endif
    up(&vpp_sem);
#else
    amp_vpp_cec_tasklet.data = reg; // bug fix puneet
    tasklet_hi_schedule(&amp_vpp_cec_tasklet);
#endif

    //amp_vpp_cec_tasklet.data = reg;
    //tasklet_hi_schedule(&amp_vpp_cec_tasklet);
    return IRQ_HANDLED;
}

#ifdef CONFIG_IRQ_LATENCY_PROFILE

typedef struct amp_irq_profiler {
    unsigned long long vppCPCB0_intr_curr;
    unsigned long long vppCPCB0_intr_last;
    unsigned long long vpp_task_sched_last;
    unsigned long long vpp_isr_start;

    unsigned long long vpp_isr_end;
    unsigned long vpp_isr_time_last;

    unsigned long vpp_isr_time_max;
    unsigned long vpp_isr_instat_max;

    INT vpp_isr_last_instat;

} amp_irq_profiler_t;

static amp_irq_profiler_t amp_irq_profiler;

#endif

static atomic_t vpp_isr_msg_err_cnt = ATOMIC_INIT(0);
static atomic_t vip_isr_msg_err_cnt = ATOMIC_INIT(0);
static atomic_t avif_isr_msg_err_cnt = ATOMIC_INIT(0);

static int isAmpReleased = 0;

static irqreturn_t amp_devices_vpp_isr(int irq, void *dev_id)
{
	INT instat;
	HDL_semaphore *pSemHandle;
	INT vpp_intr = 0;
	INT instat_used = 0;
	HRESULT ret = S_OK;
	/* disable interrupt */
#if CONFIG_VPP_IOCTL_MSG
	GA_REG_WORD32_READ(0xf7e82C00 + 0x04 + 7 * 0x14, &vpp_intr_timestamp);
	vpp_intr_timestamp = DEBUG_TIMER_VALUE - vpp_intr_timestamp;
#endif

#ifdef CONFIG_IRQ_LATENCY_PROFILE
	amp_irq_profiler.vpp_isr_start = cpu_clock(smp_processor_id());
#endif

	/* VPP interrupt handling  */
	pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
	instat = semaphore_chk_full(pSemHandle, -1);

	if (bTST(instat, avioDhubSemMap_vpp_vppCPCB0_intr)
#ifdef NEW_ISR
	    && (vpp_intr_status[avioDhubSemMap_vpp_vppCPCB0_intr])
#endif
	    ) {
#ifdef NEW_ISR
		vpp_intr |= bSETMASK(avioDhubSemMap_vpp_vppCPCB0_intr);
#else
		vpp_intr = 1;
#endif
		bSET(instat_used, avioDhubSemMap_vpp_vppCPCB0_intr);

#ifdef CONFIG_IRQ_LATENCY_PROFILE
		amp_irq_profiler.vppCPCB0_intr_curr =
		    cpu_clock(smp_processor_id());
#endif
		/* CPCB-0 interrupt */
		/* clear interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr, 1);
		semaphore_clr_full(pSemHandle,
				   avioDhubSemMap_vpp_vppCPCB0_intr);

		/* Clear the bits for CPCB0 VDE interrupt */
		if (bTST(instat, avioDhubSemMap_vpp_vppOUT4_intr)) {
			semaphore_pop(pSemHandle,
				      avioDhubSemMap_vpp_vppOUT4_intr, 1);
			semaphore_clr_full(pSemHandle,
					   avioDhubSemMap_vpp_vppOUT4_intr);
			bCLR(instat, avioDhubSemMap_vpp_vppOUT4_intr);
		}

		start_vbi_dma_transaction(0);
		start_vbi_bcm_transaction(0);
		send_bcm_sche_cmd(BCM_SCHED_Q0);
	}

	if (bTST(instat, avioDhubSemMap_vpp_vppCPCB1_intr)
#ifdef NEW_ISR
	    && (vpp_intr_status[avioDhubSemMap_vpp_vppCPCB1_intr])
#endif
	    ) {
#ifdef NEW_ISR
		vpp_intr |= bSETMASK(avioDhubSemMap_vpp_vppCPCB1_intr);
#else
		vpp_intr = 1;
#endif
		bSET(instat_used, avioDhubSemMap_vpp_vppCPCB1_intr);

		/* CPCB-1 interrupt */
		/* clear interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppCPCB1_intr, 1);
		semaphore_clr_full(pSemHandle,
				   avioDhubSemMap_vpp_vppCPCB1_intr);

		/* Clear the bits for CPCB1 VDE interrupt */
		if (bTST(instat, avioDhubSemMap_vpp_vppOUT5_intr)) {
			semaphore_pop(pSemHandle,
				      avioDhubSemMap_vpp_vppOUT5_intr, 1);
			semaphore_clr_full(pSemHandle,
					   avioDhubSemMap_vpp_vppOUT5_intr);
			bCLR(instat, avioDhubSemMap_vpp_vppOUT5_intr);
		}
		start_vbi_dma_transaction(1);
		start_vbi_bcm_transaction(1);
		send_bcm_sche_cmd(BCM_SCHED_Q1);
	}

	if (bTST(instat, avioDhubSemMap_vpp_vppCPCB2_intr)
#ifdef NEW_ISR
	    && (vpp_intr_status[avioDhubSemMap_vpp_vppCPCB2_intr])
#endif
	    ) {
#ifdef NEW_ISR
		vpp_intr |= bSETMASK(avioDhubSemMap_vpp_vppCPCB2_intr);
#else
		vpp_intr = 1;
#endif
		bSET(instat_used, avioDhubSemMap_vpp_vppCPCB2_intr);

		/* CPCB-2 interrupt */
		/* clear interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppCPCB2_intr, 1);
		semaphore_clr_full(pSemHandle,
				   avioDhubSemMap_vpp_vppCPCB2_intr);

		/* Clear the bits for CPCB2 VDE interrupt */
		if (bTST(instat, avioDhubSemMap_vpp_vppOUT6_intr)) {
			semaphore_pop(pSemHandle,
				      avioDhubSemMap_vpp_vppOUT6_intr, 1);
			semaphore_clr_full(pSemHandle,
					   avioDhubSemMap_vpp_vppOUT6_intr);
			bCLR(instat, avioDhubSemMap_vpp_vppOUT6_intr);
		}

		start_vbi_dma_transaction(2);
		start_vbi_bcm_transaction(2);
		send_bcm_sche_cmd(BCM_SCHED_Q2);
	}

	if (bTST(instat, avioDhubSemMap_vpp_vppOUT4_intr)
#ifdef NEW_ISR
	    && (vpp_intr_status[avioDhubSemMap_vpp_vppOUT4_intr])
#endif
	    ) {
#ifdef NEW_ISR
		vpp_intr |= bSETMASK(avioDhubSemMap_vpp_vppOUT4_intr);
#else
		vpp_intr = 1;
#endif
		bSET(instat_used, avioDhubSemMap_vpp_vppOUT4_intr);

		/* CPCB-0 VDE interrupt */
		/* clear interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr, 1);
		semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr);

		start_vde_bcm_transaction(0);
		send_bcm_sche_cmd(BCM_SCHED_Q3);
	}

	if (bTST(instat, avioDhubSemMap_vpp_vppOUT5_intr)
#ifdef NEW_ISR
	    && (vpp_intr_status[avioDhubSemMap_vpp_vppOUT5_intr])
#endif
	    ) {
#ifdef NEW_ISR
		vpp_intr |= bSETMASK(avioDhubSemMap_vpp_vppOUT5_intr);
#else
		vpp_intr = 1;
#endif
		bSET(instat_used, avioDhubSemMap_vpp_vppOUT5_intr);

		/* CPCB-1 VDE interrupt */
		/* clear interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppOUT5_intr, 1);
		semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppOUT5_intr);

		start_vde_bcm_transaction(1);
		send_bcm_sche_cmd(BCM_SCHED_Q4);
	}

	if (bTST(instat, avioDhubSemMap_vpp_vppOUT6_intr)
#ifdef NEW_ISR
	    && (vpp_intr_status[avioDhubSemMap_vpp_vppOUT6_intr])
#endif
	    ) {
#ifdef NEW_ISR
		vpp_intr |= bSETMASK(avioDhubSemMap_vpp_vppOUT6_intr);
#else
		vpp_intr = 1;
#endif
		bSET(instat_used, avioDhubSemMap_vpp_vppOUT6_intr);

		/* CPCB-2 VDE interrupt */
		/* clear interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppOUT6_intr, 1);
		semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppOUT6_intr);

		start_vde_bcm_transaction(2);
		send_bcm_sche_cmd(BCM_SCHED_Q5);
	}

	if (bTST(instat, avioDhubSemMap_vpp_vppOUT3_intr)
#ifdef NEW_ISR
	    && (vpp_intr_status[avioDhubSemMap_vpp_vppOUT3_intr])
#endif
	    ) {
#ifdef NEW_ISR
		vpp_intr |= bSETMASK(avioDhubSemMap_vpp_vppOUT3_intr);
#else
		vpp_intr = 1;
#endif
		bSET(instat_used, avioDhubSemMap_vpp_vppOUT3_intr);

		/* VOUT3 interrupt */
		/* clear interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppOUT3_intr, 1);
		semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppOUT3_intr);
	}

#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
	if (bTST(instat, avioDhubSemMap_vpp_CH10_intr)
#ifdef NEW_ISR
	    && (vpp_intr_status[avioDhubSemMap_vpp_CH10_intr])
#endif
	    ) {
		bSET(instat_used, avioDhubSemMap_vpp_CH10_intr);
		/* HDMI audio interrupt */
		/* clear interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vpp_CH10_intr, 1);
		semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_CH10_intr);
		aout_resume_cmd(HDMI_PATH);
		tasklet_hi_schedule(&amp_hdmi_tasklet);
		bCLR(instat, avioDhubSemMap_vpp_CH10_intr);
	}
#else
	if (bTST(instat, avioDhub_channel_num)
#ifdef NEW_ISR
	    && (vpp_intr_status[avioDhub_channel_num])
#endif
	    ) {
		bSET(instat_used, avioDhub_channel_num);
		/* HDMI audio interrupt */
		/* clear interrupt */
		semaphore_pop(pSemHandle, avioDhub_channel_num, 1);
		semaphore_clr_full(pSemHandle, avioDhub_channel_num);

		aout_resume_cmd(HDMI_PATH);
		tasklet_hi_schedule(&amp_hdmi_tasklet);
		bCLR(instat, avioDhub_channel_num);
	}
#endif

	if (bTST(instat, avioDhubSemMap_vpp_vppOUT0_intr)
#ifdef NEW_ISR
	    && (vpp_intr_status[avioDhubSemMap_vpp_vppOUT0_intr])
#endif
	    ) {
#ifdef NEW_ISR
		vpp_intr |= bSETMASK(avioDhubSemMap_vpp_vppOUT0_intr);
#else
		vpp_intr = 1;
#endif
		bSET(instat_used, avioDhubSemMap_vpp_vppOUT0_intr);

		/* VOUT0 interrupt */
		/* clear interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr, 1);
		semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr);
	}

	if (bTST(instat, avioDhubSemMap_vpp_vppOUT1_intr)
#ifdef NEW_ISR
	    && (vpp_intr_status[avioDhubSemMap_vpp_vppOUT1_intr])
#endif
	    ) {
#ifdef NEW_ISR
		vpp_intr |= bSETMASK(avioDhubSemMap_vpp_vppOUT1_intr);
#else
		vpp_intr = 1;
#endif
		bSET(instat_used, avioDhubSemMap_vpp_vppOUT1_intr);

		/* VOUT1 interrupt */
		/* clear interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppOUT1_intr, 1);
		semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppOUT1_intr);
	}
#ifdef VPP_DBG
//    amp_trace("ISR instat:%x\n", instat);
#endif

	if (vpp_intr) {
#if CONFIG_VPP_IOCTL_MSG

#if CONFIG_VPP_ISR_MSGQ
		MV_CC_MSG_t msg = { VPP_CC_MSG_TYPE_VPP,
#ifdef NEW_ISR
			vpp_intr,
#else
			instat,
#endif
			vpp_intr_timestamp
		};
		spin_lock(&msgQ_spinlock);
		ret = PEMsgQ_Add(&hPEMsgQ, &msg);
		spin_unlock(&msgQ_spinlock);
		if (ret != S_OK) {
			if (!atomic_read(&vpp_isr_msg_err_cnt)) {
				amp_error
				    ("[vpp isr] MsgQ full\n");
			}
			atomic_inc(&vpp_isr_msg_err_cnt);
			return IRQ_HANDLED;
		}
#else
		vpp_instat = instat;
#endif
		up(&vpp_sem);
#else
		amp_vpp_tasklet.data = instat;
		tasklet_hi_schedule(&amp_vpp_tasklet);
#endif

#ifdef CONFIG_IRQ_LATENCY_PROFILE

		amp_irq_profiler.vpp_isr_end = cpu_clock(smp_processor_id());
		unsigned long isr_time =
		    amp_irq_profiler.vpp_isr_end -
		    amp_irq_profiler.vpp_isr_start;
		int32_t jitter = 0;
		isr_time /= 1000;

		if (bTST(instat, avioDhubSemMap_vpp_vppCPCB0_intr)) {
			if (amp_irq_profiler.vppCPCB0_intr_last) {
				jitter =
				    (int64_t) amp_irq_profiler.
				    vppCPCB0_intr_curr -
				    (int64_t) amp_irq_profiler.
				    vppCPCB0_intr_last;

				//nanosec_rem = do_div(interval, 1000000000);
				// transform to us unit
				jitter /= 1000;
				jitter -= 16667;
			}
			amp_irq_profiler.vppCPCB0_intr_last =
			    amp_irq_profiler.vppCPCB0_intr_curr;
		}

		if ((jitter > 670) || (jitter < -670) || (isr_time > 1000)) {
			amp_trace
			    (" W/[vpp isr] jitter:%6d > +-670 us, instat:0x%x last_instat:"
			     "0x%0x max_instat:0x%0x, isr_time:%d us last:%d max:%d \n",
			     jitter, instat_used,
			     amp_irq_profiler.vpp_isr_last_instat,
			     amp_irq_profiler.vpp_isr_instat_max, isr_time,
			     amp_irq_profiler.vpp_isr_time_last,
			     amp_irq_profiler.vpp_isr_time_max);
		}

		amp_irq_profiler.vpp_isr_last_instat = instat_used;
		amp_irq_profiler.vpp_isr_time_last = isr_time;

		if (isr_time > amp_irq_profiler.vpp_isr_time_max) {
			amp_irq_profiler.vpp_isr_time_max = isr_time;
			amp_irq_profiler.vpp_isr_instat_max = instat_used;
		}
#endif
	}
#ifdef CONFIG_IRQ_LATENCY_PROFILE
	else {
		amp_irq_profiler.vpp_isr_end = cpu_clock(smp_processor_id());
		unsigned long isr_time =
		    amp_irq_profiler.vpp_isr_end -
		    amp_irq_profiler.vpp_isr_start;
		isr_time /= 1000;

		if (isr_time > 1000) {
			amp_trace("###isr_time:%d us instat:%x last:%x\n",
				  isr_time, vpp_intr, instat,
				  amp_irq_profiler.vpp_isr_last_instat);
		}

		amp_irq_profiler.vpp_isr_time_last = isr_time;
	}
#endif

	return IRQ_HANDLED;
}

static void start_vip_vbi_bcm(void)
{
	unsigned int bcm_sched_cmd[2];

	if (vip_vbi_info.DmaLen) {
		dhub_channel_generate_cmd(&(VPP_dhubHandle.dhub),
					  avioDhubChMap_vpp_BCM_R_auto,
					  (INT) vip_vbi_info.DmaAddr,
					  (INT) vip_vbi_info.DmaLen * 8, 0, 0,
					  0, 1, bcm_sched_cmd);
		while (!BCM_SCHED_PushCmd(BCM_SCHED_Q12, bcm_sched_cmd, NULL)) ;
	}

	/* invalid vbi_bcm_info */
	vip_vbi_info.DmaLen = 0;
}

static void start_vip_dvi_bcm(void)
{
	unsigned int bcm_sched_cmd[2];

	if (vip_dma_info.DmaLen) {
		dhub_channel_generate_cmd(&(VPP_dhubHandle.dhub),
					  avioDhubChMap_vpp_BCM_R_auto,
					  (INT) vip_dma_info.DmaAddr,
					  (INT) vip_dma_info.DmaLen * 8, 0, 0,
					  0, 1, bcm_sched_cmd);
		while (!BCM_SCHED_PushCmd(BCM_SCHED_Q12, bcm_sched_cmd, NULL)) ;
	}
	/* invalid vbi_bcm_info */
	vip_dma_info.DmaLen = 0;
}

static void start_vip_sd_wr_bcm(void)
{
	unsigned int bcm_sched_cmd[2];

	if (vip_sd_wr_info.DmaLen) {
		dhub_channel_generate_cmd(&(VPP_dhubHandle.dhub),
					  avioDhubChMap_vpp_BCM_R_auto,
					  (INT) vip_sd_wr_info.DmaAddr,
					  (INT) vip_sd_wr_info.DmaLen * 8, 0, 0,
					  0, 1, bcm_sched_cmd);
		while (!BCM_SCHED_PushCmd(BCM_SCHED_Q12, bcm_sched_cmd, NULL)) ;
	}
	/* invalid vbi_bcm_info */
	vip_sd_wr_info.DmaLen = 0;
}

static void start_vip_sd_rd_bcm(void)
{
	unsigned int bcm_sched_cmd[2];

	if (vip_sd_rd_info.DmaLen) {
		dhub_channel_generate_cmd(&(VPP_dhubHandle.dhub),
					  avioDhubChMap_vpp_BCM_R_auto,
					  (INT) vip_sd_rd_info.DmaAddr,
					  (INT) vip_sd_rd_info.DmaLen * 8, 0, 0,
					  0, 1, bcm_sched_cmd);
		while (!BCM_SCHED_PushCmd(BCM_SCHED_Q12, bcm_sched_cmd, NULL)) ;
	}
	/* invalid vbi_bcm_info */
	vip_sd_rd_info.DmaLen = 0;
}

#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
static irqreturn_t amp_devices_vip_isr(int irq, void *dev_id)
{
	INT instat;
	HDL_semaphore *pSemHandle;
	INT vip_intr = 0, hrx_intr = 0;
	INT chanId;
	UINT8 en2, en3, stat2, stat3, intr2, intr3, en0, en1, stat0, stat1,
	    intr0, intr1;
	GA_REG_WORD32_READ(0xf7e82C00 + 0x04 + 7 * 0x14, &hrx_intr_timestamp);
	hrx_intr_timestamp = DEBUG_TIMER_VALUE - hrx_intr_timestamp;

	/* VIP interrupt handling  */
	pSemHandle = dhub_semaphore(&VIP_dhubHandle.dhub);
	instat = semaphore_chk_full(pSemHandle, -1);

	if (bTST(instat, avioDhubSemMap_vip_wre_event_intr)
#ifdef NEW_ISR
	    && (vip_intr_status[avioDhubSemMap_vip_wre_event_intr])
#endif
	    ) {
		/* SD decoder write enable interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vip_wre_event_intr, 1);
		semaphore_clr_full(pSemHandle,
				   avioDhubSemMap_vip_wre_event_intr);

#ifdef NEW_ISR
		vip_intr |= bSETMASK(avioDhubSemMap_vip_wre_event_intr);
#else
		vip_intr = 1;
#endif
	}

	if (bTST(instat, avioDhubSemMap_vip_dvi_vde_intr)
#ifdef NEW_ISR
	    && (vip_intr_status[avioDhubSemMap_vip_dvi_vde_intr])
#endif
	    ) {
		/* DVI VDE interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vip_dvi_vde_intr, 1);
		semaphore_clr_full(pSemHandle, avioDhubSemMap_vip_dvi_vde_intr);

		start_vip_dvi_bcm();
#ifdef NEW_ISR
		vip_intr |= bSETMASK(avioDhubSemMap_vip_dvi_vde_intr);
#else
		vip_intr = 1;
#endif
	}
	if (bTST(instat, avioDhubSemMap_vip_vbi_vde_intr)
#ifdef NEW_ISR
	    && (vip_intr_status[avioDhubSemMap_vip_vbi_vde_intr])
#endif
	    ) {
		/* VBI VDE interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vip_vbi_vde_intr, 1);
		semaphore_clr_full(pSemHandle, avioDhubSemMap_vip_vbi_vde_intr);

		start_vip_vbi_bcm();
#ifdef NEW_ISR
		vip_intr |= bSETMASK(avioDhubSemMap_vip_vbi_vde_intr);
#else
		vip_intr = 1;
#endif
	}
#if CONFIG_HRX_IOCTL_MSG
	if (bTST(instat, avioDhubSemMap_vip_hdmirx_intr)) {

		GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_STATUS_2 << 2),
				 &stat2);
		GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_2 << 2), &en2);
		en2 = en2 & stat2;

		GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_STATUS_3 << 2),
				 &stat3);
		GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_3 << 2), &en3);
		en3 = en3 & stat3;
		GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_STATUS_0 << 2),
				 &stat0);
		GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_0 << 2), &en0);
		en0 = en0 & stat0;
		GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_STATUS_1 << 2),
				 &stat1);
		GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_1 << 2), &en1);
		en1 = en1 & stat1;
		hrx_intr = 0xff;
		if ((en2 & (HDMI_RX_INT_VRES_CHG | HDMI_RX_INT_HRES_CHG)) ||
		    (en3 & (HDMI_RX_INT_5V_PWR | HDMI_RX_INT_CLOCK_CHANGE))) {
			hrx_intr = HDMIRX_INTR_SYNC;
			GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_2 << 2),
					 &intr2);
			GA_REG_BYTE_WRITE(SOC_VPP_BASE +
					  (HDMIRX_INTR_EN_2 << 2),
					  intr2 & ~(HDMI_RX_INT_SYNC_DET |
						    HDMI_RX_INT_VRES_CHG |
						    HDMI_RX_INT_HRES_CHG));
			GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_3 << 2),
					 &intr3);
			GA_REG_BYTE_WRITE(SOC_VPP_BASE +
					  (HDMIRX_INTR_EN_3 << 2),
					  intr3 & ~(HDMI_RX_INT_5V_PWR |
						    HDMI_RX_INT_CLOCK_CHANGE));
			GA_REG_BYTE_WRITE(SOC_VPP_BASE +
					  (HDMIRX_INTR_EN_2 << 2), HDMI_RX_INT_AUTH_STARTED);
			GA_REG_BYTE_WRITE(SOC_VPP_BASE +
					  (HDMIRX_INTR_EN_3 << 2), 0x0);
			GA_REG_BYTE_WRITE(SOC_VPP_BASE +
					  (HDMIRX_INTR_EN_0 << 2), 0x0);
			GA_REG_BYTE_WRITE(SOC_VPP_BASE +
					  (HDMIRX_INTR_EN_1 << 2), 0x0);
			/* calibrate trigger */
			/*
			   GA_REG_BYTE_WRITE(SOC_VPP_BASE + (HDMIRX_PHY_CAL_TRIG << 2), 0x01);
			   GA_REG_BYTE_WRITE(SOC_VPP_BASE + (HDMIRX_PHY_CAL_TRIG << 2), 0x00);
			 */
		} else if (en0 & HDMI_RX_INT_GCP_AVMUTE) {
			hrx_intr = HDMIRX_INTR_AVMUTE;
			GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_0 << 2),
					 &intr0);
			GA_REG_BYTE_WRITE(SOC_VPP_BASE +
					  (HDMIRX_INTR_EN_0 << 2),
					  intr0 & ~(HDMI_RX_INT_GCP_AVMUTE));
		} else if (en2 & HDMI_RX_INT_VSI_STOP) {
			hrx_intr = HDMIRX_INTR_VSI_STOP;
			GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_2 << 2),
					 &intr2);
			GA_REG_BYTE_WRITE(SOC_VPP_BASE +
					  (HDMIRX_INTR_EN_2 << 2),
					  intr2 & ~(HDMI_RX_INT_VSI_STOP));
		} else if (en1 &
			   (HDMI_RX_INT_AVI_INFO | HDMI_RX_INT_VENDOR_PKT)) {
			hrx_intr = HDMIRX_INTR_PKT;
			GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_1 << 2),
					 &intr1);
			GA_REG_BYTE_WRITE(SOC_VPP_BASE +
					  (HDMIRX_INTR_EN_1 << 2),
					  intr1 & ~(HDMI_RX_INT_AVI_INFO |
						    HDMI_RX_INT_VENDOR_PKT));
		} else if (en1 & HDMI_RX_INT_GAMUT_PKT) {
			hrx_intr = HDMIRX_INTR_GMD_PKT;
			GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_1 << 2),
					 &intr1);
			GA_REG_BYTE_WRITE(SOC_VPP_BASE +
					  (HDMIRX_INTR_EN_1 << 2),
					  intr1 & ~(HDMI_RX_INT_GAMUT_PKT));
		} else if (en1 &
			   (HDMI_RX_INT_CHNL_STATUS | HDMI_RX_INT_AUD_INFO)) {
			hrx_intr = HDMIRX_INTR_CHNL_STS;
			GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_1 << 2),
					 &intr1);
			GA_REG_BYTE_WRITE(SOC_VPP_BASE +
					  (HDMIRX_INTR_EN_1 << 2),
					  intr1 & ~(HDMI_RX_INT_CHNL_STATUS |
						    HDMI_RX_INT_AUD_INFO));
		} else if (en0 & HDMI_RX_INT_GCP_COLOR_DEPTH) {
			hrx_intr = HDMIRX_INTR_CLR_DEPTH;
			GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_0 << 2),
					 &intr0);
			GA_REG_BYTE_WRITE(SOC_VPP_BASE +
					  (HDMIRX_INTR_EN_0 << 2),
					  intr0 &
					  ~(HDMI_RX_INT_GCP_COLOR_DEPTH));
		}
		else if (en2 & (HDMI_RX_INT_AUTH_STARTED | HDMI_RX_INT_AUTH_COMPLETE)) {
			hrx_intr = HDMIRX_INTR_HDCP;
			//GA_REG_BYTE_READ(SOC_VPP_BASE + (HDMIRX_INTR_EN_2 <<2), &intr2);
			//GA_REG_BYTE_WRITE(SOC_VPP_BASE + (HDMIRX_INTR_EN_2 <<2), intr2 & ~ (HDMI_RX_INT_AUTH_STARTED | HDMI_RX_INT_AUTH_COMPLETE));
		}
		/* HDMI Rx interrupt */
		semaphore_pop(pSemHandle, avioDhubSemMap_vip_hdmirx_intr, 1);
		semaphore_clr_full(pSemHandle, avioDhubSemMap_vip_hdmirx_intr);
		if (hrx_intr != 0xff) {
			/* process rx intr */
			MV_CC_MSG_t msg = { /*VPP_CC_MSG_TYPE_VPP */ 0,
				hrx_intr,
				hrx_intr_timestamp
			};
			PEMsgQ_Add(&hPEHRXMsgQ, &msg);
			up(&hrx_sem);
		}

	}
#endif

	for (chanId = avioDhubChMap_vip_MIC0_W;
	     chanId <= avioDhubChMap_vip_MIC3_W; chanId++) {
		if (bTST(instat, chanId)) {
			semaphore_pop(pSemHandle, chanId, 1);
			semaphore_clr_full(pSemHandle, chanId);
			if (chanId == avioDhubChMap_vip_MIC0_W) {
				spin_lock(&aip_spinlock);
				aip_resume_cmd();
				spin_unlock(&aip_spinlock);

				tasklet_hi_schedule(&amp_aip_tasklet);
			}
		}
	}

	if (vip_intr) {
#if CONFIG_VIP_IOCTL_MSG
#if CONFIG_VIP_ISR_MSGQ
		MV_CC_MSG_t msg = { /*VPP_CC_MSG_TYPE_VPP */ 0,
#ifdef NEW_ISR
			vip_intr,
#else
			instat,
#endif
			vip_intr_timestamp
		};
		HRESULT ret = S_OK;
		ret = PEMsgQ_Add(&hPEVIPMsgQ, &msg);

		if (ret != S_OK) {
			if (!atomic_read(&vip_isr_msg_err_cnt)) {
				amp_error("[VIP isr] MsgQ full\n");
			}
			atomic_inc(&vip_isr_msg_err_cnt);
			return IRQ_HANDLED;
		}
#endif
		up(&vip_sem);
#else
		tasklet_hi_schedule(&amp_vip_tasklet);
#endif
	}

	return IRQ_HANDLED;
}

static irqreturn_t amp_devices_avif_isr(int irq, void *dev_id)
{
    HDL_semaphore *pSemHandle;
    HRESULT rc = S_OK;
    INT chanId;
    INT channel;
    INT instat, avif_intr = 0;
    UINT8 enreg0, enreg1, enreg2, enreg3, enreg4;
    UINT8 en0, en1, en2, en3;
    UINT8 stat0, stat1, stat2, stat3;
    UINT8 intr0, intr1,intr2, intr3;
    UINT8 masksts0, masksts1, masksts2, masksts3, masksts4;
    UINT8 decreg1, decreg2, regval;
    UINT32 hdmi_port = 0;
    UINT32 port_offset = 0;
    UINT32 instat0 = 0, instat1 = 0;
    INT hrx_intr = 0xff;

    GA_REG_BYTE_READ(CPU_INTR_MASK0_EXTREG_ADDR,&enreg0);
    GA_REG_BYTE_READ(CPU_INTR_MASK0_STATUS_EXTREG_ADDR, &masksts0);

    GA_REG_BYTE_READ(CPU_INTR_MASK1_EXTREG_ADDR,&enreg1);
    GA_REG_BYTE_READ(CPU_INTR_MASK1_STATUS_EXTREG_ADDR,&masksts1);

    GA_REG_BYTE_READ(CPU_INTR_MASK2_EXTREG_ADDR, &enreg2);
    GA_REG_BYTE_READ(CPU_INTR_MASK2_STATUS_EXTREG_ADDR,&masksts2);

    GA_REG_BYTE_READ(CPU_INTR_MASK3_EXTREG_ADDR, &enreg3);
    GA_REG_BYTE_READ(CPU_INTR_MASK3_STATUS_EXTREG_ADDR,&masksts3);

    GA_REG_BYTE_READ(CPU_INTR_MASK4_EXTREG_ADDR, &enreg4);
    GA_REG_BYTE_READ(CPU_INTR_MASK4_STATUS_EXTREG_ADDR,&masksts4);

    instat0 = (masksts0 << 0) | (masksts1<<8) | (masksts2<<16) | (masksts3 <<24);

    GA_REG_BYTE_READ(CPU_INTCPU_2_EXTCPU_MASK0INTR_ADDR, &regval);
    instat1 |= regval;
    if(regval&0x1)
        GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR0_ADDR, 0x00);
    if(regval&0x2)
        GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR1_ADDR, 0x00);
    if(regval&0x4)
        GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR2_ADDR, 0x00);
    if(regval&0x8)
        GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR3_ADDR, 0x00);
    if(regval&0x10)
        GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR4_ADDR, 0x00);
    if(regval&0x20)
        GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR5_ADDR, 0x00);
    if(regval&0x40)
        GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR6_ADDR, 0x00);
    if(regval&0x80)
        GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR7_ADDR, 0x00);

    //Clear interrupts
    GA_REG_BYTE_WRITE(CPU_INTR_CLR0_EXTREG_ADDR, masksts0);
    GA_REG_BYTE_WRITE(CPU_INTR_CLR1_EXTREG_ADDR, masksts1);
    GA_REG_BYTE_WRITE(CPU_INTR_CLR2_EXTREG_ADDR, masksts2);
    GA_REG_BYTE_WRITE(CPU_INTR_CLR3_EXTREG_ADDR, masksts3);
    GA_REG_BYTE_WRITE(CPU_INTR_CLR4_EXTREG_ADDR, masksts4);

    /* DHUB Interrupt status */
    pSemHandle = dhub_semaphore(&AVIF_dhubHandle.dhub);
    instat = semaphore_chk_full(pSemHandle, -1);

    if(masksts0 & (AVIF_INTR_PIP_VDE |AVIF_INTR_MAIN_VDE)) //VDE Interrupt
    {
        //VDE interrupt info to application
	    avif_intr = masksts0;
    }
    if (((masksts1 & 0x0F)&(~enreg1)) || ((masksts2 & 0x0F) & (~enreg2))) {
        //Interrupt for which HDMI port
       hdmi_port = (masksts1 & 0x0F) | (masksts2 & 0x0F);

        printk("HDMIRX interrupt from port %d\r\n", hdmi_port);
        //hdmi_port = 0x01; //
	if(hdmi_port == 0x01)
	    port_offset = 0x00;
	else if(hdmi_port == 0x02)
	    port_offset = AVIF_HRX_BASE_OFFSET<<2;
	else if(hdmi_port == 0x04)
           port_offset = (2 * AVIF_HRX_BASE_OFFSET)<<2;
        else
            port_offset = (3 * AVIF_HRX_BASE_OFFSET)<<2;

       GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_STATUS_2,&stat2);
       GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_EN_2, &en2);
       en2 = en2 & stat2;
       GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_STATUS_3,&stat3);
       GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_EN_3, &en3);
       en3 = en3 & stat3;
       GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_STATUS_0 ,&stat0);
       GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_EN_0, &en0);
       en0 = en0 & stat0;
       GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_STATUS_1,&stat1);
       GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_EN_1, &en1);
       en1 = en1 & stat1;
        printk("enreg1 = 0x%x enreg2 = 0x%x masksts1 = 0x%x masksts2 = 0x%x en(0x%x 0x%x 0x%x 0x%x)\r\n", enreg1, enreg2, masksts1,masksts2,en0,en1,en2,en3);
       hrx_intr = 0xff;
       if ((en2 & (HDMI_RX_INT_VRES_CHG | HDMI_RX_INT_HRES_CHG)) ||
               (en3 & (HDMI_RX_INT_5V_PWR | HDMI_RX_INT_CLOCK_CHANGE))) {
            hrx_intr = HDMIRX_INTR_SYNC;
            GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_EN_2,&intr2);
            GA_REG_BYTE_WRITE(port_offset+AVIF_HRX_INTR_EN_2,intr2 & ~(HDMI_RX_INT_SYNC_DET |
                        HDMI_RX_INT_VRES_CHG |HDMI_RX_INT_HRES_CHG));
            GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_EN_3,&intr3);
            GA_REG_BYTE_WRITE(port_offset+AVIF_HRX_INTR_EN_3,intr3 & ~(HDMI_RX_INT_5V_PWR |
                        HDMI_RX_INT_CLOCK_CHANGE));
            GA_REG_BYTE_WRITE(port_offset+AVIF_HRX_INTR_EN_2, HDMI_RX_INT_AUTH_STARTED);
            GA_REG_BYTE_WRITE(port_offset+AVIF_HRX_INTR_EN_3, 0x0);
            GA_REG_BYTE_WRITE(port_offset+AVIF_HRX_INTR_EN_0, 0x0);
            GA_REG_BYTE_WRITE(port_offset+AVIF_HRX_INTR_EN_1, 0x0);

       } else if (en0 & HDMI_RX_INT_GCP_AVMUTE) {
            hrx_intr = HDMIRX_INTR_AVMUTE;
            GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_EN_0,&intr0);
            GA_REG_BYTE_WRITE(port_offset+AVIF_HRX_INTR_EN_0,
                  intr0 & ~(HDMI_RX_INT_GCP_AVMUTE));
       } else if (en2 & HDMI_RX_INT_VSI_STOP) {
            hrx_intr = HDMIRX_INTR_VSI_STOP;
            GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_EN_2,&intr2);
            GA_REG_BYTE_WRITE(port_offset+AVIF_HRX_INTR_EN_2,
                  intr2 & ~(HDMI_RX_INT_VSI_STOP));
       } else if (en1 &(HDMI_RX_INT_AVI_INFO | HDMI_RX_INT_VENDOR_PKT)) {
            hrx_intr = HDMIRX_INTR_PKT;
            GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_EN_1,&intr1);
            GA_REG_BYTE_WRITE(port_offset+AVIF_HRX_INTR_EN_1,
                  intr1 & ~(HDMI_RX_INT_AVI_INFO |HDMI_RX_INT_VENDOR_PKT));
        } else if (en1 & HDMI_RX_INT_GAMUT_PKT) {
            hrx_intr = HDMIRX_INTR_GMD_PKT;
            GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_EN_1 ,&intr1);
            GA_REG_BYTE_WRITE(port_offset+AVIF_HRX_INTR_EN_1,
                  intr1 & ~(HDMI_RX_INT_GAMUT_PKT));
        } else if (en1 &(HDMI_RX_INT_CHNL_STATUS | HDMI_RX_INT_AUD_INFO)) {
            hrx_intr = HDMIRX_INTR_CHNL_STS;
            GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_EN_1,&intr1);
            GA_REG_BYTE_WRITE(port_offset+AVIF_HRX_INTR_EN_1,
                  intr1 & ~(HDMI_RX_INT_CHNL_STATUS |HDMI_RX_INT_AUD_INFO));
        } else if (en0 & HDMI_RX_INT_GCP_COLOR_DEPTH) {
            hrx_intr = HDMIRX_INTR_CLR_DEPTH;
            GA_REG_BYTE_READ(port_offset+AVIF_HRX_INTR_EN_0,&intr0);
            GA_REG_BYTE_WRITE(port_offset+AVIF_HRX_INTR_EN_0,
                  intr0 &~(HDMI_RX_INT_GCP_COLOR_DEPTH));
        }
        else if (en2 & (HDMI_RX_INT_AUTH_STARTED | HDMI_RX_INT_AUTH_COMPLETE)) {
            hrx_intr = HDMIRX_INTR_HDCP;
        }
    }
    if(IsPIPAudio)
        channel = avif_dhub_config_ChMap_avif_AUD_WR0_PIP;
     else
        channel = avif_dhub_config_ChMap_avif_AUD_WR0_MAIN;

    //Audio DHUB INterrupt handling
    for (chanId = channel;chanId <= (channel + 3); chanId++) {
	if (bTST(instat, chanId)) {
            semaphore_pop(pSemHandle, chanId, 1);
	    semaphore_clr_full(pSemHandle, chanId);
	    if (chanId == channel) {
		aip_avif_resume_cmd();
		tasklet_hi_schedule(&amp_aip_avif_tasklet);
	    }
	}
    }

    /* HDMI Rx interrupt */
    if (hrx_intr != 0xff) {
	printk("hrx_intr = 0x%x\r\n", hrx_intr);
	//hrx_intr |= (hdmi_port << 24); //pass port info to the user driver
        /* process rx intr */
        MV_CC_MSG_t msg = { /*VPP_CC_MSG_TYPE_VPP */ 0,
           hrx_intr, //send port info also
           hrx_intr_timestamp
        };
        rc = PEMsgQ_Add(&hPEAVIFHRXMsgQ, &msg);
	if (rc != S_OK) {
		amp_error("[AVIF HRX isr] MsgQ full\n");
            return IRQ_HANDLED;
		}
        up(&avif_hrx_sem);
    }
    //Send VDE interrupt info to AVIF driver
    if(instat0 || instat1)
    {
	MV_CC_MSG_t msg =
        { /*VPP_CC_MSG_TYPE_VPP */ 0,
            instat0,
            instat1
        };
        rc = PEMsgQ_Add(&hPEAVIFMsgQ, &msg);
	if (rc != S_OK) {
            if (!atomic_read(&avif_isr_msg_err_cnt)) {
			amp_error("[AVIF isr] MsgQ full\n");
            }
            atomic_inc(&avif_isr_msg_err_cnt);
            return IRQ_HANDLED;
		}
        up(&avif_sem);
    }
    return IRQ_HANDLED;
}
#endif



static irqreturn_t amp_devices_vdec_isr(int irq, void *dev_id)
{
	/* disable interrupt */
	disable_irq_nosync(irq);

#if CONFIG_VDEC_IRQ_CHECKER
	vdec_int_cnt++;
#endif

	tasklet_hi_schedule(&amp_vdec_tasklet);

	return IRQ_HANDLED;
}

static void *AoutFifoGetKernelRdDMAInfo(AOUT_PATH_CMD_FIFO * p_aout_cmd_fifo,
					int pair)
{
	void *pHandle;
	int rd_offset = p_aout_cmd_fifo->kernel_rd_offset;
	if (rd_offset > 8 || rd_offset < 0) {
		int i = 0, fifo_cmd_size = sizeof(AOUT_PATH_CMD_FIFO) >> 2;
		int *temp = p_aout_cmd_fifo;
		amp_trace("AOUT FIFO memory %p is corrupted! corrupted data :\n",
				  p_aout_cmd_fifo);
		for (i = 0; i < fifo_cmd_size; i++) {
			amp_trace("0x%x\n", *temp++);
		}
		rd_offset = 0;
	}
	pHandle = &(p_aout_cmd_fifo->aout_dma_info[pair][rd_offset]);

	return pHandle;
}

static void
AoutFifoKernelRdUpdate(AOUT_PATH_CMD_FIFO * p_aout_cmd_fifo, int adv)
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

static void aout_start_cmd(int *aout_info, VOID *param)
{
	int *p = aout_info;
	int i, chanId;
	AOUT_DMA_INFO *p_dma_info;

	if (*p == MULTI_PATH) {
		p_ma_fifo = (AOUT_PATH_CMD_FIFO *) param;
		for (i = 0; i < 4; i++) {
			p_dma_info =
			    (AOUT_DMA_INFO *)
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
#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
		p_sa_fifo = (AOUT_PATH_CMD_FIFO *) param;
		p_dma_info =
		    (AOUT_DMA_INFO *) AoutFifoGetKernelRdDMAInfo(p_sa_fifo, 0);
		chanId = avioDhubChMap_ag_SA_R;
		dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
				       p_dma_info->addr0, p_dma_info->size0, 0,
				       0, 0, 1, 0, 0);
#endif
	} else if (*p == SPDIF_PATH) {
#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
		p_spdif_fifo = (AOUT_PATH_CMD_FIFO *) param;
		p_dma_info =
		    (AOUT_DMA_INFO *) AoutFifoGetKernelRdDMAInfo(p_spdif_fifo,
								 0);
		chanId = avioDhubChMap_ag_SPDIF_R;
		dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
				       p_dma_info->addr0, p_dma_info->size0, 0,
				       0, 0, 1, 0, 0);
#endif
	} else if (*p == HDMI_PATH) {
		p_hdmi_fifo = (AOUT_PATH_CMD_FIFO *) param;
		p_dma_info =
		    (AOUT_DMA_INFO *) AoutFifoGetKernelRdDMAInfo(p_hdmi_fifo,
								 0);
		chanId = avioDhubChMap_vpp_HDMI_R_auto;
		dhub_channel_write_cmd(&VPP_dhubHandle.dhub, chanId,
				       p_dma_info->addr0, p_dma_info->size0, 0,
				       0, 0, 1, 0, 0);
	}
}

static void aout_stop_cmd(int path_id)
{
    if (path_id == MULTI_PATH) {
        p_ma_fifo = NULL;
    } else if (path_id == LoRo_PATH) {
        p_sa_fifo = NULL;
    } else if (path_id == SPDIF_PATH) {
        p_spdif_fifo = NULL;
    } else if (path_id == HDMI_PATH) {
        p_hdmi_fifo = NULL;
    }
}

static void aout_resume_cmd(int path_id)
{
	AOUT_DMA_INFO *p_dma_info;
	unsigned int i, chanId;

	if (path_id == MULTI_PATH) {
	  if (!p_ma_fifo) return;
		if (!p_ma_fifo->fifo_underflow)
			AoutFifoKernelRdUpdate(p_ma_fifo, 1);

		if (AoutFifoCheckKernelFullness(p_ma_fifo)) {
			p_ma_fifo->fifo_underflow = 0;
			for (i = 0; i < 4; i++) {
				p_dma_info =
				    (AOUT_DMA_INFO *)
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
#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
    if (!p_sa_fifo) return;
		if (!p_sa_fifo->fifo_underflow)
			AoutFifoKernelRdUpdate(p_sa_fifo, 1);

		if (AoutFifoCheckKernelFullness(p_sa_fifo)) {
			p_sa_fifo->fifo_underflow = 0;
			p_dma_info =
			    (AOUT_DMA_INFO *)
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
#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
    if (!p_spdif_fifo) return;
		if (!p_spdif_fifo->fifo_underflow)
			AoutFifoKernelRdUpdate(p_spdif_fifo, 1);

		if (AoutFifoCheckKernelFullness(p_spdif_fifo)) {
			p_spdif_fifo->fifo_underflow = 0;
			p_dma_info =
			    (AOUT_DMA_INFO *)
			    AoutFifoGetKernelRdDMAInfo(p_spdif_fifo, 0);
			chanId = avioDhubChMap_ag_SPDIF_R;
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
			chanId = avioDhubChMap_ag_SPDIF_R;
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
					       p_spdif_fifo->zero_buffer,
					       p_spdif_fifo->zero_buffer_size,
					       0, 0, 0, 1, 0, 0);
		}
#endif
	} else if (path_id == HDMI_PATH) {
	  if (!p_hdmi_fifo) return;
		if (!p_hdmi_fifo->fifo_underflow)
			AoutFifoKernelRdUpdate(p_hdmi_fifo, 1);

		if (AoutFifoCheckKernelFullness(p_hdmi_fifo)) {
			p_hdmi_fifo->fifo_underflow = 0;
			p_dma_info =
			    (AOUT_DMA_INFO *)
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

#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
static void *AIPFifoGetKernelPreRdDMAInfo(AIP_DMA_CMD_FIFO * p_aip_cmd_fifo,
					  int pair)
{
	void *pHandle;
	int rd_offset = p_aip_cmd_fifo->kernel_pre_rd_offset;
	if (rd_offset > 8 || rd_offset < 0) {
		int i = 0, fifo_cmd_size = sizeof(AIP_DMA_CMD_FIFO) >> 2;
		int *temp = p_aip_cmd_fifo;
		amp_trace("memory %p is corrupted! corrupted data :\n", p_aip_cmd_fifo);
		for (i = 0; i < fifo_cmd_size; i++) {
			amp_trace("0x%x\n", *temp++);
		}
		rd_offset = 0;
	}
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

static void aip_start_cmd(int *aip_info, void *param)
{
	int *p = aip_info;
	int chanId, pair;
	AIP_DMA_CMD *p_dma_cmd;

	if (*p == 1) {
		aip_i2s_pair = 1;
		p_aip_fifo = (AIP_DMA_CMD_FIFO *) param;
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
		p_aip_fifo = (AIP_DMA_CMD_FIFO *) param;
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
}

static void aip_stop_cmd(void)
{
	p_aip_fifo = NULL;

	return;
}

static void aip_resume_cmd()
{
	AIP_DMA_CMD *p_dma_cmd;
	unsigned int chanId;
	int pair;

	if (!p_aip_fifo) {
		amp_trace("aip_resume_cmd:p_aip_fifo is NULL\n");
		return;
	}

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
}

static void aip_avif_start_cmd(int *aip_info, void *param)
{
    int *p = aip_info;
    int chanId, pair;
    int channel;
    AIP_DMA_CMD *p_dma_cmd;

    printk("aip_avif_start_cmd: IsPIPAudio = %d\r\n", IsPIPAudio);
    if(IsPIPAudio)
        chanId = avif_dhub_config_ChMap_avif_AUD_WR0_PIP;
    else
        chanId = avif_dhub_config_ChMap_avif_AUD_WR0_MAIN;

    channel = chanId;
    if (*p == 1)
    {
        aip_i2s_pair = 1;
        p_aip_fifo = (AIP_DMA_CMD_FIFO *) param;
        p_dma_cmd =
            (AIP_DMA_CMD *) AIPFifoGetKernelPreRdDMAInfo(p_aip_fifo, 0);

       dhub_channel_write_cmd(&AVIF_dhubHandle.dhub, chanId,
            p_dma_cmd->addr0, p_dma_cmd->size0, 0, 0,
            0, 1, 0, 0);
        AIPFifoKernelPreRdUpdate(p_aip_fifo, 1);

        // push 2nd dHub command
        p_dma_cmd =
            (AIP_DMA_CMD *) AIPFifoGetKernelPreRdDMAInfo(p_aip_fifo, 0);
        dhub_channel_write_cmd(&AVIF_dhubHandle.dhub, chanId,
             p_dma_cmd->addr0, p_dma_cmd->size0, 0, 0,
             0, 1, 0, 0);
        AIPFifoKernelPreRdUpdate(p_aip_fifo, 1);
    } else if (*p == 4) {
        /* 4 I2S will be introduced since BG2 A0 */
        aip_i2s_pair = 4;
        p_aip_fifo = (AIP_DMA_CMD_FIFO *) param;

        for (pair = 0; pair < 4; pair++) {
            p_dma_cmd =
                (AIP_DMA_CMD *)
                AIPFifoGetKernelPreRdDMAInfo(p_aip_fifo, pair);
           chanId = channel + pair;
           dhub_channel_write_cmd(&AVIF_dhubHandle.dhub, chanId,
              p_dma_cmd->addr0,
              p_dma_cmd->size0, 0, 0, 0, 1, 0,
              0);
        }
        AIPFifoKernelPreRdUpdate(p_aip_fifo, 1);

        // push 2nd dHub command
        for (pair = 0; pair < 4; pair++) {
            p_dma_cmd = (AIP_DMA_CMD *)
                AIPFifoGetKernelPreRdDMAInfo(p_aip_fifo, pair);
            chanId = channel + pair;
            dhub_channel_write_cmd(&AVIF_dhubHandle.dhub, chanId,
                   p_dma_cmd->addr0,
                   p_dma_cmd->size0, 0, 0, 0, 1, 0,
                   0);
        }
        AIPFifoKernelPreRdUpdate(p_aip_fifo, 1);
    	}
}

static void aip_avif_stop_cmd(void)
{
    return;
}

static void aip_avif_resume_cmd()
{
    AIP_DMA_CMD *p_dma_cmd;
    unsigned int chanId, channel;
    int pair;

    if (!p_aip_fifo->fifo_overflow)
        AIPFifoKernelRdUpdate(p_aip_fifo, 1);

    if(IsPIPAudio)
        channel = avif_dhub_config_ChMap_avif_AUD_WR0_PIP;
    else
        channel = avif_dhub_config_ChMap_avif_AUD_WR0_MAIN;

    if (AIPFifoCheckKernelFullness(p_aip_fifo))
    {
        p_aip_fifo->fifo_overflow = 0;
        for (pair = 0; pair < aip_i2s_pair; pair++)
        {
            p_dma_cmd =
                (AIP_DMA_CMD *)
            AIPFifoGetKernelPreRdDMAInfo(p_aip_fifo, pair);
           chanId = channel + pair;
           dhub_channel_write_cmd(&AVIF_dhubHandle.dhub, chanId,
                      p_dma_cmd->addr0,
                      p_dma_cmd->size0, 0, 0, 0,
                      p_dma_cmd->addr1 ? 0 : 1, 0, 0);
           if (p_dma_cmd->addr1) {
               dhub_channel_write_cmd(&AVIF_dhubHandle.dhub,
                      chanId, p_dma_cmd->addr1,
                      p_dma_cmd->size1, 0, 0,
                      0, 1, 0, 0);
           }
            AIPFifoKernelPreRdUpdate(p_aip_fifo, 1);
        }
    }else {
        p_aip_fifo->fifo_overflow = 1;
        p_aip_fifo->fifo_overflow_cnt++;
        for (pair = 0; pair < aip_i2s_pair; pair++)
        {
        /* FIXME;
         *chanid should be changed if 4 pair is supported
         */
           chanId = channel + pair;
           dhub_channel_write_cmd(&AVIF_dhubHandle.dhub, chanId,
               p_aip_fifo->overflow_buffer,
               p_aip_fifo->overflow_buffer_size,
               0, 0, 0, 1, 0, 0);
        }
    }
}
#endif

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
		p_in_coef_cmd =
		    (APP_CMD_BUFFER *)
		    HwAPPFifoGetKernelInCoefRdCmdBuf(p_app_cmd_fifo);
		chanId = avioDhubChMap_ag_APPCMD_R_Z1;
		if (p_in_coef_cmd->cmd_len) {
			PA = p_in_coef_cmd->cmd_buffer_hw_base;
			cmdSize = p_in_coef_cmd->cmd_len * sizeof(int);
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId, PA,
					       cmdSize, 0, 0, 0, 0, 0, 0);
		}
		p_in_data_cmd =
		    (APP_CMD_BUFFER *)
		    HwAPPFifoGetKernelInDataRdCmdBuf(p_app_cmd_fifo);
		if (p_in_data_cmd->cmd_len) {
			PA = p_in_data_cmd->cmd_buffer_hw_base;
			cmdSize = p_in_data_cmd->cmd_len * sizeof(int);
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId, PA,
					       cmdSize, 0, 0, 0, 0, 0, 0);
		}
		p_out_coef_cmd =
		    (APP_CMD_BUFFER *)
		    HwAPPFifoGetKernelOutCoefRdCmdBuf(p_app_cmd_fifo);
		chanId = avioDhubChMap_ag_APPCMD_R_Z1;
		if (p_out_coef_cmd->cmd_len) {
			PA = p_out_coef_cmd->cmd_buffer_hw_base;
			cmdSize = p_out_coef_cmd->cmd_len * sizeof(int);
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId, PA,
					       cmdSize, 0, 0, 0, 0, 0, 0);
		}
		p_out_data_cmd =
		    (APP_CMD_BUFFER *)
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
	HwAPPFifoKernelRdUpdate(p_app_cmd_fifo, 1);
	app_start_cmd(p_app_cmd_fifo);
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

#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
	chanId = avioDhubChMap_ag_SA_R;
	if (bTST(instat, chanId)) {
		semaphore_pop(pSemHandle, chanId, 1);
		semaphore_clr_full(pSemHandle, chanId);
		aout_resume_cmd(LoRo_PATH);
		tasklet_hi_schedule(&amp_sa_tasklet);
	}
#endif

#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
	chanId = avioDhubChMap_ag_SPDIF_R;
	if (bTST(instat, chanId)) {
		semaphore_pop(pSemHandle, chanId, 1);
		semaphore_clr_full(pSemHandle, chanId);
		aout_resume_cmd(SPDIF_PATH);
		tasklet_hi_schedule(&amp_spdif_tasklet);
	}
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
#if defined (CONFIG_APP_IOCTL_MSG)
			MV_CC_MSG_t msg = { 0, 0, 0 };

			msg.m_MsgID = 1 << avioDhubSemMap_ag_app_intr2;
			PEMsgQ_Add(&hAPPMsgQ, &msg);
			up(&app_sem);
#else
			tasklet_hi_schedule(&amp_app_tasklet);
#endif
		}
	}

#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
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

static int amp_device_init(unsigned int cpu_id, void *pHandle)
{
	unsigned int vec_num;
	int err;

	vpp_cpcb0_vbi_int_cnt = 0;

    tasklet_enable(&amp_vpp_tasklet);
    tasklet_enable(&amp_vpp_cec_tasklet);
    tasklet_enable(&amp_vdec_tasklet);
    tasklet_enable(&amp_vip_tasklet);
    tasklet_enable(&amp_ma_tasklet);
    tasklet_enable(&amp_hdmi_tasklet);
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
    if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
   {
	 tasklet_enable(&amp_app_tasklet);
	 tasklet_enable(&amp_zsp_tasklet);
    }
#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
    tasklet_enable(&amp_aip_tasklet);
	tasklet_enable(&amp_spdif_tasklet);
	tasklet_enable(&amp_sa_tasklet);
	tasklet_enable(&amp_pg_done_tasklet);
    tasklet_enable(&amp_aip_avif_tasklet);
	tasklet_enable(&amp_rle_err_tasklet);
	tasklet_enable(&amp_rle_done_tasklet);
#endif

#if CONFIG_VPP_IOCTL_MSG
	sema_init(&vpp_sem, 0);
#endif

#if CONFIG_VPP_ISR_MSGQ
	err = PEMsgQ_Init(&hPEMsgQ, VPP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_trace("PEMsgQ_Init: falied, err:%8x\n", err);
		return err;
	}
#endif

    sema_init(&avif_sem, 0);
    err = PEMsgQ_Init(&hPEAVIFMsgQ, AVIF_ISR_MSGQ_SIZE);
    if (unlikely(err != S_OK)) {
       amp_trace("PEAVIFMsgQ_Init: falied, err:%8x\n", err);
       return err;
    }
    sema_init(&avif_hrx_sem, 0);
    err = PEMsgQ_Init(&hPEAVIFHRXMsgQ, AVIF_HRX_ISR_MSGQ_SIZE);
    if (unlikely(err != S_OK)) {
    amp_trace("PEAVIFHRXMsgQ_Init: falied, err:%8x\n", err);
    return err;
    }

#if CONFIG_VIP_IOCTL_MSG
	sema_init(&vip_sem, 0);
#endif
#if CONFIG_VIP_ISR_MSGQ
	err = PEMsgQ_Init(&hPEVIPMsgQ, VIP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_trace("PEVIPMsgQ_Init: falied, err:%8x\n", err);
		return err;
	}
#endif

#if CONFIG_HRX_IOCTL_MSG
	sema_init(&hrx_sem, 0);
	err = PEMsgQ_Init(&hPEHRXMsgQ, HDMIRX_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_trace("PEHRXMsgQ_Init: falied, err:%8x\n", err);
		return err;
	}
#endif

#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
	if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
	{
#if defined (CONFIG_APP_IOCTL_MSG)
		sema_init(&app_sem, 0);
		err = PEMsgQ_Init(&hAPPMsgQ, APP_ISR_MSGQ_SIZE);
		if (unlikely(err != S_OK)) {
			amp_trace("hAPPMsgQ init: failed, err:%8x\n", err);
			return err;
		}

#endif
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
	}
#endif

	amp_trace("amp_device_init ---ok");

	return S_OK;
}

static int amp_device_exit(unsigned int cpu_id, void *pHandle)
{
	int err = 0;

#if CONFIG_VIP_ISR_MSGQ
	err = PEMsgQ_Destroy(&hPEVIPMsgQ);
	if (unlikely(err != S_OK)) {
		amp_trace("vip MsgQ Destroy: falied, err:%8x\n", err);
		return err;
	}
#endif

    err = PEMsgQ_Destroy(&hPEAVIFMsgQ);
    if (unlikely(err != S_OK)) {
    amp_trace("avif MsgQ Destroy: falied, err:%8x\n", err);
    return err;
    }
    err = PEMsgQ_Destroy(&hPEAVIFHRXMsgQ);
    if (unlikely(err != S_OK)) {
    amp_trace("AVIF hrx MsgQ Destroy: falied, err:%8x\n", err);
    return err;
    }


#if CONFIG_HRX_IOCTL_MSG
	err = PEMsgQ_Destroy(&hPEHRXMsgQ);
	if (unlikely(err != S_OK)) {
		amp_trace("hrx MsgQ Destroy: falied, err:%8x\n", err);
		return err;
	}
#endif

#if CONFIG_VPP_ISR_MSGQ
	err = PEMsgQ_Destroy(&hPEMsgQ);
	if (unlikely(err != S_OK)) {
		amp_trace("vpp MsgQ Destroy: falied, err:%8x\n", err);
		return err;
	}
#endif
#if defined (CONFIG_APP_IOCTL_MSG)
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
	if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
	{
		err = PEMsgQ_Destroy(&hAPPMsgQ);
		if (unlikely(err != S_OK)) {
			amp_trace("app MsgQ Destroy: failed, err:%8x\n", err);
			return err;
		}
	}
#endif

	tasklet_disable(&amp_vpp_tasklet);
	tasklet_disable(&amp_vpp_cec_tasklet);
	tasklet_disable(&amp_vdec_tasklet);
	tasklet_disable(&amp_vip_tasklet);
	tasklet_disable(&amp_avif_tasklet);
	tasklet_disable(&amp_ma_tasklet);
	tasklet_disable(&amp_hdmi_tasklet);

#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
	if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
	{
		tasklet_disable(&amp_app_tasklet);
		tasklet_disable(&amp_zsp_tasklet);
	}
#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
	tasklet_disable(&amp_aip_tasklet);
	tasklet_disable(&amp_sa_tasklet);
	tasklet_disable(&amp_pg_done_tasklet);
	tasklet_disable(&amp_spdif_tasklet);
    tasklet_disable(&amp_aip_avif_tasklet);
	tasklet_disable(&amp_rle_err_tasklet);
	tasklet_disable(&amp_rle_done_tasklet);
#endif

	amp_trace("amp_device_exit ok");

	return S_OK;
}

/*******************************************************************************
  Module API
  */

static int amp_driver_open(struct inode *inode, struct file *filp)
{
	unsigned int vec_num;
	void *pHandle = &amp_dev;
	int err = 0;

#ifdef CONFIG_BERLIN_FASTLOGO
	printk(KERN_NOTICE "drv stop fastlogo\n");
	fastlogo_stop();
#endif

#ifdef CONFIG_BERLIN_BOOTLOGO
#if (BERLIN_CHIP_VERSION > BERLIN_BG2_A0)
	GA_REG_WORD32_WRITE(MEMMAP_AVIO_BCM_REG_BASE + RA_AVIO_BCM_AUTOPUSH, 0x0);
#endif
#endif

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
#if (BERLIN_CHIP_VERSION != BERLIN_BG2_CD) && (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)
    DhubInitialization(CPUINDEX, AVIF_DHUB_BASE, AVIF_HBO_SRAM_BASE,
			   &AVIF_dhubHandle, AVIF_config, AVIF_NUM_OF_CHANNELS);
#endif
#if (BERLIN_CHIP_VERSION < BERLIN_BG2_DTV)
#if (BERLIN_CHIP_VERSION != BERLIN_BG2_CD) && (BERLIN_CHIP_VERSION != BERLIN_BG2CDP)
    DhubInitialization(CPUINDEX, VIP_DHUB_BASE, VIP_HBO_SRAM_BASE,
           &VIP_dhubHandle, VIP_config, VIP_NUM_OF_CHANNELS);
#endif
#endif

	/* register and enable cec interrupt */
	vec_num = amp_irqs[IRQ_SM_CEC];
	err =
	    request_irq(vec_num, amp_devices_vpp_cec_isr, IRQF_DISABLED,
			"amp_module_vpp_cec", pHandle);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}

	/* register and enable VPP ISR */
	vec_num = amp_irqs[IRQ_DHUBINTRAVIO0];
	err =
	    request_irq(vec_num, amp_devices_vpp_isr, IRQF_DISABLED,
			"amp_module_vpp", pHandle);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}

#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
    //vec_num = 0x4a; //0x2a + 0x20
    /* register and enable AVIF ISR */
    vec_num = amp_irqs[IRQ_DHUBINTRAVIIF0];
    err =
    request_irq(vec_num, amp_devices_avif_isr, IRQF_DISABLED,
        "amp_module_avif", pHandle);
    if (unlikely(err < 0)) {
    amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
    return err;
    }

	/* register and enable VIP ISR */
	vec_num = amp_irqs[IRQ_DHUBINTRAVIO2];
	err =
	    request_irq(vec_num, amp_devices_vip_isr, IRQF_DISABLED,
			"amp_module_vip", pHandle);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}
#endif

	/* register and enable VDEC ISR */
	vec_num = amp_irqs[IRQ_DHUBINTRVPRO];
	/*
	 * when amp service is killed there may be pending interrupts
	 * do not enable VDEC interrupts automatically here unless requested by VDEC module
	 */
	irq_modify_status(vec_num, 0, IRQ_NOAUTOEN);
	err =
	    request_irq(vec_num, amp_devices_vdec_isr, IRQF_DISABLED,
			"amp_module_vdec", pHandle);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}

	/* register and enable audio out ISR */
	vec_num = amp_irqs[IRQ_DHUBINTRAVIO1];
	err =
	    request_irq(vec_num, amp_devices_aout_isr, IRQF_DISABLED,
			"amp_module_aout", pHandle);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}

#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
	if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
	{
		/* register and enable ZSP ISR */
		vec_num = amp_irqs[IRQ_ZSPINT];
		err =
		 request_irq(vec_num, amp_devices_zsp_isr, IRQF_DISABLED,
			"amp_module_zsp", pHandle);
		if (unlikely(err < 0)) {
			amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
			return err;
		}
	}

    amp_trace("amp_driver_open ok\n");
    isAmpReleased = 0;

	return 0;
}

#define VPP_BASE 0xf7f60000
#define CPCB0_OO_LAY12          0x14EA //FF
#define CPCB0_OO_LAY34          0x14EB //FF
#define CPCB0_OO_LAY56          0x1E54 //FF
#define CPCB0_OO_LAY7           0x1E55 //07

#define CPCB1_OO_LAY12          0x16EA //FF
#define CPCB1_OO_LAY34          0x16EB //FF
#define CPCB1_OO_LAY56          0x1EBC //FF
#define CPCB1_OO_LAY7           0x1EBD //07
static int amp_driver_release(struct inode *inode, struct file *filp)
{
    void *pHandle = &amp_dev;
    unsigned int vec_num;
    int err = 0;
    unsigned long aoutirq;

    GA_REG_WORD32_WRITE(VPP_BASE + (CPCB0_OO_LAY12 << 2), 0x0);
    GA_REG_WORD32_WRITE(VPP_BASE + (CPCB0_OO_LAY34 << 2), 0x0);
    GA_REG_WORD32_WRITE(VPP_BASE + (CPCB0_OO_LAY56 << 2), 0x0);
    GA_REG_WORD32_WRITE(VPP_BASE + (CPCB0_OO_LAY7 << 2), 0x0);

    GA_REG_WORD32_WRITE(VPP_BASE + (CPCB1_OO_LAY12 << 2), 0x0);
    GA_REG_WORD32_WRITE(VPP_BASE + (CPCB1_OO_LAY34 << 2), 0x0);
    GA_REG_WORD32_WRITE(VPP_BASE + (CPCB1_OO_LAY56 << 2), 0x0);
    GA_REG_WORD32_WRITE(VPP_BASE + (CPCB1_OO_LAY7 << 2), 0x0);

    if(isAmpReleased)
	return 0;

    /* unregister cec interrupt */
    free_irq(amp_irqs[IRQ_SM_CEC], pHandle);
    /* unregister VPP interrupt */
    free_irq(amp_irqs[IRQ_DHUBINTRAVIO0], pHandle);
#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
    /* unregister VIP interrupt */
    free_irq(amp_irqs[IRQ_DHUBINTRAVIO2], pHandle);
#endif
    /* unregister VDEC interrupt */
    free_irq(amp_irqs[IRQ_DHUBINTRVPRO], pHandle);
    /* unregister audio out interrupt */
    free_irq(amp_irqs[IRQ_DHUBINTRAVIO1], pHandle);
    /* unregister ZSP interrupt */
    free_irq(amp_irqs[IRQ_ZSPINT], pHandle);
#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
    /* unregister AVIF interrupt */
    free_irq(amp_irqs[IRQ_DHUBINTRAVIIF0], pHandle);
#endif

    /* Stop all commands */
    spin_lock_irqsave(&aout_spinlock, aoutirq);
    aout_stop_cmd(MULTI_PATH);
    aout_stop_cmd(LoRo_PATH);
    aout_stop_cmd(SPDIF_PATH);
    aout_stop_cmd(HDMI_PATH);
    spin_unlock_irqrestore(&aout_spinlock, aoutirq);

    isAmpReleased = 1;
	  amp_trace("amp_driver_release ok\n");

	return 0;
}

static long
amp_driver_ioctl_unlocked(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	VPP_DMA_INFO user_dma_info;
	int bcmbuf_info[2];
	int aout_info[2];
	int app_info[2];
	int aip_info[2];
	unsigned long irqstat, aoutirq, appirq, aipirq;
	unsigned int bcm_sche_cmd_info[3], q_id;
	int shm_mem = 0;
	PVOID param = NULL;

	switch (cmd) {
	case VPP_IOCTL_BCM_SCHE_CMD:
		if (copy_from_user
		    (bcm_sche_cmd_info, (void __user *)arg,
		     3 * sizeof(unsigned int)))
			return -EFAULT;
		q_id = bcm_sche_cmd_info[2];
		if (q_id > BCM_SCHED_Q5) {
			amp_trace("error BCM queue ID = %d\n", q_id);
			return -EFAULT;
		}
		bcm_sche_cmd[q_id][0] = bcm_sche_cmd_info[0];
		bcm_sche_cmd[q_id][1] = bcm_sche_cmd_info[1];
		break;

	case VPP_IOCTL_VBI_DMA_CFGQ:
		if (copy_from_user
		    (&user_dma_info, (void __user *)arg, sizeof(VPP_DMA_INFO)))
			return -EFAULT;

		dma_info[user_dma_info.cpcbID].DmaAddr =
		    (UINT32) MV_SHM_GetCacheVirtAddr(user_dma_info.DmaAddr);
		dma_info[user_dma_info.cpcbID].DmaLen = user_dma_info.DmaLen;
		break;

	case VPP_IOCTL_VBI_BCM_CFGQ:
		if (copy_from_user
		    (&user_dma_info, (void __user *)arg, sizeof(VPP_DMA_INFO)))
			return -EFAULT;

		vbi_bcm_info[user_dma_info.cpcbID].DmaAddr =
		    (UINT32) MV_SHM_GetCachePhysAddr(user_dma_info.DmaAddr);
		vbi_bcm_info[user_dma_info.cpcbID].DmaLen =
		    user_dma_info.DmaLen;
		break;

	case VPP_IOCTL_VDE_BCM_CFGQ:
		if (copy_from_user
		    (&user_dma_info, (void __user *)arg, sizeof(VPP_DMA_INFO)))
			return -EFAULT;

		vde_bcm_info[user_dma_info.cpcbID].DmaAddr =
		    (UINT32) MV_SHM_GetCachePhysAddr(user_dma_info.DmaAddr);
		vde_bcm_info[user_dma_info.cpcbID].DmaLen =
		    user_dma_info.DmaLen;
		break;

	case VPP_IOCTL_GET_MSG:
		{
#if CONFIG_VPP_IOCTL_MSG
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;

			rc = down_interruptible(&vpp_sem);
			if (rc < 0)
				return rc;

#ifdef CONFIG_IRQ_LATENCY_PROFILE
			amp_irq_profiler.vpp_task_sched_last =
			    cpu_clock(smp_processor_id());
#endif
#if CONFIG_VPP_ISR_MSGQ

			// check fullness, clear message queue once.
			// only send latest message to task.
			if (PEMsgQ_Fullness(&hPEMsgQ) <= 0) {
				//amp_trace(" E/[vpp isr task]  message queue empty\n");
				return -EFAULT;
			}

			PEMsgQ_DequeueRead(&hPEMsgQ, &msg);

			if (!atomic_read(&vpp_isr_msg_err_cnt)) {
				// msgQ get full, if isr task can run here, reset msgQ
				//fullness--;
				//PEMsgQ_Dequeue(&hPEMsgQ, fullness);
				atomic_set(&vpp_isr_msg_err_cnt, 0);
			}
#else
			msg.m_MsgID = VPP_CC_MSG_TYPE_VPP;
			msg.m_Param1 = vpp_instat;
			msg.m_Param2 = vpp_intr_timestamp;
#endif
			if (copy_to_user
			    ((void __user *)arg, &msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			break;
#else
			return -EFAULT;
#endif
		}
	case CEC_IOCTL_RX_MSG_BUF_MSG:	// copy cec rx message to user space buffer
		if (copy_to_user
		    ((void __user *)arg, &rx_buf, sizeof(VPP_CEC_RX_MSG_BUF)))
			return -EFAULT;

		return S_OK;
		break;
	case VPP_IOCTL_START_BCM_TRANSACTION:
		if (copy_from_user
		    (bcmbuf_info, (void __user *)arg, 2 * sizeof(int)))
			return -EFAULT;
		spin_lock_irqsave(&bcm_spinlock, irqstat);
		dhub_channel_write_cmd(&(VPP_dhubHandle.dhub),
				       avioDhubChMap_vpp_BCM_R_auto, bcmbuf_info[0],
				       bcmbuf_info[1], 0, 0, 0, 1, 0, 0);
		spin_unlock_irqrestore(&bcm_spinlock, irqstat);
		break;

	case VPP_IOCTL_INTR_MSG:
		//get VPP INTR MASK info
		{
			INTR_MSG vpp_intr_info = { 0, 0 };

			if (copy_from_user
			    (&vpp_intr_info, (void __user *)arg,
			     sizeof(INTR_MSG)))
				return -EFAULT;

			if (vpp_intr_info.DhubSemMap < MAX_INTR_NUM)
				vpp_intr_status[vpp_intr_info.DhubSemMap] =
				    vpp_intr_info.Enable;
			else
				return -EFAULT;

			break;
		}
    case AVIF_IOCTL_GET_MSG:
    {
        MV_CC_MSG_t msg = { 0 };
        HRESULT rc = S_OK;
        rc = down_interruptible(&avif_sem);
        if (rc < 0)
            return rc;

        rc = PEMsgQ_ReadTry(&hPEAVIFMsgQ, &msg);
        if (unlikely(rc != S_OK)) {
            amp_trace("AVIF read message queue failed\n");
            return -EFAULT;
        }
        PEMsgQ_ReadFinish(&hPEAVIFMsgQ);
        if (!atomic_read(&avif_isr_msg_err_cnt)) {
            atomic_set(&avif_isr_msg_err_cnt, 0);
        }
        if (copy_to_user
            ((void __user *)arg, &msg, sizeof(MV_CC_MSG_t)))
            return -EFAULT;
        break;
    }
    case AVIF_IOCTL_VDE_CFGQ:
        break;
    case AVIF_IOCTL_VBI_CFGQ:
        break;
    case AVIF_IOCTL_SD_WRE_CFGQ:
        break;
    case AVIF_IOCTL_SD_RDE_CFGQ:
        break;
    case AVIF_IOCTL_SEND_MSG:
    {
         //get msg from AVIF
         int avif_msg = 0;
         if (copy_from_user(&avif_msg, (void __user *)arg, sizeof(int)))
             return -EFAULT;

         if (avif_msg == AVIF_MSG_DESTROY_ISR_TASK) {
            //force one more INT to AVIF to destroy ISR task
             printk("Destroy ISR Task...\r\n");
            up(&avif_sem);
        }
         printk("Destroyed ISR Task...\r\n");
         break;
    }
    case AVIF_IOCTL_INTR_MSG:
    {
        //get AVIF INTR MASK info
        INTR_MSG avif_intr_info = { 0, 0 };

        if (copy_from_user(&avif_intr_info, (void __user *)arg, sizeof(INTR_MSG)))
           return -EFAULT;

       if (avif_intr_info.DhubSemMap<MAX_INTR_NUM)
            avif_intr_status[avif_intr_info.DhubSemMap] = avif_intr_info.Enable;
        else
            return -EFAULT;
       break;
    }
    case AVIF_HRX_IOCTL_GET_MSG:
    {
        MV_CC_MSG_t msg = { 0 };
        HRESULT rc = S_OK;
        rc = down_interruptible(&avif_hrx_sem);
        if (rc < 0)
           return rc;

        rc = PEMsgQ_ReadTry(&hPEAVIFHRXMsgQ, &msg);
        if (unlikely(rc != S_OK)) {
            amp_trace("HRX read message queue failed\n");
            return -EFAULT;
        }
        PEMsgQ_ReadFinish(&hPEAVIFHRXMsgQ);
        if (copy_to_user
            ((void __user *)arg, &msg, sizeof(MV_CC_MSG_t)))
            return -EFAULT;
        break;
    }
    case AVIF_HRX_IOCTL_SEND_MSG: /* get msg from User Space */
    {
        int hrx_msg = 0;
        if (copy_from_user
            (&hrx_msg, (void __user *)arg, sizeof(int)))
           return -EFAULT;

        if (hrx_msg == AVIF_HRX_DESTROY_ISR_TASK) {
        /* force one more INT to HRX to destroy ISR task */
            up(&avif_hrx_sem);
         }

         break;
    }

	    /**************************************
             * VIP IOCTL
             **************************************/
	case VIP_IOCTL_GET_MSG:
		{
#if CONFIG_VIP_IOCTL_MSG
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;

			rc = down_interruptible(&vip_sem);
			if (rc < 0)
				return rc;

#if CONFIG_VIP_ISR_MSGQ
			rc = PEMsgQ_ReadTry(&hPEVIPMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("VIP read message queue failed\n");
				return -EFAULT;
			}
			PEMsgQ_ReadFinish(&hPEVIPMsgQ);
			if (!atomic_read(&vip_isr_msg_err_cnt)) {
				atomic_set(&vip_isr_msg_err_cnt, 0);
			}
#endif
			if (copy_to_user
			    ((void __user *)arg, &msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			break;
#else
			return -EFAULT;
#endif
		}
	case VIP_IOCTL_VDE_BCM_CFGQ:
		//TODO: get VBI data BCM CFGQ from user space
		{
			VIP_DMA_INFO user_vip_info;

			if (copy_from_user
			    (&user_vip_info, (void __user *)arg,
			     sizeof(VIP_DMA_INFO)))
				return -EFAULT;

			vip_vbi_info.DmaAddr =
			    (UINT32) MV_SHM_GetCachePhysAddr(user_vip_info.
							     DmaAddr);
			vip_vbi_info.DmaLen = user_vip_info.DmaLen;

			break;
		}
	case VIP_IOCTL_VBI_BCM_CFGQ:
		//TODO: get BCM CFGQ from user space
		{
			VIP_DMA_INFO user_vip_info;

			if (copy_from_user
			    (&user_vip_info, (void __user *)arg,
			     sizeof(VIP_DMA_INFO)))
				return -EFAULT;

			vip_dma_info.DmaAddr =
			    (UINT32) MV_SHM_GetCachePhysAddr(user_vip_info.
							     DmaAddr);
			vip_dma_info.DmaLen = user_vip_info.DmaLen;

			break;
		}

	case VIP_IOCTL_SD_WRE_CFGQ:
		//TODO: get BCM CFGQ from user space
		{
			VIP_DMA_INFO user_vip_info;

			if (copy_from_user
			    (&user_vip_info, (void __user *)arg,
			     sizeof(VIP_DMA_INFO)))
				return -EFAULT;

			vip_sd_wr_info.DmaAddr =
			    (UINT32) MV_SHM_GetCachePhysAddr(user_vip_info.
							     DmaAddr);
			vip_sd_wr_info.DmaLen = user_vip_info.DmaLen;

			//setup WR DMA in advance!
			start_vip_sd_wr_bcm();

			break;
		}
	case VIP_IOCTL_SD_RDE_CFGQ:
		//TODO: get BCM CFGQ from user space
		{
			VIP_DMA_INFO user_vip_info;

			if (copy_from_user
			    (&user_vip_info, (void __user *)arg,
			     sizeof(VIP_DMA_INFO)))
				return -EFAULT;

			vip_sd_rd_info.DmaAddr =
			    (UINT32) MV_SHM_GetCachePhysAddr(user_vip_info.
							     DmaAddr);
			vip_sd_rd_info.DmaLen = user_vip_info.DmaLen;

			//setup RD DMA in advance!
			start_vip_sd_rd_bcm();

			break;
		}

	case VIP_IOCTL_SEND_MSG:
		//get msg from VIP
		{
			int vip_msg = 0;
			if (copy_from_user
			    (&vip_msg, (void __user *)arg, sizeof(int)))
				return -EFAULT;

			if (vip_msg == VIP_MSG_DESTROY_ISR_TASK) {
				//force one more INT to VIP to destroy ISR task
				up(&vip_sem);
			}

			break;
		}

	case VIP_IOCTL_INTR_MSG:
		//get VIP INTR MASK info
		{
			INTR_MSG vip_intr_info = { 0, 0 };

			if (copy_from_user
			    (&vip_intr_info, (void __user *)arg,
			     sizeof(INTR_MSG)))
				return -EFAULT;

			if (vip_intr_info.DhubSemMap < MAX_INTR_NUM)
				vip_intr_status[vip_intr_info.DhubSemMap] =
				    vip_intr_info.Enable;
			else
				return -EFAULT;

			break;
		}

	case VDEC_IOCTL_ENABLE_INT:
#if CONFIG_VDEC_IRQ_CHECKER
		vdec_enable_int_cnt++;
#endif
		enable_irq(amp_irqs[IRQ_DHUBINTRVPRO]);

		break;
	    /**************************************
             * HDMI RX IOCTL
             **************************************/
#if CONFIG_HRX_IOCTL_MSG
	case HDMIRX_IOCTL_GET_MSG:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;

			rc = down_interruptible(&hrx_sem);
			if (rc < 0)
				return rc;

			rc = PEMsgQ_ReadTry(&hPEHRXMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("HRX read message queue failed\n");
				return -EFAULT;
			}
			PEMsgQ_ReadFinish(&hPEHRXMsgQ);
			if (copy_to_user
			    ((void __user *)arg, &msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			break;
		}
	case HDMIRX_IOCTL_SEND_MSG:	/* get msg from User Space */
		{
			int hrx_msg = 0;
			if (copy_from_user
			    (&hrx_msg, (void __user *)arg, sizeof(int)))
				return -EFAULT;

			if (hrx_msg == HRX_MSG_DESTROY_ISR_TASK) {
				/* force one more INT to HRX to destroy ISR task */
				up(&hrx_sem);
			}

			break;
		}
#endif
	case AOUT_IOCTL_START_CMD:
		if (copy_from_user
		    (aout_info, (void __user *)arg, 2 * sizeof(int)))
			return -EFAULT;
		shm_mem = aout_info[1];
		param = MV_SHM_GetNonCacheVirtAddr(shm_mem);
		if (!param) {
			amp_trace("ctl:%x bad shm mem offset:%x\n", AOUT_IOCTL_START_CMD, shm_mem);
			return -EFAULT;
		}
		//MV_SHM_Takeover(shm_mem);
		spin_lock_irqsave(&aout_spinlock, aoutirq);
		aout_start_cmd(aout_info, param);
		spin_unlock_irqrestore(&aout_spinlock, aoutirq);
		break;

#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
	case AIP_IOCTL_START_CMD:
		if (copy_from_user
		    (aip_info, (void __user *)arg, 2 * sizeof(int))) {
			return -EFAULT;
		}
		shm_mem = aip_info[1];
		param = MV_SHM_GetNonCacheVirtAddr(shm_mem);
		if (!param) {
			amp_trace("ctl:%x bad shm mem offset:%x\n", AIP_IOCTL_START_CMD, shm_mem);
			return -EFAULT;
		}
		//MV_SHM_Takeover(shm_mem);
		spin_lock_irqsave(&aip_spinlock, aipirq);
		aip_start_cmd(aip_info, param);
		spin_unlock_irqrestore(&aip_spinlock, aipirq);
		break;

	case AIP_IOCTL_STOP_CMD:
		spin_lock_irqsave(&aip_spinlock, aipirq);
		aip_stop_cmd();
		spin_unlock_irqrestore(&aip_spinlock, aipirq);
		break;
    case AIP_AVIF_IOCTL_START_CMD:
    if (copy_from_user
        (aip_info, (void __user *)arg, 2 * sizeof(int))) {
        return -EFAULT;
    }
    shm_mem = aip_info[1];
    param = MV_SHM_GetNonCacheVirtAddr(shm_mem);
    if (!param) {
        amp_trace("ctl:%x bad shm mem offset:%x\n", AIP_AVIF_IOCTL_START_CMD, shm_mem);
        return -EFAULT;
    }
    spin_lock_irqsave(&aip_spinlock, aipirq);
    aip_avif_start_cmd(aip_info, param);
    spin_unlock_irqrestore(&aip_spinlock, aipirq);
    break;

    case AIP_AVIF_IOCTL_SET_MODE: //MAIN/PIP Audio
    {
        int audio_mode = 0;
        if (copy_from_user(&audio_mode, (void __user *)arg, sizeof(int)))
           return -EFAULT;
        printk("Audio switch mode = 0x%x\r\n", audio_mode);
        IsPIPAudio = audio_mode;
    }
    break;
    case AIP_AVIF_IOCTL_STOP_CMD:
        aip_avif_stop_cmd();
    break;
#endif

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

	case APP_IOCTL_GET_MSG_CMD:
#if defined CONFIG_APP_IOCTL_MSG
#if (BERLIN_CHIP_VERSION == BERLIN_BG2CDP)
		if (berlin_chip_revision < BERLIN_BG2CDP_A0_EXT)
#endif
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;
			rc = down_interruptible(&app_sem);
			if (rc < 0)
				return rc;
			rc = PEMsgQ_ReadTry(&hAPPMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("HRX read message queue failed\n");
				return -EFAULT;
			}
			PEMsgQ_ReadFinish(&hAPPMsgQ);
			if (copy_to_user
			    ((void __user *)arg, &msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			}
			break;
#else
			return -EFAULT;
#endif

	default:
		break;
	}

	return 0;
}

static int
read_proc_status(char *page, char **start, off_t offset,
		 int count, int *eof, void *data)
{
	int len = 0;
	int cnt;

	len +=
	    sprintf(page + len, "%-25s : %d %d  %x\n", "PE Module IRQ cnt",
		    vpp_cpcb0_vbi_int_cnt, dma_cnt / 3,
		    (UINT32) __pa(dma_proc));

	for (cnt = 0; cnt < 1000; cnt++) {
		len +=
		    sprintf(page + len, "%s :  %x, %x, %x\n", "DMA",
			    dma_proc[3 * cnt], dma_proc[3 * cnt + 1],
			    dma_proc[3 * cnt + 2]);
	}

	amp_debug("%s OK. (%d / %d)\n", __func__, len, count);

	*eof = 1;

	return ((count < len) ? count : len);
}

static int
read_proc_detail(char *page, char **start, off_t offset,
		 int count, int *eof, void *data)
{
	int len = 0;

	len +=
	    sprintf(page + len, "%-25s : %d\n", "PE Module IRQ cnt",
		    vpp_cpcb0_vbi_int_cnt);

#if CONFIG_VDEC_IRQ_CHECKER
	len +=
	    sprintf(page + len, "VMETA interrupt cnt %d enable_cnt %d\n",
	        vdec_int_cnt, vdec_enable_int_cnt);
#endif
    amp_debug("%s OK. (%d / %d)\n", __func__, len, count);

	*eof = 1;

	return ((count < len) ? count : len);
}

/*******************************************************************************
  Module Register API
  */
static int
amp_driver_setup_cdev(struct cdev *dev, int major, int minor,
          struct file_operations *fops)
{
    cdev_init(dev, fops);
    dev->owner = THIS_MODULE;
    dev->ops = fops;
    return cdev_add(dev, MKDEV(major, minor), 1);
}

int __init amp_drv_init(struct amp_device_t *amp_device)
{
    int res;

    /* Now setup cdevs. */
    res =
    amp_driver_setup_cdev(&amp_device->cdev, amp_device->major,
              amp_device->minor, amp_device->fops);
    if (res) {
    amp_error("amp_driver_setup_cdev failed.\n");
    res = -ENODEV;
    goto err_add_device;
    }
    amp_trace("setup cdevs device minor [%d]\n", amp_device->minor);

    /* add PE devices to sysfs */
    amp_device->dev_class = class_create(THIS_MODULE, amp_device->dev_name);
    if (IS_ERR(amp_device->dev_class)) {
    amp_error("class_create failed.\n");
    res = -ENODEV;
    goto err_add_device;
    }

    device_create(amp_device->dev_class, NULL,
          MKDEV(amp_device->major, amp_device->minor), NULL,
          amp_device->dev_name);
    amp_trace("create device sysfs [%s]\n", amp_device->dev_name);

    /* create hw device */
    if (amp_device->dev_init) {
    res = amp_device->dev_init(amp_device, 0);
    if (res != 0) {
        amp_error("amp_int_init failed !!! res = 0x%08X\n",
              res);
        res = -ENODEV;
        goto err_add_device;
    }
    }
    /* create PE device proc file */
    amp_device->dev_procdir = proc_mkdir(amp_device->dev_name, NULL);
    //amp_driver_procdir->owner = THIS_MODULE;
    create_proc_read_entry(AMP_DEVICE_PROCFILE_STATUS, 0,
               amp_device->dev_procdir, read_proc_status, NULL);
    create_proc_read_entry(AMP_DEVICE_PROCFILE_DETAIL, 0,
               amp_device->dev_procdir, read_proc_detail, NULL);

    return 0;

 err_add_device:

    if (amp_device->dev_procdir) {
    remove_proc_entry(AMP_DEVICE_PROCFILE_DETAIL,
              amp_device->dev_procdir);
    remove_proc_entry(AMP_DEVICE_PROCFILE_STATUS,
              amp_device->dev_procdir);
    remove_proc_entry(amp_device->dev_name, NULL);
    }

    if (amp_device->dev_class) {
    device_destroy(amp_device->dev_class,
               MKDEV(amp_device->major, amp_device->minor));
    class_destroy(amp_device->dev_class);
    }

    cdev_del(&amp_device->cdev);

    return res;
}

int __exit amp_drv_exit(struct amp_device_t *amp_device)
{
    int res;

    amp_trace("amp_drv_exit [%s] enter\n", amp_device->dev_name);

    /* destroy kernel API */
    if (amp_device->dev_exit) {
    res = amp_device->dev_exit(amp_device, 0);
    if (res != 0)
        amp_error("dev_exit failed !!! res = 0x%08X\n", res);
    }

    if (amp_device->dev_procdir) {
    /* remove PE device proc file */
    remove_proc_entry(AMP_DEVICE_PROCFILE_DETAIL,
              amp_device->dev_procdir);
    remove_proc_entry(AMP_DEVICE_PROCFILE_STATUS,
              amp_device->dev_procdir);
    remove_proc_entry(amp_device->dev_name, NULL);
    }

    if (amp_device->dev_class) {
    /* del sysfs entries */
    device_destroy(amp_device->dev_class,
               MKDEV(amp_device->major, amp_device->minor));
    amp_trace("delete device sysfs [%s]\n", amp_device->dev_name);

    class_destroy(amp_device->dev_class);
    }
    /* del cdev */
    cdev_del(&amp_device->cdev);

    return 0;
}

struct amp_device_t legacy_pe_dev = {
    .dev_name = AMP_DEVICE_NAME,
    .minor = AMP_MINOR,
    .dev_init = amp_device_init,
    .dev_exit = amp_device_exit,
    .fops = &amp_ops,
};

static int __init amp_module_init(void)
{
    int i, res;
    dev_t pedev;
    struct device_node *np, *iter;
    struct resource r;
    u32 chip_ext, channel_num;

#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
    np = of_find_compatible_node(NULL, NULL, "marvell,berlin-amp");
#else //CD is using old driver
    np = of_find_compatible_node(NULL, NULL, "mrvl,berlin-amp");
#endif
    if (!np)
    return -ENODEV;
    for (i = 0; i <= IRQ_DHUBINTRVPRO; i++) {
    of_irq_to_resource(np, i, &r);
    amp_irqs[i] = r.start;
    }
#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
    //TODO:get the interrupt number dynamically
    amp_irqs[IRQ_DHUBINTRAVIIF0] = 0x4a;
#endif

    for_each_child_of_node(np, iter) {
#if !(defined(BERLIN_BG2_CD) || defined(BERLIN_BG2CDP))
    if (of_device_is_compatible(iter, "marvell,berlin-cec")) {
#else
    if (of_device_is_compatible(iter, "mrvl,berlin-cec")) {
#endif
        of_irq_to_resource(iter, 0, &r);
        amp_irqs[IRQ_SM_CEC] = r.start;
	}
#if defined (BERLIN_BG2CDP)
	if (of_device_is_compatible(iter, "mrvl,berlin-chip-ext")) {
		 if (!of_property_read_u32(iter, "revision", &chip_ext))
			berlin_chip_revision = chip_ext;
	}

	if (of_device_is_compatible(iter, "mrvl,berlin-avio")) {
		 if (!of_property_read_u32(iter, "num", &channel_num))
			avioDhub_channel_num = channel_num;
	}
#endif
    }
    of_node_put(np);

    res = alloc_chrdev_region(&pedev, 0, AMP_MAX_DEVS, AMP_DEVICE_NAME);
    amp_major = MAJOR(pedev);
    if (res < 0) {
    amp_error("alloc_chrdev_region() failed for amp\n");
    goto err_reg_device;
    }
    amp_trace("register cdev device major [%d]\n", amp_major);

    legacy_pe_dev.major = amp_major;
    res = amp_drv_init(&legacy_pe_dev);
    amp_dev = legacy_pe_dev.cdev;

    snd_dev.major = amp_major;
    res = amp_drv_init(&snd_dev);

    res = vmeta_sched_driver_init(amp_major, AMP_VMETA_MINOR);
    if (res != 0)
    amp_error("vmeta_sched_driver_init failed !!! res = 0x%08X\n",
          res);
    amp_trace("amp_driver_init OK\n");

    return 0;
 err_reg_device:
    amp_drv_exit(&legacy_pe_dev);
    amp_drv_exit(&snd_dev);
    vmeta_sched_driver_exit();

    unregister_chrdev_region(MKDEV(amp_major, 0), AMP_MAX_DEVS);

    amp_trace("amp_driver_init failed !!! (%d)\n", res);

    return res;
}

static void __exit amp_module_exit(void)
{
    amp_trace("amp_driver_exit 1\n");

    amp_drv_exit(&legacy_pe_dev);

    amp_trace("amp_driver_exit 2\n");
    amp_drv_exit(&snd_dev);
    amp_trace("amp_driver_exit 3\n");
    vmeta_sched_driver_exit();

    unregister_chrdev_region(MKDEV(amp_major, 0), AMP_MAX_DEVS);
    amp_trace("unregister cdev device major [%d]\n", amp_major);
    amp_major = 0;

    amp_trace("amp_driver_exit OK\n");
}

module_init(amp_module_init);
module_exit(amp_module_exit);

/*******************************************************************************
    Module Descrption
*/
MODULE_AUTHOR("marvell");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("pe module template");
