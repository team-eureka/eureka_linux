/*************************************************************************************
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
***************************************************************************************/
#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/socket.h>
#include <linux/file.h>
#include <linux/completion.h>
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>

#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/rawmidi.h>
#include <sound/hwdep.h>
#include <sound/initval.h>
#include <sound/pcm-indirect.h>

#include "berlin_memmap.h"
#include "api_avio_dhub.h"
#include "api_dhub.h"
#include "api_aio.h"
#include "api_spdif.h"
#include "spdif_enc.h"
#include "api_avpll.h"

#define MAX_BUFFER_SIZE        (32 * 1024)
#define IRQ_GIC_START          (32)
#define IRQ_dHubIntrAvio1      (0x22)
#define IRQ_DHUBINTRAVIO1      (IRQ_GIC_START + IRQ_dHubIntrAvio1)
#define bTST(x, b)             (((x) >> (b)) & 1)

enum {
	SNDRV_BERLIN_GET_OUTPUT_MODE = 0x1001,
	SNDRV_BERLIN_SET_OUTPUT_MODE,
};

enum {
	SND_BERLIN_STATUS_STOP = 0,
	SND_BERLIN_STATUS_PLAY,
};

enum {
	SND_BERLIN_OUTPUT_ANALOG = 0x200,
	SND_BERLIN_OUTPUT_SPDIF,
};

struct snd_berlin_chip {
	struct snd_card *card;
	struct snd_hwdep *hwdep;
	struct snd_pcm *pcm;
};

static volatile unsigned int berlin_output_mode = SND_BERLIN_OUTPUT_ANALOG;

struct snd_berlin_card_pcm {
	unsigned int pcm_size;
	unsigned int pcm_count;
	unsigned int pcm_bps;          /* bytes per second */
	unsigned int pcm_buf_pos;      /* position in buffer */
	unsigned int dma_size;         /* dma trunk size in byte */

	/* spdif DMA buffer */
	unsigned char *spdif_dma_area; /* dma buffer for spdif output */
	unsigned int spdif_dma_addr;   /* physical address of spdif buffer */
	unsigned int spdif_dma_bytes;  /* size of spdif dma area */

	/* PCM DMA buffer */
	unsigned char *pcm_dma_area;
	unsigned int pcm_dma_addr;
	unsigned int pcm_dma_bytes;
	unsigned int pcm_virt_bytes;

	/* hw parameter */
	unsigned int sample_rate;
	unsigned int sample_format;
	unsigned int channel_num;
	unsigned int pcm_ratio;
	unsigned int spdif_ratio;

	/* for spdif encoding */
	unsigned int spdif_frames;
	unsigned char channel_status[24];

	/* playback status */
	volatile unsigned int playback_state;
	volatile unsigned int output_mode;

	struct snd_pcm_substream *substream;
	struct snd_pcm_indirect pcm_indirect;
};

static struct snd_card *snd_berlin_card;

static DEFINE_MUTEX(berlin_mutex);

static void berlin_set_aio(unsigned int sample_rate, unsigned int output_mode)
{
	unsigned int analog_div, spdif_div;

	AIO_SetAudChMute(AIO_SEC, AIO_TSD0, AUDCH_CTRL_MUTE_MUTE_ON);
	AIO_SetAudChEn(AIO_SEC, AIO_TSD0, AUDCH_CTRL_ENABLE_DISABLE);

	switch(sample_rate) {
	case 48000 :
	case 44100 :
	case 32000 :
		analog_div = AIO_DIV8;
		spdif_div  = AIO_DIV4;
		break;
	case 96000 :
	case 88200 :
		analog_div = AIO_DIV4;
		spdif_div  = AIO_DIV2;
		break;
	default:
		break;
	}

	if(output_mode == SND_BERLIN_OUTPUT_ANALOG) {
		AIO_SetClkDiv(AIO_SEC, analog_div);
		AIO_SetCtl(AIO_SEC, AIO_I2S_MODE, AIO_32CFM, AIO_24DFM);
	} else if(output_mode == SND_BERLIN_OUTPUT_SPDIF) {
		AIO_SetClkDiv(AIO_SEC, spdif_div);
		AIO_SetCtl(AIO_SEC, AIO_I2S_MODE, AIO_32CFM, AIO_32DFM);
	}

	AIO_SetAudChEn(AIO_SEC, AIO_TSD0, AUDCH_CTRL_ENABLE_ENABLE);
	AIO_SetAudChMute(AIO_SEC, AIO_TSD0, AUDCH_CTRL_MUTE_MUTE_OFF);
}

