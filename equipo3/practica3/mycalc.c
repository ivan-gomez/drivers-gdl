#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/ioctl.h>
#include "operations.h"

#define DEVICE_NAME "mycalc"

#define IOC_MAGIC   0x94

#define MYCALC_IOC_SET_NUM1 _IOW(IOC_MAGIC, 0x1, long)         /* Defines our ioctl call. */
#define MYCALC_IOC_SET_NUM2 _IOW(IOC_MAGIC, 0x2, long)         /* defines our ioctl call. */

#define MYCALC_IOC_SET_OPERATION    _IOW(IOC_MAGIC, 0x3, char) /* Defines our ioctl call. */
#define MYCALC_IOC_GET_RESULT   _IO(IOC_MAGIC, 0x4 )           /* Defines our ioctl call. */
#define MYCALC_IOC_DO_OPERATION _IO(IOC_MAGIC, 0x5 )           /* Defines our ioctl call. */

static unsigned major = 0;                                     /* Major number */
static struct cdev my_dev;                                     /* Register with kernel */
static struct class *mydev_class = NULL;                       /* Register with sysfs */
static struct device* mydev_device;                            /* Register with sysfs */
int size, tmp;
char kbuff[128];

long num1;
long num2;
char operand;
long result;
int flagl;
int flagr;

void parseFunction (char *buffer, long *num1, long *num2, char *operand)
{
    int length;
    int index, operandPosition;

    * num1 = 0;
    * num2 = 0;
    length = strlen (buffer);
    printk (KERN_INFO "PARSING LENGTH: %d \n", length);

    /* Operator */
    * operand = 'x';

    for (index = 0; index < length; index++) {
        switch (buffer[index]) {
            case '+':
                * operand = buffer[index];
                operandPosition = index;
                break;

            case '-':
                * operand = buffer[index];
                operandPosition = index;
                break;

            case '*':
                * operand = buffer[index];
                operandPosition = index;
                break;

            case '/':
                * operand = buffer[index];
                operandPosition = index;
                break;

            default:
                break;
        }
    }
    printk (KERN_INFO "PARSING OP POSITION: %d \n", operandPosition);

    /* Left operand */
    index = 0;
    flagl = 1;

    /* Eat whitespace */
    while (((buffer[index] < '0') || (buffer[index] > '9')) && (index < length)) { index++; }

    while ((buffer[index] >= '0') && (buffer[index] <= '9') && (index < length)) {
        if (buffer[index] >= '0' && buffer[index] <= '9') {
            * num1 = (*num1 * 10) + buffer[index] - '0';
            flagl = 0;
        }
        index++;
    }

    /* Right operand */
    index = operandPosition + 1;
    flagr = 1;

    /* Eat whitespace */
    while (((buffer[index] < '0') || (buffer[index] > '9')) && (index < length)) { index++; }

    while ((buffer[index] >= '0') && (buffer[index] <= '9') && (index < length)) {
        if (buffer[index] >= '0' && buffer[index] <= '9') {
            * num2 = (*num2 * 10) + buffer[index] - '0';
            flagr = 0;
        }
        index++;
    }
}

static ssize_t my_read(struct file *filp, char __user *buf,
                    size_t nbuf, loff_t *offs)
{

    copy_to_user (buf, kbuff, size);

    printk (KERN_INFO "%s:%d\n", __func__, __LINE__);
    printk (KERN_INFO "READ: %s\n", kbuff);

    if (size > 0) {
        tmp = size;
        * offs += 5;
        size = 0;
        return tmp;
    } else { return 0; }
}

static ssize_t my_write(struct file *filp, const char __user *buf,
                    size_t nbuf, loff_t *offs)
{

    /* Copy from user space */
    copy_from_user (kbuff, buf, nbuf);
    printk (KERN_INFO "%s:%d\n", __func__, __LINE__);
    printk (KERN_INFO "WRITE: %s", kbuff);

    /* Parse function */
    parseFunction (kbuff, &num1, &num2, &operand);
    printk (KERN_INFO "NUM1: %ld\n", num1);
    printk (KERN_INFO "NUM2: %ld\n", num2);
    printk (KERN_INFO "OPERAND: %c\n", operand);

    /* Do operation */
    if (operand == 'x' || (num2 == 0 && operand == '/') || (flagl == 1 || flagr == 1)) {
        printk (KERN_INFO "ERROR: BAD OPERATION\n");
        sprintf (kbuff, "ERROR: BAD OPERATION\n");
        size = strlen (kbuff);
        return nbuf;
    } else {
        result = operation (operand, num1, num2);
        sprintf (kbuff, "%ld\n", result);
        size = strlen (kbuff);
        printk (KERN_INFO "RESULT: %ld", result);
        printk (KERN_INFO "RESULT: %s", kbuff);
        return nbuf;
    }
}

