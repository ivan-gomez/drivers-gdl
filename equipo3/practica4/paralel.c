#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/kernel.h>
#include "config.h"


#define DEVICE_NAME "para"
#define IOC_MAGIC   0x94
#define PARALLEL_PORT_INTERRUPT 7
#define BASEPORT	0x378

#define PARA_IOC_SET_WRITE		_IO(IOC_MAGIC, 0x1)           /* Defines our ioctl call. */
#define PARA_IOC_SET_READ		_IO(IOC_MAGIC, 0x2)           /* Defines our ioctl call. */
#define PARA_IOC_SET_STROBE		_IO(IOC_MAGIC, 0x3)           /* Defines our ioctl call. */
#define PARA_IOC_CLEAR_STROBE	_IO(IOC_MAGIC, 0x4)           /* Defines our ioctl call. */

#define PARALEL_READ 0
#define PARALEL_WRITE 1
DECLARE_WAIT_QUEUE_HEAD(program_queue);

static unsigned major = 0;                                     /* Major number */
static struct cdev my_dev;                                     /* Register with kernel */
static struct class *mydev_class = NULL;                       /* Register with sysfs */
static struct device *mydev_device;                            /* Register with sysfs */
int size, tmp;
char kbuff[128];
long num1;
long num2;
char operand;
long result;
int dataReadyFlag;
int portMode;
int port;
int instances;
uint8_t byte;

#if ENABLE_INTERRUPT
static irqreturn_t isr_code(int irq, void *dummy, struct pt_regs *regs)
{
	byte = inb(0x37A);
	byte = byte & 0xEF; /* hex EF = binary 11101111 */
	outb(byte, 0x37A);
	byte = inb(0x37A);
	byte = byte | 0x10; /* hex 10 = binary 00010000 */
	outb(byte, 0x37A);
	pr_info("MY_IRQ: Interrupt handler executed!\n");
	dataReadyFlag = 1;
	wake_up_interruptible(&program_queue);
	return IRQ_HANDLED;
}
#endif

static ssize_t my_read(struct file *filp, char __user *buf,
					size_t nbuf, loff_t *offs)
{
	#if ENABLE_INTERRUPT
	/* Wait until data ready and port in read mode */
	if (wait_event_interruptible(program_queue, (dataReadyFlag == 1) && (portMode == PARALEL_READ))) {
		pr_info("Error in interruptible wait.\n");
		return -ERESTARTSYS;
	}
	#endif

	/* read port */
	kbuff[0] = inb(0x378);

	/* set flag */
	dataReadyFlag = 0;

    /* transfer data to user space */
	copy_to_user(buf, &kbuff[0], 1);

	/* Manage offset */
	if (*offs == 0) {
		*offs += 1;
		return 1;
	} else {
		return 0;
	}

}

static ssize_t my_write(struct file *filp, const char __user *buf,
						size_t nbuf, loff_t *offs)
{

	/* Copy from user space */
	copy_from_user(kbuff, buf, 1);
	pr_info("%s:%d\n", __func__, __LINE__);
	pr_info("WRITE: %X", kbuff[0]);

	/* Write to the port */
	outb(kbuff[0], 0x378);

	/* Some info for the user */
	pr_info("STATUS: Data written\n");
	sprintf(kbuff, "STATUS: Data written\n");
	size = strlen(kbuff);

	return 1;
}

static int my_open(struct inode *ip, struct file *filp)
{
	int ret;

    /* Keep track of the number of instances */
	instances++;

    #if ENABLE_INTERRUPT
	if (instances == 1) {

		/* Request ISR */

		/* Enable interrupts on pin */

		ret = request_irq(PARALLEL_PORT_INTERRUPT, isr_code,
							IRQF_DISABLED, "parallelport", 0);
		if (ret)	{
			pr_info("parint: error requesting irq 7: returned %d\n", ret);
			pr_info(">cannot register IRQ %d\n", 7);
			return -EIO;
		}
		/*enable_irq(PARALLEL_PORT_INTERRUPT);*/
		outb_p(0x10, BASEPORT + 2);
		pr_info("Interrupt enabled.\n");
	}
	#endif
	/* Log */
	pr_info("%s:%d\n", __func__, __LINE__);
	return 0;
}

