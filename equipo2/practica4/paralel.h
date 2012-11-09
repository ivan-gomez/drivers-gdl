#ifndef __PARALEL_H
#define __PARALEL_H

#include <linux/ioctl.h>

/* Magic Number */
#define MYPORT_IOC_MAGIC		0x94

/* Macros para IOCTL */
#define PARPORTSALIDA		_IO(MYPORT_IOC_MAGIC, 0x1)
#define PARPORTENTRADA		_IO(MYPORT_IOC_MAGIC, 0x2)
#define STROBE			_IO(MYPORT_IOC_MAGIC, 0x3)
#define CLEARSTROBE		_IO(MYPORT_IOC_MAGIC, 0x4)

#endif
