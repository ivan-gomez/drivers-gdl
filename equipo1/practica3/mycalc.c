#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include "operations.h"
#include "mycalc.h"


#define DEVICE_NAME	    "mycalc"
#define MAX_SIZE	    64
#define OPERAND_MAX_SIZE    11


static unsigned major = 0;			/* major number */
static struct cdev my_dev;			/* to register with kernel */
static struct class *mydev_class = NULL;	/* to register with sysfs */
static struct device *mydev_device;		/* to register with sysfs */
static char buffer[MAX_SIZE];
static long ret;
static char temp[MAX_SIZE];
long result;
long opr1Int,opr2Int;
char operation;
int size;

static ssize_t myCalc_read(struct file *filp, char __user *buf, 
					size_t nbuf, loff_t *offs)
{		
	printk(KERN_INFO "%s:%d\n", __func__, __LINE__);
	copy_to_user(__user buf, buffer, nbuf);	
	return nbuf;
}

static ssize_t myCalc_write(struct file *filp, const char __user *buf,
					size_t nbuf, loff_t *offs)
{
	int count = 0;
	int counter = 0;
	char *referencePtr, *operand1, *operand2;
	char operand1Buff[OPERAND_MAX_SIZE];
	char operand2Buff[OPERAND_MAX_SIZE];
	char operandsIndex; 
	printk(KERN_INFO "%s:%d\n", __func__, __LINE__);
	copy_from_user(buffer, __user buf, nbuf);
	referencePtr = buffer;
        operand1 = &operand1Buff[0];
	operand2 = &operand2Buff[0];
	operandsIndex = 0;
	printk(KERN_INFO "nbuf: %d\n", nbuf);
	printk(KERN_INFO "buf: %s\n", buf);
	printk(KERN_INFO "buffer: %s\n", buffer);
	while(counter != (nbuf))
	{
		if(	*referencePtr == '+' || 
			*referencePtr == '-' || 
			*referencePtr == '*' || 
			*referencePtr == '/')
		{
			operation = *referencePtr;
			count = 0;
			operandsIndex = 1;
		}
		else
		{
		if((*referencePtr >= '0' && *referencePtr <= '9')  && count < OPERAND_MAX_SIZE)
			{
				if(operandsIndex == 0)
				{
					*operand1 = *referencePtr;
					operand1++;
				}
				else
				{
					*operand2 = *referencePtr;
					operand2++;
				}
				count++;
			}
		}

		counter++;
		referencePtr++;
	}
	// End of string value
	*operand1 =  '\0';
	*operand2 =  '\0';
	
	// Convert string to int
	opr1Int = simple_strtol(operand1Buff, '\0', 10);
	opr2Int = simple_strtol(operand2Buff, '\0', 10);
	printk(KERN_INFO "OPR1: %d", opr1Int);
	printk(KERN_INFO "OPR2: %d", opr2Int);
	buffer[1] =0;
	buffer[0] =OK;
	switch(operation)
	{
		case '+':
			result = sum(opr1Int, opr2Int);
		break;

		case '-':
			result = substraction(opr1Int, opr2Int);
		break;

		case '*':
			result = mult(opr1Int, opr2Int);
		break;

		case '/':
			// division by zero not allowed
			if(opr2Int == 0)
			{
			 buffer[0] = DIVISION_BY_ZERO;
			}
			else	
				result = div(opr1Int, opr2Int);
		break;
		default:
		buffer[0] = STRING_BAD_FORMAT; 
		break;
	}
	if( buffer[0] == OK)
	{
	buffer[1]=strlen(buffer);
	sprintf(&buffer[2], "%ld\0", result);
	printk(KERN_INFO "resultado: %s", &buffer[2]);
	}
	return nbuf;
}

static int myCalc_open(struct inode *ip, struct file *filp)
{
	printk(KERN_INFO "%s:%d\n", __func__, __LINE__);
	return 0;
}

static int myCalc_close(struct inode *ip, struct file *filp)
{
	printk(KERN_INFO "%s:%d\n", __func__, __LINE__);
	return 0;
}


static int myCalc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{	
	switch(cmd){
	case  MYCALC_IOC_SET_NUM1:
	opr1Int=arg;
	break;
	case  MYCALC_IOC_SET_NUM2:
	opr2Int=arg;
	break;
	case  MYCALC_IOC_SET_OPERATION:
	operation=arg;
	break;
	case  MYCALC_IOC_GET_RESULT:
	result=oper(opr1Int,opr2Int,operation);
	printk(KERN_INFO "R: %ld\n", result);
	return (int)result;
	break;
	case  MYCALC_IOC_DO_OPERATION:
	result=oper(opr1Int,opr2Int,operation);
	printk(KERN_INFO "R: %ld\n", result);
	return (int)result;
	break;
	default:
	return -ENOTTY;
	break;
	}
	printk(KERN_INFO "%s:%d\n", __func__, __LINE__);
	return 0;
}


static const struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.read = myCalc_read,
	.write = myCalc_write,
	.unlocked_ioctl = myCalc_ioctl,
	.open = myCalc_open,
	.release = myCalc_close,
	
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
MODULE_DESCRIPTION("Practica 3");

