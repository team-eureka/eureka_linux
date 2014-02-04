/** @file moal_debug.c
  *
  * @brief This file contains functions for debug proc file.
  *
  * Copyright (C) 2008-2011, Marvell International Ltd.
  *
  * This software file (the "File") is distributed by Marvell International
  * Ltd. under the terms of the GNU General Public License Version 2, June 1991
  * (the "License").  You may use, redistribute and/or modify this File in
  * accordance with the terms and conditions of the License, a copy of which
  * is available by writing to the Free Software Foundation, Inc.,
  * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
  * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
  *
  * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
  * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
  * this warranty disclaimer.
  *
  */

/********************************************************
Change log:
    11/03/2008: initial version
********************************************************/

#include	"moal_main.h"
#include "mlan_decl.h"
#include "linux/seq_file.h"
#include "linux/list.h"

/********************************************************
		Global Variables
********************************************************/
/** MLAN debug info */
extern mlan_debug_info info;

/********************************************************
		Local Variables
********************************************************/
#ifdef CONFIG_PROC_FS

/** Get info item size */
#define item_size(n) (sizeof(info.n))
/** Get info item address */
#define item_addr(n) ((t_ptr) &(info.n))

/** Get moal_private member size */
#define item_priv_size(n) (sizeof((moal_private *)0)->n)
/** Get moal_private member address */
#define item_priv_addr(n) ((t_ptr) &((moal_private *)0)->n)

/** Get moal_handle member size */
#define item_handle_size(n) (sizeof((moal_handle *)0)->n)
/** Get moal_handle member address */
#define item_handle_addr(n) ((t_ptr) &((moal_handle *)0)->n)

#ifdef STA_SUPPORT
static struct debug_data items[] = {
#ifdef DEBUG_LEVEL1
	{"drvdbg", sizeof(drvdbg), (t_ptr) & drvdbg}
	,
#endif
	{"wmm_ac_vo", item_size(wmm_ac_vo), item_addr(wmm_ac_vo)}
	,
	{"wmm_ac_vi", item_size(wmm_ac_vi), item_addr(wmm_ac_vi)}
	,
	{"wmm_ac_be", item_size(wmm_ac_be), item_addr(wmm_ac_be)}
	,
	{"wmm_ac_bk", item_size(wmm_ac_bk), item_addr(wmm_ac_bk)}
	,
	{"max_tx_buf_size", item_size(max_tx_buf_size),
	 item_addr(max_tx_buf_size)}
	,
	{"tx_buf_size", item_size(tx_buf_size), item_addr(tx_buf_size)}
	,
	{"curr_tx_buf_size", item_size(curr_tx_buf_size),
	 item_addr(curr_tx_buf_size)}
	,
	{"ps_mode", item_size(ps_mode), item_addr(ps_mode)}
	,
	{"ps_state", item_size(ps_state), item_addr(ps_state)}
	,
	{"is_deep_sleep", item_size(is_deep_sleep), item_addr(is_deep_sleep)}
	,
	{"wakeup_dev_req", item_size(pm_wakeup_card_req),
	 item_addr(pm_wakeup_card_req)}
	,
	{"wakeup_tries", item_size(pm_wakeup_fw_try),
	 item_addr(pm_wakeup_fw_try)}
	,
	{"hs_configured", item_size(is_hs_configured),
	 item_addr(is_hs_configured)}
	,
	{"hs_activated", item_size(hs_activated), item_addr(hs_activated)}
	,
	{"tx_pkts_queued", item_size(tx_pkts_queued), item_addr(tx_pkts_queued)}
	,
	{"pps_uapsd_mode", item_size(pps_uapsd_mode), item_addr(pps_uapsd_mode)}
	,
	{"sleep_pd", item_size(sleep_pd), item_addr(sleep_pd)}
	,
	{"qos_cfg", item_size(qos_cfg), item_addr(qos_cfg)}
	,
	{"tx_lock_flag", item_size(tx_lock_flag), item_addr(tx_lock_flag)}
	,
	{"port_open", item_size(port_open), item_addr(port_open)}
	,
	{"bypass_pkt_count", item_size(bypass_pkt_count),
	 item_addr(bypass_pkt_count)}
	,
	{"scan_processing", item_size(scan_processing),
	 item_addr(scan_processing)}
	,
	{"num_tx_timeout", item_size(num_tx_timeout), item_addr(num_tx_timeout)}
	,
	{"num_cmd_timeout", item_size(num_cmd_timeout),
	 item_addr(num_cmd_timeout)}
	,
	{"timeout_cmd_id", item_size(timeout_cmd_id), item_addr(timeout_cmd_id)}
	,
	{"timeout_cmd_act", item_size(timeout_cmd_act),
	 item_addr(timeout_cmd_act)}
	,
	{"last_cmd_id", item_size(last_cmd_id), item_addr(last_cmd_id)}
	,
	{"last_cmd_act", item_size(last_cmd_act), item_addr(last_cmd_act)}
	,
	{"last_cmd_index", item_size(last_cmd_index), item_addr(last_cmd_index)}
	,
	{"last_cmd_resp_id", item_size(last_cmd_resp_id),
	 item_addr(last_cmd_resp_id)}
	,
	{"last_cmd_resp_index", item_size(last_cmd_resp_index),
	 item_addr(last_cmd_resp_index)}
	,
	{"last_event", item_size(last_event), item_addr(last_event)}
	,
	{"last_event_index", item_size(last_event_index),
	 item_addr(last_event_index)}
	,
	{"num_no_cmd_node", item_size(num_no_cmd_node),
	 item_addr(num_no_cmd_node)}
	,
	{"num_cmd_h2c_fail", item_size(num_cmd_host_to_card_failure),
	 item_addr(num_cmd_host_to_card_failure)}
	,
	{"num_cmd_sleep_cfm_fail",
	 item_size(num_cmd_sleep_cfm_host_to_card_failure),
	 item_addr(num_cmd_sleep_cfm_host_to_card_failure)}
	,
	{"num_tx_h2c_fail", item_size(num_tx_host_to_card_failure),
	 item_addr(num_tx_host_to_card_failure)}
	,
	{"num_cmdevt_c2h_fail", item_size(num_cmdevt_card_to_host_failure),
	 item_addr(num_cmdevt_card_to_host_failure)}
	,
	{"num_rx_c2h_fail", item_size(num_rx_card_to_host_failure),
	 item_addr(num_rx_card_to_host_failure)}
	,
	{"num_int_read_fail", item_size(num_int_read_failure),
	 item_addr(num_int_read_failure)}
	,
	{"last_int_status", item_size(last_int_status),
	 item_addr(last_int_status)}
	,
#ifdef SDIO_MULTI_PORT_TX_AGGR
	{"mpa_sent_last_pkt", item_size(mpa_sent_last_pkt),
	 item_addr(mpa_sent_last_pkt)}
	,
	{"mpa_sent_no_ports", item_size(mpa_sent_no_ports),
	 item_addr(mpa_sent_no_ports)}
	,
#endif
	{"num_evt_deauth", item_size(num_event_deauth),
	 item_addr(num_event_deauth)}
	,
	{"num_evt_disassoc", item_size(num_event_disassoc),
	 item_addr(num_event_disassoc)}
	,
	{"num_evt_link_lost", item_size(num_event_link_lost),
	 item_addr(num_event_link_lost)}
	,
	{"num_cmd_deauth", item_size(num_cmd_deauth), item_addr(num_cmd_deauth)}
	,
	{"num_cmd_assoc_ok", item_size(num_cmd_assoc_success),
	 item_addr(num_cmd_assoc_success)}
	,
	{"num_cmd_assoc_fail", item_size(num_cmd_assoc_failure),
	 item_addr(num_cmd_assoc_failure)}
	,
	{"cmd_sent", item_size(cmd_sent), item_addr(cmd_sent)}
	,
	{"data_sent", item_size(data_sent), item_addr(data_sent)}
	,
	{"mp_rd_bitmap", item_size(mp_rd_bitmap), item_addr(mp_rd_bitmap)}
	,
	{"curr_rd_port", item_size(curr_rd_port), item_addr(curr_rd_port)}
	,
	{"mp_wr_bitmap", item_size(mp_wr_bitmap), item_addr(mp_wr_bitmap)}
	,
	{"curr_wr_port", item_size(curr_wr_port), item_addr(curr_wr_port)}
	,
	{"cmd_resp_received", item_size(cmd_resp_received),
	 item_addr(cmd_resp_received)}
	,
	{"event_received", item_size(event_received), item_addr(event_received)}
	,