static void berlin_set_pll(unsigned int sample_rate)
{
	int apll;

	switch (sample_rate) {
	case 44100 :
	case 88200 :
		apll = 22579200;
		break;
	case 32000 :
		apll = 16384000;
		break;
	case 48000 :
	case 96000 :
		apll = 24576000;
		break;
	default :
		apll = 24576000;
		break;
	}

	AVPLL_Set(0, 3, apll);
	AVPLL_Set(0, 4, apll);
}

static void push_dhub_cmd(struct snd_pcm_substream *substream, unsigned int update)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_berlin_card_pcm *berlin_pcm = runtime->private_data;
	unsigned int chanId;

	if(berlin_pcm->playback_state != SND_BERLIN_STATUS_PLAY)
		return;

	if(berlin_pcm->output_mode != berlin_output_mode) {
		berlin_set_aio(berlin_pcm->sample_rate, berlin_output_mode);
		berlin_pcm->output_mode = berlin_output_mode;
	}

	if(update) {
		berlin_pcm->pcm_buf_pos += berlin_pcm->dma_size;
		berlin_pcm->pcm_buf_pos = berlin_pcm->pcm_buf_pos%berlin_pcm->pcm_virt_bytes;
	}

	chanId = avioDhubChMap_ag_SA0_R_A0;
	if(berlin_pcm->output_mode == SND_BERLIN_OUTPUT_ANALOG) {
		dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
			berlin_pcm->pcm_dma_addr+berlin_pcm->pcm_buf_pos*berlin_pcm->pcm_ratio,
			berlin_pcm->dma_size*berlin_pcm->pcm_ratio, 0, 0, 0, 1, 0, 0);
	} else {
		dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
			berlin_pcm->spdif_dma_addr+berlin_pcm->pcm_buf_pos*berlin_pcm->spdif_ratio,
			berlin_pcm->dma_size*berlin_pcm->spdif_ratio, 0, 0, 0, 1, 0, 0);
	}
}

static irqreturn_t berlin_devices_aout_isr(int irq, void *dev_id)
{
	struct snd_pcm_substream *substream = (struct snd_pcm_substream *)dev_id;
	HDL_semaphore *pSemHandle;
	unsigned int chanId, instat;

	pSemHandle = dhub_semaphore(&AG_dhubHandle.dhub);
	instat = semaphore_chk_full(pSemHandle, -1);

	chanId = avioDhubChMap_ag_SA0_R_A0;
	if (bTST(instat, chanId)) {
		semaphore_pop(pSemHandle, chanId, 1);
		semaphore_clr_full(pSemHandle, chanId);
		push_dhub_cmd(substream, 1);
		snd_pcm_period_elapsed(substream);
	}

	return IRQ_HANDLED;
}

static void snd_berlin_playback_trigger_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_berlin_card_pcm *berlin_pcm = runtime->private_data;

	if(berlin_pcm->playback_state == SND_BERLIN_STATUS_PLAY)
		return;

	berlin_pcm->playback_state = SND_BERLIN_STATUS_PLAY;
	berlin_pcm->pcm_indirect.hw_io = bytes_to_frames(runtime, berlin_pcm->pcm_buf_pos);
	berlin_pcm->pcm_indirect.hw_data = berlin_pcm->pcm_indirect.hw_io;
	substream->ops->ack(substream);
	push_dhub_cmd(substream, 0);
}

static void snd_berlin_playback_trigger_stop(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_berlin_card_pcm *berlin_pcm = runtime->private_data;

	berlin_pcm->playback_state = SND_BERLIN_STATUS_STOP;
}

