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
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <asm/uaccess.h>

/*******************************************************************************
  Local head files
*/
#include "vmeta_sched_priv.h"

/*******************************************************************************
  Macro Defined
  */
#define VMETA_TAG "[vmeta_scheduler]"
#define VMETA_STATUS "status"

#define vmeta_trace(...) \
    if (enable_trace != '0') { \
        printk(KERN_WARNING VMETA_TAG __VA_ARGS__); \
    }

#define vmeta_error(...)   printk(KERN_ERR VMETA_TAG __VA_ARGS__)

static int vmeta_driver_open(struct inode *inode, struct file *filp);
static int vmeta_driver_release(struct inode *inode, struct file *filp);
static long vmeta_driver_ioctl_unlocked(struct file *filp, unsigned int cmd,
                       unsigned long arg);

/*******************************************************************************
  Module Variable
*/
typedef struct {
    struct file         *filp[MAX_SCHED_SLOTS_NUMBER];
    bool                is_waiting[MAX_SCHED_SLOTS_NUMBER];
    wait_queue_head_t   wait_head[MAX_SCHED_SLOTS_NUMBER];

    int             owner;
    unsigned int    user_count;
} vmeta_scheduler;

static vmeta_scheduler *g_sched = NULL;
static struct cdev vmeta_dev;
static struct class *vmeta_dev_class;
static struct proc_dir_entry *vmeta_driver_procdir;
static struct proc_dir_entry *vmeta_driver_state;
static int vmeta_major, vmeta_minor;
static char enable_trace = '0';

static struct file_operations vmeta_ops = {
    .open = vmeta_driver_open,
    .release = vmeta_driver_release,
    .unlocked_ioctl = vmeta_driver_ioctl_unlocked,
    .owner = THIS_MODULE,
};

static DEFINE_SPINLOCK(vmeta_spinlock);

/*******************************************************************************
  Module API
  */
static int read_proc_status(char *page, char **start, off_t offset,
                int count, int *eof, void *data) {
    int len = 0;
    int i = 0;

    spin_lock(&vmeta_spinlock);
    len += sprintf(page + len, "vmeta user count %d, owner %d\n",
            g_sched->user_count, g_sched->owner);
    for (i = 0; i < MAX_SCHED_SLOTS_NUMBER; i++) {
        len += sprintf(page + len, "slot %d filp %p waiting %d\n",
                i, g_sched->filp[i], g_sched->is_waiting[i]);
    }
    spin_unlock(&vmeta_spinlock);
    *eof = 1;
    return ((count < len) ? count : len);
}

ssize_t write_proc_status(struct file *filp, const char __user *buff,
                unsigned long len, void *data) {
    if (copy_from_user(&enable_trace, buff, 1)) {
        return -EFAULT;
    }
    return len;
}

static int get_index_locked(struct file *filp) {
    int i = 0;
    for (i = 0; i < MAX_SCHED_SLOTS_NUMBER; i++) {
        if (filp == g_sched->filp[i])
            return i;
    }
    return -1;
}

static int vmeta_driver_open(struct inode *inode, struct file *filp) {
    int i = 0;

    spin_lock(&vmeta_spinlock);
    if (g_sched->user_count < MAX_SCHED_SLOTS_NUMBER) {
        for (i = 0; i < MAX_SCHED_SLOTS_NUMBER; i++) {
            if (!g_sched->filp[i]) {
                g_sched->filp[i] = filp;
                init_waitqueue_head(&g_sched->wait_head[i]);
                g_sched->user_count++;

                vmeta_trace("%p.%d: slot open\n", filp, i);
                spin_unlock(&vmeta_spinlock);
                return 0;
            }
        }
    }
    spin_unlock(&vmeta_spinlock);
    vmeta_error("run out of vmeta scheduler slots!\n");
    return -1;
}

static int vmeta_driver_release(struct inode *inode, struct file *filp) {
    int index;

    spin_lock(&vmeta_spinlock);
    index = get_index_locked(filp);
    if (index >= 0) {
        g_sched->filp[index] = NULL;
        g_sched->user_count--;

        // All players gone, reset owner.
        if (g_sched->user_count == 0)
            g_sched->owner = -1;

        vmeta_trace("%p.%d: slot release\n", filp, index);
        spin_unlock(&vmeta_spinlock);
        return 0;
    }
    spin_unlock(&vmeta_spinlock);
    vmeta_error("error in release vmeta scheduler slot!\n");
    return -1;
}

static int get_next_player_locked(int index) {
    int next;
    int curr = index;

    do {
        next = (++curr) % MAX_SCHED_SLOTS_NUMBER;
        if ((g_sched->filp[next] != NULL) &&
            (g_sched->is_waiting[next] == true)) {
            return next;
        }
    } while(next != index);

    return index;
}