	{"ioctl_pending", item_handle_size(ioctl_pending),
	 item_handle_addr(ioctl_pending)}
	,
	{"tx_pending", item_handle_size(tx_pending),
	 item_handle_addr(tx_pending)}
	,
	{"rx_pending", item_handle_size(rx_pending),
	 item_handle_addr(rx_pending)}
	,
	{"lock_count", item_handle_size(lock_count),
	 item_handle_addr(lock_count)}
	,
	{"malloc_count", item_handle_size(malloc_count),
	 item_handle_addr(malloc_count)}
	,
	{"vmalloc_count", item_handle_size(vmalloc_count),
	 item_handle_addr(vmalloc_count)}
	,
	{"mbufalloc_count", item_handle_size(mbufalloc_count),
	 item_handle_addr(mbufalloc_count)}
	,
	{"main_state", item_handle_size(main_state),
	 item_handle_addr(main_state)}
	,
	{"driver_state", item_handle_size(driver_state),
	 item_handle_addr(driver_state)}
	,
#ifdef SDIO_MMC_DEBUG
	{"sdiocmd53w", item_handle_size(cmd53w), item_handle_addr(cmd53w)}
	,
	{"sdiocmd53r", item_handle_size(cmd53r), item_handle_addr(cmd53r)}
	,
#endif
#if defined(SDIO_SUSPEND_RESUME)
	{"hs_skip_count", item_handle_size(hs_skip_count),
	 item_handle_addr(hs_skip_count)}
	,
	{"hs_force_count", item_handle_size(hs_force_count),
	 item_handle_addr(hs_force_count)}
	,
#endif
};

#endif

