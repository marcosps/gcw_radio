/* 
 * data.h - Definitions that data.c will share with other modules
 *
 * Author: Marcos Paulo de Souza <marcos.souza.org@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* modes for acessing the data files */
enum file_modes {
	FILE_FREQ_READ,
	FILE_FREQ_WRITE,
	FILE_VOLUME_READ,
	FILE_VOLUME_WRITE,
	FILE_FAVRAD_READ,
	FILE_FAVRAD_WRITE,
	FILE_FAVRAD_DELETE,
	MODE_GET,
	MODE_SET
};

/* save and retrieve last freq from last_freq file */
void handle_user_freq(int mode, float *freq);

/* save and retrieve last volume level from last_volume file */
void handle_sound_level(int mode, long *volume);

/* save the last mode, if it was in speakers or headphone */
void handle_mode(int mode, int *value);

/* handle favorite radios, ir we want to add or maybe remove radio stations */
void handle_fav_radios(int mode, char *value, int pos);


int set_home_path();
