#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/kernel.h>
#include "config.h"

#define DEVICE_NAME "blaster"
#define PARALLEL_PORT_INTERRUPT	7
#define BASEPORT				0x378
#define DATAPORT				0x378
#define STATUSPORT				0x379
#define CONTROLPORT				0x37A
#define BUFFER_SIZE				0x400

#define DATA_NEEDED_FLAG	(1<<5)

#define STATUS7			(1<<7);
#define STATUS6			(1<<6);
#define STATUS5			(1<<5);
#define STATUS4			(1<<4);
#define STATUS3			(1<<3);

#define CONTROL0		(1<<0);
#define CONTROL1		(1<<1);
#define CONTROL2		(1<<2);
#define CONTROL3		(1<<3);

DECLARE_WAIT_QUEUE_HEAD(program_queue);
DEFINE_MUTEX(lock1);

static unsigned major;					/* Major number */
static struct cdev my_dev;				/* Register with kernel */
static struct class *mydev_class;		/* Register with sysfs */
static struct device *mydev_device;		/* Register with sysfs */

char kbuff[BUFFER_SIZE];
int size, tmp;
int ack_flag;
int port;
int instances;
uint8_t byte;
uint8_t	blaster_status;

static irqreturn_t isr_code(int irq, void *dummy, struct pt_regs *regs)
{
	/* Post kernel information */
	pr_info("MY_IRQ: Interrupt handler executed!\n");

	/* Read status */
	blaster_status = inb(STATUSPORT) & (0xF8);

	/* Acknowledge the interrupt */
	byte = inb(0x37A);
	byte = byte & 0xEF; /* hex EF = binary 11101111 */
	outb(byte, 0x37A);
	byte = inb(0x37A);
	byte = byte | 0x10; /* hex 10 = binary 00010000 */
	outb(byte, 0x37A);

	/* Set flags and wake up events */
	ack_flag = 1;
	wake_up_interruptible(&program_queue);
	return IRQ_HANDLED;
}

void set_write(void)
{
	/* Configure as output */
	byte = inb(0x37A);
	byte = byte & 0xDF; /* hex DF = binary 11011111 */
	outb(byte, 0x37A);
}

void set_read(void)
{
	/* Configure as input */
	byte = inb(0x37A);
	byte = byte | 0x20; /* hex 10 = binary 00100000 */
	outb(byte, 0x37A);
}

void clear_strobe(void)
{
	/* Clear strobe */
	byte = inb(0x37A);
	byte = byte & 0xFE; /* hex EF = binary 11111110 */
	outb(byte, 0x37A);
}

void set_strobe(void)
{
	/* Set strobe */
	byte = inb(0x37A);
	byte = byte | 0x01; /* hex 10 = binary 00000001 */
	outb(byte, 0x37A);
}

void tx_blaster(uint8_t cmd, uint8_t data)
{
	mutex_lock(&lock1);
	cmd &= 0x0F;
	byte = inb(CONTROLPORT) & 0xF0;
	outb(byte | cmd, CONTROLPORT);
	outb(data, DATAPORT);
	set_strobe();
	udelay(1);
	clear_strobe();
	if (wait_event_interruptible(program_queue, (ack_flag == 1))) {
		pr_info("Error waiting for acknowledge\n");
		return;
	}
	mutex_unlock(&lock1);
}

static ssize_t my_read(struct file *filp, char __user *buf,
					size_t nbuf, loff_t *offs)
{

	/* read port */
	kbuff[0] = inb(0x378);

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
	uint16_t i;

	/* Verify size */
	if (nbuf > BUFFER_SIZE)
		return -1;

	/* Copy from user space */
	copy_from_user(kbuff, buf, nbuf);
	pr_info("%s:%d\n", __func__, __LINE__);
	pr_info("WRITE: %X", kbuff[0]);

	for (i = 0; i < nbuf; i++) {
		if (wait_event_interruptible(program_queue, (blaster_status &= DATA_NEEDED_FLAG))) {
			pr_info("Error in interruptible wait.\n");
			return -ERESTARTSYS;
		}
		/* Write to the port */
		tx_blaster(CMD_NEW_SAMPLE, kbuff[i]);
	}
	/* Some info for the user */
	pr_info("STATUS: Data written\n");
	sprintf(kbuff, "STATUS: Data written\n");
	size = strlen(kbuff);
	*offs += nbuf;
	return nbuf;
}

static int my_open(struct inode *ip, struct file *filp)
{
	int ret;

    /* Keep track of the number of instances */
	instances++;

	if (instances == 1) {

		/* Request ISR */
		ret = request_irq(PARALLEL_PORT_INTERRUPT, isr_code,
			IRQF_DISABLED, "parallelport", 0);
		if (ret)	{
			pr_info("parint: error requesting irq 7: returned %d\n",
			ret);
			pr_info(">cannot register IRQ %d\n", 7);
			return -EIO;
		}

		/* Enable interrupts on pin */
		outb_p(0x10, BASEPORT + 2);
		pr_info("Interrupt enabled.\n");

		/* Configure sample rate */
		tx_blaster(CMD_SET_RATE, 0x02);
	}

	/* Log */
	pr_info("%s:%d\n", __func__, __LINE__);
	return 0;
}

static int my_close(struct inode *ip, struct file *filp)
{
	/* Keep track of the number of instances */
	instances--;
	if (instances == 0) {
		/* Disable interrupts on pin */
		byte = inb(0x37A);
		byte = byte & 0xEF; /* hex EF = binary 11101111 */
		outb(byte, 0x37A);

		/* Free ISR */
		free_irq(7, NULL);
		pr_info("Interrupt disabled.\n");
	}

	/* Log */
	pr_info("%s:%d\n", __func__, __LINE__);
	return 0;
}




static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	pr_info("IOCTL> ENTERED CMD #%d\n", cmd);
	switch (cmd) {
	case BLAST_IOC_SET_VOLUME:
		/* Set volume */
		tx_blaster(CMD_SET_VOLUME, arg);
		pr_info("IOCTL> Volume changed\n");
		break;

	case BLAST_IOC_SET_SAMPLE_RATE:
		/* Set sample rate */
		tx_blaster(CMD_SET_RATE, arg);
		pr_info("IOCTL> Sample rate set\n");
		break;

	case BLAST_IOC_SET_NUMERATOR:
		/* Set numerator */
		tx_blaster(CMD_NUMERATOR, arg);
		pr_info("IOCTL> Numerator\n");
		break;

	case BLAST_IOC_SET_DENOMINATOR:
		/* Set denominator */
		tx_blaster(CMD_DENOMINATOR, arg);
		pr_info("IOCTL> Denominator\n");
		break;
	case BLAST_IOC_PLAY:
		/* Play */
		tx_blaster(CMD_PLAY, 0x00);
		pr_info("IOCTL> Play\n");
		break;
	case BLAST_IOC_STOP:
		/* Stop */
		tx_blaster(CMD_STOP, 0x00);
		pr_info("IOCTL> Stop\n");
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
	*mydev_class = NULL
	if (ret) {
		pr_info("Error: Failed registering major number\n");
		return -1;
	}

	/* Save MAJOR number */
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

	/* Set port mode as write mode */
	byte = inb(0x37A);
	byte = byte & 0xDF; /* hex DF = binary 11011111 */
	outb(byte, 0x37A);

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