static int my_close(struct inode *ip, struct file *filp)
{
	/* Keep track of the number of instances */
	instances--;
	#if ENABLE_INTERRUPT
	if (instances == 0) {
		/* Disable interrupts on pin */
		byte = inb(0x37A);
		byte = byte & 0xEF; /* hex EF = binary 11101111 */
		outb(byte, 0x37A);

		/* Free ISR */
		/*disable_irq(7);*/
		free_irq(7, NULL);
		pr_info("Interrupt disabled.\n");
	}
	#endif

	/* Log */
	pr_info("%s:%d\n", __func__, __LINE__);
	return 0;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	pr_info("IOCTL> ENTERED CMD #%d\n", cmd);
	switch (cmd) {
	case PARA_IOC_SET_WRITE:
		/* Configure as output */
		byte = inb(0x37A);
		byte = byte & 0xDF; /* hex DF = binary 11011111 */
		outb(byte, 0x37A);
		portMode = PARALEL_WRITE;
		pr_info("IOCTL> Changed to write mode\n");
		break;

	case PARA_IOC_SET_READ:
		/* Configure as input */
		byte = inb(0x37A);
		byte = byte | 0x20; /* hex 10 = binary 00100000 */
		outb(byte, 0x37A);
		portMode = PARALEL_READ;
		pr_info("IOCTL> Changed to read mode\n");
		break;

	case PARA_IOC_SET_STROBE:
		/* Set strobe */
		byte = inb(0x37A);
		byte = byte | 0x01; /* hex 10 = binary 00000001 */
		outb(byte, 0x37A);
		pr_info("IOCTL> Strobe set\n");
		break;

	case PARA_IOC_CLEAR_STROBE:
		/* Clear strobe */
		byte = inb(0x37A);
		byte = byte & 0xFE; /* hex EF = binary 11111110 */
		outb(byte, 0x37A);
		pr_info("IOCTL> Strobe cleared\n");
		break;

	default:
		return -ENOTTY;
		break;
	}

	return 0;
}

static const struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.read = my_read,
	.write = my_write,
	.unlocked_ioctl = device_ioctl,
	.open = my_open,
	.release = my_close,
};

static int __init mymodule_init(void)
{
	dev_t dev_id;
	int ret;

	/* Allocate major/minor numbers */
	ret = alloc_chrdev_region(&dev_id, 0, 1, DEVICE_NAME);

	if (ret) {
		pr_info("Error: Failed registering major number\n");
		return -1;
	}

	/* save MAJOR number */
	major = MAJOR(dev_id);

	/* Initialize cdev structure and register it with the kernel*/
	cdev_init(&my_dev, &mydev_fops);
	my_dev.owner = THIS_MODULE;

    /* Register the char device with the kernel */
	ret = cdev_add(&my_dev, dev_id, 1);

	if (ret) {
		pr_info("Error: Failed registering with the kernel\n");
		unregister_chrdev_region(dev_id, 1);
		return -1;
	}

	/* Create a class and register with the sysfs. (Failure is not fatal) */
	mydev_class = class_create(THIS_MODULE, DEVICE_NAME);

	if (IS_ERR(mydev_class)) {
		pr_info("class_create() failed: %ld\n", PTR_ERR(mydev_class));
		mydev_class = NULL;
	} else {
		/* Register device with sysfs (creates device node in /dev) */
		mydev_device = device_create(mydev_class,
						NULL, dev_id, NULL, DEVICE_NAME "0");

		if (IS_ERR(mydev_device)) {
			pr_info("device_create() failed: %ld\n",
					PTR_ERR(mydev_device));
			mydev_device = NULL;
		}
	}

	/* Claim parallel port region */
	port =  check_region(0x378, 3);
	if (port) {
		pr_info("<1>parlelport: cannot reserve 0x378-0x37A\n");
		return -1;
	}
	request_region(0x378, 3, DEVICE_NAME);

	/* Init some variables */
	portMode = PARALEL_WRITE;

	pr_info("Driver \"%s\" loaded...\n", DEVICE_NAME);
	return 0;
}

static void __exit mymodule_exit(void)
{
	/* Free paralel port */
	release_region(0x378, 3);

	/* Deregister services from kernel */
	cdev_del(&my_dev);

	/* Release major/minor numbers */
	unregister_chrdev_region(MKDEV(major, 0), 1);

	/* Undo everything initialized in init function */
	if (mydev_class != NULL) {
		device_destroy(mydev_class, MKDEV(major, 0));
		class_destroy(mydev_class);
	}

	pr_info("Driver \"%s\" unloaded...\n", DEVICE_NAME);
}


module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_AUTHOR("Alfredo/Ignacio<equipo3@itesm.mx>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Paralel port driver");
