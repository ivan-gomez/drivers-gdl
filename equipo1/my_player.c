#include "my_player.h"

int main(int argc, char *argv[])
{
	unsigned int volume, sample_freq;
	unsigned char play_record, samplingTimeInSec;
	if (argc < PARAMETERSNEEDED && *argv[1] != 'H') {
		printf("ERROR: Enter all the needed parameters use H as entry to see more info.\n");
		return -1;
	}
	if (*argv[1] == 'R') {
		printf("Record\n");
		play_record = 1;
	} else if (*argv[1] == 'P') {
		printf("Play\n");
		play_record = 0;
	} else if (*argv[1] == 'H') {
		printf("Parameters needed are:\n");
		printf("- R to record or P to play.\n");
		printf("- 0 to 10 to establish volume level.\n");
		printf("- 0 to 65535 to establish sampling rate.\n");
		printf("- file name to save or sound data to play.\n");
		printf("- integer value to specify time in seconds to play or record.\n");
		return 0;
	} else {
		printf("ERROR: error in entry command, use R, P or H\n");
		return -1;
	}
	volume = (unsigned int) atoi(argv[2]);
	if (volume < 0 || volume > 10) {
		printf("ERROR: volume needs to be between 0 and 10\n");
		return -1;
	}
	sample_freq = (unsigned int) atoi(argv[3]);
	if (sample_freq < 0 || sample_freq > 65535) {
		printf("Frecuency out of range ( 0 - 65535)\n");
		return -1;
	}
	samplingTimeInSec = (unsigned int) atoi(argv[5]);
	if (samplingTimeInSec < 0 || samplingTimeInSec > 30) {
		printf("sampling time out of range ( 0 - 30)\n");
		return -1;
	} else {
	/*Default sampling time*/
		samplingTimeInSec = 5;
	}
	printf("Frecuency: %d\n", sample_freq);
	printf("Volume: %d\n", volume);
	printf("Time: %d\n", samplingTimeInSec);
	printf("File name: %s\n", argv[4]);
	alsa_driver_do_command(play_record, argv[4], sample_freq, volume, samplingTimeInSec);
	printf("Finished running my_player\n");
	return 0;
}

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

void alsa_driver_do_command(unsigned char play_record, char *file_to_open, unsigned int sample_freq, unsigned int volume, unsigned char samplingTimeInSec)
{
	unsigned int direction, status, size;
	unsigned long loops;
	char *buffer;
	FILE *id;
	SetAlsaMasterVolume(volume*10);
	mixer_element = snd_mixer_first_elem(mixer);
	snd_mixer_selem_set_playback_volume_range(mixer_element, 0, 10);
	snd_mixer_selem_set_playback_volume_all(mixer_element, 5);
	frames = 32;
	printf("Opening %s...\n", file_to_open);
	if (play_record == 0)
		id = fopen(file_to_open, "r");
	else
		id = fopen(file_to_open, "w");
	printf("file open\n");
	printf("Opening device driver...\n");
	if (id != 0) {
		if (play_record == 0) {
			status = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
		} else if (play_record == 1) {
			status = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
		} else if (play_record != 1 || play_record != 0) {
			printf("ERROR occured while opening device driver.\n");
			return;
		}
		if (status < 0) {
			printf("ERROR occured while opening device driver.\n");
			exit(1);
		}
		printf("Device open\n");
		/* Allocate a hardware parameters object. */
		snd_pcm_hw_params_alloca(&params);
		/* Fill it in with default values. */
		snd_pcm_hw_params_any(handle, params);
		/* Set the desired hardware parameters. */
		/* Interleaved mode */
		snd_pcm_hw_params_set_access(handle,  params, SND_PCM_ACCESS_RW_INTERLEAVED);
		/* Signed 16-bit little-endian format */
		snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
		/* Two channels (stereo) */
		snd_pcm_hw_params_set_channels(handle, params, 2);
		/* sample_freq bits/second sampling rate */
	snd_pcm_hw_params_set_rate_near(handle, params, &sample_freq, &direction);
		/* Set period size to 32 frames. */
		frames = 32;
		snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &direction);
		/* Write the parameters to the driver */
		printf("Setting hardware driver..\n");
		status = snd_pcm_hw_params(handle, params);
		if (status < 0) {
			fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(status));
			exit(1);
		}
		printf("hardware parameters loaded correctly\n");
		/* Use a buffer large enough to hold one period */
		snd_pcm_hw_params_get_period_size(params, &frames, &direction);
		size = frames * 4; /* 2 bytes/sample, 2 channels */
		buffer = (char *) malloc(size);
		/* We want to loop for samplingTimeInSec seconds */
		snd_pcm_hw_params_get_period_time(params, &sample_freq, &direction);
		loops  = 1000000 / sample_freq;
		loops *= samplingTimeInSec;
		printf("Hardware ready\n");
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
		printf("closing device..\n");
		snd_pcm_drain(handle);
		snd_pcm_close(handle);
		printf("Releasing memory..\n");
		free(buffer);
		fclose(id);
	} else {
		printf("Error while accesing file.");
	}
}