static int my_open (struct inode *ip, struct file *filp)
{
    printk (KERN_INFO "%s:%d\n", __func__, __LINE__);
    return 0;
}

static int my_close (struct inode *ip, struct file *filp)
{
    printk (KERN_INFO "%s:%d\n", __func__, __LINE__);
    return 0;
}

static long device_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{

    printk (KERN_INFO "IOCTL> ENTERED CMD #%d\n", cmd);

    switch (cmd) {
        case MYCALC_IOC_SET_NUM1:
            num1 = arg;
            printk (KERN_INFO "IOCTL> NUM1: %ld\n", num1);
            break;

        case MYCALC_IOC_SET_NUM2:
            num2 = arg;
            printk (KERN_INFO "IOCTL> NUM2: %ld\n", num2);
            break;

        case MYCALC_IOC_SET_OPERATION:
            operand = arg;
            printk (KERN_INFO "IOCTL> OP: %c\n", operand);
            break;

        case MYCALC_IOC_GET_RESULT:
            printk (KERN_INFO "IOCTL> RET RESULT: %ld\n", result);
            return result;
            break;

        case MYCALC_IOC_DO_OPERATION:
            result = operation (operand, num1, num2);
            printk (KERN_INFO "IOCTL> RESULT: %ld\n", result);
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

static int __init mymodule_init (void)
{
    dev_t dev_id;
    int ret;


    /* Allocate major/minor numbers */
    ret = alloc_chrdev_region (&dev_id, 0, 1, DEVICE_NAME);

    if (ret) {
        printk (KERN_INFO "Error: Failed registering major number\n");
        return -1;
    }

    /* save MAJOR number */
    major = MAJOR (dev_id);

    /* Initialize cdev structure and register it with the kernel*/
    cdev_init (&my_dev, &mydev_fops);
    my_dev.owner = THIS_MODULE;

    /* Register the char device with the kernel */
    ret = cdev_add (&my_dev, dev_id, 1);

    if (ret) {
        printk (KERN_INFO "Error: Failed registering with the kernel\n");
        unregister_chrdev_region (dev_id, 1);
        return -1;
    }

    /* Create a class and register with the sysfs. (Failure is not fatal) */
    mydev_class = class_create (THIS_MODULE, DEVICE_NAME);

    if (IS_ERR (mydev_class)) {
        printk (KERN_INFO "class_create() failed: %ld\n", PTR_ERR (mydev_class));
        mydev_class = NULL;
    } else {
        /* Register device with sysfs (creates device node in /dev) */
        mydev_device = device_create (mydev_class, NULL, dev_id, NULL, DEVICE_NAME "0");

        if (IS_ERR (mydev_device)) {
            printk (KERN_INFO "device_create() failed: %ld\n", PTR_ERR (mydev_device));
            mydev_device = NULL;
        }
    }

    printk (KERN_INFO "Driver \"%s\" loaded...\n", DEVICE_NAME);
    return 0;
}

static void __exit mymodule_exit (void)
{
    /* Deregister services from kernel */
    cdev_del (&my_dev);

    /* Release major/minor numbers */
    unregister_chrdev_region (MKDEV (major, 0), 1);

    /* Undone everything initialized in init function */
    if (mydev_class != NULL) {
        device_destroy (mydev_class, MKDEV (major, 0));
        class_destroy (mydev_class);
    }

    printk (KERN_INFO "Driver \"%s\" unloaded...\n", DEVICE_NAME);
}

module_init (mymodule_init);
module_exit (mymodule_exit);

MODULE_AUTHOR ("Alfredo/Ignacio<equipo3@itesm.mx>");
MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION ("Calculator driver");