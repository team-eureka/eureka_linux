/** @file mlan_uap_ioctl.c
 *
 *  @brief This file contains the handling of AP mode ioctls
 *
 *  Copyright (C) 2009-2014, Marvell International Ltd.
 *
 *  This software file (the "File") is distributed by Marvell International
 *  Ltd. under the terms of the GNU General Public License Version 2, June 1991
 *  (the "License").  You may use, redistribute and/or modify this File in
 *  accordance with the terms and conditions of the License, a copy of which
 *  is available by writing to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
 *  worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 *  THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 *  ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
 *  this warranty disclaimer.
 */

/********************************************************
Change log:
    02/05/2009: initial version
********************************************************/

#include "mlan.h"
#include "mlan_util.h"
#include "mlan_fw.h"
#ifdef STA_SUPPORT
#include "mlan_join.h"
#endif
#include "mlan_main.h"
#include "mlan_uap.h"
#include "mlan_sdio.h"
#include "mlan_11n.h"
#include "mlan_fw.h"

/********************************************************
			Global Variables
********************************************************/
extern t_u8 tos_to_tid_inv[];

/********************************************************
			Local Functions
********************************************************/
/**
 *  @brief Stop BSS
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_bss_ioctl_stop(IN pmlan_adapter pmadapter,
			IN pmlan_ioctl_req pioctl_req)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	pmlan_private pmpriv = pmadapter->priv[pioctl_req->bss_index];

	ENTER();

	ret = wlan_prepare_cmd(pmpriv,
			       HOST_CMD_APCMD_BSS_STOP,
			       HostCmd_ACT_GEN_SET,
			       0, (t_void *) pioctl_req, MNULL);
	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief Start BSS
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_bss_ioctl_start(IN pmlan_adapter pmadapter,
			 IN pmlan_ioctl_req pioctl_req)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	pmlan_private pmpriv = pmadapter->priv[pioctl_req->bss_index];

	ENTER();

	ret = wlan_prepare_cmd(pmpriv,
			       HOST_CMD_APCMD_BSS_START,
			       HostCmd_ACT_GEN_SET,
			       0, (t_void *) pioctl_req, MNULL);
	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief reset BSS
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_bss_ioctl_reset(IN pmlan_adapter pmadapter,
			 IN pmlan_ioctl_req pioctl_req)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	pmlan_private pmpriv = pmadapter->priv[pioctl_req->bss_index];
	t_u8 i = 0;

	ENTER();

	/*
	 * Reset any uap private parameters here
	 */
	for (i = 0; i < pmadapter->max_mgmt_ie_index; i++)
		memset(pmadapter, &pmpriv->mgmt_ie[i], 0, sizeof(custom_ie));
	pmpriv->add_ba_param.timeout = MLAN_DEFAULT_BLOCK_ACK_TIMEOUT;
	pmpriv->add_ba_param.tx_win_size = MLAN_UAP_AMPDU_DEF_TXWINSIZE;
	pmpriv->add_ba_param.rx_win_size = MLAN_UAP_AMPDU_DEF_RXWINSIZE;
	for (i = 0; i < MAX_NUM_TID; i++) {
		pmpriv->aggr_prio_tbl[i].ampdu_user = tos_to_tid_inv[i];
		pmpriv->aggr_prio_tbl[i].amsdu = BA_STREAM_NOT_ALLOWED;
		pmpriv->addba_reject[i] = ADDBA_RSP_STATUS_ACCEPT;
	}
	pmpriv->aggr_prio_tbl[6].ampdu_user =
		pmpriv->aggr_prio_tbl[7].ampdu_user = BA_STREAM_NOT_ALLOWED;

	/* hs_configured, hs_activated are reset by main loop */
	pmadapter->hs_cfg.conditions = HOST_SLEEP_DEF_COND;
	pmadapter->hs_cfg.gpio = HOST_SLEEP_DEF_GPIO;
	pmadapter->hs_cfg.gap = HOST_SLEEP_DEF_GAP;

	ret = wlan_prepare_cmd(pmpriv,
			       HOST_CMD_APCMD_SYS_RESET,
			       HostCmd_ACT_GEN_SET,
			       0, (t_void *) pioctl_req, MNULL);
	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief Set/Get MAC address
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_bss_ioctl_mac_address(IN pmlan_adapter pmadapter,
			       IN pmlan_ioctl_req pioctl_req)
{
	mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_ds_bss *bss = MNULL;
	mlan_status ret = MLAN_STATUS_SUCCESS;
	t_u16 cmd_action = 0;

	ENTER();

	bss = (mlan_ds_bss *) pioctl_req->pbuf;
	if (pioctl_req->action == MLAN_ACT_SET) {
		memcpy(pmadapter, pmpriv->curr_addr, &bss->param.mac_addr,
		       MLAN_MAC_ADDR_LENGTH);
		cmd_action = HostCmd_ACT_GEN_SET;
	} else
		cmd_action = HostCmd_ACT_GEN_GET;
	/* Send request to firmware */
	ret = wlan_prepare_cmd(pmpriv, HOST_CMD_APCMD_SYS_CONFIGURE,
			       cmd_action, 0, (t_void *) pioctl_req, MNULL);

	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief Get Uap statistics
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_get_stats(IN pmlan_adapter pmadapter, IN pmlan_ioctl_req pioctl_req)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	pmlan_private pmpriv = pmadapter->priv[pioctl_req->bss_index];

	ENTER();

	ret = wlan_prepare_cmd(pmpriv,
			       HostCmd_CMD_802_11_SNMP_MIB,
			       HostCmd_ACT_GEN_GET,
			       0, (t_void *) pioctl_req, MNULL);
	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief Set/Get AP config
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_bss_ioctl_config(IN pmlan_adapter pmadapter,
			  IN pmlan_ioctl_req pioctl_req)
{
	mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_status ret = MLAN_STATUS_SUCCESS;
	t_u16 cmd_action = 0;

	ENTER();

	if (pioctl_req->action == MLAN_ACT_SET)
		cmd_action = HostCmd_ACT_GEN_SET;
	else
		cmd_action = HostCmd_ACT_GEN_GET;

	/* Send request to firmware */
	ret = wlan_prepare_cmd(pmpriv, HOST_CMD_APCMD_SYS_CONFIGURE,
			       cmd_action, 0, (t_void *) pioctl_req, MNULL);

	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief deauth sta
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_bss_ioctl_deauth_sta(IN pmlan_adapter pmadapter,
			      IN pmlan_ioctl_req pioctl_req)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	pmlan_private pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_ds_bss *bss = MNULL;

	ENTER();

	bss = (mlan_ds_bss *) pioctl_req->pbuf;
	ret = wlan_prepare_cmd(pmpriv,
			       HOST_CMD_APCMD_STA_DEAUTH,
			       HostCmd_ACT_GEN_SET,
			       0,
			       (t_void *) pioctl_req,
			       (t_void *) & bss->param.deauth_param);
	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief Get station list
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_get_sta_list(IN pmlan_adapter pmadapter, IN pmlan_ioctl_req pioctl_req)
{
	pmlan_private pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_status ret = MLAN_STATUS_SUCCESS;

	ENTER();

	/* Send request to firmware */
	ret = wlan_prepare_cmd(pmpriv,
			       HOST_CMD_APCMD_STA_LIST,
			       HostCmd_ACT_GEN_GET,
			       0, (t_void *) pioctl_req, MNULL);

	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief soft_reset
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_misc_ioctl_soft_reset(IN pmlan_adapter pmadapter,
			       IN pmlan_ioctl_req pioctl_req)
{
	mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_status ret = MLAN_STATUS_SUCCESS;

	ENTER();

	ret = wlan_prepare_cmd(pmpriv,
			       HostCmd_CMD_SOFT_RESET,
			       HostCmd_ACT_GEN_SET,
			       0, (t_void *) pioctl_req, MNULL);
	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief Tx data pause
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_misc_ioctl_txdatapause(IN pmlan_adapter pmadapter,
				IN pmlan_ioctl_req pioctl_req)
{
	mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_ds_misc_cfg *pmisc = (mlan_ds_misc_cfg *) pioctl_req->pbuf;
	mlan_status ret = MLAN_STATUS_SUCCESS;
	t_u16 cmd_action = 0;

	ENTER();

	if (pioctl_req->action == MLAN_ACT_SET)
		cmd_action = HostCmd_ACT_GEN_SET;
	else
		cmd_action = HostCmd_ACT_GEN_GET;
	ret = wlan_prepare_cmd(pmpriv,
			       HostCmd_CMD_CFG_TX_DATA_PAUSE,
			       cmd_action,
			       0,
			       (t_void *) pioctl_req,
			       &(pmisc->param.tx_datapause));
	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief Set/Get Power mode
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_pm_ioctl_mode(IN pmlan_adapter pmadapter,
		       IN pmlan_ioctl_req pioctl_req)
{
	mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_ds_pm_cfg *pm = MNULL;
	mlan_status ret = MLAN_STATUS_SUCCESS;
	t_u16 cmd_action = 0;
	t_u32 cmd_oid = 0;

	ENTER();

	pm = (mlan_ds_pm_cfg *) pioctl_req->pbuf;
	if (pioctl_req->action == MLAN_ACT_SET) {
		if (pm->param.ps_mgmt.ps_mode == PS_MODE_INACTIVITY) {
			cmd_action = EN_AUTO_PS;
			cmd_oid = BITMAP_UAP_INACT_PS;
		} else if (pm->param.ps_mgmt.ps_mode == PS_MODE_PERIODIC_DTIM) {
			cmd_action = EN_AUTO_PS;
			cmd_oid = BITMAP_UAP_DTIM_PS;
		} else {
			cmd_action = DIS_AUTO_PS;
			cmd_oid = BITMAP_UAP_INACT_PS | BITMAP_UAP_DTIM_PS;
		}
	} else {
		cmd_action = GET_PS;
		cmd_oid = BITMAP_UAP_INACT_PS | BITMAP_UAP_DTIM_PS;
	}
	/* Send request to firmware */
	ret = wlan_prepare_cmd(pmpriv,
			       HostCmd_CMD_802_11_PS_MODE_ENH,
			       cmd_action, cmd_oid, (t_void *) pioctl_req,
			       (t_void *) & pm->param.ps_mgmt);
	if ((ret == MLAN_STATUS_SUCCESS) &&
	    (pioctl_req->action == MLAN_ACT_SET) &&
	    (cmd_action == DIS_AUTO_PS)) {
		ret = wlan_prepare_cmd(pmpriv,
				       HostCmd_CMD_802_11_PS_MODE_ENH, GET_PS,
				       0, MNULL, MNULL);
	}
	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief Set WAPI IE
 *
 *  @param priv         A pointer to mlan_private structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return             MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_set_wapi_ie(mlan_private * priv, pmlan_ioctl_req pioctl_req)
{
	mlan_ds_misc_cfg *misc = MNULL;
	mlan_status ret = MLAN_STATUS_SUCCESS;

	ENTER();
	misc = (mlan_ds_misc_cfg *) pioctl_req->pbuf;
	if (misc->param.gen_ie.len) {
		if (misc->param.gen_ie.len > sizeof(priv->wapi_ie)) {
			PRINTM(MWARN, "failed to copy WAPI IE, too big\n");
			pioctl_req->status_code = MLAN_ERROR_INVALID_PARAMETER;
			LEAVE();
			return MLAN_STATUS_FAILURE;
		}
		memcpy(priv->adapter, priv->wapi_ie, misc->param.gen_ie.ie_data,
		       misc->param.gen_ie.len);
		priv->wapi_ie_len = misc->param.gen_ie.len;
		PRINTM(MIOCTL, "Set wapi_ie_len=%d IE=%#x\n", priv->wapi_ie_len,
		       priv->wapi_ie[0]);
		DBG_HEXDUMP(MCMD_D, "wapi_ie", priv->wapi_ie,
			    priv->wapi_ie_len);
		if (priv->wapi_ie[0] == WAPI_IE)
			priv->sec_info.wapi_enabled = MTRUE;
	} else {
		memset(priv->adapter, priv->wapi_ie, 0, sizeof(priv->wapi_ie));
		priv->wapi_ie_len = misc->param.gen_ie.len;
		PRINTM(MINFO, "Reset wapi_ie_len=%d IE=%#x\n",
		       priv->wapi_ie_len, priv->wapi_ie[0]);
		priv->sec_info.wapi_enabled = MFALSE;
	}

	/* Send request to firmware */
	ret = wlan_prepare_cmd(priv, HOST_CMD_APCMD_SYS_CONFIGURE,
			       HostCmd_ACT_GEN_SET, 0, (t_void *) pioctl_req,
			       MNULL);

	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief Set generic IE
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_misc_ioctl_gen_ie(IN pmlan_adapter pmadapter,
			   IN pmlan_ioctl_req pioctl_req)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_ds_misc_cfg *misc = MNULL;
	IEEEtypes_VendorHeader_t *pvendor_ie = MNULL;

	ENTER();

	misc = (mlan_ds_misc_cfg *) pioctl_req->pbuf;

	if ((misc->param.gen_ie.type == MLAN_IE_TYPE_GEN_IE) &&
	    (pioctl_req->action == MLAN_ACT_SET)) {
		if (misc->param.gen_ie.len) {
			pvendor_ie =
				(IEEEtypes_VendorHeader_t *) misc->param.gen_ie.
				ie_data;
			if (pvendor_ie->element_id == WAPI_IE) {
				/* IE is a WAPI IE so call set_wapi function */
				ret = wlan_uap_set_wapi_ie(pmpriv, pioctl_req);
			}
		} else {
			/* clear WAPI IE */
			ret = wlan_uap_set_wapi_ie(pmpriv, pioctl_req);
		}
	}
	LEAVE();
	return ret;
}

/**
 *  @brief Set/Get WAPI status
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_SUCCESS --success
 */
static mlan_status
wlan_uap_sec_ioctl_wapi_enable(IN pmlan_adapter pmadapter,
			       IN pmlan_ioctl_req pioctl_req)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_ds_sec_cfg *sec = MNULL;
	ENTER();
	sec = (mlan_ds_sec_cfg *) pioctl_req->pbuf;
	if (pioctl_req->action == MLAN_ACT_GET) {
		if (pmpriv->wapi_ie_len)
			sec->param.wapi_enabled = MTRUE;
		else
			sec->param.wapi_enabled = MFALSE;
	} else {
		if (sec->param.wapi_enabled == MFALSE) {
			memset(pmpriv->adapter, pmpriv->wapi_ie, 0,
			       sizeof(pmpriv->wapi_ie));
			pmpriv->wapi_ie_len = 0;
			PRINTM(MINFO, "Reset wapi_ie_len=%d IE=%#x\n",
			       pmpriv->wapi_ie_len, pmpriv->wapi_ie[0]);
			pmpriv->sec_info.wapi_enabled = MFALSE;
		}
	}
	pioctl_req->data_read_written = sizeof(t_u32) + MLAN_SUB_COMMAND_SIZE;
	LEAVE();
	return ret;
}

/**
 *  @brief report mic error
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_sec_ioctl_report_mic_error(IN pmlan_adapter pmadapter,
				    IN pmlan_ioctl_req pioctl_req)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	pmlan_private pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_ds_sec_cfg *sec = MNULL;

	ENTER();

	sec = (mlan_ds_sec_cfg *) pioctl_req->pbuf;
	ret = wlan_prepare_cmd(pmpriv,
			       HOST_CMD_APCMD_REPORT_MIC,
			       HostCmd_ACT_GEN_SET,
			       0,
			       (t_void *) pioctl_req,
			       (t_void *) sec->param.sta_mac);
	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/**
 *  @brief Set encrypt key
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_sec_ioctl_set_encrypt_key(IN pmlan_adapter pmadapter,
				   IN pmlan_ioctl_req pioctl_req)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_ds_sec_cfg *sec = MNULL;

	ENTER();
	sec = (mlan_ds_sec_cfg *) pioctl_req->pbuf;
	if (pioctl_req->action != MLAN_ACT_SET) {
		pioctl_req->status_code = MLAN_ERROR_IOCTL_INVALID;
		LEAVE();
		return MLAN_STATUS_FAILURE;
	}
	if (!sec->param.encrypt_key.key_remove &&
	    !sec->param.encrypt_key.key_len) {
		PRINTM(MCMND, "Skip set key with key_len = 0\n");
		LEAVE();
		return ret;
	}
	ret = wlan_prepare_cmd(pmpriv,
			       HostCmd_CMD_802_11_KEY_MATERIAL,
			       HostCmd_ACT_GEN_SET,
			       KEY_INFO_ENABLED,
			       (t_void *) pioctl_req, &sec->param.encrypt_key);

	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;
	LEAVE();
	return ret;
}

/**
 *  @brief Get BSS information
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_SUCCESS --success
 */
static mlan_status
wlan_uap_get_bss_info(IN pmlan_adapter pmadapter, IN pmlan_ioctl_req pioctl_req)
{
	pmlan_private pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_status ret = MLAN_STATUS_SUCCESS;
	mlan_ds_get_info *info;

	ENTER();

	info = (mlan_ds_get_info *) pioctl_req->pbuf;
	/* Connection status */
	info->param.bss_info.media_connected = pmpriv->media_connected;

	/* Radio status */
	info->param.bss_info.radio_on = pmadapter->radio_on;

	/* BSSID */
	memcpy(pmadapter, &info->param.bss_info.bssid, pmpriv->curr_addr,
	       MLAN_MAC_ADDR_LENGTH);
	info->param.bss_info.is_hs_configured = pmadapter->is_hs_configured;
	pioctl_req->data_read_written =
		sizeof(mlan_bss_info) + MLAN_SUB_COMMAND_SIZE;

	LEAVE();
	return ret;
}

/**
 *  @brief Set Host Sleep configurations
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_SUCCES/MLAN_STATUS_PENDING --success, otherwise fail
 */
static mlan_status
wlan_uap_pm_ioctl_deepsleep(IN pmlan_adapter pmadapter,
			    IN pmlan_ioctl_req pioctl_req)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_index];
	mlan_ds_pm_cfg *pm = MNULL;
	mlan_ds_auto_ds auto_ds;
	t_u32 mode;

	ENTER();

	if (pioctl_req->buf_len < sizeof(mlan_ds_pm_cfg)) {
		PRINTM(MWARN, "MLAN bss IOCTL length is too short.\n");
		pioctl_req->data_read_written = 0;
		pioctl_req->buf_len_needed = sizeof(mlan_ds_pm_cfg);
		pioctl_req->status_code = MLAN_ERROR_INVALID_PARAMETER;
		LEAVE();
		return MLAN_STATUS_RESOURCE;
	}
	pm = (mlan_ds_pm_cfg *) pioctl_req->pbuf;

	if (pioctl_req->action == MLAN_ACT_GET) {
		if (pmadapter->is_deep_sleep) {
			pm->param.auto_deep_sleep.auto_ds = DEEP_SLEEP_ON;
			pm->param.auto_deep_sleep.idletime =
				pmadapter->idle_time;
		} else
			pm->param.auto_deep_sleep.auto_ds = DEEP_SLEEP_OFF;
	} else {
		if (pmadapter->is_deep_sleep &&
		    pm->param.auto_deep_sleep.auto_ds == DEEP_SLEEP_ON) {
			PRINTM(MMSG, "uAP already in deep sleep mode\n");
			LEAVE();
			return MLAN_STATUS_FAILURE;
		}
		if (((mlan_ds_pm_cfg *) pioctl_req->pbuf)->param.
		    auto_deep_sleep.auto_ds == DEEP_SLEEP_ON) {
			auto_ds.auto_ds = DEEP_SLEEP_ON;
			mode = EN_AUTO_PS;
			PRINTM(MINFO, "Auto Deep Sleep: on\n");
		} else {
			mode = DIS_AUTO_PS;
			auto_ds.auto_ds = DEEP_SLEEP_OFF;
			PRINTM(MINFO, "Auto Deep Sleep: off\n");
		}
		if (((mlan_ds_pm_cfg *) pioctl_req->pbuf)->param.
		    auto_deep_sleep.idletime)
			auto_ds.idletime =
				((mlan_ds_pm_cfg *) pioctl_req->pbuf)->param.
				auto_deep_sleep.idletime;
		else
			auto_ds.idletime = pmadapter->idle_time;
		ret = wlan_prepare_cmd(pmpriv,
				       HostCmd_CMD_802_11_PS_MODE_ENH,
				       (t_u16) mode,
				       BITMAP_AUTO_DS,
				       (t_void *) pioctl_req, &auto_ds);
		if (ret == MLAN_STATUS_SUCCESS)
			ret = MLAN_STATUS_PENDING;
	}
	LEAVE();
	return ret;
}

/**
 *  @brief Callback to finish domain_info handling
 *  Not to be called directly to initiate domain_info setting.
 *
 *  @param pmpriv   A pointer to mlan_private structure (cast from t_void*)
 *
 *  @return     MLAN_STATUS_PENDING --success, otherwise fail
 *  @sa         wlan_uap_domain_info
 */
static mlan_status
wlan_uap_callback_domain_info(IN t_void * priv)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	mlan_private *pmpriv = (mlan_private *) priv;
	wlan_uap_get_info_cb_t *puap_state_chan_cb = &pmpriv->uap_state_chan_cb;
	mlan_ds_11d_cfg *cfg11d;
	t_u8 band;
	pmlan_adapter pmadapter = pmpriv->adapter;
	pmlan_callbacks pcb = &pmadapter->callbacks;

	ENTER();
	/* clear callback now that we're here */
	puap_state_chan_cb->get_chan_callback = MNULL;

	if (!puap_state_chan_cb->pioctl_req_curr) {
		PRINTM(MERROR, "pioctl_req_curr is null\n");
		LEAVE();
		return ret;
	}
	cfg11d = (mlan_ds_11d_cfg *) puap_state_chan_cb->pioctl_req_curr->pbuf;
	band = BAND_B;

	ret = wlan_11d_handle_uap_domain_info(pmpriv, band,
					      cfg11d->param.domain_tlv,
					      puap_state_chan_cb->
					      pioctl_req_curr);
	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;
	else {
		puap_state_chan_cb->pioctl_req_curr->status_code =
			MLAN_STATUS_FAILURE;
		pcb->moal_ioctl_complete(pmadapter->pmoal_handle,
					 puap_state_chan_cb->pioctl_req_curr,
					 MLAN_STATUS_FAILURE);
	}

	puap_state_chan_cb->pioctl_req_curr = MNULL;	/* prevent re-use */
	LEAVE();
	return ret;
}

/**
 *  @brief Set Domain Info for 11D
 *
 *  @param pmadapter    A pointer to mlan_adapter structure
 *  @param pioctl_req   A pointer to ioctl request buffer
 *
 *  @return     MLAN_STATUS_PENDING --success, otherwise fail
 *  @sa         wlan_uap_callback_domain_info
 */
static mlan_status
wlan_uap_domain_info(IN pmlan_adapter pmadapter, IN pmlan_ioctl_req pioctl_req)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_index];

	ENTER();

	if (pioctl_req->buf_len < sizeof(mlan_ds_11d_cfg)) {
		PRINTM(MWARN, "MLAN 11d_cfg IOCTL length is too short.\n");
		pioctl_req->data_read_written = 0;
		pioctl_req->buf_len_needed = sizeof(mlan_ds_11d_cfg);
		LEAVE();
		return MLAN_STATUS_RESOURCE;
	}

	if ((pioctl_req->action == MLAN_ACT_SET) && pmpriv->uap_bss_started) {
		PRINTM(MIOCTL,
		       "Domain_info cannot be changed while UAP bss is started.\n");
		pioctl_req->data_read_written = 0;
		LEAVE();
		return MLAN_STATUS_FAILURE;
	}

	/* store params, issue command to get UAP channel, whose CMD_RESP will
	   callback remainder of domain_info handling */
	pmpriv->uap_state_chan_cb.pioctl_req_curr = pioctl_req;
	pmpriv->uap_state_chan_cb.get_chan_callback =
		wlan_uap_callback_domain_info;

	ret = wlan_uap_get_channel(pmpriv);
	if (ret == MLAN_STATUS_SUCCESS)
		ret = MLAN_STATUS_PENDING;

	LEAVE();
	return ret;
}

