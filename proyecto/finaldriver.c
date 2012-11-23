#include <linux/module.h>	/* Dynamic loading of modules into the kernel */
#include <linux/fs.h>		/* file defs of important data structures */
#include <linux/cdev.h>		/* char devices structure and aux functions */
#include <linux/device.h>	/* generic, centralized driver model */
#include <linux/uaccess.h>	/* for accessing user-space */
#include <linux/slab.h>		/* kmalloc */
#include <linux/ioport.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/kfifo.h>
#include <linux/mutex.h>
#include <linux/param.h>
#include <linux/delay.h>


#define KBUFF_MAX_SIZE	5	/* size of the kernel side buffer */
#define DEVICE_NAME	"final"

#define DATA_REGISTER		0x378
#define STATUS_REGISTER		0x379
#define CONTROL_REGISTER	0x37A

#define STROBE_PIN	0x01
#define INTERRUPT_PIN	0x10
#define DIRECTION_PIN	0x20

#define FINAL_IOC_MAGIC		'R'
#define FINAL_IOC_SET_INP	_IOW(FINAL_IOC_MAGIC, 0xE0, int)
#define FINAL_IOC_SET_OUT	_IOW(FINAL_IOC_MAGIC, 0xE1, int)
#define FINAL_IOC_STROBE_HL	_IOW(FINAL_IOC_MAGIC, 0xE2, int)

static unsigned major;				/* major number */
static struct cdev final;			/* to register with kernel */
static struct class *final_class;		/* to register with sysfs */
static struct device *final_device;		/* to register with sysfs */

static char *kbuffer;				/* kernel side buffer */
static unsigned long kbuffer_size;		/* kernel buffer length */

static int x, y;

static DECLARE_WAIT_QUEUE_HEAD(Wait);
static int flag;
static int apps_open;
static int start_threads;

static DEFINE_MUTEX(my_mutex);
static DECLARE_WAIT_QUEUE_HEAD(Wait1);
static DECLARE_WAIT_QUEUE_HEAD(Wait2);
static int flag1;
static int flag2;
struct task_struct *task1;
struct task_struct *task2;

struct resource *port_req;

long cos(long xi)
{
	return 100-xi*xi/2+xi*xi*xi*xi/24;
}
long sin(long xi)
{
	return xi-xi*xi*xi/6+xi*xi*xi*xi*xi/120;
}

irqreturn_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	flag = 1;
	return IRQ_HANDLED;
}

void set_port(char *data)
{
	pr_info("Writing data: 0x%x...", *data);

	/* Write to data register */
	outb(*data, DATA_REGISTER);

	kbuffer_size = 0;
}

void get_port(char *data)
{
	pr_info("Obtaining data: 0x%x...", *data);
	/* Read from data register */
	*data = inb(DATA_REGISTER);
	kbuffer_size = 3;
	pr_info("Obtained data: 0x%x", *data);
}

int thread_delta1(void *data_arg)
{
	char t;

	for (t = 0; t < 60; t++) {
		pr_info("%s: %d\n", __func__, __LINE__);
		if (mutex_lock_interruptible(&my_mutex))
			return -EINTR;
		x += 3;
		schedule();
		y -= 3;
		mutex_unlock(&my_mutex);
		/* Wait for interrupt */
		flag1 = 0;
		wait_event_interruptible(Wait1, flag1 != 0);
	}
	for (t = 0; t < 60; t++) {
		pr_info("%s: %d\n", __func__, __LINE__);
		if (mutex_lock_interruptible(&my_mutex))
			return -EINTR;
		x += 3;
		schedule();
		y += 3;
		mutex_unlock(&my_mutex);
		/* Wait for interrupt */
		flag1 = 0;
		wait_event_interruptible(Wait1, flag1 != 0);
	}
	for (t = 0; t < 60; t++) {
		pr_info("%s: %d\n", __func__, __LINE__);
		if (mutex_lock_interruptible(&my_mutex))
			return -EINTR;
		x -= 3;
		schedule();
		y += 3;
		mutex_unlock(&my_mutex);
		/* Wait for interrupt */
		flag1 = 0;
		wait_event_interruptible(Wait1, flag1 != 0);
	}
	for (t = 0; t < 60; t++) {
		pr_info("%s: %d\n", __func__, __LINE__);
		if (mutex_lock_interruptible(&my_mutex))
			return -EINTR;
		x -= 3;
		schedule();
		y -= 3;
		mutex_unlock(&my_mutex);
		/* Wait for interrupt */
		flag1 = 0;
		wait_event_interruptible(Wait1, flag1 != 0);
	}

	start_threads--;
	pr_info("End thread delta 1\n");
	do_exit(0);
}