#ifdef UAP_SUPPORT
static struct debug_data uap_items[] = {
#ifdef DEBUG_LEVEL1
	{"drvdbg", sizeof(drvdbg), (t_ptr) & drvdbg}
	,
#endif
	{"wmm_ac_vo", item_size(wmm_ac_vo), item_addr(wmm_ac_vo)}
	,
	{"wmm_ac_vi", item_size(wmm_ac_vi), item_addr(wmm_ac_vi)}
	,
	{"wmm_ac_be", item_size(wmm_ac_be), item_addr(wmm_ac_be)}
	,
	{"wmm_ac_bk", item_size(wmm_ac_bk), item_addr(wmm_ac_bk)}
	,
	{"max_tx_buf_size", item_size(max_tx_buf_size),
	 item_addr(max_tx_buf_size)}
	,
	{"tx_buf_size", item_size(tx_buf_size), item_addr(tx_buf_size)}
	,
	{"curr_tx_buf_size", item_size(curr_tx_buf_size),
	 item_addr(curr_tx_buf_size)}
	,
	{"ps_mode", item_size(ps_mode), item_addr(ps_mode)}
	,
	{"ps_state", item_size(ps_state), item_addr(ps_state)}
	,
	{"wakeup_dev_req", item_size(pm_wakeup_card_req),
	 item_addr(pm_wakeup_card_req)}
	,
	{"wakeup_tries", item_size(pm_wakeup_fw_try),
	 item_addr(pm_wakeup_fw_try)}
	,
	{"hs_configured", item_size(is_hs_configured),
	 item_addr(is_hs_configured)}
	,
	{"hs_activated", item_size(hs_activated), item_addr(hs_activated)}
	,
	{"tx_pkts_queued", item_size(tx_pkts_queued), item_addr(tx_pkts_queued)}
	,
	{"bypass_pkt_count", item_size(bypass_pkt_count),
	 item_addr(bypass_pkt_count)}
	,
	{"num_bridge_pkts", item_size(num_bridge_pkts),
	 item_addr(num_bridge_pkts)}
	,
	{"num_drop_pkts", item_size(num_drop_pkts), item_addr(num_drop_pkts)}
	,
	{"num_tx_timeout", item_size(num_tx_timeout), item_addr(num_tx_timeout)}
	,
	{"num_cmd_timeout", item_size(num_cmd_timeout),
	 item_addr(num_cmd_timeout)}
	,
	{"timeout_cmd_id", item_size(timeout_cmd_id), item_addr(timeout_cmd_id)}
	,
	{"timeout_cmd_act", item_size(timeout_cmd_act),
	 item_addr(timeout_cmd_act)}
	,
	{"last_cmd_id", item_size(last_cmd_id), item_addr(last_cmd_id)}
	,
	{"last_cmd_act", item_size(last_cmd_act), item_addr(last_cmd_act)}
	,
	{"last_cmd_index", item_size(last_cmd_index), item_addr(last_cmd_index)}
	,
	{"last_cmd_resp_id", item_size(last_cmd_resp_id),
	 item_addr(last_cmd_resp_id)}
	,
	{"last_cmd_resp_index", item_size(last_cmd_resp_index),
	 item_addr(last_cmd_resp_index)}
	,
	{"last_event", item_size(last_event), item_addr(last_event)}
	,
	{"last_event_index", item_size(last_event_index),
	 item_addr(last_event_index)}
	,
	{"num_no_cmd_node", item_size(num_no_cmd_node),
	 item_addr(num_no_cmd_node)}
	,
	{"num_cmd_h2c_fail", item_size(num_cmd_host_to_card_failure),
	 item_addr(num_cmd_host_to_card_failure)}
	,
	{"num_cmd_sleep_cfm_fail",
	 item_size(num_cmd_sleep_cfm_host_to_card_failure),
	 item_addr(num_cmd_sleep_cfm_host_to_card_failure)}
	,
	{"num_tx_h2c_fail", item_size(num_tx_host_to_card_failure),
	 item_addr(num_tx_host_to_card_failure)}
	,
	{"num_cmdevt_c2h_fail", item_size(num_cmdevt_card_to_host_failure),
	 item_addr(num_cmdevt_card_to_host_failure)}
	,
	{"num_rx_c2h_fail", item_size(num_rx_card_to_host_failure),
	 item_addr(num_rx_card_to_host_failure)}
	,
	{"num_int_read_fail", item_size(num_int_read_failure),
	 item_addr(num_int_read_failure)}
	,
	{"last_int_status", item_size(last_int_status),
	 item_addr(last_int_status)}
	,
#ifdef SDIO_MULTI_PORT_TX_AGGR
	{"mpa_sent_last_pkt", item_size(mpa_sent_last_pkt),
	 item_addr(mpa_sent_last_pkt)}
	,
	{"mpa_sent_no_ports", item_size(mpa_sent_no_ports),
	 item_addr(mpa_sent_no_ports)}
	,
#endif
	{"cmd_sent", item_size(cmd_sent), item_addr(cmd_sent)}
	,
	{"data_sent", item_size(data_sent), item_addr(data_sent)}
	,
	{"mp_rd_bitmap", item_size(mp_rd_bitmap), item_addr(mp_rd_bitmap)}
	,
	{"curr_rd_port", item_size(curr_rd_port), item_addr(curr_rd_port)}
	,
	{"mp_wr_bitmap", item_size(mp_wr_bitmap), item_addr(mp_wr_bitmap)}
	,
	{"curr_wr_port", item_size(curr_wr_port), item_addr(curr_wr_port)}
	,
	{"cmd_resp_received", item_size(cmd_resp_received),
	 item_addr(cmd_resp_received)}
	,
	{"event_received", item_size(event_received), item_addr(event_received)}
	,

	{"ioctl_pending", item_handle_size(ioctl_pending),
	 item_handle_addr(ioctl_pending)}
	,
	{"tx_pending", item_handle_size(tx_pending),
	 item_handle_addr(tx_pending)}
	,
	{"rx_pending", item_handle_size(rx_pending),
	 item_handle_addr(rx_pending)}
	,
	{"lock_count", item_handle_size(lock_count),
	 item_handle_addr(lock_count)}
	,
	{"malloc_count", item_handle_size(malloc_count),
	 item_handle_addr(malloc_count)}
	,
	{"vmalloc_count", item_handle_size(vmalloc_count),
	 item_handle_addr(vmalloc_count)}
	,
	{"mbufalloc_count", item_handle_size(mbufalloc_count),
	 item_handle_addr(mbufalloc_count)}
	,
	{"main_state", item_handle_size(main_state),
	 item_handle_addr(main_state)}
	,
	{"driver_state", item_handle_size(driver_state),
	 item_handle_addr(driver_state)}
	,
#ifdef SDIO_MMC_DEBUG
	{"sdiocmd53w", item_handle_size(cmd53w), item_handle_addr(cmd53w)}
	,
	{"sdiocmd53r", item_handle_size(cmd53r), item_handle_addr(cmd53r)}
	,
#endif
#if defined(SDIO_SUSPEND_RESUME)
	{"hs_skip_count", item_handle_size(hs_skip_count),
	 item_handle_addr(hs_skip_count)}
	,
	{"hs_force_count", item_handle_size(hs_force_count),
	 item_handle_addr(hs_force_count)}
	,
#endif
};
#endif /* UAP_SUPPORT */

/* Histogram support*/
struct _hgm_seq_data {
	int   histDataSize;
	int   pos;
	int   numHgmSamples;
	char  *pHgmData;
};

static struct _hgm_seq_data hgm_seq_data;
static unsigned int woal_debug_get_uint(char *pBuf);

static void
hgm_seq_init_globals()
{
	int i;
	ENTER();

	if (!hgm_seq_data.pHgmData){
		hgm_seq_data.histDataSize = (RX_RATE_MAX+SNR_MAX+NOISE_FLR_MAX+SIG_STRENGTH_MAX) * sizeof(unsigned int);
		hgm_seq_data.pHgmData = (char *) kmalloc(hgm_seq_data.histDataSize, GFP_KERNEL);

		if (!hgm_seq_data.pHgmData){
			printk(KERN_ERR "hgm_seq_init_globals: Could not allocate memory\n");
			LEAVE();
			return;
		}

		for (i = 0; i < hgm_seq_data.histDataSize; i++)
			hgm_seq_data.pHgmData[i] = 0;
	}

	LEAVE();
	return;
}

static void *
hgm_seq_start(struct seq_file *s, loff_t *pos)
{

	ENTER();

	if (unlikely(!hgm_seq_data.pHgmData)){
		LEAVE();
		return NULL;
	}
	else if (0 == *pos) {
		if (0 != mlan_hist_data_get( hgm_seq_data.pHgmData, &(hgm_seq_data.numHgmSamples) )){
			LEAVE();
			return NULL;
		}
	}

	hgm_seq_data.pos = *pos;
	if ((*pos) >= hgm_seq_data.histDataSize) {     // are we done?
		LEAVE();
		return NULL;
	}

	LEAVE();
	return (void *) &hgm_seq_data;
}


