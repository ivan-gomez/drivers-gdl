#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include "paralel.h"

#define IRQ

#define DEVICE_NAME	"paralelo"
#define MAX_SIZE	128
#define KBUFF_SIZE	4

static unsigned major;				/* major number */
static struct cdev parallel_port;		/* to register with kernel */
static struct class *parallel_port_class;	/* to register with sysfs */
static struct device *parallel_port_device;	/* to register with sysfs */
static char pbuffer[64];
static bool in_out;				/* Out - 0 <---> In - 1 */
int size;
int opens;
struct resource *ppres;

/* Macro de compilacion para IRQ */
#ifdef IRQ
static bool flag;
static DECLARE_WAIT_QUEUE_HEAD(wq);
void *dev_id;
#endif

#ifdef IRQ
/* Declaracion de la interrupcion para puerto paralelo */
static irqreturn_t paralelo_irq(int irq, void *dev_id)
{
	char ctrl;

	/* Validacion para reseteo de bandera y volver a colocar
	la interrupcion */
	ctrl = inb(0x37A);
	ctrl = ctrl & 0xEF;
	outb(ctrl, 0x37A);
	ctrl = inb(0x37A);
	ctrl = ctrl | 0x10;
	outb(ctrl, 0x37A);
	pr_info("hola!");
	flag = 1;
	wake_up_interruptible(&wq);
	pr_info("adios!");
	return IRQ_HANDLED;
}
#endif

/* Funcion de lectura del puerto paralelo */
static ssize_t parallelport_read(struct file *filp, char __user *buf,
					size_t nbuf, loff_t *offs)
{
	char port_buff[] = "0\n";
	#ifdef IRQ
	/* Validar la condicion para generar la interrupcion
	en este caso se necesita cambio de la bandera de
	interrupcion y de que estamos en modo de lectura */
	if (wait_event_interruptible(wq, flag != 0 && in_out != 0) < 0) {
		pr_info("Error de Interrupcion\n");
		return -ERESTARTSYS;
	}
	/* Resteo de la bandera de interrupcion */
	flag = 0;
	pr_info("Super Interrupcion");
	#endif
	/* Manipular registros para que el puerto sea de lectura */
	port_buff[0] = inb(0x378);
	copy_to_user(__user buf, &port_buff, 3);
	/* Validacion de offsets */
	if (*offs == 0) {
		*offs += 1;
		return 3;
	} else {
		return 0;
	}
}

/* Funcion de escritura del puerto paralelo */
static ssize_t parallelport_write(struct file *filp, const char __user *buf,
					size_t nbuf, loff_t *offs)
{
	/* Locacion en memoria de buffer */
	memset(pbuffer, 0, 64);
	/* Copia del buffer de usuario al de kernel */
	copy_from_user(&pbuffer, __user buf, nbuf);
	outb(pbuffer[0], 0x378);
	pr_info("Data to Write: %c\n", pbuffer[0]);
	return 1;
}

/* Funcion de apertura del puerto paralelo */
static int parallelport_open(struct inode *ip, struct file *filp)
{
	#ifdef IRQ
	/* Cuidar que se haga un request de interrupcion si es la
	primera vez que se abre el archivo */
	if (opens == 0) {
		char interrupt_status;
		interrupt_status = request_irq(7, paralelo_irq, IRQF_DISABLED,
								"paralelo", 0);
		/* Validacion de error de request de interrupciones */
		if (interrupt_status < 0) {
			pr_info("Error of request for IRQ\n");
			return -1;
		}
		outb_p(0x10, 0x378 + 2);
		pr_info("Interrupcion habilitada.\n");
	}
	#endif
	opens++;
	return 0;
}

/* Funcion para cerrar el puerto paralelo */
static int parallelport_close(struct inode *ip, struct file *filp)
{
	char ctrl;
	#ifdef IRQ
	/* Si fue abierto el archivo */
	if (opens != 0) {
		/* Se deshabilita el puerto paralelo y a la vez
		se liberan las interrupciones */
		ctrl = inb(0x37A);
		ctrl = ctrl & 0xEF;
		outb(ctrl, 0x37A);
		free_irq(7, 0);
		pr_info("Interrupcion deshabilitada.\n");
	}
	#endif
	pr_info("Puerto Cerrado.\n");
	return 0;
}