int thread_delta2(void *data_arg)
{
	char t;

	for (t = 0; t < 60; t++) {
		pr_info("%s: %d\n", __func__, __LINE__);
		if (mutex_lock_interruptible(&my_mutex))
			return -EINTR;
		x -= 3;
		schedule();
		y -= 3;
		mutex_unlock(&my_mutex);
		/* Wait for interrupt */
		flag2 = 0;
		wait_event_interruptible(Wait2, flag2 != 0);
	}
	for (t = 0; t < 60; t++) {
		pr_info("%s: %d\n", __func__, __LINE__);
		if (mutex_lock_interruptible(&my_mutex))
			return -EINTR;
		x += 3;
		schedule();
		y -= 3;
		mutex_unlock(&my_mutex);
		/* Wait for interrupt */
		flag2 = 0;
		wait_event_interruptible(Wait2, flag2 != 0);
	}
	for (t = 0; t < 60; t++) {
		pr_info("%s: %d\n", __func__, __LINE__);
		if (mutex_lock_interruptible(&my_mutex))
			return -EINTR;
		x += 3;
		schedule();
		y += 3;
		mutex_unlock(&my_mutex);
		/* Wait for interrupt */
		flag2 = 0;
		wait_event_interruptible(Wait2, flag2 != 0);
	}
	for (t = 0; t < 60; t++) {
		pr_info("%s: %d\n", __func__, __LINE__);
		if (mutex_lock_interruptible(&my_mutex))
			return -EINTR;
		x -= 3;
		schedule();
		y += 3;
		mutex_unlock(&my_mutex);
		/* Wait for interrupt */
		flag2 = 0;
		wait_event_interruptible(Wait2, flag2 != 0);
	}

	start_threads--;
	pr_info("End thread delta 2\n");
	do_exit(0);
}

/*
 * final_read()
 * This function implements the read function in the file operation structure.
 */
static ssize_t final_read(struct file *filp, char __user *buffer,
		size_t nbuf, loff_t *offset)
{
	int ret = 0;
	pr_info("Driver's read function was called");

	/* Supposing there is three bytes ready to read */
	kbuffer_size = 3;
	*offset = 0;

	pr_info("[%d] %s\n", __LINE__, __func__);
	pr_info("[%d] nbuf = %d, offset = %d",
		__LINE__, (int)nbuf, (int)*offset);
	/* If offset is greater than kbuffer, there is nothing to copy */
	if (*offset > kbuffer_size) {
		pr_info("[%d] Offset greater than kbuffer size: %d >= %ld",
			__LINE__, (int)*offset, kbuffer_size);
		goto out;
	}

	/* Check for maximum size of kernel buffer */
	if ((nbuf + *offset) > kbuffer_size) {
		pr_info("[%d] Answering less data: %d instead of %d",
		__LINE__, (int)(kbuffer_size - (int)*offset), (int)nbuf);
		nbuf = kbuffer_size - *offset;
		if (nbuf == 0)
			goto out;
	}

	if (start_threads == 0) {
		/* Reading port */
		get_port(kbuffer);

		kbuffer[1] = kbuffer[0] & 0xF0;
		kbuffer[0] <<= 4;

		kbuffer[0] /= 16;
		kbuffer[1] /= 16;


		if (flag) {
			flag = 0;
			kbuffer[2] = 1;
		} else
			kbuffer[2] = 0;

	} else {
		kbuffer[0] = 0;
		kbuffer[1] = 0;
		flag1 = 1;
		flag2 = 1;
		wake_up_interruptible(&Wait1);
		wake_up_interruptible(&Wait2);
		kbuffer[0] = x;
		kbuffer[1] = y;
		x = 0;
		y = 0;
		kbuffer[2] = 2;
	}


	/* fill the buffer, return the buffer size */
	pr_info("[%d] Copying Buffer from kernel to user space...\n",
		__LINE__);
	ret = copy_to_user(buffer, kbuffer + (*offset), nbuf);
	if (ret) {
		pr_info("copy_to_user failed: 0x%x", ret);
		ret = -EFAULT;
		goto out;
	}

	*offset += nbuf;
	ret = nbuf;

out:
	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf,
		(int)*offset);
	return ret;
}

/*
 * final_write()
 * This function implements the write function in the file operation structure.
 */
static ssize_t final_write(struct file *filp, const char __user *buffer,
						size_t nbuf, loff_t *offset)
{
	int ret = 0;

	pr_info("Driver's write function was called");
	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf,
		(int)*offset);

	if (*offset >= KBUFF_MAX_SIZE) {
		pr_info("[%d] Offset greater than kbuffer size: %d >= %ld",
			__LINE__, (int)*offset, kbuffer_size);
		goto out;
	}

	/* Check for maximum size of kernel buffer */
	if ((nbuf + *offset) > KBUFF_MAX_SIZE)
		nbuf = KBUFF_MAX_SIZE - *offset;

	/* fill the buffer, return the buffer size */
	pr_info("[%d] Copying Buffer from user to kernel space\n",
		__LINE__);
	ret = copy_from_user(kbuffer + (*offset), buffer, nbuf);
	if (ret) {
		pr_info("copy_from_user failed: 0x%x", ret);
		ret = -EFAULT;
		goto out;
	}

	/* Writing port */
	set_port(kbuffer);
	/* For now, it is enough to say 1 byte was written */
	kbuffer_size = 1;

out:
	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf,
		(int)*offset);
	return nbuf;
}