static const struct snd_pcm_hardware snd_berlin_playback_hw = {
	.info = (SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_MMAP_VALID),
	.formats = (SNDRV_PCM_FMTBIT_S32_LE
		| SNDRV_PCM_FMTBIT_S16_LE),
	.rates = (SNDRV_PCM_RATE_48000
		| SNDRV_PCM_RATE_96000
		| SNDRV_PCM_RATE_44100
		| SNDRV_PCM_RATE_88200
		| SNDRV_PCM_RATE_32000),
	.rate_min = 32000,
	.rate_max = 96000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = (256 << 3),
	.period_bytes_max = (256 << 3),
	.periods_min = 16,
	.periods_max = 16,
	.fifo_size = 0
};

static void snd_berlin_runtime_free(struct snd_pcm_runtime *runtime)
{
	struct snd_berlin_card_pcm *berlin_pcm = runtime->private_data;
	if(berlin_pcm) {
		dma_free_coherent(NULL, berlin_pcm->spdif_dma_bytes,
			berlin_pcm->spdif_dma_area, berlin_pcm->spdif_dma_addr);
		dma_free_coherent(NULL, berlin_pcm->pcm_dma_bytes,
			berlin_pcm->pcm_dma_area, berlin_pcm->pcm_dma_addr);
		kfree(berlin_pcm);
	}
}

static int snd_berlin_playback_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_berlin_card_pcm *berlin_pcm;
	unsigned int vec_num;
	int err;

	berlin_pcm = kzalloc(sizeof(struct snd_berlin_card_pcm), GFP_KERNEL);
	if (berlin_pcm == NULL)
		return -ENOMEM;

	berlin_pcm->playback_state = SND_BERLIN_STATUS_STOP;
	berlin_pcm->output_mode = berlin_output_mode;
	berlin_pcm->pcm_ratio = 1;
	berlin_pcm->spdif_ratio = 2;
	berlin_pcm->substream = substream;
	runtime->private_data = berlin_pcm;
	runtime->private_free = snd_berlin_runtime_free;
	runtime->hw = snd_berlin_playback_hw;

	/* Dhub configuration */
	DhubInitialization(0, AG_DHUB_BASE, AG_HBO_SRAM_BASE,
			&AG_dhubHandle, AG_config, AG_NUM_OF_CHANNELS_A0);

	/* register and enable audio out ISR */
	vec_num = IRQ_DHUBINTRAVIO1;
	err = request_irq(vec_num, berlin_devices_aout_isr, IRQF_DISABLED,
				"berlin_module_aout", substream);
	if (unlikely(err < 0)) {
		snd_printk("irq register error: vec_num:%5d, err:%8x\n", vec_num, err);
	}

	DhubEnableIntr(0, &AG_dhubHandle, avioDhubChMap_ag_SA0_R_A0, 1);

	snd_printk("playback_open succeeds.\n");

	return 0;
}

static int snd_berlin_playback_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned int vec_num = IRQ_DHUBINTRAVIO1;

	DhubEnableIntr(0, &AG_dhubHandle, avioDhubChMap_ag_SA0_R_A0, 0);

	free_irq(vec_num, substream);

	snd_printk("playback_close succeeds.\n");

	return 0;
}

static int snd_berlin_playback_hw_free(struct snd_pcm_substream *substream)
{
	return snd_pcm_lib_free_pages(substream);
}

