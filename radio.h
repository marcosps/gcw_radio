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

/* wrapper function to call GCW settings or Dingoo A320 settings */
void mixer_control(int mode, long *volume, long *min, long *max);

void mixer_control_gcw(int mode, long *volume, long *min, long *max);
void mixer_control_a320(int mode, long *volume, long *min, long *max);

float seek_radio_station(int mode);

void init_controls(void);

/* Modes to interact with the mixer interface */
enum mixer_modes {
        /* Used by the wrapper */
	VOLUME_GET,          /* Get actual volume */
        VOLUME_SET,          /* Set actual volume */
	TURN_ON,             /* Turn on the Line In */
	TURN_OFF,            /* Turn off the Line In */
	BYPASS_TURN_OFF,     /* Turn off the bypass */

	/* Used by Dingoo A320 */
	CAPTURE_VOLUME_GET,  /* Get the actual volume */
	CAPTURE_VOLUME_SET,  /* Set the volume with a new value */
	CAPTURE_TURN_ON,     /* Turn on the radio capture mode */
	CAPTURE_TURN_OFF,    /* Turn off the radio capture mode */
	BYPASS_PLAYBACK_ON,  /* Play the radio without capture/playback*/
	BYPASS_PLAYBACK_OFF, /* Stop the bypass */
	BYPASS_VERIFICATION,  /* Verificate if the bypass is turned on */

	/* GCW ONLY */
	PLAYBACK_VOLUME_GET,  /* Get actual volume */
	PLAYBACK_VOLUME_SET,  /* Set actual volume */
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

	/* GCW only */
	HEADPHONE,
	SPEAKER
};

/* suported devices */
enum device {
	DINGOO_A320,
	GCW
};

/* Current Device - This could be DINGOO_A320 or GCW */
int current_device;

/* this return 1 if the current device is GCW */
int is_gcw();

/* this return 1 if the current device is DINGOO_A320 */
int is_dingooa320();
