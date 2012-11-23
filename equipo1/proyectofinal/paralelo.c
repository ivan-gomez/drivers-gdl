#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include "paralelo.h"
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/unistd.h>

#define DEVICE_NAME	    "paralelo"
#define MAX_SIZE	    64
#define OPERAND_MAX_SIZE    11
#define PARPORT	0x378
#define PARPORTSTATUS	    0x37A
#define PARALELO_INTERRUPT  7



static unsigned	major;/* major number */
static struct cdev	my_dev;/* to register with kernel */
static struct class	*mydev_class;/* to register with sysfs  */
static struct device	*mydev_device;/* to register with sysfs  */
static bool	Read_Write;/* write =0 read=1 */
static	DEFINE_MUTEX(data_lock);
char	puertopar_buffer[] = "0000\n";
int	size;
static ssize_t paralelo_read(struct file *filp, char __user *buf,
				size_t nbuf, loff_t *offs)
{
	char puerto;
	char ultrasonido;
	char pir;
	static bool down;
	static char pirOn;
	static unsigned long cont;
	/* Read the parrallel port */
	puerto = inb(PARPORT);
	/* Ultrasound sensor is in pin 2 in parallel port */
	ultrasonido = puerto & 0x02;
	/* PIR sensor is in pin 1 in parallel port*/
	pir = puerto & 0x01;
	/* Check if Pir is detecting presence*/
		if (pir == 0x01) {
			cont = puerto;
			/* Assign value to status */
			pirOn = 1;
		} else {
			cont = puerto;
			/* No presence detected */
			pirOn = 0;
		}
	/* Reset Variables */
	down = false;
	cont = 0;
	/* check for rising edge in ultrasound sensor */
	while (ultrasonido != 0x02 && cont < 8000) {
		/* staying in this loop until timeout occur(8 ms) */
		/* or rising edge is polled */
		down = true;
		cont++;
		udelay(1);
	 puerto = inb(PARPORT);
	 ultrasonido = puerto & 0x02;
	}
	/* Timeout occur so don't check for time untile lower edge */
	if (cont >= 8000)
		down = false;
	puerto = inb(PARPORT);
	ultrasonido = puerto & 0x02;
	/* Reset cont */
	cont = 0;
	/* Check for the amount of microseconds */
	/* the high pulse is maintained or if a timeout occur. */
	while ((ultrasonido == 0x02 && down == true) && (cont < 100000)) {
		cont++;
		udelay(1);
		puerto = inb(PARPORT);
		ultrasonido = puerto & 0x02;
	}
	down = false;
	/* Kernel information  */
	pr_info("Buffer  %lu\n" , cont);
	/* Reset buffer and convert from unsigned long to char array. */
	puertopar_buffer[0] = 0;
	puertopar_buffer[1] = 0;
	puertopar_buffer[2] = 0;
	puertopar_buffer[3] = 0;
	puertopar_buffer[4] = 0;
	puertopar_buffer[0] = (cont >> 24);
	puertopar_buffer[1] = (cont >> 16);
	puertopar_buffer[2] = (cont >> 8);
	puertopar_buffer[3] = (cont & 0xFF);
	/* assign pir status */
	puertopar_buffer[4] = pirOn;
	/* send the buffer to user */
	copy_to_user(__user buf, puertopar_buffer, 5);
	if (*offs == 0) {
		*offs += 1;
		return 5;
	} else {
		return 0;
	}
}

static ssize_t paralelo_write(struct file *filp, const char __user *buf,
					size_t nbuf, loff_t *offs)
{
	return 1;
}

static int paralelo_open(struct inode *ip, struct file *filp)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	return 0;
}

static int paralelo_close(struct inode *ip, struct file *filp)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	return 0;
}


static int paralelo_ioctl(struct file *filp,
			unsigned int cmd, unsigned long arg)
{
	char temp_reg_value;
	switch (cmd) {
	case  PARALELO_IOC_OUT:
	/* configurar puerto paralelo como salida */
		Read_Write = 0;
		temp_reg_value = inb(PARPORTSTATUS);
		temp_reg_value = temp_reg_value & 0xDF;
		outb(temp_reg_value, PARPORTSTATUS);
		pr_info("Write ioctl");
	break;
	case  PARALELO_IOC_IN:
		temp_reg_value = inb(PARPORTSTATUS);
		temp_reg_value = temp_reg_value | 0x20;
		outb(temp_reg_value, PARPORTSTATUS);
		pr_info("read ioctl");
		Read_Write = 1;
	/* configurar puerto paralelo como entrada */
	break;
	case  PARALELO_IOC_STROBE_UP:
		temp_reg_value = inb(PARPORTSTATUS);
		temp_reg_value = temp_reg_value | 0x01;
		outb(temp_reg_value, PARPORTSTATUS);
	break;
	case PARALELO_IOC_STROBE_DOWN:
		temp_reg_value = inb(PARPORTSTATUS);
		temp_reg_value = temp_reg_value & 0xFE;
		outb(temp_reg_value, PARPORTSTATUS);
	break;
	case PARALELO_DATAOUT_IN:
		/* Set registers as write state */
		Read_Write = 0;
		temp_reg_value = inb(PARPORTSTATUS);
		temp_reg_value = temp_reg_value & 0xDF;
		outb(temp_reg_value, PARPORTSTATUS);
		outb(0x00, PARPORT);
		udelay(10000);
		outb(0xff, PARPORT);
		/* Send a 5 microseconds high pulse */
		udelay(5);
		/* clear port */
		outb(0x00, PARPORT);
		udelay(50);
		/* Return state to read */
		temp_reg_value = inb(PARPORTSTATUS);
		temp_reg_value = temp_reg_value | 0x20;
		outb(temp_reg_value, PARPORTSTATUS);
		pr_info("read ioctl");
		Read_Write = 1;
	break;
	default:
	return -ENOTTY;
	break;
	}
	return 0;
}


static const struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.read = paralelo_read,
	.write = paralelo_write,
	.unlocked_ioctl = paralelo_ioctl,
	.open = paralelo_open,
	.release = paralelo_close,
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
		pr_info("class_create() failed: %ld\n",
						PTR_ERR(mydev_class));
		mydev_class = NULL;
	} else {
		/* Register device with sysfs (creates device node in /dev) */
		mydev_device = device_create(mydev_class, NULL, dev_id,
							NULL, DEVICE_NAME"0");
		if (IS_ERR(mydev_device)) {
			pr_info("device_create() failed: %ld\n",
						PTR_ERR(mydev_device));
			mydev_device = NULL;
		}
	}
	return 0;
}

static void __exit mymodule_exit(void)
{
	/* Deregister services from kernel */
	cdev_del(&my_dev);
	/* Release major/minor numbers */
	unregister_chrdev_region(MKDEV(major, 0), 1);
	/* Undone everything initialized in init function */
	if (mydev_class != NULL) {
		device_destroy(mydev_class, MKDEV(major, 0));
		class_destroy(mydev_class);
	}
	pr_info("Driver \"%s\" unloaded...\n", DEVICE_NAME);
}

module_init(mymodule_init);
module_exit(mymodule_exit);
MODULE_AUTHOR("Equipo 1");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Proyecto final");