static int snd_berlin_playback_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_berlin_card_pcm *berlin_pcm = runtime->private_data;
	struct spdif_channel_status *chnsts;
	int err;

	mutex_lock(&berlin_mutex);

	err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (err < 0) {
		snd_printk("pcm_lib_malloc failed to allocated pages for buffers\n");
		return err;
	}

	berlin_pcm->sample_rate = params_rate(params);
	berlin_pcm->sample_format = params_format(params);
	berlin_pcm->channel_num = params_channels(params);

	if(berlin_pcm->sample_format == SNDRV_PCM_FORMAT_S16_LE) {
		berlin_pcm->pcm_ratio   *= 2;
		berlin_pcm->spdif_ratio *= 2;
	}

	if(berlin_pcm->channel_num == 1) {
		berlin_pcm->pcm_ratio   *= 2;
		berlin_pcm->spdif_ratio *= 2;
	}

	if ((berlin_pcm->pcm_dma_area =
		dma_alloc_coherent(NULL, MAX_BUFFER_SIZE*berlin_pcm->pcm_ratio,
			&berlin_pcm->pcm_dma_addr, GFP_KERNEL)) == NULL) {
		return -ENOMEM;
	}
	berlin_pcm->pcm_virt_bytes = MAX_BUFFER_SIZE;
	berlin_pcm->pcm_dma_bytes = MAX_BUFFER_SIZE*berlin_pcm->pcm_ratio;
	memset(berlin_pcm->pcm_dma_area, 0, berlin_pcm->pcm_dma_bytes);

	if ((berlin_pcm->spdif_dma_area =
		dma_alloc_coherent(NULL, MAX_BUFFER_SIZE*berlin_pcm->spdif_ratio,
			&berlin_pcm->spdif_dma_addr, GFP_KERNEL)) == NULL) {
		dma_free_coherent(NULL, berlin_pcm->pcm_dma_bytes,
			berlin_pcm->pcm_dma_area, berlin_pcm->pcm_dma_addr);
		return -ENOMEM;
	}
	berlin_pcm->spdif_dma_bytes = MAX_BUFFER_SIZE*berlin_pcm->spdif_ratio;
	memset(berlin_pcm->spdif_dma_area, 0, berlin_pcm->spdif_dma_bytes);

	/* initialize spdif channel status */
	chnsts = (struct spdif_channel_status *)&(berlin_pcm->channel_status[0]);
	spdif_init_channel_status(chnsts, berlin_pcm->sample_rate);

	/* AVPLL configuration */
	berlin_set_pll(berlin_pcm->sample_rate);

	/* AIO configuration */
	berlin_set_aio(berlin_pcm->sample_rate, berlin_pcm->output_mode);

	mutex_unlock(&berlin_mutex);

	return 0;
}

static int snd_berlin_playback_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_berlin_card_pcm *berlin_pcm = runtime->private_data;
	unsigned int bps;

	bps = runtime->rate * runtime->channels;
	bps *= snd_pcm_format_width(runtime->format);
	bps /= 8;
	if (bps <= 0)
		return -EINVAL;
	berlin_pcm->pcm_bps = bps;
	berlin_pcm->pcm_size = snd_pcm_lib_buffer_bytes(substream);
	berlin_pcm->pcm_count = snd_pcm_lib_period_bytes(substream);
	berlin_pcm->pcm_buf_pos = 0;
	berlin_pcm->dma_size = 256 * 4 * 2;

	memset(&berlin_pcm->pcm_indirect, 0, sizeof(berlin_pcm->pcm_indirect));
	berlin_pcm->pcm_indirect.hw_buffer_size = MAX_BUFFER_SIZE;
	berlin_pcm->pcm_indirect.sw_buffer_size = snd_pcm_lib_buffer_bytes(substream);
	snd_printk("prepare ok bps: %d size: %d count: %d\n",
		berlin_pcm->pcm_bps, berlin_pcm->pcm_size, berlin_pcm->pcm_count);

	return 0;
}

static int snd_berlin_playback_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_berlin_card_pcm *berlin_pcm = runtime->private_data;
	struct snd_berlin_chip *chip = snd_pcm_substream_chip(substream);
	unsigned int result = 0;

	if (cmd == SNDRV_PCM_TRIGGER_START) {
		snd_berlin_playback_trigger_start(substream);
		snd_printk("playback starts.\n");
	} else if (cmd == SNDRV_PCM_TRIGGER_STOP) {
		snd_berlin_playback_trigger_stop(substream);
		snd_printk("playback ends.\n");
	} else {
		result = -EINVAL;
	}

	return result;
}

static snd_pcm_uframes_t
snd_berlin_playback_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_berlin_card_pcm *berlin_pcm = runtime->private_data;

	return snd_pcm_indirect_playback_pointer(substream, &berlin_pcm->pcm_indirect,
						berlin_pcm->pcm_buf_pos);
}

