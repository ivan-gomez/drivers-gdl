#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "paralelo.h"



int main()
{
	unsigned char readParallel[1];
	char Length;
	int fd;
	long resultByIoctl;
	ssize_t bytes;
	char result[1];
	fd=open("/dev/paralelo0", O_RDWR);
	if(fd<0)
	{
		printf("File Error");
		return 1;
	}
	printf("Write init...\n");
	ioctl(fd, PARALELO_IOC_OUT,      1);
	printf("Write is going to happen\n");
	write(fd,10,3);
#ifndef IRQ_DRIVER
	printf("wait for enter\n");
	gets(&result) ;
#endif
	ioctl(fd, PARALELO_IOC_IN,      1);
	printf("Read init...\n");
	printf("Read going to happen\n");
	read(fd,readParallel,1);
	//printf("Data read: %c\n", readParallel[0]);
	printf("Data read hex   0x%x\n", readParallel[0]);
	close(fd);
}