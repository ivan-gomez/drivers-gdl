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
#include "paralelo.h"
#include "conf.h"


#define DEVICE_NAME	    "paralelo"
#define MAX_SIZE	    64
#define OPERAND_MAX_SIZE    11
#define PARPORT       0x378
#define PARPORTSTATUS 0x37A
#define PARALELO_INTERRUPT 7



static unsigned major = 0;			/* major number */
static struct cdev my_dev;			/* to register with kernel */
static struct class *mydev_class = NULL;	/* to register with sysfs */
static struct device *mydev_device;		/* to register with sysfs */
static char buffer[MAX_SIZE];
static bool Read_Write = 0; // write =0 read=1	
static char numbersOfOpens = 0; 		
#ifdef IRQ_DRIVER
static bool flag = 0;		
static DECLARE_WAIT_QUEUE_HEAD(wq);
void *dev_id;
#endif

int size;
operation_t *operationIoctl;


/*
 8
 Interrupt function
 flag = 1;
 wake_up_interruptible(&wq);

*/
#ifdef IRQ_DRIVER
static irqreturn_t paralelo_interrupt(int irq, void *dev_id)
{
	char temporalRegister;
	temporalRegister = inb(PARPORTSTATUS);
	temporalRegister = temporalRegister & 0xEF; /* hex EF = binary 11101111 */
	outb(temporalRegister, PARPORTSTATUS);
		
	temporalRegister = inb(PARPORTSTATUS);
	temporalRegister = temporalRegister | 0x10; /* hex 10 = binary 00010000 */
	outb(temporalRegister, PARPORTSTATUS);
  flag = 1;
	 printk(KERN_INFO "INTERRUPT\n");
  wake_up_interruptible(&wq);
  return IRQ_HANDLED;
}
#endif 
static ssize_t paralelo_read(struct file *filp, char __user *buf, 
					size_t nbuf, loff_t *offs)
{	
     #ifdef IRQ_DRIVER
     if(wait_event_interruptible(wq, flag != 0 && Read_Write!=0 )<0)
    {
        printk(KERN_INFO "ERROR\n");
        return -ERESTARTSYS;
    }
    else
    {
	flag=0;
       /* Variable declaration */
	char temp_reg_value;
	char puertopar_buffer[] = "0\n";
	/* Read the parrallel port */
	puertopar_buffer[0] = inb(PARPORT);
	/* Kernel information  */
	printk(KERN_INFO "ENTER: %s: nbuf = %ld\n", __func__, (long)nbuf);


	/* Copy th data read to the user space */
	printk(KERN_INFO "Copying Buffer from kernel to user space... %d\n",puertopar_buffer[0]);
	copy_to_user(__user buf, &puertopar_buffer, 3);
	/* Kernel information  */
	printk(KERN_INFO "EXIT: %s: nbuf = %ld\n", __func__, (long)nbuf);
	if (*offs == 0) {
		*offs += 1;
		return 3;
	} else {
		return 0;
	}
    }
	#else

	char puertopar_buffer[] = "0\n";
	/* Read the parrallel port */
	puertopar_buffer[0] = inb(PARPORT);
	/* Kernel information  */
	printk(KERN_INFO "ENTER: %s: nbuf = %ld\n", __func__, (long)nbuf);
	/* Copy th data read to the user space */
	printk(KERN_INFO "Copying Buffer from kernel to user space... %d\n",puertopar_buffer[0]);
	copy_to_user(__user buf, &puertopar_buffer, 3);
	/* Kernel information  */
	printk(KERN_INFO "EXIT: %s: nbuf = %ld\n", __func__, (long)nbuf);
	if (*offs == 0) {
		*offs += 1;
		return 3;
	} else {
		return 0;
	}
	#endif
}