static int snd_berlin_playback_copy(struct snd_pcm_substream *substream,
				int channel, snd_pcm_uframes_t pos,
				void *buf, size_t bytes)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_berlin_card_pcm *berlin_pcm = runtime->private_data;
	unsigned char *pcm_buf, *spdif_buf;
	unsigned int i, sync_word;
	unsigned int *pcm, *spdif;
	unsigned short *s16_pcm;
	unsigned char c;
	if(berlin_pcm->sample_format == SNDRV_PCM_FORMAT_S16_LE) {
		if(berlin_pcm->channel_num == 1) {
			pcm_buf = berlin_pcm->pcm_dma_area +
				pos*berlin_pcm->pcm_ratio;
			s16_pcm = (unsigned short *)buf;
			for(i=0; i<(bytes>>1); i++)
			{
				*((unsigned int *)pcm_buf) = (*s16_pcm)<<16;
				pcm_buf += 4;
				*((unsigned int *)pcm_buf) = (*s16_pcm)<<16;
				pcm_buf += 4;
				s16_pcm += 1;
			}

			spdif_buf = berlin_pcm->spdif_dma_area +
				pos*berlin_pcm->spdif_ratio;
			s16_pcm = (unsigned short *)buf;
			spdif   = (unsigned int *)spdif_buf;
			for(i=0; i<(bytes>>1); i++)
			{
				c = spdif_get_channel_status(berlin_pcm->channel_status,
								berlin_pcm->spdif_frames);

				sync_word = berlin_pcm->spdif_frames ? TYPE_M : TYPE_B;
				spdif_enc_subframe(spdif, (*s16_pcm)<<16, sync_word, 0, 0, c);

				sync_word = TYPE_W;
				spdif_enc_subframe((spdif+2), (*s16_pcm)<<16, sync_word, 0, 0, c);

				s16_pcm += 1;
				spdif   += 4;
				berlin_pcm->spdif_frames += 1;
				berlin_pcm->spdif_frames = (berlin_pcm->spdif_frames)%SPDIF_BLOCK_SIZE;
			}
		} else {
			pcm_buf = berlin_pcm->pcm_dma_area +
				pos*berlin_pcm->pcm_ratio;
			s16_pcm = (unsigned short *)buf;
			for(i=0; i<(bytes>>1); i++)
			{
				*((unsigned int *)pcm_buf) = (*s16_pcm)<<16;
				pcm_buf += 4;
				s16_pcm += 1;
			}

			spdif_buf = berlin_pcm->spdif_dma_area +
				pos*berlin_pcm->spdif_ratio;
			s16_pcm = (unsigned short *)buf;
			spdif   = (unsigned int *)spdif_buf;
			for(i=0; i<(bytes>>2); i++)
			{
				c = spdif_get_channel_status(berlin_pcm->channel_status,
								berlin_pcm->spdif_frames);

				sync_word = berlin_pcm->spdif_frames ? TYPE_M : TYPE_B;
				spdif_enc_subframe(spdif, (*s16_pcm)<<16, sync_word, 0, 0, c);

				sync_word = TYPE_W;
				spdif_enc_subframe((spdif+2), (*(s16_pcm+1))<<16, sync_word, 0, 0, c);

				s16_pcm += 2;
				spdif   += 4;
				berlin_pcm->spdif_frames += 1;
				berlin_pcm->spdif_frames = (berlin_pcm->spdif_frames)%SPDIF_BLOCK_SIZE;
			}
		}
	} else { // SNDRV_PCM_FORMAT_S32_LE
		if(berlin_pcm->channel_num == 1) {
			pcm_buf = berlin_pcm->pcm_dma_area +
				pos*berlin_pcm->pcm_ratio;
			pcm = (unsigned int *)buf;
			for(i=0; i<(bytes>>2); i++)
			{
				*((unsigned int *)pcm_buf) = *pcm;
				pcm_buf += 4;
				*((unsigned int *)pcm_buf) = *pcm;
				pcm_buf += 4;
				pcm     += 1;
			}

			spdif_buf = berlin_pcm->spdif_dma_area +
				pos*berlin_pcm->spdif_ratio;
			pcm   = (unsigned int *)buf;
			spdif = (unsigned int *)spdif_buf;
			for(i=0; i<(bytes>>2); i++)
			{
				c = spdif_get_channel_status(berlin_pcm->channel_status,
								berlin_pcm->spdif_frames);

				sync_word = berlin_pcm->spdif_frames ? TYPE_M : TYPE_B;
				spdif_enc_subframe(spdif, *pcm, sync_word, 0, 0, c);

				sync_word = TYPE_W;
				spdif_enc_subframe((spdif+2), *pcm, sync_word, 0, 0, c);

				pcm   += 1;
				spdif += 4;
				berlin_pcm->spdif_frames += 1;
				berlin_pcm->spdif_frames = (berlin_pcm->spdif_frames)%SPDIF_BLOCK_SIZE;
			}
		} else {
			pcm_buf = berlin_pcm->pcm_dma_area +
				pos*berlin_pcm->pcm_ratio;
			pcm = (unsigned int *)buf;
			for(i=0; i<(bytes>>2); i++)
			{
				*((unsigned int *)pcm_buf) = *pcm;
				pcm_buf += 4;
				pcm     += 1;
			}

			spdif_buf = berlin_pcm->spdif_dma_area +
				pos*berlin_pcm->spdif_ratio;
			pcm   = (unsigned int *)buf;
			spdif = (unsigned int *)spdif_buf;
			for(i=0; i<(bytes>>3); i++)
			{
				c = spdif_get_channel_status(berlin_pcm->channel_status,
								berlin_pcm->spdif_frames);

				sync_word = berlin_pcm->spdif_frames ? TYPE_M : TYPE_B;
				spdif_enc_subframe(spdif, *pcm, sync_word, 0, 0, c);

				sync_word = TYPE_W;
				spdif_enc_subframe((spdif+2), *(pcm+1), sync_word, 0, 0, c);

				pcm   += 2;
				spdif += 4;
				berlin_pcm->spdif_frames += 1;
				berlin_pcm->spdif_frames = (berlin_pcm->spdif_frames)%SPDIF_BLOCK_SIZE;
			}
		}
	}

	return 0;
}