/********************************************************
			Global Functions
********************************************************/

/**
 *  @brief Issue CMD to UAP firmware to get current channel
 *
 *  @param pmpriv   A pointer to mlan_private structure
 *
 *  @return         MLAN_STATUS_SUCCESS --success, otherwise fail
 */
mlan_status
wlan_uap_get_channel(IN pmlan_private pmpriv)
{
	MrvlIEtypes_channel_band_t tlv_chan_band;
	mlan_status ret = MLAN_STATUS_SUCCESS;

	ENTER();
	memset(pmpriv->adapter, &tlv_chan_band, 0, sizeof(tlv_chan_band));
	tlv_chan_band.header.type = TLV_TYPE_UAP_CHAN_BAND_CONFIG;
	tlv_chan_band.header.len = sizeof(MrvlIEtypes_channel_band_t)
		- sizeof(MrvlIEtypesHeader_t);

	ret = wlan_prepare_cmd(pmpriv, HOST_CMD_APCMD_SYS_CONFIGURE,
			       HostCmd_ACT_GEN_GET, 0, MNULL, &tlv_chan_band);
	LEAVE();
	return ret;
}

/**
 *  @brief MLAN uap ioctl handler
 *
 *  @param adapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_SUCCESS --success, otherwise fail
 */
mlan_status
wlan_ops_uap_ioctl(t_void * adapter, pmlan_ioctl_req pioctl_req)
{
	pmlan_adapter pmadapter = (pmlan_adapter) adapter;
	mlan_status status = MLAN_STATUS_SUCCESS;
	mlan_ds_bss *bss = MNULL;
	mlan_ds_get_info *pget_info = MNULL;
	mlan_ds_misc_cfg *misc = MNULL;
	mlan_ds_sec_cfg *sec = MNULL;
	mlan_ds_pm_cfg *pm = MNULL;
	mlan_ds_11d_cfg *cfg11d = MNULL;
	mlan_ds_radio_cfg *radiocfg = MNULL;
	mlan_ds_rate *rate = MNULL;
	mlan_ds_reg_mem *reg_mem = MNULL;
	mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_index];

	ENTER();

	switch (pioctl_req->req_id) {
	case MLAN_IOCTL_BSS:
		bss = (mlan_ds_bss *) pioctl_req->pbuf;
		if (bss->sub_command == MLAN_OID_BSS_MAC_ADDR)
			status = wlan_uap_bss_ioctl_mac_address(pmadapter,
								pioctl_req);
		else if (bss->sub_command == MLAN_OID_BSS_STOP)
			status = wlan_uap_bss_ioctl_stop(pmadapter, pioctl_req);
		else if (bss->sub_command == MLAN_OID_BSS_START)
			status = wlan_uap_bss_ioctl_start(pmadapter,
							  pioctl_req);
		else if (bss->sub_command == MLAN_OID_UAP_BSS_CONFIG)
			status = wlan_uap_bss_ioctl_config(pmadapter,
							   pioctl_req);
		else if (bss->sub_command == MLAN_OID_UAP_DEAUTH_STA)
			status = wlan_uap_bss_ioctl_deauth_sta(pmadapter,
							       pioctl_req);
		else if (bss->sub_command == MLAN_OID_UAP_BSS_RESET)
			status = wlan_uap_bss_ioctl_reset(pmadapter,
							  pioctl_req);
#if defined(STA_SUPPORT) && defined(UAP_SUPPORT)
		else if (bss->sub_command == MLAN_OID_BSS_ROLE)
			status = wlan_bss_ioctl_bss_role(pmadapter, pioctl_req);
#endif
#ifdef WIFI_DIRECT_SUPPORT
		else if (bss->sub_command == MLAN_OID_WIFI_DIRECT_MODE)
			status = wlan_bss_ioctl_wifi_direct_mode(pmadapter,
								 pioctl_req);
#endif
		else if (bss->sub_command == MLAN_OID_BSS_REMOVE)
			status = wlan_bss_ioctl_bss_remove(pmadapter,
							   pioctl_req);
		break;
	case MLAN_IOCTL_GET_INFO:
		pget_info = (mlan_ds_get_info *) pioctl_req->pbuf;
		if (pget_info->sub_command == MLAN_OID_GET_VER_EXT)
			status = wlan_get_info_ver_ext(pmadapter, pioctl_req);
		else if (pget_info->sub_command == MLAN_OID_GET_DEBUG_INFO)
			status = wlan_get_info_debug_info(pmadapter,
							  pioctl_req);
		else if (pget_info->sub_command == MLAN_OID_GET_STATS)
			status = wlan_uap_get_stats(pmadapter, pioctl_req);
		else if (pget_info->sub_command == MLAN_OID_UAP_STA_LIST)
			status = wlan_uap_get_sta_list(pmadapter, pioctl_req);
		else if (pget_info->sub_command == MLAN_OID_GET_BSS_INFO)
			status = wlan_uap_get_bss_info(pmadapter, pioctl_req);
		else if (pget_info->sub_command == MLAN_OID_GET_FW_INFO) {
			pioctl_req->data_read_written =
				sizeof(mlan_fw_info) + MLAN_SUB_COMMAND_SIZE;
			memcpy(pmadapter, &pget_info->param.fw_info.mac_addr,
			       pmpriv->curr_addr, MLAN_MAC_ADDR_LENGTH);
			pget_info->param.fw_info.fw_ver =
				pmadapter->fw_release_number;
			pget_info->param.fw_info.fw_bands = pmadapter->fw_bands;
			pget_info->param.fw_info.hw_dev_mcs_support =
				pmadapter->hw_dev_mcs_support;
			pget_info->param.fw_info.hw_dot_11n_dev_cap =
				pmadapter->hw_dot_11n_dev_cap;
			pget_info->param.fw_info.region_code =
				pmadapter->region_code;
		}
		break;
	case MLAN_IOCTL_MISC_CFG:
		misc = (mlan_ds_misc_cfg *) pioctl_req->pbuf;
		if (misc->sub_command == MLAN_OID_MISC_INIT_SHUTDOWN)
			status = wlan_misc_ioctl_init_shutdown(pmadapter,
							       pioctl_req);
		if (misc->sub_command == MLAN_OID_MISC_SOFT_RESET)
			status = wlan_uap_misc_ioctl_soft_reset(pmadapter,
								pioctl_req);
		if (misc->sub_command == MLAN_OID_MISC_HOST_CMD)
			status = wlan_misc_ioctl_host_cmd(pmadapter,
							  pioctl_req);
		if (misc->sub_command == MLAN_OID_MISC_GEN_IE)
			status = wlan_uap_misc_ioctl_gen_ie(pmadapter,
							    pioctl_req);
		if (misc->sub_command == MLAN_OID_MISC_CUSTOM_IE)
			status = wlan_misc_ioctl_custom_ie_list(pmadapter,
								pioctl_req,
								MTRUE);
		if (misc->sub_command == MLAN_OID_MISC_TX_DATAPAUSE)
			status = wlan_uap_misc_ioctl_txdatapause(pmadapter,
								 pioctl_req);
		if (misc->sub_command == MLAN_OID_MISC_RX_MGMT_IND)
			status = wlan_reg_rx_mgmt_ind(pmadapter, pioctl_req);
#ifdef DEBUG_LEVEL1
		if (misc->sub_command == MLAN_OID_MISC_DRVDBG)
			status = wlan_set_drvdbg(pmadapter, pioctl_req);
#endif
		if (misc->sub_command == MLAN_OID_MISC_TXCONTROL)
			status = wlan_misc_ioctl_txcontrol(pmadapter,
							   pioctl_req);
		if (misc->sub_command == MLAN_OID_MISC_MAC_CONTROL)
			status = wlan_misc_ioctl_mac_control(pmadapter,
							     pioctl_req);
#ifdef WIFI_DIRECT_SUPPORT
		if (misc->sub_command == MLAN_OID_MISC_WIFI_DIRECT_CONFIG)
			status = wlan_misc_p2p_config(pmadapter, pioctl_req);
#endif
		break;
	case MLAN_IOCTL_PM_CFG:
		pm = (mlan_ds_pm_cfg *) pioctl_req->pbuf;
		if (pm->sub_command == MLAN_OID_PM_CFG_PS_MODE)
			status = wlan_uap_pm_ioctl_mode(pmadapter, pioctl_req);
		if (pm->sub_command == MLAN_OID_PM_CFG_DEEP_SLEEP)
			status = wlan_uap_pm_ioctl_deepsleep(pmadapter,
							     pioctl_req);
		if (pm->sub_command == MLAN_OID_PM_CFG_HS_CFG)
			status = wlan_pm_ioctl_hscfg(pmadapter, pioctl_req);
		if (pm->sub_command == MLAN_OID_PM_HS_WAKEUP_REASON)
			status = wlan_get_hs_wakeup_reason(pmadapter,
							   pioctl_req);
		if (pm->sub_command == MLAN_OID_PM_INFO)
			status = wlan_get_pm_info(pmadapter, pioctl_req);
		break;
	case MLAN_IOCTL_SEC_CFG:
		sec = (mlan_ds_sec_cfg *) pioctl_req->pbuf;
		if (sec->sub_command == MLAN_OID_SEC_CFG_ENCRYPT_KEY)
			status = wlan_uap_sec_ioctl_set_encrypt_key(pmadapter,
								    pioctl_req);
		if (sec->sub_command == MLAN_OID_SEC_CFG_WAPI_ENABLED)
			status = wlan_uap_sec_ioctl_wapi_enable(pmadapter,
								pioctl_req);
		if (sec->sub_command == MLAN_OID_SEC_CFG_REPORT_MIC_ERR)
			status = wlan_uap_sec_ioctl_report_mic_error(pmadapter,
								     pioctl_req);
		break;
	case MLAN_IOCTL_11N_CFG:
		status = wlan_11n_cfg_ioctl(pmadapter, pioctl_req);
		break;
	case MLAN_IOCTL_11D_CFG:
		cfg11d = (mlan_ds_11d_cfg *) pioctl_req->pbuf;
		if (cfg11d->sub_command == MLAN_OID_11D_DOMAIN_INFO)
			status = wlan_uap_domain_info(pmadapter, pioctl_req);
		break;
	case MLAN_IOCTL_RADIO_CFG:
		radiocfg = (mlan_ds_radio_cfg *) pioctl_req->pbuf;
		if (radiocfg->sub_command == MLAN_OID_RADIO_CTRL)
			status = wlan_radio_ioctl_radio_ctl(pmadapter,
							    pioctl_req);
#ifdef WIFI_DIRECT_SUPPORT
		if (radiocfg->sub_command == MLAN_OID_REMAIN_CHAN_CFG)
			status = wlan_radio_ioctl_remain_chan_cfg(pmadapter,
								  pioctl_req);
#endif
		break;
	case MLAN_IOCTL_RATE:
		rate = (mlan_ds_rate *) pioctl_req->pbuf;
		if (rate->sub_command == MLAN_OID_RATE_CFG)
			status = wlan_rate_ioctl_cfg(pmadapter, pioctl_req);
		else if (rate->sub_command == MLAN_OID_GET_DATA_RATE)
			status = wlan_rate_ioctl_get_data_rate(pmadapter,
							       pioctl_req);
		break;
	case MLAN_IOCTL_REG_MEM:
		reg_mem = (mlan_ds_reg_mem *) pioctl_req->pbuf;
		if (reg_mem->sub_command == MLAN_OID_REG_RW)
			status = wlan_reg_mem_ioctl_reg_rw(pmadapter,
							   pioctl_req);
		else if (reg_mem->sub_command == MLAN_OID_EEPROM_RD)
			status = wlan_reg_mem_ioctl_read_eeprom(pmadapter,
								pioctl_req);
		else if (reg_mem->sub_command == MLAN_OID_MEM_RW)
			status = wlan_reg_mem_ioctl_mem_rw(pmadapter,
							   pioctl_req);
		break;
	case MLAN_IOCTL_WMM_CFG:
		status = wlan_wmm_cfg_ioctl(pmadapter, pioctl_req);
		break;
	default:
		pioctl_req->status_code = MLAN_ERROR_IOCTL_INVALID;
		break;
	}
	LEAVE();
	return status;
}
