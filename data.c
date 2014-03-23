/* 
 * data.c - The interface between the application and user preferences
 *          This file will handle the information about the user, like
 *	    your last radio station, volume level and others preferences
 *
 * Author: Marcos Paulo de Souza <marcos.souza.org@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#include "data.h"

/* Path to the radio player dir */
static char path[255];

/* verify if the home/.radioplayer dir exists */
static int verify_dir(void)
{
	char *home = getenv("HOME");
	DIR *dir;

	if (home) {
		sprintf(path, "%s/%s", home, ".radioplayer");

		dir = opendir(path);

		if (dir) {
			closedir(dir);
		} else {
			/* creates the .radioplayer is it don't exist */
			return mkdir(path, 0777);
		}

		return 0;
	}

	return 1;
}

/* get the last frequency that the user was listening before turn off */
void handle_user_freq(int mode, float *freq)
{
	char aux_path[255];
	char line[7];
	FILE *file = NULL;

	if (path[0] != '0') {

		sprintf(aux_path, "%s/%s", path, "last_freq");

		if (mode == FILE_FREQ_READ) {
			file = fopen(aux_path, "r");

			if (!file) {
				*freq = 0;
				return;
			}

			fgets(line, 7, file);
			sscanf(line, "%f", freq);

		} else if (mode == FILE_FREQ_WRITE) {
			file = fopen(aux_path, "w");
		
			if (!file) {
				fprintf(stderr, "Cannot store the curr freq!\n");
				return;
			}

			fprintf(file, "%.1f", *freq);
		}

		if (file)
			fclose(file);
	}
}

/* save/restore sound level */
void handle_sound_level(int mode, long *volume)
{
	char aux_path[255], line[3];
	FILE *file = NULL;

	if (path[0] != '0') {
		sprintf(aux_path, "%s/%s", path, "last_volume");

		if (mode == FILE_VOLUME_READ) {
			file = fopen(aux_path, "r");

			/* let the volume parameter as it was */
			if (!file)
				return;

			fgets(line, 3, file);
			sscanf(line, "%ld", volume);

		} else if (mode == FILE_VOLUME_WRITE) {
			file = fopen(aux_path, "w");

			if (!file) {
				fprintf(stderr, "Cannot set the actual volume level!\n");
				return;
			}

			fprintf(file, "%ld", *volume);
		}
		if (file)
			fclose(file);
	}
}

void handle_mode(int mode, int *value)
{
	char aux_path[255], aux_mode[2];
	FILE *file = NULL;

	if (path[0] != '0') {
		sprintf(aux_path, "%s/%s", path, "last_mode");
		if (mode == MODE_GET) {
			file = fopen(aux_path, "r");

			if (!file)
				return;

			fgets(aux_mode, 2, file);
			sscanf(aux_mode, "%d", value);

			printf("got %s\n", aux_mode);
			printf("mode get == %d\n", *value);

		} else if (mode == MODE_SET) {
			file = fopen(aux_path, "w");

			if (!file) {
				fprintf(stderr, "Cannot set the actual mode!\n");
				return;
			}

			fprintf(file, "%i", *value);
			printf("mode set == %d\n", *value);
		}

		if (file)
			fclose(file);
	}
}

/* Set the path global variable */
int set_home_path()
{
	memset(path, 0, sizeof(path));
	return verify_dir();
}