static void berlin_playback_transfer(struct snd_pcm_substream *substream,
				   struct snd_pcm_indirect *rec, size_t bytes)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_berlin_card_pcm *berlin_pcm = runtime->private_data;
	void *src = (void *)(runtime->dma_area + rec->sw_data);

	if (src == NULL)
		snd_printk("!!!!! src is NULL !!!!\n");
	else
		snd_berlin_playback_copy(substream, berlin_pcm->channel_num, rec->sw_data, src, bytes);
}

static int snd_berlin_playback_ack(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_berlin_card_pcm *berlin_pcm = runtime->private_data;
	struct snd_pcm_indirect *pcm_indirect = &berlin_pcm->pcm_indirect;
	pcm_indirect->hw_queue_size = runtime->hw.buffer_bytes_max;
	snd_pcm_indirect_playback_transfer(substream, pcm_indirect,
						berlin_playback_transfer);
	return 0;
}

static int snd_berlin_playback_silence(struct snd_pcm_substream *substream,
					int channel, snd_pcm_uframes_t pos,
					snd_pcm_uframes_t count)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_berlin_card_pcm *berlin_pcm = runtime->private_data;
	unsigned char *pcm_buf, *spdif_buf;
	unsigned int i, sync_word;
	unsigned int *spdif;
	unsigned char c;

	pcm_buf = berlin_pcm->pcm_dma_area +
			frames_to_bytes(runtime, pos)*berlin_pcm->pcm_ratio;
	memset(pcm_buf, 0, frames_to_bytes(runtime, count)*berlin_pcm->pcm_ratio);

	if(berlin_pcm->sample_format == SNDRV_PCM_FORMAT_S16_LE) {
		if(berlin_pcm->channel_num == 1) {
			spdif_buf = berlin_pcm->spdif_dma_area +
				frames_to_bytes(runtime, pos)*berlin_pcm->spdif_ratio;
			spdif = (unsigned int *)spdif_buf;
			for(i=0; i<(frames_to_bytes(runtime, count)>>1); i++)
			{
				c = spdif_get_channel_status(berlin_pcm->channel_status,
								berlin_pcm->spdif_frames);

				sync_word = berlin_pcm->spdif_frames ? TYPE_M : TYPE_B;
				spdif_enc_subframe(spdif, 0, sync_word, 0, 0, c);

				sync_word = TYPE_W;
				spdif_enc_subframe((spdif+2), 0, sync_word, 0, 0, c);

				spdif += 4;
				berlin_pcm->spdif_frames += 1;
				berlin_pcm->spdif_frames = (berlin_pcm->spdif_frames)%SPDIF_BLOCK_SIZE;
			}
		} else {
			spdif_buf = berlin_pcm->spdif_dma_area +
				frames_to_bytes(runtime, pos)*berlin_pcm->spdif_ratio;
			spdif = (unsigned int *)spdif_buf;
			for(i=0; i<(frames_to_bytes(runtime, count)>>2); i++)
			{
				c = spdif_get_channel_status(berlin_pcm->channel_status,
								berlin_pcm->spdif_frames);

				sync_word = berlin_pcm->spdif_frames ? TYPE_M : TYPE_B;
				spdif_enc_subframe(spdif, 0, sync_word, 0, 0, c);

				sync_word = TYPE_W;
				spdif_enc_subframe((spdif+2), 0, sync_word, 0, 0, c);

				spdif += 4;
				berlin_pcm->spdif_frames += 1;
				berlin_pcm->spdif_frames = (berlin_pcm->spdif_frames)%SPDIF_BLOCK_SIZE;
			}
		}
	} else {
		if(berlin_pcm->channel_num == 1) {
			spdif_buf = berlin_pcm->spdif_dma_area +
				frames_to_bytes(runtime, pos)*berlin_pcm->spdif_ratio;
			spdif = (unsigned int *)spdif_buf;
			for(i=0; i<(frames_to_bytes(runtime, count)>>2); i++)
                        {
				c = spdif_get_channel_status(berlin_pcm->channel_status,
								berlin_pcm->spdif_frames);

				sync_word = berlin_pcm->spdif_frames ? TYPE_M : TYPE_B;
				spdif_enc_subframe(spdif, 0, sync_word, 0, 0, c);

				sync_word = TYPE_W;
				spdif_enc_subframe((spdif+2), 0, sync_word, 0, 0, c);

				spdif += 4;
				berlin_pcm->spdif_frames += 1;
				berlin_pcm->spdif_frames = (berlin_pcm->spdif_frames)%SPDIF_BLOCK_SIZE;
			}
		} else {
			spdif_buf = berlin_pcm->spdif_dma_area +
				frames_to_bytes(runtime, pos)*berlin_pcm->spdif_ratio;
			spdif = (unsigned int *)spdif_buf;
			for(i=0; i<(frames_to_bytes(runtime, count)>>3); i++)
			{
				c = spdif_get_channel_status(berlin_pcm->channel_status,
								berlin_pcm->spdif_frames);

				sync_word = berlin_pcm->spdif_frames ? TYPE_M : TYPE_B;
				spdif_enc_subframe(spdif, 0, sync_word, 0, 0, c);

				sync_word = TYPE_W;
				spdif_enc_subframe((spdif+2), 0, sync_word, 0, 0, c);

				spdif += 4;
				berlin_pcm->spdif_frames += 1;
				berlin_pcm->spdif_frames = (berlin_pcm->spdif_frames)%SPDIF_BLOCK_SIZE;
			}
		}
	}

	return 0;
}