static ssize_t paralelo_write(struct file *filp, const char __user *buf,
					size_t nbuf, loff_t *offs)
{ 
	/* Kernel information */
	printk(KERN_INFO "ENTER: %s: nbuf = %ld\n", __func__, (long)nbuf);
	/* Allocate memory */
	memset(buffer, 0,  MAX_SIZE);
	/* Copy the data two be sent from the user space */
	if (copy_from_user(buffer, __user buf, nbuf))
	strcat(buffer, "\nFrom Kernel...\n");
	/* Write the data to the parallel port */
	outb(buffer[0], PARPORT);
	printk(KERN_INFO "el valor escrito es: %c ", buffer[0]);
	printk(KERN_INFO "EXIT: %s: nbuf = %ld\n", __func__, (long)nbuf);
	return 1;

}

static int paralelo_open(struct inode *ip, struct file *filp)
{
	#ifdef IRQ_DRIVER
	if(numbersOfOpens==0)
	{
	 request_irq(PARALELO_INTERRUPT, paralelo_interrupt, IRQF_DISABLED,"paralelo", dev_id);
  	 outb_p(0x10, PARPORTSTATUS); 
	 printk(KERN_INFO "INTERRUPT OPEN\n");		
	}
	numbersOfOpens++;
	printk(KERN_INFO "Open %d\n", numbersOfOpens);
	#endif
	printk(KERN_INFO "%s:%d\n", __func__, __LINE__);	
	return 0;
}

static int paralelo_close(struct inode *ip, struct file *filp)
{
	#ifdef IRQ_DRIVER
	numbersOfOpens--;
	
	printk(KERN_INFO "Close %d\n", numbersOfOpens);
	if(numbersOfOpens==0)
	{
	 free_irq(PARALELO_INTERRUPT,dev_id);
	}
	#endif
	printk(KERN_INFO "%s:%d\n", __func__, __LINE__);
	;
	return 0;
}


static int paralelo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{	
	char temp_reg_value;
	switch(cmd){
	case  PARALELO_IOC_OUT:
	// configurar puerto paralelo como salida
		Read_Write=0;
		temp_reg_value = inb(PARPORTSTATUS);
		temp_reg_value = temp_reg_value & 0xDF;
		outb(temp_reg_value, PARPORTSTATUS);
		printk(KERN_INFO "Write ioctl");
	break;
	case  PARALELO_IOC_IN:
		temp_reg_value = inb(PARPORTSTATUS);
		temp_reg_value = temp_reg_value | 0x20;
		outb(temp_reg_value, PARPORTSTATUS);
		printk(KERN_INFO "read ioctl");
	 	Read_Write=1;
	// configurar puerto paralelo como entrada
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
	default:
	return -ENOTTY;
	break;
	}
	//printk(KERN_INFO "%s:%d\n", __func__, __LINE__);
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

static int __init mymodule_init (void)
{
	dev_t dev_id;
	int ret;	

	/* Allocate major/minor numbers */
	ret = alloc_chrdev_region(&dev_id, 0, 1, DEVICE_NAME);
	if (ret) {
		printk(KERN_INFO "Error: Failed registering major number\n");
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
		printk(KERN_INFO "Error: Failed registering with the kernel\n");
		unregister_chrdev_region(dev_id, 1);
		return -1;
	}

	/* Create a class and register with the sysfs. (Failure is not fatal) */
	mydev_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(mydev_class)) {
		printk(KERN_INFO "class_create() failed: %ld\n",
						PTR_ERR(mydev_class));
		mydev_class = NULL;
	} else {
		/* Register device with sysfs (creates device node in /dev) */
		mydev_device = device_create(mydev_class, NULL, dev_id,
							NULL, DEVICE_NAME"0");
		 
		if (IS_ERR(mydev_device)) {
			printk(KERN_INFO "device_create() failed: %ld\n",
						PTR_ERR(mydev_device));
			mydev_device = NULL;
		}
	}

	printk(KERN_INFO "Driver \"%s\" loaded...\n", DEVICE_NAME);
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

	printk(KERN_INFO "Driver \"%s\" unloaded...\n", DEVICE_NAME);
} 

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_AUTHOR("Equipo 1"); 
MODULE_LICENSE("GPL"); 
MODULE_DESCRIPTION("Practica 4");
