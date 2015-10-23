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
#ifndef _VMETA_SCHED_PRIV_H
#define _VMETA_SCHED_PRIV_H

#define MAX_SCHED_SLOTS_NUMBER 21

#define VMETA_CMD_APPROVED 'A'
#define VMETA_CMD_CANCELED 'C'
#define VMETA_RESULT_SIZE 1

#define VMETA_IOCTL_LOCK             (0xbeef7001)
#define VMETA_IOCTL_UNLOCK           (0xbeef7002)
#define VMETA_IOCTL_CANCEL           (0xbeef7003)
#define VMETA_IOCTL_WAITINT          (0xbeef7004)

#define VMETA_SCHED_NAME "vmeta_sched"
#define VMETA_SCHED_PATH_NAME ("/dev/"VMETA_SCHED_NAME)

#endif				/* _VMETA_SCHED_PRIV_H */
