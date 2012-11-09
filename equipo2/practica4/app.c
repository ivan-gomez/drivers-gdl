#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "paralel.h"


int main()
{
	/* Declaracion de variables */
	unsigned char data, enter;
	unsigned char size;
	unsigned int *buff;
	unsigned char parallel_read[1];
	unsigned int i;
	char tmp;
	int fd;

	/* Se abre el archivo paralelo0 */
	fd = open("/dev/paralelo0", O_RDWR, 666);
	/* Validacion de error al abrir el archivo */
	if (fd == -1) {
		printf("Error al abrir el archivo\n");
		return 1;
	}

	/* IOCTL para mandar asercion de STROBE */
	ioctl(fd, STROBE, 1);
	/* Tiempo de espera recomendado para visualizar la interrupcion */
	usleep(1000000);
	/* IOCTL para mandar deasercion de STROBE */
	ioctl(fd, CLEARSTROBE, 1);

	/* Se cierra el archivo */
	close(fd);
}