/* my_ioctl from f_ops */
static long final_ioctl(struct file *file,
		 unsigned int ioctl_num,
		 unsigned long ulparam)
{
	char ctl;

	pr_info("Drivers ioctl function was called");

	switch (ioctl_num) {
	case FINAL_IOC_SET_INP:
		/* Set port as input */
		ctl = inb(CONTROL_REGISTER);
		outb(ctl | DIRECTION_PIN, CONTROL_REGISTER);
		break;
	case FINAL_IOC_SET_OUT:
		/* Set port as output */
		ctl = inb(CONTROL_REGISTER);
		outb(ctl & (0xFF - DIRECTION_PIN), CONTROL_REGISTER);
		break;
	case FINAL_IOC_STROBE_HL:
		/* Set STROBE to 1 */
		ctl = inb(CONTROL_REGISTER);
		outb(ctl & (0xFF - STROBE_PIN), CONTROL_REGISTER);
	}
	return 0;
}

/*
 * Function called when the user side application opens a handle to final
 * driver by calling the open() function.
 */

static int final_open(struct inode *ip, struct file *filp)
{
	int int_req;
	char ctl;

	pr_info("Driver's open function was called\n");

	if (apps_open == 0) {
		int_req = request_irq(7, (irq_handler_t)irq_handler,
				IRQF_DISABLED, "final_int", NULL);
		if (int_req > 0) {
			pr_info("The interrupt request wasn't completed");
			return -EAGAIN;
		}

		/* Enable interrupt */
		ctl = inb(CONTROL_REGISTER);
		outb(ctl | INTERRUPT_PIN, CONTROL_REGISTER);

		kbuffer = kmalloc(KBUFF_MAX_SIZE, GFP_KERNEL);
		if (kbuffer == NULL)
			return -ENOMEM;
	}
	apps_open++;
	return 0;
}

/*
 * Function called when the user side application closes a handle to final
 * driver by calling the close() function.
 */

static int final_close(struct inode *ip, struct file *filp)
{
	pr_info("Driver's close function was called\n");
	if (apps_open == 1) {
		kfree(kbuffer);
		free_irq(7, NULL);
	}
	apps_open--;
	return 0;
}


/*
 * File operation structure
 */
static const struct file_operations final_fops = {
	.open = final_open,
	.release = final_close,
	.owner = THIS_MODULE,
	.read = final_read,
	.write = final_write,
	.unlocked_ioctl = final_ioctl,
};

/*
 * init final_init()
 * Module init function. It is called when insmod is executed.
 */
static int __init final_init(void)
{
	int ret;
	dev_t devid;

	/* Dynamic allocation of MAJOR and MINOR numbers */
	ret = alloc_chrdev_region(&devid, 0, 1, DEVICE_NAME);

	if (ret) {
		pr_info("Error: Failed registering major number\n");
		return ret;
	}

	/* save MAJOR number */
	major = MAJOR(devid);

	/* Initalize the cdev structure */
	cdev_init(&final, &final_fops);

	/* Register the char device with the kernel */
	ret = cdev_add(&final, devid, 1);
	if (ret) {
		pr_info("Error: Failed registering with the kernel\n");
		unregister_chrdev_region(devid, 1);
		return ret;
	}

	/* Create a class and register with the sysfs (Failure is not fatal) */
	final_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(final_class)) {
		pr_info("class_create() failed: %ld\n",
			PTR_ERR(final_class));
		final_class = NULL;
	} else {
		/* Register device with sysfs (creates device node in /dev) */
		final_device = device_create(final_class, NULL, devid,
			NULL, DEVICE_NAME"0");
		if (IS_ERR(final_device)) {
			pr_info("device_create() failed: %ld\n",
				PTR_ERR(final_device));
			final_device = NULL;
		}
	}

	/* Register port */
	port_req = request_region(DATA_REGISTER, 3, "final");
	if (port_req == NULL) {
		pr_info("Parallel port not available");
		return -EBUSY;
	}

	/* Number of apps calling the driver */
	apps_open = 0;

	x = 0;
	y = 0;

	/* Threads */
	start_threads = 2;

	task1 = kthread_create(thread_delta1, NULL, "final1");
	task2 = kthread_create(thread_delta2, NULL, "final2");

	wake_up_process(task1);
	wake_up_process(task2);

	pr_info("Driver \"%s\" loaded...\n", DEVICE_NAME);
	return 0;
}

/*
 * init final_init()
 * Module exit function. It is called when rmmod is executed.
 */
static void __exit final_exit(void)
{
	/* Deregister char device from kernel */
	cdev_del(&final);

	/* Release MAJOR and MINOR numbers */
	unregister_chrdev_region(MKDEV(major, 0), 1);

	/* Deregister device from sysfs */
	if (final_class != NULL) {
		device_destroy(final_class, MKDEV(major, 0));
		class_destroy(final_class);
	}

	if (port_req != NULL)
		release_region(DATA_REGISTER, 3);

	pr_info("Driver \"%s\" unloaded...\n", DEVICE_NAME);
}

module_init(final_init);
module_exit(final_exit);

MODULE_AUTHOR("Equipo 4");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Proyecto Final");
