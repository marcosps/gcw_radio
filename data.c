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

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "data.h"
#include "radio.h"

/* Path to the radio player dir */
static char path[255];

/* Auxiliary path to get the other dirs */
static char aux_path[255];

/* Auxiliary FILE pointer */
FILE *file = NULL;

/* verify if the home/.radioplayer dir exists */
static int verify_dir(void)
{
	char *home = getenv("HOME");
	DIR *dir;

	if (home) {
		sprintf(path, "%s/%s", home, ".radioplayer");
		dir = opendir(path);

		/* creates the .radioplayer is it don't exist */
		if (dir)
			closedir(dir);
		else
			return mkdir(path, 0777);
		return 0;
	}

	return 1;
}

/* get the last frequency that the user was listening before turn off */
void handle_user_freq(int mode, float *freq)
{
	char line[7];

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
	char line[3];

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
	char  aux_mode[2];

	if (path[0] != '0') {
		sprintf(aux_path, "%s/%s", path, "last_mode");
		if (mode == MODE_GET) {
			file = fopen(aux_path, "r");

			if (!file)
				return;

			fgets(aux_mode, 2, file);
			sscanf(aux_mode, "%d", value);
		} else if (mode == MODE_SET) {
			file = fopen(aux_path, "w");

			if (!file) {
				fprintf(stderr, "Cannot set the actual mode!\n");
				return;
			}

			fprintf(file, "%i", *value);
		}

		if (file)
			fclose(file);
	}
}

static void remove_new_line(char *orig)
{
	int i;
	for (i = 0; i < strlen(orig); i++)
		if (orig[i] == '\n')
			orig[i] = '\0';
}

static void write_favradios()
{
	sprintf(aux_path, "%s/%s", path, "favorite_radios");
	file = fopen(aux_path, "w");

	if (file) {
		int i;
		for (i = 0; i < favrads.num_radios; i++)
			fprintf(file, "%s\n", favrads.radio[i]);

		fclose(file);
	}
}

void handle_fav_radios(int mode, char *value)
{
	if (path[0] != '0') {
		sprintf(aux_path, "%s/%s", path, "favorite_radios");
		if (mode == FILE_FAVRAD_READ) {
			file = fopen(aux_path, "r");

			if (!file)
				return;

			static char *line = NULL;
			size_t len;
			ssize_t read;

			favrads.num_radios = 0;

			while ((read = getline(&line, &len, file)) != -1 && favrads.num_radios < 5) {
				remove_new_line(line);
				strncpy(favrads.radio[favrads.num_radios++], line, strlen(line) + 1);
			}
			fclose(file);

		} else if (mode == FILE_FAVRAD_WRITE) {
			if (favrads.num_radios < 5) {
				strcpy(favrads.radio[favrads.num_radios++], value);
				write_favradios();
			}
		} else if (mode == FILE_FAVRAD_DELETE) {
			/*
			int i;
			for (i = 0; i < favrads.num_radios; i++)
				if (!strncmp(favrads.radio[i], value, strlen(value)))
					strcpy(favrads.radio[i], "0");
			*/
		}
	}
}

/* Set the path global variable */
int set_home_path()
{
	memset(path, 0, sizeof(path));
	return verify_dir();
}