static int
hgm_seq_show(struct seq_file *s, void *v)
{
	struct _hgm_seq_data *hgm = (struct _hgm_seq_data *) v;
	int pos;
	int rx_max = RX_RATE_MAX;
	int snr_max = rx_max + SNR_MAX;
	int nflr_max = snr_max + NOISE_FLR_MAX;
	int sigs_max = nflr_max + SIG_STRENGTH_MAX;
	int i,nTemp;
	struct debug_data_priv *pdbg_data_priv;
	struct debug_data *pdbg_data;
	moal_private *pmoal_priv=NULL;

	if (s){
		pdbg_data_priv = (struct debug_data_priv *)s->private;
		pdbg_data = pdbg_data_priv->items;
		pmoal_priv =  pdbg_data_priv->priv;
	}
	else
		PRINTM(MERROR,"hgm_seq_show(): s == NULL!\n");


	pos = hgm->pos;

	if (unlikely(pos == 0)){
		// Position 0, print banner, num items and first entry.
		seq_printf(s, "total samples = %d \n", hgm->numHgmSamples);
		seq_printf(s, "rx rates (in Mbps): 0=1M   1=2M   2=5.5M  3=11M   4=6M  5=9M  6=12M\n");
		seq_printf(s, "                    7=18M  8=24M  9=36M  10=48M  11=54M   12-19 == MCS0-7(BW20)\n\n");

		i = woal_debug_get_uint((char *) (pos + hgm->pHgmData));
		if (i){
			seq_printf(s, "rx_rate[%02d] = %d\n", pos, i);
		}
	}
	else if (pos < rx_max*sizeof(unsigned int)){
		i = woal_debug_get_uint((char *) (pos + hgm->pHgmData));
		if (i){
			seq_printf(s, "rx_rate[%02d] = %d\n", pos/sizeof(unsigned int), i);
		}
	}
	else if (pos < snr_max*sizeof(unsigned int)){
		i = woal_debug_get_uint((char *) (pos + hgm->pHgmData));
		if (i){
			seq_printf(s, "snr[%02ddB] = %d\n", (pos/sizeof(unsigned int)) - rx_max, i);
		}
	}
	else if (pos < nflr_max*sizeof(unsigned int)){
		i = woal_debug_get_uint((char *) (pos + hgm->pHgmData));
		if (i){
			seq_printf(s, "noise_flr[-%02ddBm] = %d\n", ((pos/sizeof(unsigned int))-snr_max)-128, i);
		}
	}
	else if (pos < sigs_max*sizeof(unsigned int)){
		i = woal_debug_get_uint((char *) (pos + hgm->pHgmData));
		if (i){
			seq_printf(s, "sig_strength[-%02ddBm] = %d\n", (pos/sizeof(unsigned int)) - nflr_max, i);
		}
	}
	else{
		return 0;
	}

	return 0;
}

static void *
hgm_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct _hgm_seq_data *hgm = (struct _hgm_seq_data *) v;

	if ((*pos) >= hgm->histDataSize)     // we are done
	   return NULL;

	(*pos) += sizeof(unsigned int);                // increase my position counter
	hgm->pos = *pos;

	return v;
}

static void
hgm_seq_stop(struct seq_file *s, void *v)
{
	return;
}

static struct seq_operations hgm_seq_ops = {
	.start = hgm_seq_start,
	.next  = hgm_seq_next,
	.stop  = hgm_seq_stop,
	.show  = hgm_seq_show
};

static int hgm_seq_open(struct inode *inode, struct file *file)
{
	int ret;
	ret =  seq_open(file, &hgm_seq_ops);
	if (!ret){
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
		((struct seq_file *)file->private_data)->private = PDE_DATA(inode);
#else
		((struct seq_file *)file->private_data)->private = PDE(inode)->data;
#endif
	}
	return ret;
}

static unsigned int woal_debug_get_uint(char *pBuf)
{
	unsigned int n;
	mlan_memcpy((void *) &n, (void *) pBuf, sizeof(unsigned int));
	return n;
}

static int
woal_debug_write_histogram(struct file *filp, const char __user *buf,
	size_t count, loff_t *ppos)
{
	mlan_hist_data_clear();
	return count;
}

/**************** Peers Support ************************/

/*
<mask>  : the bit mask of management frame reception.
	: Bit 0 - Association Request
	: Bit 1 - Association Response
	: Bit 2 - Re-Association Request
	: Bit 3 - Re-Association Response
	: Bit 4 - Probe Request
	: Bit 5 - Probe Response
	: Bit 8 - Beacon Frames
 */
#define MGMT_FRAME_MASK_PROBE_REQ 0x10
#define PEER_PRINT_SIZE 1500
static int peerScanEnable;
static struct semaphore woal_peer_sem;

typedef struct {
	t_s8 snr; //signal to noise ratio
	t_s8 nf;  //noise floor in dBm
	t_s8 sig_str; //signal strength in dBm
	mlan_802_11_mac_addr mac; //mac address of peer
	struct list_head list;
} _peer_data;

_peer_data *pPeerList;
static t_u16 woal_peer_list_size;

static t_u16 seq_read_numItemsDone;

static int woal_peer_add_peer(t_s8 snr, t_s8 nf, t_s8 sig_str, mlan_802_11_mac_addr mac);
static int woal_peer_delete_peer_list();

static int peer_get_mgmt_frame_mask(moal_private *priv, int *pmask, int get)
{
	int ret = -1;
	mlan_ioctl_req *preq = NULL;
	mlan_ds_misc_cfg *pmgmt_cfg = NULL;

	preq = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
	if (preq == NULL){
		return -ENOMEM;
	}

	pmgmt_cfg = (mlan_ds_misc_cfg *) preq->pbuf;
	preq->req_id = MLAN_IOCTL_MISC_CFG;
	pmgmt_cfg->sub_command = MLAN_OID_MISC_RX_MGMT_IND;

	if (get)
		preq->action = MLAN_ACT_GET;
	else {
		preq->action = MLAN_ACT_SET;
		pmgmt_cfg->param.mgmt_subtype_mask = *pmask;
	}

	if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, preq, MOAL_IOCTL_WAIT)) {
		kfree(preq);
		return -EFAULT;
	}
	*pmask = pmgmt_cfg->param.mgmt_subtype_mask;
	kfree(preq);
	return 0;
}

int peer_seq_lock()
{
	down_interruptible(&woal_peer_sem);
	return 0;
}

int peer_seq_unlock()
{
	up(&woal_peer_sem);
	return 0;
}

