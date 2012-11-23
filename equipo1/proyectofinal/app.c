#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "paralelo.h"
#include <alsa/asoundlib.h>
#include <string.h>

/* Number of samples */
#define SAMPLES_Q	40
/* PIR DEFINES */
#define NO_ONE		0
#define	SOMEONE		1
/* ULTRASOUND DEFINES  */
#define MUY_LEJOS	8600 // 6000 nadie
#define LEJOS		5100
#define MEDIO		1800
#define CERCA		800
#define MUY_CERCA	0

/* states that help state machine to know which sound file to play*/
enum STATES {
	HumpTheBump,
	AlguienAhi,
	Hey,
	Hola,
	OyeTu,
	NoTeVayas,
	AcercateMas,
	UnPocoMas,
	Chiste,
	Chiste2,
	NoTanCerca,
	DameEspacio
} states;


void SetAlsaMasterVolume(long volume)
{
	long min, max;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "Master";
	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, card);
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);
	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t *elem = snd_mixer_find_selem(handle, sid);
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);
	snd_mixer_close(handle);
}



void alsa_driver_do_command(unsigned char play_record,
				char *file_to_open,
				unsigned int sample_freq,
				unsigned int volume,
				unsigned char samplingTimeInSec)
{
	unsigned int direction, status, size;
	unsigned long loops;
	char *buffer;
	FILE *id;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;
	SetAlsaMasterVolume(volume*10);
	frames = 32;
	/* printf("Opening %s...\n", file_to_open); */
	if (play_record == 0)
		id = fopen(file_to_open, "r");
	else
		id = fopen(file_to_open, "w");
	/* printf("file open\n"); */
	/* printf("Opening device driver...\n"); */
	if (id != 0) {
		if (play_record == 0) {
			status = snd_pcm_open(&handle,
						"default",
						SND_PCM_STREAM_PLAYBACK,
						 0);
		} else if (play_record == 1) {
			status = snd_pcm_open(&handle,
						"default",
						SND_PCM_STREAM_CAPTURE,
						0);
		} else if (play_record != 1 || play_record != 0) {
			printf("ERROR occured while opening device driver.\n");
			return;
		}
		if (status < 0) {
			printf("ERROR occured while opening device driver.\n");
			exit(1);
		}
		/* printf("Device open\n"); */
		/* Allocate a hardware parameters object. */
		snd_pcm_hw_params_alloca(&params);
		/* Fill it in with default values. */
		snd_pcm_hw_params_any(handle, params);
		/* Set the desired hardware parameters. */
		/* Interleaved mode */
		snd_pcm_hw_params_set_access(handle,
						params,
						SND_PCM_ACCESS_RW_INTERLEAVED);
		/* Signed 16-bit little-endian format */
		snd_pcm_hw_params_set_format(handle,
						params, SND_PCM_FORMAT_S16_LE);
		/* Two channels (stereo) */
		snd_pcm_hw_params_set_channels(handle, params, 2);
		/* sample_freq bits/second sampling rate */
		snd_pcm_hw_params_set_rate_near(handle, params,
						&sample_freq, &direction);
		/* Set period size to 32 frames. */
		frames = 32;
		snd_pcm_hw_params_set_period_size_near(handle, params,
							&frames, &direction);
		/* Write the parameters to the driver */
		/* printf("Setting hardware driver..\n"); */
		status = snd_pcm_hw_params(handle, params);
		if (status < 0) {
			fprintf(stderr, "unable to set hw parameters: %s\n",
				snd_strerror(status));
			exit(1);
		}
		/* printf("hardware parameters loaded correctly\n"); */
		/* Use a buffer large enough to hold one period */
		snd_pcm_hw_params_get_period_size(params, &frames, &direction);
		size = frames * 4; /* 2 bytes/sample, 2 channels */
		buffer = (char *) malloc(size);
		/* We want to loop for samplingTimeInSec seconds */
		snd_pcm_hw_params_get_period_time(params,
						&sample_freq, &direction);
		loops  = 1000000 / sample_freq;
		loops *= samplingTimeInSec;
		/* printf("Hardware ready\n"); */
		while (loops > 0) {
			loops--;
			if (play_record == 0) {
				status = fread(buffer, size, 1, id);
				if (status == 0) {
					printf("End of file\n");
					break;
				}
				status = snd_pcm_writei(handle, buffer, frames);
				if (status == -EPIPE) {
					printf("overrun occurred\n");
					snd_pcm_prepare(handle);
				} else if (status < 0) {
					printf("Error while writing\n");
				}
			} else {
				status = snd_pcm_readi(handle, buffer, frames);
				if (status == -EPIPE) {
					printf("overrun occurred\n");
					snd_pcm_prepare(handle);
				} else if (status < 0) {
					printf("Error while reading.");
				}
				status = fwrite(buffer, size, 1, id);
			}
		}
		/* printf("closing device..\n"); */
		snd_pcm_drain(handle);
		snd_pcm_close(handle);
		/* printf("Releasing memory..\n"); */
		free(buffer);
		fclose(id);
	} else {
		printf("Error while accesing file.");
	}
}