static long vmeta_driver_ioctl_unlocked(struct file *filp, unsigned int cmd,
                       unsigned long arg) {
    int index, next;
    int res = 0;
    char result;

    spin_lock(&vmeta_spinlock);
    index = get_index_locked(filp);

    switch(cmd) {
        case VMETA_IOCTL_LOCK:
            if (g_sched->owner == -1) {
                // Only one player.
                g_sched->owner = index;
            } else if (g_sched->owner != index) {
                g_sched->is_waiting[index] = true;

                spin_unlock(&vmeta_spinlock);
                interruptible_sleep_on(&g_sched->wait_head[index]);
                spin_lock(&vmeta_spinlock);

                g_sched->is_waiting[index] = false;
            }

            // Only two results for lock, "Approved" or "Canceled".
            if (g_sched->owner == index) {
                vmeta_trace("%p.%d: approved vmeta\n", filp, index);
                result = VMETA_CMD_APPROVED;
            } else {
                vmeta_trace("%p.%d: canceled vmeta\n", filp, index);
                result = VMETA_CMD_CANCELED;
            }
            res = copy_to_user((void __user *)arg,
                    (const void *)&result, VMETA_RESULT_SIZE);
            break;

        case VMETA_IOCTL_UNLOCK:
            if (g_sched->owner != index) {
                vmeta_error("%p: release while don't have ownership\n", filp);
                res = -1;
            } else {
                next = get_next_player_locked(index);
                if (next != index) {
                    g_sched->owner = next;
                    vmeta_trace("%p.%d: release vmeta to %d\n", filp, index, next);
                    wake_up(&g_sched->wait_head[next]);
                }
            }
            break;

        case VMETA_IOCTL_CANCEL:
            if (g_sched->is_waiting[index] == true) {
                g_sched->is_waiting[index] = false;
                wake_up(&g_sched->wait_head[index]);
            } else {
                vmeta_trace("%p.%d: no need to cancel\n", filp, index);
            }
            break;
        case VMETA_IOCTL_WAITINT:
            // TODO: handle vmeta interrupt here.
            break;
    }

    spin_unlock(&vmeta_spinlock);
    return res;
}
/*******************************************************************************
  Module Register API
 */
static int vmeta_driver_setup_cdev(struct cdev *dev, int major, int minor,
                   struct file_operations *fops) {
    cdev_init(dev, fops);
    dev->owner = THIS_MODULE;
    dev->ops = fops;
    return cdev_add(dev, MKDEV(major, minor), 1);
}

int __init vmeta_sched_driver_init(int major, int minor) {
    int res;

    /* Now setup cdevs. */
    vmeta_major = major;
    vmeta_minor = minor;
    res =
        vmeta_driver_setup_cdev(&vmeta_dev, vmeta_major,
                       vmeta_minor, &vmeta_ops);
    if (res) {
        vmeta_error("pe_agent_driver_setup_cdev failed.\n");
        goto err_reg_device;
    }

    vmeta_trace("setup cdevs device minor [%d]\n",
               vmeta_minor);

    g_sched = (vmeta_scheduler *) kmalloc(sizeof(vmeta_scheduler), GFP_KERNEL);
    if (g_sched == NULL) {
        vmeta_error("malloc failed.\n");
        res = -ENODEV;
        goto err_add_device;
    }
    memset(g_sched, 0, sizeof(vmeta_scheduler));
    g_sched->owner = -1;

    /* add vmeta device */
    vmeta_dev_class = class_create(THIS_MODULE, VMETA_SCHED_NAME);
    if (IS_ERR(vmeta_dev_class)) {
        vmeta_error("class_create failed.\n");
        res = -ENODEV;
        goto err_add_device;
    }

    device_create(vmeta_dev_class, NULL,
              MKDEV(vmeta_major, vmeta_minor),
              NULL, VMETA_SCHED_NAME);
    vmeta_trace("create device [%s]\n", VMETA_SCHED_NAME);

    /* create vmeta device proc file */
    vmeta_driver_procdir = proc_mkdir(VMETA_SCHED_NAME, NULL);
    if (vmeta_driver_procdir == NULL) {
        vmeta_error("make proc dir 0 failed.\n");
        res = -ENODEV;
        goto err_add_device;
    }

    vmeta_driver_state = create_proc_entry(VMETA_STATUS, 0644, vmeta_driver_procdir);
    if (vmeta_driver_state == NULL) {
        vmeta_error("make proc dir 1 failed.\n");
        res = -ENODEV;
        remove_proc_entry(VMETA_SCHED_NAME, NULL);
        goto err_add_device;
    }

    vmeta_driver_state->read_proc = read_proc_status;
    vmeta_driver_state->write_proc = write_proc_status;

    vmeta_trace("vmeta_sched_driver_init OK\n");
    return 0;

 err_add_device:

    cdev_del(&vmeta_dev);

 err_reg_device:

    vmeta_trace("vmeta_driver_init failed !!! (%d)\n", res);

    return res;
}

void __exit vmeta_sched_driver_exit(void) {
    if (g_sched) {
        kfree(g_sched);
        g_sched = NULL;
    }
    /* remove vmeta device proc file */
    remove_proc_entry(VMETA_STATUS, vmeta_driver_procdir);
    remove_proc_entry(VMETA_SCHED_NAME, NULL);

    /* delete device */
    device_destroy(vmeta_dev_class,
               MKDEV(vmeta_major, vmeta_minor));
    vmeta_trace("delete device [%s]\n", VMETA_SCHED_NAME);

    class_destroy(vmeta_dev_class);

    /* del cdev */
    cdev_del(&vmeta_dev);

    vmeta_trace("vmeta_driver_exit OK\n");
}
