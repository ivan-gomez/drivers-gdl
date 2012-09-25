#include <alsa/asoundlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PARAMS 5

void init_alsa_driver(unsigned char play_record,char *file_to_open,unsigned int sample_freq,unsigned int volume);