static struct snd_pcm_ops snd_berlin_playback_ops = {
	.open    = snd_berlin_playback_open,
	.close   = snd_berlin_playback_close,
	.ioctl   = snd_pcm_lib_ioctl,
	.hw_params = snd_berlin_playback_hw_params,
	.hw_free   = snd_berlin_playback_hw_free,
	.prepare = snd_berlin_playback_prepare,
	.trigger = snd_berlin_playback_trigger,
	.pointer = snd_berlin_playback_pointer,
//	.copy    = snd_berlin_playback_copy,
	.ack     = snd_berlin_playback_ack,
//	.silence = snd_berlin_playback_silence,
};

static int snd_berlin_card_new_pcm(struct snd_berlin_chip *chip)
{
	struct snd_pcm *pcm;
	int err;

	if ((err =
	snd_pcm_new(chip->card, "Berlin ALSA PCM", 0, 1, 0, &pcm)) < 0)
		return err;

	chip->pcm = pcm;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
			&snd_berlin_playback_ops);
	pcm->private_data = chip;
	pcm->info_flags = 0;
	strcpy(pcm->name, "Berlin ALSA PCM");
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
					snd_dma_continuous_data(GFP_KERNEL), 32*1024, 64*1024);
	return 0;
}

static int snd_berlin_hwdep_dummy_op(struct snd_hwdep *hw, struct file *file)
{
	return 0;
}

