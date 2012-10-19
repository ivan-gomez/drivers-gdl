#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME	"MY_DEV"

static unsigned major = 0;			/* major number */
static struct cdev my_dev;			/* to register with kernel */
static struct class *mydev_class = NULL;	/* to register with sysfs */
static struct device *mydev_device;		/* to register with sysfs */

static ssize_t my_read(struct file *filp, char __user *buf, 
					size_t nbuf, loff_t *offs)
{
	printk(KERN_INFO "%s:%d\n", __func__, __LINE__);
	return 0;
}

static ssize_t my_write(struct file *filp, const char __user *buf,
					size_t nbuf, loff_t *offs)
{
	printk(KERN_INFO "%s:%d\n", __func__, __LINE__);
	return nbuf;
}

static int my_open(struct inode *ip, struct file *filp)
{
	printk(KERN_INFO "%s:%d\n", __func__, __LINE__);
	return 0;
}

static int my_close(struct inode *ip, struct file *filp)
{
	printk(KERN_INFO "%s:%d\n", __func__, __LINE__);
	return 0;
}


static const struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.read = my_read,
	.write = my_write,
	.open = my_open,
	.release = my_close,
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

MODULE_AUTHOR("Ivan Gomez Castellanos <ivan.gomez@itesm.mx>"); 
MODULE_LICENSE("GPL"); 
MODULE_DESCRIPTION("Char driver skeleton");

