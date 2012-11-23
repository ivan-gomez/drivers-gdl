#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "mycalc.h"
#include "conf.h"

int main()
{
	int fd;
	char* op1; 
	char* op2;
	char* operation;
	char* sendtosystem;
	long result, result2;
	fd = open("/dev/mycalc0", O_RDWR, 666);
	if (fd == -1) {
		printf("error al abrir el archivo");
		return 1;
	}

#ifdef CALC_IOCTL
	ioctl(fd, MYCALC_IOC_SET_NUM1, 15);
	ioctl(fd, MYCALC_IOC_SET_NUM2, 2);
	ioctl(fd, MYCALC_IOC_SET_OPERATION, '/');
	result = ioctl(fd, MYCALC_IOC_GET_RESULT);
	result2 = ioctl(fd, MYCALC_IOC_DO_OPERATION);
	printf("El resultado1 es: %ld\n", result);
	printf("El resultado2 es: %ld\n", result2);
#endif

#ifdef CALC_NORMAL
	printf("Que operacion deseas realizar?\n");
	scanf("%s", &operation);
	printf("Selecciona el primer operando: \n");
	scanf("%s", &op1);
	printf("Selecciona el segundo operando: \n");
	scanf("%s", &op2);

	sendtosystem = malloc(strlen(&op1) + strlen (&op2) + strlen(&operation) + 23);
	strcpy(sendtosystem, "echo ");
	strcat(sendtosystem, &op1);
	strcat(sendtosystem, " ");
	strcat(sendtosystem, &operation);
	strcat(sendtosystem, " ");
	strcat(sendtosystem, &op2);
	strcat(sendtosystem, " > /dev/mycalc0");

	system(sendtosystem);
	system("cat /dev/mycalc0");
#endif
	close(fd);
}