static void *
peers_seq_start(struct seq_file *s, loff_t *pos)
{
	peer_seq_lock();
	if ( 0 == *pos) {
            seq_read_numItemsDone = 0;
	    peer_seq_unlock();
		PRINTM(MINFO, "peers_seq_start: pos == 0, list_size = %d\n",woal_peer_list_size);
            return &seq_read_numItemsDone;
	}
	else if (*pos < woal_peer_list_size) {
	    peer_seq_unlock();
		PRINTM(MINFO, "peers_seq_start pos =  %d list_size = %d\n",*pos,woal_peer_list_size);
	    return &seq_read_numItemsDone;
	}
	else {
	    peer_seq_unlock();
		PRINTM(MINFO, "peers_seq_start DONE!! pos =  %d\n",*pos);
	    return NULL;
	}
}


static int
peers_seq_show(struct seq_file *s, void *v)
{
	struct debug_data_priv *pdbg_data_priv;
	struct debug_data *pdbg_data;
	moal_private *pmoal_priv=NULL;
	t_u16 *pnumItemsDone = (t_u16 *) v;
	t_u16 curr_pos = 0;
	t_u16 ix = 0;


	_peer_data *ptmp;
	struct list_head *pos, *q;
	t_u8 temp_buf[100];

	if (s){
		pdbg_data_priv = (struct debug_data_priv *)s->private;
		pdbg_data = pdbg_data_priv->items;
		pmoal_priv =  pdbg_data_priv->priv;
	}

	peer_seq_lock();
	PRINTM(MINFO, "peers_seq_show v=%x, numDone=%d total available=%d\n",v,*pnumItemsDone,woal_peer_list_size);
	if (0 == *pnumItemsDone){
		seq_printf(s, "enable=%d\n",peerScanEnable);
		seq_printf(s, "num entries=%d\n",woal_peer_list_size);
	}

	if (!pPeerList){
		peer_seq_unlock();
		return 0;
	}

	list_for_each(pos, &pPeerList->list){
		ptmp= list_entry(pos, _peer_data, list);
		ix++;

		if ((ix) && (ix <= *pnumItemsDone)) continue;

		if (curr_pos > PEER_PRINT_SIZE) {
			peer_seq_unlock();
			//To make sure we are well within page size
			PRINTM(MINFO,"\n\n curr_pos > %d\n",PEER_PRINT_SIZE);
			return 0;
		}

		sprintf(temp_buf,"%3d) %02x:%02x:%02x:%02x:%02x:%02x  noise_flr= -%ddBm  sig_strength= %ddBm  snr= %ddBm\n",
				ix,ptmp->mac[0],ptmp->mac[1],ptmp->mac[2],ptmp->mac[3],ptmp->mac[4],ptmp->mac[5],
				ptmp->nf,ptmp->sig_str,ptmp->snr);

		curr_pos += strlen(temp_buf);
		seq_printf(s, "%s",temp_buf);
		(*pnumItemsDone)++;

	}

	PRINTM(MINFO, "peers_seq_show RETURNING: v=%x, numDone=%d\n",v,*pnumItemsDone);
	peer_seq_unlock();
	return 0;
}

static void *
peers_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	t_u16 *pnumItemsDone = (t_u16 *) v;

	peer_seq_lock();
	*pos = *pnumItemsDone;

	if (!pPeerList){
		peer_seq_unlock();
		return NULL;
	}

	if (*pnumItemsDone < woal_peer_list_size){
		PRINTM(MINFO, "peers_seq_next,retrying:v = %x num_done:%d, list size:%d\n",v,*pnumItemsDone, woal_peer_list_size);
		peer_seq_unlock();
		return v;
	}

	PRINTM(MINFO," peers_seq_next: Done printing (%d items),total=%d\n",*pnumItemsDone,woal_peer_list_size);
	peer_seq_unlock();
	return NULL;
}

static void
peers_seq_stop(struct seq_file *s, void *v)
{
	PRINTM(MINFO, "peers_seq_stop\n");
	return;
}

static struct seq_operations peer_seq_ops = {
	.start = peers_seq_start,
	.next  = peers_seq_next,
	.stop  = peers_seq_stop,
	.show  = peers_seq_show
};

static int peers_seq_open(struct inode *inode, struct file *file)
{
	int ret;
	ret =  seq_open(file, &peer_seq_ops);
	if (!ret){
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
		((struct seq_file *)file->private_data)->private = PDE_DATA(inode);
#else
		((struct seq_file *)file->private_data)->private = PDE(inode)->data;
#endif
	}
	return ret;
}


static int
woal_debug_write_peers(struct file *filp, const char __user *buf,
	size_t count, loff_t *ppos)
{
	int r;
	char *pdata;
	char *p0,*p1,*p2;

	struct seq_file *s = filp->private_data;
	struct debug_data_priv *pdbg_data_priv;
	struct debug_data *pdbg_data;
	moal_private *pmoal_priv=NULL;

	if (s){
		pdbg_data_priv = (struct debug_data_priv *)s->private;
		pdbg_data = pdbg_data_priv->items;
		pmoal_priv =  pdbg_data_priv->priv;
	}

	pdata = (char *) kmalloc( count+1, GFP_KERNEL);
	if (NULL == pdata){
		return 0;
	}
	memset(pdata, 0, count+1);

	if (copy_from_user(pdata, buf, count)){
		PRINTM(MERROR, "Copy from user failed \n");
	        kfree(pdata);
		return 0;
	}

	p0 = pdata;
	p1 = strstr(p0,"enable");
	if (NULL == p1){
		kfree(pdata);
		return 0;
	}

	p2 = strstr(p1,"=");
	if (NULL == p2){
		kfree(pdata);
		return 0;
	}
	p2++;
	r = woal_string_to_number(p2);


	peer_seq_lock();
	if (0 == r){
		peerScanEnable = 0;
	}
	else {
		peerScanEnable = 1;
	}
	peer_seq_unlock();

	do {
		int mgmt_mask, get;

		get = 1;
		if (0 != peer_get_mgmt_frame_mask(pmoal_priv, &mgmt_mask, get)){
			PRINTM(MERROR,"Failed to get mgmt_mask\n");
			break;
		}

		get = 0;
		peer_seq_lock();
		if (peerScanEnable)
			mgmt_mask |= MGMT_FRAME_MASK_PROBE_REQ;
		else
			mgmt_mask &= ~MGMT_FRAME_MASK_PROBE_REQ;
		peer_seq_unlock();
		if (0 != peer_get_mgmt_frame_mask(pmoal_priv, &mgmt_mask, get)){
			PRINTM(MERROR,"Failed to set mgmt_mask\n");
			break;
		}
	} while (0);

	peer_seq_lock();
	if (!peerScanEnable){
		peer_seq_unlock();
		woal_peer_delete_peer_list();
	}
	else {
		peer_seq_unlock();
	}

	kfree(pdata);

return count;
}