int main()
{
	/* Read buffer */
	unsigned char readParallel[7];
	/* Samples array based on SAMPLE_Q define*/
	unsigned long samples[SAMPLES_Q];
	/* Number of samples taken*/
	unsigned char numberSamples;
	/* Delay for playing audio in logic intervals */
	unsigned char Delay;
	/* File identifier */
	int fd;
	/* Filtered sample */
	unsigned long ultraSoundRespond;
	/* String array */
	char *Sounds[12];
	/* Flag that help when user was near and now is going far*/
	char    isGoingOutFlag = 0;
	char	volume = 10;
	states = HumpTheBump;
	/* Set the string array with the corresponding string, */
	/* index order needs to match with STATES */
	/* User is far away or there is no user at all */
	Sounds[0] = "audio/newfiles/humpthebump.raw";
	Sounds[1] = "audio/newfiles/hayalguienahi.raw";
	Sounds[2] = "audio/newfiles/hey.raw";
	/* PIR detect presence and ultrasound detects the person is far */
	Sounds[3] = "audio/newfiles/hola.raw";
	Sounds[4] = "audio/newfiles/oyeTu.raw";
	/* The person was near and now is far */
	Sounds[5] = "audio/newfiles/noTeVayas.raw";
	/* Medium range */
	Sounds[6] = "audio/newfiles/acercateMas.raw";
	Sounds[7] = "audio/newfiles/UnPocoMas.raw";
	/* Person is near */
	Sounds[8] = "audio/newfiles/chiste.raw";
	Sounds[9] = "audio/newfiles/rickroll.raw";
	/* Person is almost hitting the sensor*/
	Sounds[10] = "audio/newfiles/notancerca.raw";
	Sounds[11] = "audio/newfiles/dameespacio.raw";
	fd = open("/dev/paralelo0", O_RDWR);
	if (fd < 0) {
		printf("File Error");
		return 1;
	}
	for (;;) {
		numberSamples = 0;
		ultraSoundRespond = 0;
		/* Take samples */
		while (numberSamples < SAMPLES_Q) {
			ioctl(fd, PARALELO_DATAOUT_IN, 1);
			read(fd, readParallel, 5);
			samples[numberSamples] = 0;
			samples[numberSamples]  = readParallel[0];
			samples[numberSamples]  = samples[numberSamples] << 8;
			samples[numberSamples] += readParallel[1];
			samples[numberSamples]  = samples[numberSamples] << 8;
			samples[numberSamples] += readParallel[2];
			samples[numberSamples]  = samples[numberSamples] << 8;
			samples[numberSamples] += readParallel[3];
			ultraSoundRespond += samples[numberSamples];
			numberSamples++;
		 }
		numberSamples--;
		/* get the average */
		ultraSoundRespond /= numberSamples;
		/* Print debug*/
		printf("Data read: %lu , Pir %u\n",
			ultraSoundRespond, readParallel[4]);
		if (Delay > 2) {
			/* Check if person is far or pir is */
			/* not detecting anyone */
			if ((ultraSoundRespond > MUY_LEJOS) ||
				(readParallel[4] == NO_ONE)) {
				if (states > Hey)
					states = HumpTheBump;
			/* if user is going out, call him back*/
				if (isGoingOutFlag == 1) {
					states = NoTeVayas;
					/* Clear the flag */
					isGoingOutFlag = 0;
				}
			} else if (ultraSoundRespond > LEJOS) {
				/* Alternate between files*/
				if (states < Hola || states > OyeTu)
					states = Hola;
			} else if (ultraSoundRespond > MEDIO) {
				/* Alternate between files*/
				volume = 7;
				if (states < AcercateMas || states > UnPocoMas)
					states = AcercateMas;
				/* Set the flag to notify user is now near*/
				isGoingOutFlag = 1;
			} else if (ultraSoundRespond > CERCA) {
				/* Alternate between files*/
				volume = 5;
				if (states < Chiste || states > Chiste2)
					states = Chiste;
			} else if (ultraSoundRespond > MUY_CERCA) {
				/* Alternate between files*/
				volume = 7;
				if (states < NoTanCerca || states > DameEspacio)
					states = NoTanCerca;
				/* Set the flag to notify user is now near*/
				isGoingOutFlag = 1;
			}
			/* Play audio */
			alsa_driver_do_command(0, Sounds[states],
						16000, volume, 8);
			printf("File played: %s\n", Sounds[states]);
			states++;
			volume = 10;
			states %= 12;
			Delay = 0;
		} else {
			Delay++;
		}
	}
	close(fd);
}
