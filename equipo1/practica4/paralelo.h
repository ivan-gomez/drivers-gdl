#include <linux/ioctl.h>




typedef struct
{
    long opr1Int;
    long opr2Int;
    int  operand;
    long result;
} operation_t;

#define PARALELO_IOC_MAGIC 		0x94

//#define IRQ_DRIVER

#define PARALELO_IOC_OUT		_IOW(PARALELO_IOC_MAGIC, 0x1, long)
#define PARALELO_IOC_IN			_IOW(PARALELO_IOC_MAGIC, 0x2, long)
#define PARALELO_IOC_STROBE_UP		_IOW(PARALELO_IOC_MAGIC, 0x3, char)
#define PARALELO_IOC_STROBE_DOWN	_IOW(PARALELO_IOC_MAGIC, 0x4, char)
