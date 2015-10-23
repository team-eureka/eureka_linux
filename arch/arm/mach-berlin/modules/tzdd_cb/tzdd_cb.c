#include <linux/module.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>

#include "teec_cb.h"

MODULE_LICENSE("GPL");

#define KCB_VERSION "TZDD CB Version 0.0.1"
#define QDEPTH 20

/* extract from vmeta_cmd.h */
#define TA_VEMATA_CB_UUID {0x79b77788, 0x9789, 0x4a7a,{'a', 'm', 'p', 'c', 'b', 'a', 'c', 'k'}}
enum{
    VDEC_ENABLE_CB = 1000,
    VDEC_WAIT_CB   = 1001,
    VDEC_DISABLE_CB = 1002,
};
/* extract end */

static const teec_cb_uuid_t _g_cb_uuid = TA_VEMATA_CB_UUID;

int tzddCB_major;
int tzddCB_minor;

typedef struct {
    struct cdev cdev;
} tzddCB_dev_t;

typedef struct _vmeta_cb_arg_t
{
    int32_t nbytes;     /* sizeof(args), max = 4K - sizeof(uint32_t) */
    uint8_t *args;   /* the arguments, sizeof(args) == nbytes */
} vmeta_cb_arg_t;


typedef struct __CC_Q_{
    uint32_t num;
    uint32_t w_index;
    uint32_t r_index;
    vmeta_cb_arg_t* base;
}CB_CC_Q;

typedef struct __tzddCB_private_data_ {
    struct semaphore sem;
    teec_cb_handle_t hcb;
    CB_CC_Q  cbQ;
} tzddCB_private_data;

tzddCB_dev_t *tzddCB_dev;

static struct class* tzddCB_class;

static int CC_Q_Init(CB_CC_Q *q){
    if(q == NULL)
        return -1;

    q->num = QDEPTH;
    q->w_index = 0;
    q->r_index = 0;
    q->base = (vmeta_cb_arg_t*)kmalloc(sizeof(vmeta_cb_arg_t)*QDEPTH,
            GFP_KERNEL|__GFP_NOWARN);
    if(q->base == NULL)
        return -ENOMEM;

    return 0;
}

static int CC_Q_Add(CB_CC_Q *q, vmeta_cb_arg_t* arg){
    int wr_offset;

    if(q == NULL || arg == NULL)
        return -1;

    wr_offset = q->w_index - q->r_index;
    if(wr_offset == -1 || wr_offset == (q->num - 2))
        return -2;

    memcpy((uint8_t*)(q->base+q->w_index), (uint8_t*)arg,
            sizeof(vmeta_cb_arg_t));
    q->w_index++;
    q->w_index %= q->num;

    return 0;
}

static int CC_Q_Read_Try(CB_CC_Q *q, vmeta_cb_arg_t* arg){
    int rd_offset;

    if(q == NULL || arg == NULL)
        return -1;

    rd_offset = q->r_index - q->w_index;
    if(rd_offset != 0){
        memcpy((uint8_t*)arg,(uint8_t*)(q->base +q->r_index), 
                sizeof(vmeta_cb_arg_t));
        return 0;
    }
    return -2;
}

static int CC_Q_ReadFinish(CB_CC_Q *q){
    int rd_offset;

    if(q == NULL)
        return -1;

    rd_offset = q->r_index - q->w_index;
    if (rd_offset != 0){
        q->r_index ++;
        q->r_index %= q->num;
        return 0;
    }
    return -2;
}


static void CC_Q_Destroy(CB_CC_Q *q){
    if(q == NULL)
        return;

    if(q->base){
        vmeta_cb_arg_t arg;

        while(1){
            if(CC_Q_Read_Try(q, &arg))
                break;
            kfree(arg.args);
            if(CC_Q_ReadFinish(q))
                break;
        }
        kfree(q->base);
        q->base = NULL;
    }
}
static teec_cb_stat_t tzddCB_callback(tee_cb_arg_t *arg, teec_cb_cntx_t cntx){
    tzddCB_private_data* pri_data = NULL;

    pri_data = cntx;
    if (NULL == pri_data){
        printk("%s,%d invalid pri data\n", __FUNCTION__, __LINE__);
        return TEEC_ERROR_BAD_PARAMETERS;
    }

    if(arg != NULL){
        vmeta_cb_arg_t p;
        
        p.nbytes = arg->nbytes;
        p.args = (uint8_t*)kmalloc(arg->nbytes, GFP_KERNEL|__GFP_NOWARN);
        if(p.args == NULL)
            return TEEC_ERROR_OUT_OF_MEMORY;

        memcpy(p.args, arg->args, arg->nbytes);
        if(CC_Q_Add(&pri_data->cbQ, &p)){
            kfree(p.args);
            return TEEC_ERROR_GENERIC;
        }
    }
    up(&(pri_data->sem));
    return TEEC_SUCCESS;
}

static int tzddCB_open(struct inode *inode, struct file *filp){
    tzddCB_private_data* pri_data = NULL;

    if (NULL == filp)
        return -EINVAL;

    pri_data = (tzddCB_private_data*)kmalloc(sizeof(tzddCB_private_data),
            GFP_KERNEL|__GFP_NOWARN);

    if (NULL == pri_data)
        return -ENOMEM;

    memset(pri_data, 0, sizeof(tzddCB_private_data));
    sema_init(&(pri_data->sem), 0);

    if(CC_Q_Init(&pri_data->cbQ)){
        kfree(pri_data);
        return -ENOMEM;
    }

    filp->private_data = pri_data;

    printk(KERN_NOTICE"%s\n", __FUNCTION__);
    return 0;
}

