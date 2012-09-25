#define ALSA_PCM_NEW_HW_PARAMS_API

#include <stdlib.h>
#include "errno.h"
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>

void play(int sec, int freq, char *file)
{
	long loops;
	int rc;
	int size;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	char *buffer;
	int fd;

	/* Check ranges */
	if (sec < 1 || sec > 4000) {
		printf("WARNING: Incorrect time to play range [1,4000] s\n");
		printf("\tSetting time to play to 5s...\n");
		sec = 5;
	}
	if (freq < 1000 || freq > 100000) {
		printf("ERROR: Incorrect frequency range [1000,100000] Hz\n");
		printf("\tSetting frequency to 44.1 kHz...\n");
		freq = 44100;
	}

	/* Open file */
	fd = open(file, O_RDONLY);
	if (fd < 0) {
		/* There was an error opening the file */
		printf("ERROR: Couldn't open file to play\n");
		printf("\tPlease make sure file exists\n");
	} else {
		/* Print that the file is playing with its parameters */
		printf("Playing file %s for %d seconds", file, sec);
		printf(" and frequency %d...\n", freq);

		/* Open PCM device for playback */
		rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK,
			0);
		if (rc < 0) {
			fprintf(stderr, "unable to open pcm device: %s\n",
				snd_strerror(rc));
			exit(1);
		}

		/* Allocate a hardware parameters object */
		snd_pcm_hw_params_alloca(&params);
		/* Fill it in with default values */
		snd_pcm_hw_params_any(handle, params);

		/* Set hardware parameters */

		/* Interleaved mode */
		snd_pcm_hw_params_set_access(handle, params,
		SND_PCM_ACCESS_RW_INTERLEAVED);
		/* Signed 16-bit little-endian format */
		snd_pcm_hw_params_set_format(handle, params,
			SND_PCM_FORMAT_S16_LE);
		/* Two channels (stereo) */
		snd_pcm_hw_params_set_channels(handle, params, 2);
		/* freq bits/second sampling rate (CD quality) */
		val = freq;
		snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
		/* Set period size to 32 frames */
		frames = 32;
		snd_pcm_hw_params_set_period_size_near(handle, params,
			&frames, &dir);
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
		/* We want to loop for sec seconds */
		snd_pcm_hw_params_get_period_time(params, &val, &dir);
		/* sec seconds in microseconds divided by period time */
		loops = sec*1000000 / val;

		while (loops > 0) {
			loops--;
			rc = read(fd, buffer, size);
			if (rc == 0) {
				fprintf(stderr, "end of file on input\n");
				break;
			} else if (rc != size) {
				fprintf(stderr, "short read: read %d bytes\n",
					rc);
			}
			rc = snd_pcm_writei(handle, buffer, frames);
			if (rc == -EPIPE) {
				/* EPIPE means underrun */
				fprintf(stderr, "underrun occurred\n");
				snd_pcm_prepare(handle);
			} else if (rc < 0) {
				fprintf(stderr, "error from writei: %s\n",
				snd_strerror(rc));
			}  else if (rc != (int)frames) {
				fprintf(stderr,
					"short write, write %d frames\n", rc);
			}
		}
		/*Close handle and free buffer*/
		snd_pcm_drain(handle);
		snd_pcm_close(handle);
		free(buffer);
	}
}

