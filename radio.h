/* 
 * radio.h - useful definitions for screen.c and radio_settings.c
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

void setup(float frequency);
void set_down(void);

void mixer_control(int mode, long *volume, long *min, long *max);

float seek_radio_station(int mode);

void init_controls(void);

/* Modes to interact with the mixer interface */
enum mixer_modes {
	VOLUME_GET,          /* Get actual volume */
        VOLUME_SET,          /* Set actual volume */

	BYPASS_VERIFICATION,  /* Verificate if the bypass is turned on */

	HEADPHONE_TURN_ON,    /* Turn on the LineIn to play radio */
	HEADPHONE_TURN_OFF,   /* Turn off the LineIn to play radio */
	SPEAKER_TURN_ON,      /* Turn on speakers */
	SPEAKER_TURN_OFF      /* Turn off speakers */
};

/* all available modes for draw in the screen, or
   interact with the radio */
enum modes {
	STARTUP,
	VOLUME_UP,
	VOLUME_DOWN,
	WIDTH_CENTER,
	HEIGHT_CENTER,
	SEEK_UP,
	SEEK_DOWN,
};
