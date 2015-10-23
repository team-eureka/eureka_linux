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
#ifndef _AMP_DRIVER_H_
#define _AMP_DRIVER_H_

// server id for ICC message queue
#define MV_GS_DMX_Serv						(0x00060008)
#define MV_GS_VPP_Serv						(0x00060009)
#define MV_GS_AUD_Serv						(0x0006000a)
#define MV_GS_ZSP_Serv						(0x0006000b)
#define MV_GS_VDEC_Serv						(0x0006000c)
#define MV_GS_RLE_Serv						(0x0006000d)
#define MV_GS_VIP_Serv						(0x0006000e)
#define MV_GS_AIP_Serv						(0x0006000f)
#define MV_GS_VDM_Serv						(0x00060010)
#define MV_GS_AGENT_Serv					(0x00060011)
#define MV_GS_HWAPP_Serv					(0x00060012)
#define MV_GS_AVIF_Serv						(0x00060013)

#define MV_BERLIN_CPU0					0
#define AMP_MODULE_MSG_ID_VPP			MV_GS_VPP_Serv
#define AMP_MODULE_MSG_ID_VDEC			MV_GS_VDEC_Serv
#define AMP_MODULE_MSG_ID_AUD			MV_GS_AUD_Serv
#define AMP_MODULE_MSG_ID_ZSP			MV_GS_ZSP_Serv
#define AMP_MODULE_MSG_ID_RLE			MV_GS_RLE_Serv
#define AMP_MODULE_MSG_ID_VIP			MV_GS_VIP_Serv
#define AMP_MODULE_MSG_ID_AIP			MV_GS_AIP_Serv
#define AMP_MODULE_MSG_ID_HWAPP			MV_GS_HWAPP_Serv
#define AMP_MODULE_MSG_ID_AVIF			MV_GS_AVIF_Serv

#if 0//(BERLIN_CHIP_VERSION >= BERLIN_BG2_Q)
enum {
	IRQ_ZSPINT,
	IRQ_DHUBINTRAVIO0,
	IRQ_DHUBINTRAVIIF0, //interrupt number 42 + 32
	IRQ_DHUBINTRAVIO2,
	IRQ_DHUBINTRVPRO,
	IRQ_SM_CEC,
	IRQ_AMP_MAX,
};
#else
enum {
	IRQ_ZSPINT,
	IRQ_DHUBINTRAVIO0,
	IRQ_DHUBINTRAVIO1,
	IRQ_DHUBINTRAVIO2,
	IRQ_DHUBINTRVPRO,
	IRQ_SM_CEC,
	IRQ_DHUBINTRAVIIF0,
	IRQ_AMP_MAX,
};
#endif
extern int amp_irqs[IRQ_AMP_MAX];
#define AMP_MAX_DEVS            8
#define AMP_MINOR               0
#define AMP_VMETA_MINOR         1
#define AMP_SND_MINOR           2

#define MAX_CHANNELS 8
typedef enum {
	MULTI_PATH = 0,
	LoRo_PATH = 1,
	SPDIF_PATH = 2,
	HDMI_PATH = 3,
	MAX_OUTPUT_AUDIO = 5,
} AUDIO_PATH;

typedef enum {
	MAIN_AUDIO = 0,
	SECOND_AUDIO = 1,
	EFFECT_AUDIO = 2,
	EXT_IN_AUDIO = 10,
	BOX_EFFECT_AUDIO = 11,
	MAX_INTERACTIVE_AUDIOS = 8,
	MAX_INPUT_AUDIO = 12,
	AUDIO_INPUT_IRRELEVANT,
	NOT_DEFINED_AUDIO,
} AUDIO_CHANNEL;

extern INT32 pri_audio_chanId_Z1[4];
extern INT32 pri_audio_chanId_A0[4];

typedef struct aout_dma_info_t {
	UINT32 addr0;
	UINT32 size0;
	UINT32 addr1;
	UINT32 size1;
} AOUT_DMA_INFO;

typedef struct aout_path_cmd_fifo_t {
	AOUT_DMA_INFO aout_dma_info[4][8];
	UINT32 update_pcm[8];
	UINT32 takeout_size[8];
	UINT32 size;
	UINT32 wr_offset;
	UINT32 rd_offset;
	UINT32 kernel_rd_offset;
	UINT32 zero_buffer;
	UINT32 zero_buffer_size;
	UINT32 fifo_underflow;
} AOUT_PATH_CMD_FIFO;

typedef struct aip_dma_cmd_t {
	UINT32 addr0;
	UINT32 size0;
	UINT32 addr1;
	UINT32 size1;
} AIP_DMA_CMD;

typedef struct aip_cmd_fifo_t {
	AIP_DMA_CMD aip_dma_cmd[4][8];
	UINT32 update_pcm[8];
	UINT32 takein_size[8];
	UINT32 size;
	UINT32 wr_offset;
	UINT32 rd_offset;
	UINT32 kernel_rd_offset;
	UINT32 prev_fifo_overflow_cnt;
	/* used by kernel */
	UINT32 kernel_pre_rd_offset;
	UINT32 overflow_buffer;
	UINT32 overflow_buffer_size;
	UINT32 fifo_overflow;
	UINT32 fifo_overflow_cnt;
} AIP_DMA_CMD_FIFO;

typedef struct {
	UINT uiShmOffset;	/**< Not used by kernel */
	UINT *unaCmdBuf;
	UINT *cmd_buffer_base;
	UINT max_cmd_size;
	UINT cmd_len;
	UINT cmd_buffer_hw_base;
	VOID *p_cmd_tag;
} APP_CMD_BUFFER;

typedef struct {
	UINT m_block_samps;
	UINT m_fs;
	UINT m_special_flags;
} HWAPP_DATA_DS;

typedef struct {
	HWAPP_DATA_DS m_stream_in_data[MAX_INPUT_AUDIO];
	HWAPP_DATA_DS m_path_out_data[MAX_OUTPUT_AUDIO];
} HWAPP_EVENT_CTX;

typedef struct {
	UINT uiShmOffset;	/**< Not used by kernel */
	APP_CMD_BUFFER in_coef_cmd[8];
	APP_CMD_BUFFER out_coef_cmd[8];
	APP_CMD_BUFFER in_data_cmd[8];
	APP_CMD_BUFFER out_data_cmd[8];
	UINT size;
	UINT wr_offset;
	UINT rd_offset;
	UINT kernel_rd_offset;
/************** used by Kernel *****************/
	UINT kernel_idle;
} HWAPP_CMD_FIFO;

struct amp_device_t;

typedef struct amp_device_t {
	unsigned char *dev_name;
	struct cdev cdev;
	struct class *dev_class;
	struct file_operations *fops;
	struct mutex mutex;
	int major;
	int minor;
	struct proc_dir_entry *dev_procdir;
	void *private_data;

	int (*dev_init) (struct amp_device_t *, unsigned int);
	int (*dev_exit) (struct amp_device_t *, unsigned int);

};

#endif				//_PE_DRIVER_H_