void record(int sec, int freq, char *file)
{
	long loops;
	int rc;
	int size;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	char *buffer;
	int fd;

	/* Check ranges */
	if (sec < 1 || sec > 4000) {
		printf("WARNING: Incorrect time to record range [1,4000] s\n");
		printf("\tSetting time to record to 5s...\n");
		sec = 5;
	}
	if (freq < 1000 || freq > 100000) {
		printf("ERROR: Incorrect frequency range [1000,100000] Hz\n");
		printf("\tSetting frequency to 44.1 kHz...\n");
		freq = 44100;
	}

	/* Open file to write, clear it at start and create it if necesssary */
	fd = open(file, O_WRONLY|O_TRUNC|O_CREAT,
		S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
	if (fd < 0) {
		/* There was an error opening or creating the file */
		printf("ERROR: Couldn't open/create file to record\n");
	} else {
		/* Print that the file is playing with its parameters */
		printf("Recording file %s for %d seconds", file, sec);
		printf(" and frequency %d...\n", freq);

		/* Open PCM device for recording (capture) */
		rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE,
			0);
		if (rc < 0) {
			fprintf(stderr, "unable to open pcm device: %s\n",
				snd_strerror(rc));
			exit(1);
		}
		/* Allocate a hardware parameters object */
		snd_pcm_hw_params_alloca(&params);
		/* Fill it in with default values */
		snd_pcm_hw_params_any(handle, params);

		/* Set hardware parameters. */

		/* Interleaved mode */
		snd_pcm_hw_params_set_access(handle, params,
		SND_PCM_ACCESS_RW_INTERLEAVED);
		/* Signed 16-bit little-endian format */
		snd_pcm_hw_params_set_format(handle, params,
		SND_PCM_FORMAT_S16_LE);
		/* Two channels (stereo) */
		snd_pcm_hw_params_set_channels(handle, params, 2);
		/* freq bits/second sampling rate (CD quality) */
		val = freq;
		snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
		/* Set period size to 32 frames */
		frames = 32;
		snd_pcm_hw_params_set_period_size_near(handle, params,
			&frames, &dir);
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

		/* We want to loop for sec seconds */
		snd_pcm_hw_params_get_period_time(params, &val, &dir);
		/* sec seconds in microseconds divided by period time */
		loops = sec*1000000 / val;

		while (loops > 0) {
			loops--;
			rc = snd_pcm_readi(handle, buffer, frames);
			if (rc == -EPIPE) {
				/* EPIPE means overrun */
				fprintf(stderr, "overrun occurred\n");
				snd_pcm_prepare(handle);
			} else if (rc < 0) {
				fprintf(stderr,
					"error from read: %s\n",
				snd_strerror(rc));
			} else if (rc != (int)frames) {
				fprintf(stderr, "short read, read %d frames\n"
					, rc);
			}
			rc = write(fd, buffer, size);
			if (rc != size)
				fprintf(stderr,
					"short write: wrote %d bytes\n", rc);
		}
		/*Close handle and free buffer*/
		snd_pcm_drain(handle);
		snd_pcm_close(handle);
		free(buffer);
	}
}

void set_volume(int vol)
{
	long min, max;
	snd_mixer_t *handlev;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "Master";

	if (vol < 1 || vol > 100) {
		printf("ERROR: Volume out of range [0,100] %%\n");
		printf("\tSetting volume to 50%%...\n");
		vol = 50;
	}

	snd_mixer_open(&handlev, 0);
	snd_mixer_attach(handlev, card);
	snd_mixer_selem_register(handlev, NULL, NULL);
	snd_mixer_load(handlev);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t *elem = snd_mixer_find_selem(handlev, sid);

	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_set_playback_volume_all(elem, vol * max / 100 + min);

	snd_mixer_close(handlev);
}
int main(int argc, char *argv[])
{
	int vol = 0;
	int freq = 0;
	int sec = 0;

	/* Help request */
	if (argc == 2 && *argv[1] == 'h') {
		printf("\n------PLAYER HELP-------\n");
		printf("Format is the following:\n\n");
		printf("./player func vol freq filename [sec]\n\n");
		printf("\tfunc:\t'R' or 'P' to record or play audio.\n");
		printf("\tvol:\tVolume percent, range is from 1 to 100.\n");
		printf("\tfreq:\tFrequency, range is 1000 to 100000.\n");
		printf("\tfilename:\tName of file to play/record to.\n");
		printf("\tsec:\tTime to play or record in seconds, ");
			printf("range is 1 to 4000.\n\n");
		return 0;
	}
	/* Number of parameters */
	if (argc < 5) {
		printf("ERROR: Incorrect number of parameters\n");
		return -EINVAL;
	}

	/* Parse volume and frequency (at least) */
	vol = atoi(argv[2]);
	freq = atoi(argv[3]);
	/* If there is a time parameter, parse it */
	if (argc > 5)
		sec = atoi(argv[5]);
	/* If parameter sec is 0, play until it ends (max 4000 sec) */
	if (sec == 0)
		sec = 4000;
	/* Set volume */
	set_volume(vol);

	/* Play/record file */
	if (*argv[1] == 'R') {
		record(sec, freq, argv[4]);
	} else if (*argv[1] == 'P') {
		play(sec, freq, argv[4]);
	} else {
		printf("ERROR: Invalid argument 1 ");
		printf("(Options: Record (R) o Play (P))\n");
		return -EINVAL;
	}

	return 0;
}
