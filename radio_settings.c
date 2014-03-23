/*
 * radio_settings.c - Init and interact with the radio driver
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

#include<fcntl.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<linux/videodev2.h>
#include<alsa/asoundlib.h>
#include<pthread.h>

#include "radio.h"

/* file descriptor of radio device */
static int fd;

static struct v4l2_control control;
static struct v4l2_frequency freq;
static struct v4l2_tuner tuner;
static struct v4l2_hw_freq_seek seek;

/* discover what is the current device */
void set_current_device(void)
{
	snd_mixer_selem_id_t *sid;
	snd_mixer_t *handle;
	snd_mixer_elem_t *elem;

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, "default");
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_set_name(sid, "Line");
	elem = snd_mixer_find_selem(handle, sid);

	/* just Dingoo A320 has the Line attribute */
	current_device = (elem) ? DINGOO_A320 :	GCW;

	printf("Current device == %d\n", current_device);
}

/* this return 1 if the current device is GCW */
int is_gcw()
{
        return current_device == GCW;
}

/* this return 1 if the current device is DINGOO_A320 */
int is_dingooa320()
{
        return current_device == DINGOO_A320;
}

/* init the fd global variable and set the initial config for seek */
void init_controls(void)
{
	fd = open("/dev/radio0", O_RDONLY);

	if (fd < 0) {
		fprintf(stderr, "Radio device /dev/radio0 not found! Aborting.\n");
		exit(1);
	}

	/* initial values for seek */
	seek.tuner = 0;
	seek.type = V4L2_TUNER_RADIO;
	seek.wrap_around = 1;

	set_current_device(); /* discover if we're in a GCW or DINGOO_A320 */
}

/* frequency in MHz */
void setup(float frequency)
{
	/* convert MHz to Hz*/
	int n_freq = (frequency * 1000000) / 62.5;

	control.id = V4L2_CID_AUDIO_MUTE;
	control.value = 0;

	if (ioctl(fd, VIDIOC_S_CTRL, &control) < 0) {
		perror("ioctl: set: mute off");
		fprintf(stderr, "We can't continue without turns mute to off. Aborting.\n");
		exit(1);
	}

	if (ioctl(fd, VIDIOC_G_TUNER, &tuner) < 0) {
		perror("ioctl: set: get tuner");
		fprintf(stderr, "We can't continue without a tuner. Aborting.\n");
		exit(1);
	}

	freq.tuner = 0;
	freq.frequency = n_freq;
	freq.type = V4L2_TUNER_RADIO;

	if (ioctl(fd, VIDIOC_S_FREQUENCY, &freq) < 0) {
		perror("ioctl: set frequency");
		fprintf(stderr, "We can't continue without a frequency. Aborting.\n");
		exit(1);
	}

	control.id = V4L2_CID_AUDIO_VOLUME;
	control.value = 15;

	if (ioctl(fd, VIDIOC_S_CTRL, &control) < 0) {
		perror("ioctl: set volume");
		fprintf(stderr, "Using the default volume level.\n");
	}
}

/* seek for next/previous radio station */
float seek_radio_station(int mode)
{
	if (mode == SEEK_UP) {
		seek.seek_upward = 1;

		if (ioctl(fd, VIDIOC_S_HW_FREQ_SEEK, &seek) < 0) {
			perror("icotl: seek frequency up");
			fprintf(stderr, "Fail to seekup\n");
		}

	} else if (mode == SEEK_DOWN) {
		seek.seek_upward = 0;

		if (ioctl(fd, VIDIOC_S_HW_FREQ_SEEK, &seek) < 0) {
			perror("icotl: seek frequency up");
			fprintf(stderr, "Fail to seekdown\n");
		}
	}

	ioctl(fd, VIDIOC_G_FREQUENCY, &freq);

	return (freq.frequency * 62.5) / 1000000;
}

/* Close all handles and free all allocated memory */
void set_down(void)
{
	control.id = V4L2_CID_AUDIO_MUTE;
	control.value = 1;

	if (ioctl(fd, VIDIOC_S_CTRL, &control) < 0) {
		perror("radio disable");
		fprintf(stderr, "Failed to disable the radio");
	}

	fprintf(stdout, "Exiting..bye!\n");
}

