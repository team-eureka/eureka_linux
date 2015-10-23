/*
 * Copyright (c) [2009-2013] Marvell International Ltd. and its affiliates.
 * All rights reserved.
 * This software file (the "File") is owned and distributed by Marvell
 * International Ltd. and/or its affiliates ("Marvell") under the following
 * licensing terms.
 * If you received this File from Marvell, you may opt to use, redistribute
 * and/or modify this File in accordance with the terms and conditions of
 * the General Public License Version 2, June 1991 (the "GPL License"), a
 * copy of which is available along with the File in the license.txt file
 * or by writing to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307 or on the worldwide web at
 * http://www.gnu.org/licenses/gpl.txt. THE FILE IS DISTRIBUTED AS-IS,
 * WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
 * DISCLAIMED. The GPL License provides additional details about this
 * warranty disclaimer.
 */

#include "tzdd_internal.h"
#include "tzdd_pt_core.h"

#define CALL_IPI            (0xF0000000)

static uint32_t _g_tzdd_send_num;
static bool _g_pt_thread_stop_flag;
#ifdef __ENABLE_PM_
static struct pm_qos_request _g_tzdd_lpm_cons;
#endif

#ifdef TEE_DEBUG_ENALBE_PROC_FS_LOG
uint32_t g_msg_sent;
uint32_t g_msg_recv;
uint32_t g_msg_fake;
uint32_t g_msg_ignd;
uint32_t g_pre_ipi_num;
uint32_t g_pst_ipi_num;
uint32_t g_pre_dmy_num;
uint32_t g_pst_dmy_num;
uint32_t g_wait_for;
uint32_t g_wait_for_idle = 0;
uint32_t g_wait_for_command = 0;
#endif /* TEE_DEBUG_ENALBE_PROC_FS_LOG */

#ifdef __ENABLE_PM_
osa_sem_t _g_pm_lock;
#endif
extern bool tee_cm_tw_is_idle(void);

static int tzdd_proxy_thread(void *data)
{
	tzdd_dev_t *dev;
	tee_msg_head_t *tee_msg_send_head = NULL;

	//uint32_t nice;

	uint32_t res_size = 0;
	uint32_t msg_flag;

	uint32_t ret;
	bool list_ret;
	bool is_lpm_blocked = false;

	struct sched_param param;

	set_cpus_allowed_ptr(current, cpumask_of(0));
	TZDD_DBG("tzdd_proxy_thread on cpu %d\n", smp_processor_id());

	param.sched_priority = MAX_RT_PRIO - 1;

	sched_setscheduler(current, SCHED_FIFO, &param);
#if 0
	nice = task_nice(current);
	TZDD_DBG("tzdd_proxy_thread: nice = %d\n", nice);
#endif

	dev = (tzdd_dev_t *) data;

	while (1) {
		while (tzdd_get_first_req(&tee_msg_send_head)) {
			TZDD_DBG
			    ("%s: tee_msg_send_head (0x%08x) =\
			    {magic (%c%c%c%c),\
			    size (0x%08x)},\
			    cmd (0x%x)\n",\
			     __func__, (uint32_t) tee_msg_send_head,
			     tee_msg_send_head->magic[0],
			     tee_msg_send_head->magic[1],
			     tee_msg_send_head->magic[2],
			     tee_msg_send_head->magic[3],
			     tee_msg_send_head->msg_sz,
			     *((uint32_t *) (tee_msg_send_head) + 8));

			list_ret =
			    tzdd_del_node_from_req_list(&
							(tee_msg_send_head->
							 node));
			OSA_ASSERT(list_ret);

			tzdd_pt_send(tee_msg_send_head);

			if (false == is_lpm_blocked) {
				/* block LPM D1P and deeper than D1P */
#ifdef __ENABLE_PM_
				osa_wait_for_sem(_g_pm_lock, INFINITE);
				pm_qos_update_request(&_g_tzdd_lpm_cons,
					PM_QOS_CPUIDLE_BLOCK_AXI_VALUE);
#endif
				is_lpm_blocked = true;
			}
			tee_add_time_record_point("npct");
			/* Call IPI to TW */
#ifdef TEE_DEBUG_ENALBE_PROC_FS_LOG
			g_pre_ipi_num++;
#endif
			tee_cm_smi(CALL_IPI);
#ifdef TEE_DEBUG_ENALBE_PROC_FS_LOG
			g_pst_ipi_num++;
#endif
			tee_add_time_record_point("npbt");
			_g_tzdd_send_num++;
#ifdef TEE_DEBUG_ENALBE_PROC_FS_LOG
			g_msg_sent++;
#endif
		}

		res_size = tee_cm_get_data_size();

		while (res_size) {

			msg_flag = tzdd_pt_recv();
			if (msg_flag == TEE_MSG_IGNORE_COUNTER) {
				/* do nothing */
#ifdef TEE_DEBUG_ENALBE_PROC_FS_LOG
				g_msg_ignd++;
#endif
			} else {	/* TEE_MSG_FAKE && TEE_MSG_NORMAL */
#ifdef TEE_DEBUG_ENALBE_PROC_FS_LOG
				if (TEE_MSG_NORMAL == msg_flag)
					g_msg_recv++;
				else if (TEE_MSG_FAKE == msg_flag)
					g_msg_fake++;
				else
					OSA_ASSERT(0);
#endif
				_g_tzdd_send_num--;
			}

			res_size = tee_cm_get_data_size();
		}
		tee_add_time_record_point("nprm");
		if (_g_tzdd_send_num) {
#ifdef TEE_DEBUG_ENALBE_PROC_FS_LOG
			g_pre_dmy_num++;
#endif
			if (tee_cm_tw_is_idle()) {
				g_wait_for = 1;
				g_wait_for_idle++;
				ret = osa_wait_for_sem(dev->pt_sem, 5);
				g_wait_for = 0;
				if (ret != OSA_OK) {
					tee_cm_smi(0);
				}
			} else {
				tee_cm_smi(0);
			}
#ifdef TEE_DEBUG_ENALBE_PROC_FS_LOG
			g_pst_dmy_num++;
#endif
		} else {
			/* release D1P LPM constraint */
			if (true == is_lpm_blocked) {
#ifdef __ENABLE_PM_
				pm_qos_update_request(&_g_tzdd_lpm_cons,
					PM_QOS_CPUIDLE_BLOCK_DEFAULT_VALUE);
				osa_release_sem(_g_pm_lock);
#endif
				is_lpm_blocked = false;
			}

			tee_add_time_record_point("npwa");
			g_wait_for = 2;
			g_wait_for_command++;
			ret = osa_wait_for_sem(dev->pt_sem, INFINITE);
			g_wait_for = 0;
			OSA_ASSERT(!ret);

			if (true == _g_pt_thread_stop_flag) {
				TZDD_DBG("tzdd_proxy_thread(),P-break\n");
				break;
			}
			tee_add_time_record_point("npet");
		}
	}

	TZDD_DBG("proxy thread exit.\n");
	return 0;

}