static int snd_berlin_hwdep_ioctl(struct snd_hwdep *hw, struct file *file,
				unsigned int cmd, unsigned long arg)
{
	unsigned int *mode = (unsigned int *)arg;
	unsigned int s = 0;


	mutex_lock(&berlin_mutex);
	switch (cmd) {
	case SNDRV_BERLIN_GET_OUTPUT_MODE :
		s = copy_to_user(mode, &berlin_output_mode, sizeof(int));
		break;
	case SNDRV_BERLIN_SET_OUTPUT_MODE :
		berlin_output_mode = *mode;
		break;
	}
	mutex_unlock(&berlin_mutex);

	return s;
}

static int snd_berlin_card_new_hwdep(struct snd_berlin_chip *chip)
{
	struct snd_hwdep *hw;
	unsigned int err;

	err = snd_hwdep_new(chip->card, "Berlin hwdep", 0, &hw);
	if (err < 0)
		return err;

	chip->hwdep = hw;
	hw->private_data = chip;
	strcpy(hw->name, "Berlin hwdep interface");

	hw->ops.open = snd_berlin_hwdep_dummy_op;
	hw->ops.ioctl = snd_berlin_hwdep_ioctl;
	hw->ops.ioctl_compat = snd_berlin_hwdep_ioctl;
	hw->ops.release = snd_berlin_hwdep_dummy_op;

	return 0;
}

static void snd_berlin_private_free(struct snd_card *card)
{
	struct snd_berlin_chip *chip = card->private_data;

	kfree(chip);
}

static int snd_berlin_card_init(int dev)
{
	int ret;
	char id[16];
	struct snd_berlin_chip *chip;
	int err;

	snd_printk("berlin snd card is probed\n");
	sprintf(id, "MRVLBERLIN");
	ret = snd_card_create(-1, id, THIS_MODULE, 0, &snd_berlin_card);
	if (snd_berlin_card == NULL)
		return -ENOMEM;

	chip = kzalloc(sizeof(struct snd_berlin_chip), GFP_KERNEL);
	if(chip == NULL)
	{
		err = -ENOMEM;
		goto __nodev;
	}

	snd_berlin_card->private_data = chip;
	snd_berlin_card->private_free = snd_berlin_private_free;

	chip->card = snd_berlin_card;

	if ((err = snd_berlin_card_new_pcm(chip)) < 0)
		goto __nodev;

	strcpy(snd_berlin_card->driver, "Berlin SoC Alsa");
	strcpy(snd_berlin_card->shortname, "Berlin Alsa");
	sprintf(snd_berlin_card->longname, "Berlin Alsa %i", dev + 1);

	if ((err = snd_berlin_card_new_hwdep(chip)) < 0)
		goto __nodev;

	if ((err = snd_card_register(snd_berlin_card)) != 0)
		goto __nodev;
	else {
		snd_printk("berlin snd card device driver registered\n");
		return 0;
	}

__nodev:
	if(chip)
		kfree(chip);

	snd_card_free(snd_berlin_card);

	return err;
}

static void snd_berlin_card_exit(void)
{
	snd_card_free(snd_berlin_card);
}

static int __init snd_berlin_init(void)
{
	printk("snd_berlin_init\n");
	return snd_berlin_card_init(0);
}

static void __exit snd_berlin_exit(void)
{
	printk("snd_berlin_exit\n");
	snd_berlin_card_exit();
}

MODULE_LICENSE("GPL");
module_init(snd_berlin_init);
module_exit(snd_berlin_exit);