void mixer_control(int mode, long *volume, long *min, long *max)
{
	int localMode = 0;

	if (mode == VOLUME_GET)
		localMode = (is_gcw()) ? PLAYBACK_VOLUME_GET : CAPTURE_VOLUME_GET;
	else if (mode == VOLUME_SET)
		localMode = (is_gcw()) ? PLAYBACK_VOLUME_SET : CAPTURE_VOLUME_SET;
	else if (mode == BYPASS_TURN_OFF)
		localMode = (is_gcw()) ? HEADPHONE_TURN_OFF : BYPASS_PLAYBACK_OFF;
		/* In GCW this could be SPEAKER_TURN_OFF too*/
	else if (mode == BYPASS_VERIFICATION)
		localMode = BYPASS_VERIFICATION;

	/* GCW only */
	else if (mode == HEADPHONE_TURN_ON)
		localMode = HEADPHONE_TURN_ON;
	else if (mode == HEADPHONE_TURN_OFF)
		localMode = HEADPHONE_TURN_OFF;
	else if (mode == SPEAKER_TURN_ON)
		localMode = SPEAKER_TURN_ON;
	else if (mode == SPEAKER_TURN_OFF)
		localMode = SPEAKER_TURN_OFF;

	/* Dingoo A320 only */
	else if (mode == TURN_ON)
		localMode = CAPTURE_TURN_ON;
	else if (mode == TURN_OFF)
		localMode = CAPTURE_TURN_OFF;

	if (is_gcw())
		mixer_control_gcw(localMode, volume, min, max);

	else if (is_dingooa320())
		mixer_control(localMode, volume, min, max);
}

/* Controls the alsamixer atributes of GCW device */
void mixer_control_gcw(int mode, long *volume, long *min, long *max)
{
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	snd_mixer_elem_t *elem;

	snd_mixer_selem_channel_id_t channel = SND_MIXER_SCHN_FRONT_LEFT;

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, "default");
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);

	snd_mixer_selem_id_set_name(sid, "Line In");
	elem = snd_mixer_find_selem(handle, sid);

	if (mode == HEADPHONE_TURN_ON || mode == SPEAKER_TURN_ON) {
		printf("Line In: Turned on\n");
		int err = snd_mixer_selem_set_capture_switch_all(elem, 2);
		if (err < 0)
			printf("Error set line in: %s\n", snd_strerror(err));

	} else if (mode == HEADPHONE_TURN_OFF || mode == SPEAKER_TURN_OFF) {
		printf("Line In: Turned off\n");
		int err = snd_mixer_selem_set_capture_switch_all(elem, 1);
		if (err < 0)
			printf("Error set line in: %s\n", snd_strerror(err));

	} else if (mode == BYPASS_VERIFICATION) {
		int ret;
		snd_mixer_selem_get_capture_switch(elem, channel, &ret);
		printf("Line In Get: %d\n", ret);
	}

	if (mode == PLAYBACK_VOLUME_GET || mode == PLAYBACK_VOLUME_SET) {
		snd_mixer_selem_id_set_name(sid, "Headphone");
		elem = snd_mixer_find_selem(handle, sid);
	
		if (mode == PLAYBACK_VOLUME_GET) {
			snd_mixer_selem_get_playback_volume(elem, channel, volume);
			snd_mixer_selem_get_playback_volume_range(elem, min, max);

		} else if (mode == PLAYBACK_VOLUME_SET) {
			printf("GCW: Volume set to %ld\n", *volume);
			snd_mixer_selem_set_playback_volume_all(elem, *volume);
		}

		/* adjust volume to Bypass too */
		snd_mixer_selem_id_set_name(sid, "Line In Bypass");
		elem = snd_mixer_find_selem(handle, sid);

		if (mode == PLAYBACK_VOLUME_SET) {
			printf("Line in Bypass: Volume set to %ld\n", *volume);
			snd_mixer_selem_set_playback_volume_all(elem, *volume);
		}

	} else if (mode == HEADPHONE_TURN_ON || mode == HEADPHONE_TURN_OFF) {
		snd_mixer_selem_id_set_name(sid, "Headphone Source");
		elem = snd_mixer_find_selem(handle, sid);

		if (mode == HEADPHONE_TURN_ON) {
			printf("Headphone Source: Line In\n");
			snd_mixer_selem_set_enum_item(elem, channel, 2);
		} else if (mode == HEADPHONE_TURN_OFF) {
			printf("Headphone Source: PCM\n");
			snd_mixer_selem_set_enum_item(elem, channel, 3);
		}

	} else if (mode == SPEAKER_TURN_ON || mode == SPEAKER_TURN_OFF) {
		snd_mixer_selem_id_set_name(sid, "Speakers");
		elem = snd_mixer_find_selem(handle, sid);

		if (mode == SPEAKER_TURN_ON) {
			printf("Speaker turned on\n");
			snd_mixer_selem_set_playback_switch_all(elem, 1);
		} else if (mode == SPEAKER_TURN_OFF) {
			printf("Speaker turned off\n");
			snd_mixer_selem_set_playback_switch_all(elem, 0);
		}

		snd_mixer_selem_id_set_name(sid, "Line Out Source");
		elem = snd_mixer_find_selem(handle, sid);

		if (mode == SPEAKER_TURN_ON) {
			printf("Line Out Source turned on\n");
			snd_mixer_selem_set_enum_item(elem, channel, 2);
		} else if (mode == SPEAKER_TURN_OFF) {
			printf("Line Out Source turned off\n");
			snd_mixer_selem_set_enum_item(elem, channel, 3);
		}
	}
}