int woal_peer_mgmt_frame_callback( t_s8 snr, t_s8 nf, t_s8 sig_str,
			           mlan_802_11_mac_addr mac)
{
	woal_peer_add_peer(snr, nf, sig_str, mac);
	return 0;
}

static int is_mac_addr_same(t_u8 *a1, t_u8 *a2)
{
	int ix;
	for (ix = 0; ix < MLAN_MAC_ADDR_LENGTH; ix++){
		if (a1[ix] != a2[ix])
			return 0;
	}
	return 1;
}

int woal_peer_add_peer(t_s8 snr, t_s8 nf, t_s8 sig_str, mlan_802_11_mac_addr mac)
{
	/* We first check if this peer is already present*/
	/* If present, update rssi info, else add new entry*/

	PRINTM(MINFO,"woal_peer_add_peer(),mac:%02x:%02x:%02x:%02x:%02x:%02x\n",
			mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	peer_seq_lock();
	if (pPeerList){

		_peer_data *ptmp;
		struct list_head *pos;
		//Search for existing duplicate entries

		list_for_each(pos, &pPeerList->list){
			ptmp= list_entry(pos, _peer_data, list);
			if (is_mac_addr_same((t_u8 *) ptmp->mac, (t_u8 *) mac)){
				// Entry present, update RSSI
				ptmp->snr = snr;
				ptmp->nf  = nf;
				ptmp->sig_str = sig_str;
				PRINTM(MINFO,"Entry Updated!\n");
				peer_seq_unlock();
				return 0;
			}
		}

		ptmp = (_peer_data *) kmalloc(sizeof(_peer_data),GFP_KERNEL);
		if (!ptmp){
			peer_seq_unlock();
			return -1;
		}
		ptmp->snr = snr;
		ptmp->nf  = nf;
		ptmp->sig_str = sig_str;
		mlan_memcpy(ptmp->mac, mac, MLAN_MAC_ADDR_LENGTH);
		list_add(&(ptmp->list), &(pPeerList->list));
		woal_peer_list_size++;
		PRINTM(MINFO,"Entry (%02x:%02x:%02x:%02x:%02x:%02x) added, total:%d!\n",
				mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],woal_peer_list_size);

	}
	else
	{
		pPeerList = (_peer_data *) kmalloc(sizeof(_peer_data),GFP_KERNEL);
		if (!pPeerList) return -1;
		pPeerList->snr = snr;
		pPeerList->nf  = nf;
		pPeerList->sig_str = sig_str;
		mlan_memcpy(pPeerList->mac, mac, MLAN_MAC_ADDR_LENGTH);
		INIT_LIST_HEAD(&(pPeerList->list));
		PRINTM(MINFO," woal_peer_add_peer: List created (%p)!\n", pPeerList);
	}
	peer_seq_unlock();

	return 0;
}

int woal_peer_delete_peer_list()
{
	peer_seq_lock();
	if (pPeerList){
		_peer_data *ptmp, *q;
		struct list_head *pos;

		list_for_each_safe(pos, q, &pPeerList->list){
			ptmp = list_entry(pos, _peer_data, list);
			list_del(pos);
			kfree(ptmp);
		}
		woal_peer_list_size = 0;
		kfree(pPeerList);
		pPeerList = NULL;
	}
	peer_seq_unlock();
	return 0;
}

int woal_peer_mgmt_frame_ioctl(t_u16 mask)
{
	PRINTM(MINFO,"woal_peer_mgmt_frame_ioctl(): mask: 0x%x\n",mask);
	peer_seq_lock();
	if (mask & MGMT_FRAME_MASK_PROBE_REQ){
		peerScanEnable = 1;
		peer_seq_unlock();
	}
	else {
		peerScanEnable = 0;
		peer_seq_unlock();
		woal_peer_delete_peer_list();
	}
	return 0;
}

/********************************************************
		Local Functions
********************************************************/
/**
 *  @brief Proc read function
 *
 *  @param sfp 	   pointer to seq_file structure
 *  @param data
 *
 *  @return 	   Number of output data or MLAN_STATUS_FAILURE
 */
static int
woal_debug_read(struct seq_file *sfp, void *data)
{
	int val = 0;
	unsigned int i;
#ifdef SDIO_MULTI_PORT_TX_AGGR
	unsigned int j;
#endif
	struct debug_data_priv *items_priv =
		(struct debug_data_priv *)sfp->private;
	struct debug_data *d = items_priv->items;
	moal_private *priv = items_priv->priv;

	ENTER();

	if (priv == NULL) {
		LEAVE();
		return -EFAULT;
	}

	if (MODULE_GET == 0) {
		LEAVE();
		return -EFAULT;
	}

	priv->phandle->driver_state = woal_check_driver_status(priv->phandle);
	/* Get debug information */
	if (woal_get_debug_info(priv, MOAL_PROC_WAIT, &info))
		goto exit;

	for (i = 0; i < (unsigned int)items_priv->num_of_items; i++) {
		if (d[i].size == 1)
			val = *((t_u8 *) d[i].addr);
		else if (d[i].size == 2)
			val = *((t_u16 *) d[i].addr);
		else if (d[i].size == 4)
			val = *((t_ptr *) d[i].addr);
		else {
			unsigned int j;
			seq_printf(sfp, "%s=", d[i].name);
			for (j = 0; j < d[i].size; j += 2) {
				val = *(t_u16 *) (d[i].addr + j);
				seq_printf(sfp, "0x%x ", val);
			}
			seq_printf(sfp, "\n");
			continue;
		}
		if (strstr(d[i].name, "id")
		    || strstr(d[i].name, "bitmap")
			)
			seq_printf(sfp, "%s=0x%x\n", d[i].name, val);
		else
			seq_printf(sfp, "%s=%d\n", d[i].name, val);
	}
#ifdef SDIO_MULTI_PORT_TX_AGGR
	seq_printf(sfp, "last_recv_wr_bitmap=0x%x last_mp_index=%d\n",
		   info.last_recv_wr_bitmap, info.last_mp_index);
	for (i = 0; i < SDIO_MP_DBG_NUM; i++) {
		seq_printf(sfp,
			   "mp_wr_bitmap: 0x%x mp_wr_ports=0x%x len=%d curr_wr_port=0x%x\n",
			   info.last_mp_wr_bitmap[i], info.last_mp_wr_ports[i],
			   info.last_mp_wr_len[i], info.last_curr_wr_port[i]);
		for (j = 0; j < SDIO_MP_AGGR_DEF_PKT_LIMIT; j++) {
			seq_printf(sfp, "0x%02x ",
				   info.last_mp_wr_info[i *
							SDIO_MP_AGGR_DEF_PKT_LIMIT
							+ j]);
		}
		seq_printf(sfp, "\n");
	}
	seq_printf(sfp, "SDIO MPA Tx: ");
	for (i = 0; i < SDIO_MP_AGGR_DEF_PKT_LIMIT; i++)
		seq_printf(sfp, "%d ", info.mpa_tx_count[i]);
	seq_printf(sfp, "\n");
#endif
#ifdef SDIO_MULTI_PORT_RX_AGGR
	seq_printf(sfp, "SDIO MPA Rx: ");
	for (i = 0; i < SDIO_MP_AGGR_DEF_PKT_LIMIT; i++)
		seq_printf(sfp, "%d ", info.mpa_rx_count[i]);
	seq_printf(sfp, "\n");
#endif
	seq_printf(sfp, "tcp_ack_drop_cnt=%d\n", priv->tcp_ack_drop_cnt);
	seq_printf(sfp, "tcp_ack_cnt=%d\n", priv->tcp_ack_cnt);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 29)
	for (i = 0; i < 4; i++)
		seq_printf(sfp, "wmm_tx_pending[%d]:%d\n", i,
			   atomic_read(&priv->wmm_tx_pending[i]));