static int tzddCB_close(struct inode *inode, struct file *filp){
    tzddCB_private_data* pri_data;

    if (NULL == filp)
        return -EINVAL;
    
    pri_data = (tzddCB_private_data*)filp->private_data;

    if(pri_data != NULL){
        if(pri_data->hcb != NULL){
            teec_unreg_cb(pri_data->hcb);
            pri_data->hcb = NULL;
        }
        CC_Q_Destroy(&pri_data->cbQ);
        kfree(pri_data);
        filp->private_data = NULL;
    }
    return 0;
}


static long tzddCB_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    tzddCB_private_data* pri_data = NULL;
    vmeta_cb_arg_t teecb;

    if (NULL == filp)
        return -EINVAL;

    pri_data = (tzddCB_private_data*)filp->private_data;
    if (NULL == pri_data)
        return -EINVAL;

    switch(cmd){
      case VDEC_ENABLE_CB:
        //register callback
        pri_data->hcb = teec_reg_cb(&_g_cb_uuid,
                tzddCB_callback, (teec_cb_cntx_t)pri_data);
        if (NULL == pri_data->hcb){
            printk(KERN_ERR"%s,%d fail to register callback\n", __FUNCTION__, __LINE__);
            return -100;
        }
        printk(KERN_NOTICE"tzddCB_ioctl VDEC_ENABLE_CB\n");
        break;
      case VDEC_DISABLE_CB:
        teec_unreg_cb(pri_data->hcb);
        pri_data->hcb = NULL;
        printk(KERN_NOTICE"tzddCB_ioctl VDEC_DISABLE_CB\n");
        break;
      case VDEC_WAIT_CB:
        if(down_interruptible(&pri_data->sem))
            return -EAGAIN;

        if(CC_Q_Read_Try(&pri_data->cbQ,&teecb))
            return -EAGAIN;
        if(CC_Q_ReadFinish(&pri_data->cbQ))
            return -EFAULT;
        if (copy_to_user((void*)arg, teecb.args, teecb.nbytes))
            return -EFAULT;
        kfree(teecb.args);
        break;
      default:
        break;
    }
    return 0;
}

static struct file_operations tzddCB_fops = {
    .open = tzddCB_open,
    .release = tzddCB_close,
    .unlocked_ioctl = tzddCB_ioctl,
    .owner = THIS_MODULE,
};

static struct platform_driver tzddCB_drv = {
    .driver = {
        .name = "tzddCB",
        .owner = THIS_MODULE,
    },
};


static int32_t tzddCB_cdev_init(tzddCB_dev_t *dev)
{
    int32_t err = 0, dev_nr;

    err = alloc_chrdev_region(&dev_nr, 0, 1, "tzddCB");
    tzddCB_major = MAJOR(dev_nr);
    tzddCB_minor = MINOR(dev_nr);

    if (err < 0)
        return err;

    printk("%s, %d MAJOR:%d, MINOR:%d\n", __FUNCTION__, __LINE__,
            tzddCB_major, tzddCB_minor);

    cdev_init(&dev->cdev, &tzddCB_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &tzddCB_fops;

    err = cdev_add(&dev->cdev, dev_nr, 1);
    if (err){
        printk("%s, %d fail to add cdev\n", __FUNCTION__, __LINE__);
        goto _error_1;
    }

    err = platform_driver_register(&tzddCB_drv);
    if (err){
        printk("%s, %d fail to platform_driver_register\n", __FUNCTION__, __LINE__);
        goto _error_2;
    }

    tzddCB_class = class_create(THIS_MODULE, "tzddCB");
    if (IS_ERR(tzddCB_class)){
        printk("%s,%d fail to create class\n", __FUNCTION__, __LINE__);
        goto _error_3;
    }

    device_create(tzddCB_class, NULL, MKDEV(tzddCB_major, tzddCB_minor), NULL, "tzddCB");

    return 0;

_error_3:
    platform_driver_unregister(&tzddCB_drv);
_error_2:
    cdev_del(&(dev->cdev));
_error_1:
    unregister_chrdev_region(MKDEV(tzddCB_major, tzddCB_minor), 1);
    return err;
}

static void tzddCB_cdev_cleanup(tzddCB_dev_t *dev)
{
    device_destroy(tzddCB_class, MKDEV(tzddCB_major, tzddCB_minor));
    class_destroy(tzddCB_class);
    platform_driver_unregister(&tzddCB_drv);
    cdev_del(&dev->cdev);
    unregister_chrdev_region(MKDEV(tzddCB_major, tzddCB_minor), 1);
    return;
}


static int32_t __init tzddCB_init(void){
    unsigned int ret = 0;

    tzddCB_dev = kmalloc(sizeof(tzddCB_dev_t), GFP_KERNEL);
    if (NULL == tzddCB_dev)
        return -ENOMEM;

    memset(tzddCB_dev, 0, sizeof(tzddCB_dev_t));

    ret = tzddCB_cdev_init(tzddCB_dev);
    if (ret < 0){
        kfree(tzddCB_dev);
        tzddCB_dev = NULL;
        return ret;
    }
    printk("%s\n", KCB_VERSION);
    return 0;
}

static void __exit tzddCB_cleanup(void){
    if (NULL == tzddCB_dev)
        return;

    tzddCB_cdev_cleanup(tzddCB_dev);
    kfree(tzddCB_dev);
    tzddCB_dev = NULL;
}

module_init(tzddCB_init);
module_exit(tzddCB_cleanup);
