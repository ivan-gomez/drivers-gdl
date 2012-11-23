/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "configuration_app.h"

/* Funcion Principal */
int main(void)
{
	/* Declaración e inicializacion de variables */
	FILE *p = NULL;
	char *file = "./conf.h";
	char *par_file = "Modulos/parallel_driver/conf.h";
	char *calc_file = "Modulos/calc_driver/conf.h";
	char buffer[MAX_SIZE];
	char enter;
	char *data = "A";
	size_t len;
	int fd;
	int i;
	unsigned char data2;
	unsigned char serial_read[1];

	struct termios old_termios;
	struct termios new_termios;

	/* Se abre el dispositivo para Serial 01.
	Validacion de errores para Serial*/
	fd = open(dev, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		fprintf(stderr, "error, counldn't open file %s\n", dev);
		return 1;
	}
	if (tcgetattr(fd, &old_termios) != 0) {
		fprintf(stderr, "tcgetattr(fd, &old_termios) failed: %s\n", strerror(errno));
		return 1;
	}

	/* Reservar en memoria para Serial */
	memset(&new_termios, 0, sizeof(new_termios));
	/* Configuracion para los registros de Serial */
	new_termios.c_iflag = IGNPAR;
	new_termios.c_oflag = 0;
	new_termios.c_cflag = CS8 | CREAD | CLOCAL | HUPCL;
	new_termios.c_lflag = 0;
	new_termios.c_cc[VINTR]    = 0;
	new_termios.c_cc[VQUIT]    = 0;
	new_termios.c_cc[VERASE]   = 0;
	new_termios.c_cc[VKILL]    = 0;
	new_termios.c_cc[VEOF]     = 4;
	new_termios.c_cc[VTIME]    = 0;
	new_termios.c_cc[VMIN]     = 1;
	new_termios.c_cc[VSWTC]    = 0;
	new_termios.c_cc[VSTART]   = 0;
	new_termios.c_cc[VSTOP]    = 0;
	new_termios.c_cc[VSUSP]    = 0;
	new_termios.c_cc[VEOL]     = 0;
	new_termios.c_cc[VREPRINT] = 0;
	new_termios.c_cc[VDISCARD] = 0;
	new_termios.c_cc[VWERASE]  = 0;
	new_termios.c_cc[VLNEXT]   = 0;
	new_termios.c_cc[VEOL2]    = 0;

	/* Validacion de configuraciones de Baud Rate.
	Manejo de errores */

	/* Configuracion de Baud Rate - Entrada */
	if (cfsetispeed(&new_termios, B9600) != 0) {
		fprintf(stderr, "cfsetispeed(&new_termios, B9600) failed: %s\n", strerror(errno));
		return 1;
	}

	/* Configuracion de Baud Rate - Salida */
	if (cfsetospeed(&new_termios, B9600) != 0) {
		fprintf(stderr, "cfsetospeed(&new_termios, B9600) failed: %s\n", strerror(errno));
		return 1;
	}

	/* Asocio de parametros a la terminal referida */
	if (tcsetattr(fd, TCSANOW, &new_termios) != 0) {
		fprintf(stderr, "tcsetattr(fd, TCSANOW, &new_termios) failed: %s\n", strerror(errno));
		return 1;
	}

/* Declaracion para ciclo */
init:
	/* Menu de Aplicacion */
	printf("********************** BIENVENIDO - CONFIGURACION AUTOMATICA ***********************\n");
	printf("*******************Inicializando aplicacion...*********************\n");
	printf("*******************Seleccione Modulo a Cargar desde Aplicacion: ********************\n");

	/* Se realizará lectura por parte de la aplicación.
	Esta se detiene hasta que se mande el módulo a configurar*/
	read(fd, serial_read , 1);

	/* Switch para crear archivos de configuración y
	llamadas al sistema */
	switch (serial_read[0]) {

	/* Parallel Port Case - Normal Mode */
	case PARALLEL_PORT:
		/* Se necesita crear un archivo de configuracion con macro
		predefinida para que el modulo se cargue en Modo Normal */
		printf("*************Puerto Paralelo - Modo Normal**************\n");
		p = fopen(par_file, "w");

		/* Validacion de error de archivo de configuracion */
		if (p == NULL)
			printf("Error at opening Configuration File\n", file);

		/* Se escribe dentro del archivo */
		fprintf(p, "#ifndef __CONF_H\n#define __CONF_H\n\n#define PARALLEL_PORT\n\n#endif");
		fclose(p);
		printf("**************Written Succesfully in the configuration file**************\n");

		/* Se usa la función system para poder emular que nos
		encontramos en la línea de comandos. Esto ayudará a realizar
		las compilaciones del módulo */
		system("make -C Modulos/parallel_driver/ 2> background");
		system("gcc -o Modulos/parallel_driver/app_parallel Modulos/parallel_driver/app.c 2> background");
		system("gcc -o Modulos/parallel_driver/strobe_app Modulos/parallel_driver/app_Strobe.c 2> background");
		printf("**************Compile Successfull**************\n");

		/* Se inserta el modulo */
		system("sudo insmod Modulos/parallel_driver/paralelo.ko > background");
		printf("**************Modulo Paralelo insertado correctamente - Modo Normal**************\n");
		printf("**************Se ejecutara la aplicacion del modulo**************\n");

		/* Ejecucion de la Aplicacion del Puerto Paralelo */
		system("./Modulos/parallel_driver/app_parallel");

		/* Una vez finalizado se quita el modulo - Decision Tomada
		por el Usuario */
		system("rmmod paralelo");
		printf("**************Modulo removido*************\n");
	break;

	/* Parallel Port Case - IRQ Mode */
	case PARALLEL_PORT_IRQ:
		/* Se necesita crear un archivo de configuracion con macro
		predefinida para que el modulo se cargue en Modo IRQ */
		printf("**************Puerto Paralelo - Modo IRQ**************\n");
		p = fopen(par_file, "w");

		/* Validacion de error de archivo de configuracion */
		if (p == NULL)
			printf("Error at opening Configuration File\n", file);

		/* Se escribe dentro del archivo */
		fprintf(p, "#ifndef __CONF_H\n#define __CONF_H\n\n#define IRQ_DRIVER\n\n#endif");
		fclose(p);
		printf("**************Written Succesfully in the file**************\n");

		/* Se usa la función system para poder emular que nos
		encontramos en la línea de comandos. Esto ayudará a realizar
		las compilaciones del módulo */
		system("make -C Modulos/parallel_driver/ 2> background");
		system("gcc -o Modulos/parallel_driver/app_parallel Modulos/parallel_driver/app.c 2> background");
		system("gcc -o Modulos/parallel_driver/strobe_app Modulos/parallel_driver/app_Strobe.c 2> background");
		printf("**************Compile Successfull**************\n");

		/* Se inserta el modulo */
		system("sudo insmod Modulos/parallel_driver/paralelo.ko > background");
		printf("**************Modulo Paralelo insertado correctamente - Modo IRQ**************\n");

		/* Ejecucion de la Aplicacion del Puerto Paralelo */
		system("./Modulos/parallel_driver/app_parallel");

		/* Una vez finalizado se quita el modulo - Decision Tomada
		por el Usuario */
		system("rmmod paralelo");

		printf("**************Modulo removido*************\n");

	break;

	/* Calculator Case - Normal Mode */
	case CALCULATOR:
		/* Se necesita crear un archivo de configuracion con macro
		predefinida para que el modulo se cargue en Modo Normal */
		printf("**************Calculadora - Modo Normal**************\n");
		p = fopen(calc_file, "w");

		/* Validacion de error de archivo de configuracion */
		if (p == NULL)
			printf("Error at opening Configuration File\n", file);

		/* Se escribe dentro del archivo */
		fprintf(p, "#ifndef __CONF_H\n#define __CONF_H\n\n#define CALC_NORMAL\n\n#endif");
		fclose(p);

		printf("**************Written Succesfully in the file**************\n");

		/* Se usa la función system para poder emular que nos
		encontramos en la línea de comandos. Esto ayudará a realizar
		las compilaciones del módulo */
		system("make -C Modulos/calc_driver/ 2> background");
		system("gcc -o Modulos/calc_driver/app_calc Modulos/calc_driver/app.c 2> background");
		printf("**************Compile Successfull**************\n");

		/* Se inserta el modulo */
		system("sudo insmod Modulos/calc_driver/calc.ko > background");
		printf("**************Modulo Calculadora insertado correctamente - Modo Normal**************\n");

		/* Ejecucion de la Aplicacion de la Calculadora */
		system("./Modulos/calc_driver/app_calc");

		/* Una vez finalizado se quita el modulo - Decision Tomada
		por el Usuario */
		system("rmmod calc");
		printf("**************Modulo removido*************\n");

	break;

	/* Calculator Case - IOCTL Mode */
	case CALCULATOR_IOCTL:
		/* Se necesita crear un archivo de configuracion con macro
		predefinida para que el modulo se cargue en Modo Normal */
		printf("**************Calculadora - Modo IOCTL**************\n");
		p = fopen(calc_file, "w");

		/* Validacion de error de archivo de configuracion */
		if (p == NULL)
			printf("Error at opening Configuration File\n", file);

		/* Se escribe dentro del archivo */
		fprintf(p, "#ifndef __CONF_H\n#define __CONF_H\n\n#define CALC_IOCTL\n\n#endif");
		fclose(p);

		printf("**************Written Succesfully in the file**************\n");

		/* Se usa la función system para poder emular que nos
		encontramos en la línea de comandos. Esto ayudará a realizar
		las compilaciones del módulo */
		system("make -C Modulos/calc_driver/ 2> background");
		system("gcc -o Modulos/calc_driver/app_calc Modulos/calc_driver/app.c 2> background");
		printf("**************Compile Successfull**************\n");

		/* Se inserta el modulo */
		system("sudo insmod Modulos/calc_driver/calc.ko > background");
		printf("**************Modulo Calculadora insertado correctamente - Modo IOCTL**************\n");

		/* Ejecucion de la Aplicacion de la Calculadora */
		system("./Modulos/calc_driver/app_calc");

		/* Una vez finalizado se quita el modulo - Decision Tomada
		por el Usuario */
		system("rmmod calc");
		printf("**************Modulo removido*************\n");
	break;

	/* AUDIO CASE - PLAY */
	case AUDIO_PLAY:
		printf("**************Audio - ALSA**************\n");

		/* Se compila la aplicacion y se crea un ejecutable llamado
		app_audio */
		system("gcc -o Modulos/audio_app/app_audio Modulos/audio_app/my_player.c -l asound 2> background");
		printf("**************Compile Successfull**************\n");
		printf("**************Aplicacion de audio compilada correctamente - PLAY**************\n");

		/* Se ejecuta la aplicacion de la manera correcta para
		reproducir */
		system("./Modulos/audio_app/app_audio P 5 32000 Modulos/audio_app/nuevo.raw 30");

		printf("**************Aplicacion Terminada*************\n");
	break;

	/* AUDIO CASE - RECORD */
	case AUDIO_RECORD:
		printf("**************Audio - ALSA**************\n");

		/* Se compila la aplicacion y se crea un ejecutable llamado
		app_audio */
		system("gcc -o Modulos/audio_app/app_audio Modulos/audio_app/my_player.c -l asound 2> background");
		printf("**************Compile Successfull**************\n");
		printf("**************Aplicacion de audio compilada correctamente - RECORD**************\n");

		/* Se ejecuta la aplicacion de la manera correcta para
		reproducir */
		system("./Modulos/audio_app/app_audio R 5 32000 Modulos/audio_app/rec.raw 30");

		printf("**************Aplicacion Terminada*************\n");
	break;

	/* Default : No se encontro el componente */
	default:
		/* Imprimir mensaje de error */
		printf("Data invalido en la lectura\n");
	break;
	}

	/* Mandar mensaje de Acknowledge a Computadora */
	write(fd, &data, 3);

	/* Tomar dato de Linea de Comandos */
	printf("Desea continuar?? y-yes n-no\n");
	scanf("%s", &enter);

	/* Si se desea continuar insertando modulos */
	if (enter == 'y') {
		/* Ciclo de programa */
		goto init;
	} else {
		/* Resetear las configuraciones viejas de serial */
		tcsetattr(fd, TCSANOW, &old_termios);
		return 0;
	}
}
