#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <asm/uaccess.h>

#define GALOIS_PWMCH_NUM	4

#define ENABLE_REG_OFFSET	0x0
#define CONTROL_REG_OFFSET	0x4
#define DUTY_REG_OFFSET		0x8
#define TERMCNT_REG_OFFSET	0xC

/*
 * ioctl command
 */
#define PWM_IOCTL_READ  0x1234
#define PWM_IOCTL_WRITE 0x4567

typedef struct galois_pwm_rw {
	uint offset;	/* the offset of register in PWM */
	uint data;	/* the value will be read/write from/to register */
} galois_pwm_rw_t;

static int pwmch_major;
static struct class *pwmch_class;
static unsigned long pwmch_base;

static long galois_pwmch_ioctl(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	galois_pwm_rw_t pwm_rw;
	unsigned int pwmch = iminor(file->f_path.dentry->d_inode);

	switch(cmd) {
	case PWM_IOCTL_READ:
		if (copy_from_user(&pwm_rw, (void __user *)arg, sizeof(galois_pwm_rw_t))) {
			printk("PWM: copy_from_user failed\n");
			return -EFAULT;
		}
		if (((pwm_rw.offset & 0x3) != 0) || (pwm_rw.offset > 0xC)) {
			printk("PWM: error register offset 0x%x.\n", pwm_rw.offset);
			return -ENOMEM;
		}
		pwm_rw.data = __raw_readl(pwmch_base + (pwmch*0x10) + pwm_rw.offset);
		if (copy_to_user((void __user *)arg, &pwm_rw, sizeof(galois_pwm_rw_t))) {
			printk("PWM: copy_to_user failed\n");
			return -EFAULT;
		}
		break;
	case PWM_IOCTL_WRITE:
		if (copy_from_user(&pwm_rw, (void __user *)arg, sizeof(galois_pwm_rw_t))) {
			printk("PWM: copy_from_user failed\n");
			return -EFAULT;
		}
		if (((pwm_rw.offset & 0x3) != 0) || (pwm_rw.offset > 0xC)) {
			printk("PWM: error register offset 0x%x.\n", pwm_rw.offset);
			return -ENOMEM;
		}
		__raw_writel(pwm_rw.data, pwmch_base + (pwmch*0x10) + pwm_rw.offset);
		break;
	default:
		printk("PWM: unknown ioctl command.\n");
		return -EINVAL;
	}
	return 0;
}

static struct file_operations galois_pwmch_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= galois_pwmch_ioctl,
};

static struct {
	uint minor;
	char *name;
} pwmchdev_list[] = {
	{0, "pwmch0"},
	{1, "pwmch1"},
	{2, "pwmch2"},
	{3, "pwmch3"},
};

#ifdef CONFIG_PM
struct saved_pwm_status_t {
	u32 enable;
	u32 control;
	u32 duty;
	u32 termcnt;
};

static struct saved_pwm_status_t saved_pwm_status[GALOIS_PWMCH_NUM];

static int pwmch_suspend(struct device *dev)
{
	int i;

	for (i = 0; i < GALOIS_PWMCH_NUM; i++) {
		saved_pwm_status[i].control = __raw_readl(pwmch_base + (i * 0x10) + CONTROL_REG_OFFSET);
		saved_pwm_status[i].duty = __raw_readl(pwmch_base + (i * 0x10) + DUTY_REG_OFFSET);
		saved_pwm_status[i].termcnt = __raw_readl(pwmch_base + (i * 0x10) + TERMCNT_REG_OFFSET);
		saved_pwm_status[i].enable = __raw_readl(pwmch_base + (i * 0x10) + ENABLE_REG_OFFSET);
	}

	return 0;
}

static int pwmch_resume(struct device *dev)
{
	int i;

	for (i = 0; i < GALOIS_PWMCH_NUM; i++) {
		__raw_writel(saved_pwm_status[i].control, pwmch_base + (i * 0x10) + CONTROL_REG_OFFSET);
		__raw_writel(saved_pwm_status[i].duty, pwmch_base + (i * 0x10) + DUTY_REG_OFFSET);
		__raw_writel(saved_pwm_status[i].termcnt, pwmch_base + (i * 0x10) + TERMCNT_REG_OFFSET);
		__raw_writel(saved_pwm_status[i].enable, pwmch_base + (i * 0x10) + ENABLE_REG_OFFSET);
	}

	return 0;
}

static struct dev_pm_ops pwmch_pm_ops = {
	.suspend	= pwmch_suspend,
	.resume		= pwmch_resume,
};
#endif

static int __devinit pwmch_probe(struct platform_device *pdev)
{
	int i;
	struct resource *r;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r)
		return -EINVAL;
	pwmch_base = r->start;

	pwmch_major = register_chrdev(0, "pwmch", &galois_pwmch_fops);
	if (pwmch_major < 0)
		printk("unable to register chrdev for pwmch.\n");

	pwmch_class = class_create(THIS_MODULE, "pwmch");
	for (i = 0; i < ARRAY_SIZE(pwmchdev_list); i++)
		device_create(pwmch_class, NULL,
			MKDEV(pwmch_major, pwmchdev_list[i].minor), NULL, pwmchdev_list[i].name);

	return 0;
}

static int __devexit pwmch_remove(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pwmchdev_list); i++)
		device_destroy(pwmch_class, MKDEV(pwmch_major, pwmchdev_list[i].minor));
	class_destroy(pwmch_class);

	unregister_chrdev(pwmch_major, "pwmch");

	return 0;
}

static struct of_device_id pwmch_match[] = {
	{ .compatible = "mrvl,berlin-pwm", },
	{},
};

static struct platform_driver pwmch_driver = {
	.probe		= pwmch_probe,
	.remove		= __devexit_p(pwmch_remove),
	.driver = {
		.name	= "berlin-pwm",
		.owner	= THIS_MODULE,
		.of_match_table = pwmch_match,
#ifdef CONFIG_PM
		.pm	= &pwmch_pm_ops,
#endif
	},
};

module_platform_driver(pwmch_driver);

MODULE_AUTHOR("Marvell-Galois");
MODULE_DESCRIPTION("Galois PWMCH Driver");
MODULE_LICENSE("GPL");