#endif
	if (info.tx_tbl_num) {
		seq_printf(sfp, "Tx BA stream table:\n");
		for (i = 0; i < info.tx_tbl_num; i++) {
			seq_printf(sfp,
				   "tid = %d, ra = %02x:%02x:%02x:%02x:%02x:%02x amsdu=%d\n",
				   (int)info.tx_tbl[i].tid,
				   info.tx_tbl[i].ra[0], info.tx_tbl[i].ra[1],
				   info.tx_tbl[i].ra[2], info.tx_tbl[i].ra[3],
				   info.tx_tbl[i].ra[4], info.tx_tbl[i].ra[5],
				   (int)info.tx_tbl[i].amsdu);
		}
	}
	if (info.rx_tbl_num) {
		seq_printf(sfp, "Rx reorder table:\n");
		for (i = 0; i < info.rx_tbl_num; i++) {
			unsigned int j;

			seq_printf(sfp,
				   "tid = %d, ta =  %02x:%02x:%02x:%02x:%02x:%02x, start_win = %d, "
				   "win_size = %d, amsdu=%d\n",
				   (int)info.rx_tbl[i].tid,
				   info.rx_tbl[i].ta[0], info.rx_tbl[i].ta[1],
				   info.rx_tbl[i].ta[2], info.rx_tbl[i].ta[3],
				   info.rx_tbl[i].ta[4], info.rx_tbl[i].ta[5],
				   (int)info.rx_tbl[i].start_win,
				   (int)info.rx_tbl[i].win_size,
				   (int)info.rx_tbl[i].amsdu);
			seq_printf(sfp, "buffer: ");
			for (j = 0; j < info.rx_tbl[i].win_size; j++) {
				if (info.rx_tbl[i].buffer[j] == MTRUE)
					seq_printf(sfp, "1 ");
				else
					seq_printf(sfp, "0 ");
			}
			seq_printf(sfp, "\n");
		}
	}
	if (info.tdls_peer_list) {
		for (i = 0; i < info.tdls_peer_num; i++) {
			unsigned int j;
			seq_printf(sfp,
				   "tdls peer: %02x:%02x:%02x:%02x:%02x:%02x snr=%d nf=%d\n",
				   info.tdls_peer_list[i].mac_addr[0],
				   info.tdls_peer_list[i].mac_addr[1],
				   info.tdls_peer_list[i].mac_addr[2],
				   info.tdls_peer_list[i].mac_addr[3],
				   info.tdls_peer_list[i].mac_addr[4],
				   info.tdls_peer_list[i].mac_addr[5],
				   info.tdls_peer_list[i].snr,
				   -info.tdls_peer_list[i].nf);
			seq_printf(sfp, "htcap: ");
			for (j = 0; j < sizeof(IEEEtypes_HTCap_t); j++)
				seq_printf(sfp, "%02x ",
					   info.tdls_peer_list[i].ht_cap[j]);
			seq_printf(sfp, "\nExtcap: ");
			for (j = 0; j < sizeof(IEEEtypes_ExtCap_t); j++)
				seq_printf(sfp, "%02x ",
					   info.tdls_peer_list[i].ext_cap[j]);
			seq_printf(sfp, "\n");
		}
	}
exit:
	MODULE_PUT;
	LEAVE();
	return 0;
}

/**
 *  @brief Proc write function
 *
 *  @param f	   file pointer
 *  @param buf     pointer to data buffer
 *  @param count   data number to write
 *  @param off     Offset
 *
 *  @return 	   number of data
 */
