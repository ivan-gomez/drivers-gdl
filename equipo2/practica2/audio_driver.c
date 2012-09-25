#include "audio_driver.h"

int main(int argc, char *argv[])
{
	unsigned int volume, sample_freq;
	unsigned char play_record;

	/*Revisar si el numero de parametros es el correcto*/
	if (argc != MAX_PARAMS) {
		printf("ERROR: Numero incorrecto de parametros\n");
		return -1;
	}

	/*Verificar que el primer parametro sea una P o R*/
	if (*argv[1] == 'R') {
		printf("Record\n");
		play_record = 1;
	} else if (*argv[1] == 'P') {
		printf("Play\n");
		play_record = 0;
	} else{
		printf("ERROR: No es Play ni Record\n");
		return -1;
	}

	/*Guardo en un entero el valor otorgado por el string (Volumen)*/
	volume = (unsigned int) atoi(argv[2]);

	/*Si el volumen no se encuentra en dicho rango se manda error*/
	if (volume < 0 || volume > 10) {
		printf("ERROR: Volumen tiene que estar entre 0 y 10\n");
		return -1;
	}

	/*Guardo en un entero el valor otorgado por el string
	(Frecuencia de Muestreo)*/
	sample_freq = (unsigned int) atoi(argv[3]);

	/*Si la frecuencia no se encuentre en el rango de un int
	se manda un error*/
	if (sample_freq < 0 || sample_freq > 65535) {
		printf("La frecuencia se encuentra fuera de rango: 0 - 65535\n");
		return -1;
	}

	/*Funcion que maneja los metodos para el uso de ALSA*/
	init_alsa_driver(play_record, argv[4], sample_freq, volume);

	return 0;
}

/*Muchos de los parametros y metodos de esta funcion fueron tomados
via Internet para el correcto funcionamiento de ALSA, tanto para
grabar y reproducir sonidos. Favor de ver la parte de bibliografia
en el reporte para una mayor referencia a dichas funciones*/
void init_alsa_driver(unsigned char play_record, char *file_to_open, unsigned int sample_freq, unsigned int volume)
{
	/*Variables generales - Variables para ALSA*/
	unsigned int direction, status, size;
	unsigned long loops;
	char *buffer;
	FILE *id;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;
	snd_mixer_t *mixer;
	snd_mixer_elem_t *mixer_element;

	/*Antes que nada usamos el mixer de ALSA, asignamos el dispositivo
	default, y establecemos el rango de volumen, asi como el volumen
	default*/
	snd_mixer_open(&mixer, 0);
	snd_mixer_attach(mixer, "default");
	snd_mixer_selem_register(mixer, NULL, NULL);
	snd_mixer_load(mixer);
	mixer_element = snd_mixer_first_elem(mixer);
	snd_mixer_selem_set_playback_volume_range(mixer_element, 0, 10);
	snd_mixer_selem_set_playback_volume_all(mixer_element, 5);

	/*Variable para formar el tamaño del periodo - 32 frames*/
	frames = 32;

	/*Si vamos a reproducir entonces leemos el archivo indicado,
	de case contrario, grabar, escribiremos el archivo indicado o
	bien si no existe dicho archivo se creara*/
	if (play_record == 0)
		id = fopen(file_to_open, "r");
	else
		id = fopen(file_to_open, "w");

	/*Si el id es 0 existio un error al manejar el archivo,
	sino, si se quiere reproducir usamos la funcion
	snd_pcm_open, mandando como tercer parametro la macro
	definida como PLAYBACK. De caso contrario, grabar, se
	manda la macro definida como CAPTURE*/
	if (id != 0) {
		if (play_record == 0) {
			status = snd_pcm_open(&handle, "default",
			SND_PCM_STREAM_PLAYBACK, 0);
		} else if (play_record == 1) {
			status = snd_pcm_open(&handle, "default",
			SND_PCM_STREAM_CAPTURE, 0);
		} else if (play_record != 1 || play_record != 0) {
			printf("Ocurrio un error inesperado\n");
			return;
		}

		/*Validacion de error al usar las funciones de ALSA*/
		if (status < 0) {
			printf("No se pudo abrir tal dispositivo\n");
			exit(1);
		}

		/*Configuraciones generales de Hardware usando ALSA*/
		/*Se utilizo diferente documentacion para localizar
		las funciones que se muestran a continuacion, donde se
		llevan a cabo funciones de formato de informacion,
		los canales usados (stereo), se escriben los parametros
		al Hardware, etc.*/
		snd_pcm_hw_params_alloca(&params);
		snd_pcm_hw_params_any(handle, params);
		snd_pcm_hw_params_set_format(handle, params,
		SND_PCM_FORMAT_S16_LE);
		snd_pcm_hw_params_set_channels(handle, params, 2);
		snd_pcm_hw_params_set_rate_near(handle, params,
		&sample_freq, &direction);
		snd_pcm_hw_params_set_period_size_near(handle, params,
		&frames, &direction);

		status = snd_pcm_hw_params(handle, params);

		/*Validacion de error al usar las funciones de ALSA*/
		if (status < 0) {
			printf("No se pudieron colocar los parametros de Hardware\n");
			exit(1);
		}

		snd_pcm_hw_params_get_period_size(params, &frames, &direction);

		size = frames * 4;
		buffer = (char *) malloc(size);

		snd_pcm_hw_params_get_period_time(params, &sample_freq,
		&direction);

		status = snd_pcm_set_params(handle, SND_PCM_FORMAT_U8,
		SND_PCM_ACCESS_RW_INTERLEAVED, 2, sample_freq, 1, 500000);

		/*Validacion de error al usar las funciones de ALSA*/
		if (status < 0) {
			printf("Error para reproducir\n");
			exit(1);
		}

		/*Se indican cinco segundos en microsegundos (us)
		divididos por el tiempo del periodo*/
		loops = 5000000 / sample_freq;

		/*Dentro del ciclo se lee o escribe respectivamente al
		driver, se detiene el dispositivo, se cierra, se libera
		el buffer, y se cierra el archivo*/
		while (loops > 0) {
			loops--;
			if (play_record == 0) {
				status = fread(buffer, size, 1, id);
				if (status == 0) {
					printf("Error de fin archivo en entrada\n");
					break;
				}
				status = snd_pcm_writei(handle, buffer, frames);
				if (status == -EPIPE) {
					printf("Error de encimamiento\n");
					snd_pcm_prepare(handle);
				} else if (status < 0) {
					printf("Error al tratar de escribir\n");
				}
			} else {
				status = snd_pcm_readi(handle, buffer, frames);
				if (status == -EPIPE) {
					printf("Error de encimamiento\n");
					snd_pcm_prepare(handle);
				} else if (status < 0) {
					printf("Error al tratar de leer\n");
				}

				status = fwrite(buffer, size, 1, id);
			}
		}

		snd_pcm_drain(handle);
		snd_pcm_close(handle);
		free(buffer);
		fclose(id);

		if (play_record == 0)
			printf("Fin de Reproduccion - Success!");
		else
			printf("Fin de Grabacion - Success!");

	} else {
		printf("Ocurrio un problema al tratar el archivo indicado");
	}
}