//static irqreturn_t _tw_exit_idle(int irq, void *dev)
uint32_t tee_cm_get_wakeup_count(void);
static void _tw_exit_idle(void *data)
{
	static uint32_t ipi_count = 0;
	if (ipi_count != tee_cm_get_wakeup_count()) {
	   ipi_count++;
	   osa_release_sem(tzdd_dev->pt_sem);
	}
}
extern void register_user_ipi(void (*handler)(void *), void *data);
int tzdd_proxy_thread_init(tzdd_dev_t *dev)
{
#ifdef __ENABLE_PM_
	/* initialize the qos list at the first time */
	_g_tzdd_lpm_cons.name = "TZDD";
	pm_qos_add_request(&_g_tzdd_lpm_cons,
			   PM_QOS_CPUIDLE_BLOCK,
			   PM_QOS_CPUIDLE_BLOCK_DEFAULT_VALUE);
	_g_pm_lock = osa_create_sem(1);
#endif
#if 0
	ret = request_irq(0xF, _tw_exit_idle, 0, "tw_exit", NULL);
	if (ret) {
		TZDD_DBG("%s: failed to register irq. ret is %d\n", __func__, ret);
		return ret;
	}
#endif
	register_user_ipi(_tw_exit_idle, NULL);

	dev->pt_sem = osa_create_sem(0);

	dev->proxy_thd = kthread_run(tzdd_proxy_thread, dev, "tzdd");

	if (!dev->proxy_thd) {
		TZDD_DBG("%s: failed to create proxy thread.\n", __func__);
		return -1;
	}

	return 0;
}

void tzdd_proxy_thread_cleanup(tzdd_dev_t *dev)
{
	_g_pt_thread_stop_flag = true;
	osa_release_sem(dev->pt_sem);
	osa_destroy_sem(dev->pt_sem);

#ifdef __ENABLE_PM_
	osa_destroy_sem(_g_pm_lock);

	pm_qos_remove_request(&_g_tzdd_lpm_cons);
#endif

	return;
}
