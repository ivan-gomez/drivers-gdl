#include <alsa/asoundlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Number of parameters needed to achieve proper functionality.*/
#define PARAMETERSNEEDED 6

/* Function called to execute records or playbacks using alsadriver and entry parameters */
/* @param play_record is a int value to specify if a record (1) is needed or a playback command (0).*/
/* @param File_to_open is a string value to tell the driver in which file to save or get data.*/
/* @param  sample_freq tells the driver in which sample rate will the record or play will be executed. */
/* @param volume: alsa driver playback volume (0-10) */
/* @param  samplingTimeInSec: max seconds to reproduce or record. */
void alsa_driver_do_command(unsigned char play_record, char *file_to_open, unsigned int sample_freq, unsigned int volume, unsigned char samplingTimeInSec);

