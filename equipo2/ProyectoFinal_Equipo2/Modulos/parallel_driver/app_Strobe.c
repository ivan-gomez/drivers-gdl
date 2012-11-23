#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "/home/teamgearsofwarftw/ProyectoFinal_Equipo2/Modulos/parallel_driver/paralelo.h"
#include "/home/teamgearsofwarftw/ProyectoFinal_Equipo2/Modulos/parallel_driver/conf.h"



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
	ioctl(fd, PARALELO_IOC_STROBE_DOWN,      1);
	usleep(100000);
	ioctl(fd, PARALELO_IOC_STROBE_UP,        1);
	usleep(100000);
	ioctl(fd, PARALELO_IOC_STROBE_DOWN,      1);
	close(fd);
}