static ssize_t
woal_debug_write(struct file *f, const char __user * buf, size_t count,
		 loff_t * off)
{
	int r, i;
	char *pdata;
	char *p;
	char *p0;
	char *p1;
	char *p2;
	struct seq_file *sfp = f->private_data;
	struct debug_data_priv *items_priv =
		(struct debug_data_priv *)sfp->private;
	struct debug_data *d = items_priv->items;
	moal_private *priv = items_priv->priv;
#ifdef DEBUG_LEVEL1
	t_u32 last_drvdbg = drvdbg;
#endif

	ENTER();

	if (MODULE_GET == 0) {
		LEAVE();
		return MLAN_STATUS_FAILURE;
	}

	pdata = (char *)kmalloc(count + 1, GFP_KERNEL);
	if (pdata == NULL) {
		MODULE_PUT;
		LEAVE();
		return 0;
	}
	memset(pdata, 0, count + 1);

	if (copy_from_user(pdata, buf, count)) {
		PRINTM(MERROR, "Copy from user failed\n");
		kfree(pdata);
		MODULE_PUT;
		LEAVE();
		return 0;
	}

	if (woal_get_debug_info(priv, MOAL_PROC_WAIT, &info)) {
		kfree(pdata);
		MODULE_PUT;
		LEAVE();
		return 0;
	}

	p0 = pdata;
	for (i = 0; i < items_priv->num_of_items; i++) {
		do {
			p = strstr(p0, d[i].name);
			if (p == NULL)
				break;
			p1 = strchr(p, '\n');
			if (p1 == NULL)
				break;
			p0 = p1++;
			p2 = strchr(p, '=');
			if (!p2)
				break;
			p2++;
			r = woal_string_to_number(p2);
			if (d[i].size == 1)
				*((t_u8 *) d[i].addr) = (t_u8) r;
			else if (d[i].size == 2)
				*((t_u16 *) d[i].addr) = (t_u16) r;
			else if (d[i].size == 4)
				*((t_ptr *) d[i].addr) = (t_ptr) r;
			break;
		} while (MTRUE);
	}
	kfree(pdata);

#ifdef DEBUG_LEVEL1
	if (last_drvdbg != drvdbg) {
		woal_set_drvdbg(priv, drvdbg);

	}
#endif
#if 0
	/* Set debug information */
	if (woal_set_debug_info(priv, MOAL_PROC_WAIT, &info)) {
		MODULE_PUT;
		LEAVE();
		return 0;
	}
#endif
	MODULE_PUT;
	LEAVE();
	return count;
}

static int
woal_debug_proc_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
	return single_open(file, woal_debug_read, PDE_DATA(inode));
#else
	return single_open(file, woal_debug_read, PDE(inode)->data);
#endif
}

static const struct file_operations debug_proc_fops = {
	.owner = THIS_MODULE,
	.open = woal_debug_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = woal_debug_write,
};

static struct file_operations hgm_file_ops = {
	.owner   = THIS_MODULE,
	.open    = hgm_seq_open,
	.write   = woal_debug_write_histogram,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

static struct file_operations peers_file_ops = {
	.owner   = THIS_MODULE,
	.open    = peers_seq_open,
	.write   = woal_debug_write_peers,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};


/********************************************************
		Global Functions
********************************************************/
/**
 *  @brief Create debug proc file
 *
 *  @param priv	   A pointer to a moal_private structure
 *
 *  @return 	   N/A
 */
void
woal_debug_entry(moal_private * priv)
{
	struct proc_dir_entry *r;
    struct proc_dir_entry *r2;
    struct proc_dir_entry *r3;
	int i;
	int handle_items;

	ENTER();

	if (priv->proc_entry == NULL) {
		LEAVE();
		return;
	}
#ifdef STA_SUPPORT
	if (GET_BSS_ROLE(priv) == MLAN_BSS_ROLE_STA) {
		priv->items_priv.items =
			(struct debug_data *)kmalloc(sizeof(items), GFP_KERNEL);
		if (!priv->items_priv.items) {
			PRINTM(MERROR,
			       "Failed to allocate memory for debug data\n");
			LEAVE();
			return;
		}
		memcpy(priv->items_priv.items, items, sizeof(items));
		priv->items_priv.num_of_items = ARRAY_SIZE(items);
        priv->items_priv_hist.num_of_items = 0;
        priv->items_priv_peers.num_of_items = 0;
	}
#endif
#ifdef UAP_SUPPORT
	if (GET_BSS_ROLE(priv) == MLAN_BSS_ROLE_UAP) {
		priv->items_priv.items =
			(struct debug_data *)kmalloc(sizeof(uap_items),
						     GFP_KERNEL);
		if (!priv->items_priv.items) {
			PRINTM(MERROR,
			       "Failed to allocate memory for debug data\n");
			LEAVE();
			return;
		}
		memcpy(priv->items_priv.items, uap_items, sizeof(uap_items));
		priv->items_priv.num_of_items = ARRAY_SIZE(uap_items);
	}
#endif

	priv->items_priv.priv = priv;
    priv->items_priv_hist.priv = priv;
    priv->items_priv_peers.priv = priv;
	handle_items = 9;
#ifdef SDIO_MMC_DEBUG
	handle_items += 2;
#endif
#if defined(SDIO_SUSPEND_RESUME)
	handle_items += 2;
#endif
	for (i = 1; i <= handle_items; i++)
		priv->items_priv.items[priv->items_priv.num_of_items -
				       i].addr += (t_ptr) (priv->phandle);

	/* Create proc entry */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 26)
	r = proc_create_data("debug", 0644, priv->proc_entry, &debug_proc_fops,
			     &priv->items_priv);
	if (r == NULL)
#else
	r = create_proc_entry("debug", 0644, priv->proc_entry);
	if (r) {
		r->data = &priv->items_priv;
		r->proc_fops = &debug_proc_fops;
	} else
#endif
	{
		PRINTM(MMSG, "Fail to create proc debug entry\n");
		LEAVE();
		return;
	}

	/* Initialize hgm data structure */
	hgm_seq_init_globals();

	/* Create proc entry for driver histogram data */
    r2 = create_proc_entry("histogram", 0664, priv->proc_entry);
    if (r2 == NULL) {
        LEAVE();
        return;
    }
    r2->data = &priv->items_priv_hist;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
    r2->owner = THIS_MODULE;
#endif
    r2->proc_fops = &hgm_file_ops;
    r2->uid = 0;
    r2->gid = 1008; // wifi group
    mlan_hist_data_clear();

    r3 = create_proc_entry("peers", 0664, priv->proc_entry);
    if (r3 == NULL) {
        LEAVE();
        return;
    }
    r3->data = &priv->items_priv_peers;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
    r3->owner = THIS_MODULE;
#endif
    r3->proc_fops = &peers_file_ops;
    r3->uid = 0;
    r3->gid = 1008; // wifi group

    //Register the peer mgmt frame callback function.
    mlan_register_peer_mac_cb((MOAL_PEER_MGMT_FRAME_CB) &woal_peer_mgmt_frame_callback);
    woal_peer_list_size = 0;
    sema_init(&woal_peer_sem,1);

    LEAVE();
}

/**
 *  @brief Remove proc file
 *
 *  @param priv	 A pointer to a moal_private structure
 *
 *  @return 	 N/A
 */
void
woal_debug_remove(moal_private * priv)
{
	ENTER();

	kfree(priv->items_priv.items);
	/* Remove proc entry */
	remove_proc_entry("debug", priv->proc_entry);
	remove_proc_entry("histogram", priv->proc_entry);
	woal_peer_delete_peer_list();
	remove_proc_entry("peers", priv->proc_entry);

	LEAVE();
}
#endif