/* Controls the alsamixer atributes */
void mixer_control_a320(int mode, long *volume, long *min, long *max)
{
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	snd_mixer_elem_t *elem;

	snd_mixer_selem_channel_id_t channel = SND_MIXER_SCHN_SIDE_LEFT;

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, "default");
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);

	/* Controls volume */
	if (mode == CAPTURE_VOLUME_SET || mode == CAPTURE_VOLUME_GET) {
		snd_mixer_selem_id_set_name(sid, "Master");

		elem = snd_mixer_find_selem(handle, sid);
	
		if (mode == CAPTURE_VOLUME_GET) {
			snd_mixer_selem_get_capture_volume(elem, channel, volume);
			snd_mixer_selem_get_capture_volume_range(elem, min, max);

		} else if (mode == CAPTURE_VOLUME_SET){
			printf("Volume set to %ld\n", *volume);
			snd_mixer_selem_set_capture_volume_all(elem, *volume);
		}

	/* Turn on the Line control */
	} else if (mode == CAPTURE_TURN_ON || mode == CAPTURE_TURN_OFF) {
		snd_mixer_selem_id_set_name(sid, "Line");

		elem = snd_mixer_find_selem(handle, sid);

		if (mode == CAPTURE_TURN_ON) {
			snd_mixer_selem_set_capture_switch_all(elem, 1);
		} else if (mode == CAPTURE_TURN_OFF) {
			snd_mixer_selem_set_capture_switch_all(elem, 0);
		}

	/* Active bypass for play the radio directly from input */
	} else if (mode == BYPASS_PLAYBACK_ON || mode == BYPASS_PLAYBACK_OFF) {
		snd_mixer_selem_id_set_name(sid, "Output Mixer Bypass");

		elem = snd_mixer_find_selem(handle, sid);

		if (mode == BYPASS_PLAYBACK_ON) {
			snd_mixer_selem_set_playback_switch_all(elem, 1);
		} else if (mode == BYPASS_PLAYBACK_OFF) {
			snd_mixer_selem_set_playback_switch_all(elem, 0);

		}

	/* Verify if the bypass is turned on */
	} else if (mode == BYPASS_VERIFICATION) {
		snd_mixer_selem_id_set_name(sid, "Output Mixer Bypass");

		elem = snd_mixer_find_selem(handle, sid);
		/* use volume parameter to return the value */
		snd_mixer_selem_get_playback_switch(elem, channel, (int *)volume);
	}

	snd_mixer_close(handle);
}
