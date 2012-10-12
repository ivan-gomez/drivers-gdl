#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "mycalc.h"


int main()
{
	char ResultWriteRead[30];
	char StatusCode;
	char Length;
	int fd;
	long resultByIoctl, resultOneCmdIoctl;
	ssize_t bytes;
	fd=open("/dev/mycalc0", O_RDWR, 666);
	if(fd==-1)
	{
		printf("File Error");
		return 1;
	}
	write(fd,"6+6",3);
	read(fd,ResultWriteRead,20);
	switch(ResultWriteRead[0])
	{
		case OK:
		printf("Result using write and read: %s\n", &ResultWriteRead[2]);
		break;

		case STRING_BAD_FORMAT:
			printf("Buffer format error in write\n");
		break;

		case DIVISION_BY_ZERO:
			printf("Division by zero\n");
		break;
	}
	ioctl(fd, MYCALC_IOC_SET_NUM1, 30);
	ioctl(fd, MYCALC_IOC_SET_NUM2, 20);
	ioctl(fd, MYCALC_IOC_SET_OPERATION, '-');
	resultByIoctl  = ioctl(fd, MYCALC_IOC_GET_RESULT);
	resultOneCmdIoctl = ioctl(fd, MYCALC_IOC_DO_OPERATION);
	printf("Results using ioctls %ld\n", resultByIoctl);
	printf("Results using one ioctlCommand: %ld\n", resultOneCmdIoctl);

	close(fd);
	
}
