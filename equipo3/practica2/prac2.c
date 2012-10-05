/*
* Utilizacion de la API de ALSA
* 
* USO:
*	Parametro de seleccion Frecuencia de muestreo Volumen Archivo
*	Ejemplo: ./alsa R 8000 10 lol.rec
*/

#define ALSA_PCM_NEW_HW_PARAMS_API
#include "alsa/asoundlib.h"
#include "prac2.h"
#include <string.h>

int main(int argc, char *argv[])
{
	unsigned int freq;
	unsigned int vol;

	snd_mixer_t *mix;
	snd_mixer_elem_t *elem;

	if (argc != 5) /*strcmp(argv[1], "--help") == 0)*/
		help();

	/*Get the frequency and the volume*/
	freq = atoi(argv[2]);
	vol  = atoi(argv[3]);

	/*Validating volume and frequency*/
	if (vol < 0 || vol > 10 || freq < 2 || freq > 45000){
		help();
	} else {
		vol = vol*9000;
	}
	
	/*Configure volume*/
	snd_mixer_open(&mix, 0);
	snd_mixer_attach(mix, device);
	snd_mixer_selem_register(mix, NULL, NULL);
	snd_mixer_load(mix);
	elem = snd_mixer_first_elem(mix);
	snd_mixer_selem_set_playback_volume_all(elem, vol);
	snd_mixer_close(mix);

	/*Selection menu*/
	/*Play*/
	if (strcmp(argv[1], "P") == 0) {
		printf("playing\n");
		play(argv[4], freq);
	}
	/*Record*/
	else if (strcmp(argv[1], "R") == 0) {
			printf("recording\n");
			rec(argv[4], freq);
	}
	/*No Correct Option*/
	else
		help();

	return 0;
}

/*Method play*/
int play(char argv[], unsigned int fre)
{
	long loops;
	int rc;
	int size;
	int dir;
	unsigned int val;
	char *buffer;
	char *file = argv;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;
	FILE *p = NULL;

	/*Open to read the AudioFile*/
	p = fopen(file, "r");
	printf("Audio Pplayed from: %s\n", file);

	/* Open PCM device for playback. */
	rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n",
						snd_strerror(rc));
		exit(1);
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	snd_pcm_hw_params_any(handle, params);

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params,
			SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(handle, params, 2);

	/* fre bits/second sampling rate (CD quality) */
	val = fre;
	snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

	/* Set period size to 32 frames. */
	frames = 32;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n",
						snd_strerror(rc));
		exit(1);
	}

	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	size = frames * 4; /* 2 bytes/sample, 2 channels */
	buffer = (char *) malloc(size);

	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(params, &val, &dir);
	/* 10 seconds in microseconds divided by period time */
	loops = 10000000 / val;

	
	//while (loops > 0) {
	//	loops--;
	//	/*read AudioFile*/
	//	rc = fread(buffer, size, 1, p);
	//	/*write on buffer device*/
	//	rc = snd_pcm_writei(handle, buffer, frames);
	//}
	
	do{
		loops--;
		/*read AudioFile*/
		rc = fread(buffer, size, 1, p);
		/*write on buffer device*/
		(void)snd_pcm_writei(handle, buffer, frames);
	} while( rc > 0 );

	/*Close the driver*/
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);
	return 0;
}

/*Method Record*/
int rec(char argv[], unsigned int fre)
{
	long loops;
	int rc;
	int size;
	int dir;
	unsigned int val;
	char *buffer;
	char *file = argv;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;
	FILE *p = NULL;

	/*Open to write the AudioFile*/
	p = fopen(file, "w");
	printf("Audio recorded in: %s\n", argv);

	/* Open PCM device for recording (capture). */
	rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n",
						snd_strerror(rc));
		exit(1);
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	snd_pcm_hw_params_any(handle, params);

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params,
			SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(handle, params, 2);

	/* frequency(frec) bits/second sampling rate (CD quality) */
	val = fre;
	snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

	/* Set period size to 32 frames. */
	frames = 32;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n",
						snd_strerror(rc));
		exit(1);
	}

	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	size = frames * 4; /* 2 bytes/sample, 2 channels */
	buffer = (char *) malloc(size);

	/* We want to loop for 10 seconds */
	snd_pcm_hw_params_get_period_time(params, &val, &dir);
	loops = 10000000 / val;

	while (loops > 0) {
		loops--;
		/*Read from buffer device*/
		rc = snd_pcm_readi(handle, buffer, frames);
		/*Write on AudioFile*/
		rc = fwrite(buffer, size, 1, p);
	}

	/*Close the divice*/
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);

	return 0;
}

void help(void)
{
	printf("\nParameters:\n\tSelector: P to play\n\tR ro recorder"
		"\n\tFrequency: X number between 20 - 45000"
		"\n\tVolume:    X number between 0 - 10"
		"\n\tAudioFile: Write the name of the audio"
		"\n\tfile that will be recorder."
		"\n\tExecution:"
		"\n\t\toutputFile.c Selector Frequency Volume AudioFile"
		"\n\t\tExample of excecution:\n\t\t./alsa R 8000 10 abcd.rec\n");
	exit(1);
}