/* Funcion de IOCTL para el puerto paralelo */
static int parallelport_ioctl(struct file *filp, unsigned int cmd,
						unsigned long arg)
{
	char ctrl;
	pr_info("Entrando a IOCTL\n");
	switch (cmd) {
	/* Se configura el puerto paralelo como salida */
	case PARPORTSALIDA:
		/* Bandera in_out en cero para evitar que entre
		a la interrupcion */
		in_out = 0;
		ctrl = inb(0x37A);
		outb((ctrl & 0xDF), 0x37A);
		pr_info("Puerto configurado como salida\n");
	break;

	/* Se configura el puerto paralelo como entrada */
	case PARPORTENTRADA:
		/* Bandera puesta en 1 para asegurar que en
		modo de lectura active la interrupcion */
		in_out = 1;
		ctrl = inb(0x37A);
		outb((ctrl | 0x20), 0x37A);
		pr_info("Puerto configurado como entrada\n");
	break;

	/* Se realiza una asercion del pin de Strobe */
	case STROBE:
		ctrl = inb(0x37A);
		ctrl = ctrl | 0x01;
		outb(ctrl, 0x37A);
	break;

	/* Se realiza una deasercion del pin de Strobe */
	case CLEARSTROBE:
		ctrl = inb(0x37A);
		ctrl = ctrl & 0xFE;
		outb(ctrl, 0x37A);
	break;

	/* Si no se entro a ningun IOCTL se manda error */
	default:
		pr_err("Error: IOCTL no valida\n");
		return -EINVAL;
	break;
	}
	return 0;
}

/* Estructura de asignacion para puerto paralelo */
static const struct file_operations parallelport_fops = {
	.owner = THIS_MODULE,
	.read = parallelport_read,
	.write = parallelport_write,
	.open = parallelport_open,
	.release = parallelport_close,
	.unlocked_ioctl = parallelport_ioctl,
};

/* Funcion para quitar clases y registros de modulo */
static void unregister(void)
{
	cdev_del(&parallel_port);
	unregister_chrdev_region(MKDEV(major, 0), 1);

	if (parallel_port_class != NULL) {
		device_destroy(parallel_port_class, MKDEV(major, 0));
		class_destroy(parallel_port_class);
	}

	pr_info("Error al cargar modulo\n");
}

/* Funcion para inicializar el Puerto Paralelo */
static int __init parallelport_init(void)
{
	dev_t dev_id;
	int ret;

	/* Allocate major/minor numbers */
	ret = alloc_chrdev_region(&dev_id, 0, 1, DEVICE_NAME);

	if (ret < 0) {
		pr_info("Error al registrar dispositivo\n");
		unregister();
		return -1;
	}

	ret = alloc_chrdev_region(&dev_id, 0, 1, DEVICE_NAME);

	if (ret) {
		pr_info("Error: Failed registering major number\n");
		unregister();
		return -1;
	}

	/* save MAJOR number */
	major = MAJOR(dev_id);

	/* Initialize cdev structure and register it with the kernel*/
	cdev_init(&parallel_port, &parallelport_fops);
	parallel_port.owner = THIS_MODULE;
	/* Register the char device with the kernel */
	ret = cdev_add(&parallel_port, dev_id, 1);
	if (ret) {
		pr_info("Error: Failed registering with the kernel\n");
		unregister_chrdev_region(dev_id, 1);
		return -1;
	}

	/* Create a class and register with the sysfs. (Failure is not fatal) */
	parallel_port_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(parallel_port_class)) {
		pr_info("class_create() failed: %ld\n",
						PTR_ERR(parallel_port_class));
		parallel_port_class = NULL;
	} else {
		/* Register device with sysfs (creates device node in /dev) */
		parallel_port_device = device_create(parallel_port_class, NULL,
						dev_id, NULL, DEVICE_NAME"0");
		if (IS_ERR(parallel_port_device)) {
			pr_info("device_create() failed: %ld\n",
						PTR_ERR(parallel_port_device));
			parallel_port_device = NULL;
		}
	}

	ppres = request_region(0x378, 3, "PuertoParalelo");
	if (ppres == NULL) {
		pr_info("Puerto Paralelo trabajando...\n");
		unregister();
		return -1;
	}

	pr_info("Driver \"%s\" loaded...\n", DEVICE_NAME);
	return 0;
}

/* Funcion para quitar el puerto paralelo */
static void __exit parallelport_exit(void)
{
	/* Deregister services from kernel */
	cdev_del(&parallel_port);

	/* Release major/minor numbers */
	unregister_chrdev_region(MKDEV(major, 0), 1);

	/* Undone everything initialized in init function */
	if (parallel_port_class != NULL) {
		device_destroy(parallel_port_class, MKDEV(major, 0));
		class_destroy(parallel_port_class);
	}

	/* Hacer Release del puerto paralelo */
	if (ppres != NULL)
		release_region(0x378, 3);

	pr_info("Driver \"%s\" unloaded...\n", DEVICE_NAME);
}

/* Asignacion de init y exits */
module_init(parallelport_init);
module_exit(parallelport_exit);

/* Informacion */
MODULE_AUTHOR("Josue Lizarraga, Luis Daniel Aguiar");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Char driver (Parallel Port)");
