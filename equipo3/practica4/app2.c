#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include "config.h"

#if ENABLE_INTERRUPT

#else
	#define  WAIT_MANUALLY 1
#endif

#define IOC_MAGIC				0x94

#define PARA_IOC_SET_WRITE		_IO(IOC_MAGIC, 0x1)           /* Defines our ioctl call. */
#define PARA_IOC_SET_READ		_IO(IOC_MAGIC, 0x2)           /* Defines our ioctl call. */
#define PARA_IOC_SET_STROBE		_IO(IOC_MAGIC, 0x3)           /* Defines our ioctl call. */
#define PARA_IOC_CLEAR_STROBE	_IO(IOC_MAGIC, 0x4)           /* Defines our ioctl call. */

int main()
{
	char wbuffer[512];
	char rbuffer[512];
	int f, i, ret;
	long result;
	ssize_t bytes;

	#if ENABLE_INTERRUPT
	/* Open a file */
	f = open("/dev/para0", O_RDWR, 0666);

	if (f == -1) {
		printf("failed to open paralel port.\n");
		return 1;
	}

	printf("APP2>Opened.\n");
	usleep(30000);

	/* Clear strobe */
	ret = ioctl(f, PARA_IOC_SET_STROBE);
	printf("APP2>Strobe cleared.\n");
	usleep(10000);

	/* Set strobe */
	ret = ioctl(f, PARA_IOC_CLEAR_STROBE);
	printf("APP2>Strobe set.\n");

	/* Close file */
	close(f);
	printf("APP2>Closed.\n");
	#endif
}
