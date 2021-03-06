#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "mycalc.h"

int main()
{
	int fd;
	long result, result2;
	fd = open("/dev/mycalc0", O_RDWR, 666);
	if (fd == -1) {
		printf("error al abrir el archivo");
		return 1;
	}

	ioctl(fd, MYCALC_IOC_SET_NUM1, 15);
	ioctl(fd, MYCALC_IOC_SET_NUM2, 2);
	ioctl(fd, MYCALC_IOC_SET_OPERATION, '/');
	result = ioctl(fd, MYCALC_IOC_GET_RESULT);
	result2 = ioctl(fd, MYCALC_IOC_DO_OPERATION);

	printf("El resultado1 es: %ld\n", result);
	printf("El resultado2 es: %ld\n", result2);

	close(fd);
}
