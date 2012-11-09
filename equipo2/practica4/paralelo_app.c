#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "paralel.h"
/* Macro de compilacion para el uso de Interrupciones */
#define IRQ

int main()
{
	/* Declaracion de variables */
	unsigned char enter;
	unsigned int data;
	unsigned char size;
	unsigned int *buff;
	unsigned char parallel_read[1];
	unsigned int i;
	char tmp;
	int fd;

	/* File open de modulo de kernel */
	fd = open("/dev/paralelo0", O_RDWR, 666);
	/* Validacion de errores */
	if (fd == -1) {
		printf("Error al abrir el archivo\n");
		return 1;
	}

	/* Configuraciones de puerto paralelo como salida via IOCTL */
	ioctl(fd, PARPORTSALIDA, 1);
	/* Se escribe un dato en Hexadecimal */
	printf("Escribe un byte a mandar en Hexadecimal (Solo un byte): ");
	scanf("%x", &data);

	/* Se llama a la funcion write */
	write(fd, &data, 3);

	#ifdef IRQ
	/* Parte de la explicacion de espera para IRQ */
	#else

	/* Si no estamos en IRQ */
	/* Entonces esperamos a un enter para comenzar a leer */
	printf("Selecciona A y presiona enter para leer:\n");
	scanf("%x", &enter);
	printf("%x\n", enter);

	/* Validacion de error para enter */
	if (enter != 10) {
		printf("Tienes que presionar A para leer!!\n");
		return -1;
	}
	#endif

	/* Configurar puerto como entrada via IOCTL */
	ioctl(fd, PARPORTENTRADA, 1);
	/* Se llama a la funcion Read */
	read(fd, parallel_read , 1);
	printf("Estoy leyendo un 0x%x\n", parallel_read[0]);

	/* Se cierra el archivo */
	close(fd);
}
