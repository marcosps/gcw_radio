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

#include <alsa/asoundlib.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "radio.h"

/* file descriptor of radio device */
static int fd;

static struct v4l2_control control;
static struct v4l2_frequency freq;
static struct v4l2_tuner tuner;
static struct v4l2_hw_freq_seek seek;

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
}

void set_frequency(float frequency)
{
	/* convert MHz to Hz*/
	int n_freq = (frequency * 1000000) / 62.5;

	freq.tuner = 0;
	freq.frequency = n_freq;
	freq.type = V4L2_TUNER_RADIO;

	if (ioctl(fd, VIDIOC_S_FREQUENCY, &freq) < 0) {
		perror("ioctl: set frequency");
		fprintf(stderr, "We can't continue without a frequency. Aborting.\n");
		exit(1);
	}
}

/* frequency in MHz */
void setup(float frequency)
{
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

	set_frequency(frequency);

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

/* Controls the alsamixer atributes of GCW device */
void mixer_control(int mode, long *volume, long *min, long *max)
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

	if (mode == VOLUME_GET || mode == VOLUME_SET) {
		snd_mixer_selem_id_set_name(sid, "Headphone");
		elem = snd_mixer_find_selem(handle, sid);
	
		if (mode == VOLUME_GET) {
			snd_mixer_selem_get_playback_volume(elem, channel, volume);
			snd_mixer_selem_get_playback_volume_range(elem, min, max);
		} else if (mode == VOLUME_SET) {
			printf("GCW: Volume set to %ld\n", *volume);
			snd_mixer_selem_set_playback_volume_all(elem, *volume);
		}

		/* adjust volume to Bypass too */
		snd_mixer_selem_id_set_name(sid, "Line In Bypass");
		elem = snd_mixer_find_selem(handle, sid);

		if (mode == VOLUME_SET) {
			printf("Line in Bypass: Volume set to %ld\n", *volume);
			snd_mixer_selem_set_playback_volume_all(elem, *volume);
		}

	} else if (mode == HEADPHONE_TURN_ON || mode == HEADPHONE_TURN_OFF) {
		snd_mixer_selem_id_set_name(sid, "Headphone Source");
		elem = snd_mixer_find_selem(handle, sid);

		if (mode == HEADPHONE_TURN_ON) {
			printf("Headphone Source: Line In\n");
			snd_mixer_selem_set_enum_item(elem, channel, 1);
		} else if (mode == HEADPHONE_TURN_OFF) {
			printf("Headphone Source: PCM\n");
			snd_mixer_selem_set_enum_item(elem, channel, 0);
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
			snd_mixer_selem_set_enum_item(elem, channel, 1);
		} else if (mode == SPEAKER_TURN_OFF) {
			printf("Line Out Source turned off\n");
			snd_mixer_selem_set_enum_item(elem, channel, 0);
		}
	} else if (mode == BYPASS_VERIFICATION) {
		// volume here means that the radio is running in background or not
		*volume = 0;

		unsigned int setting = 0;
		snd_mixer_selem_id_set_name(sid, "Headphone Source");
		elem = snd_mixer_find_selem(handle, sid);

		snd_mixer_selem_get_enum_item(elem, channel, &setting);

		// the headphone is turned on
		if (setting == 1) {
			*volume = 1;
			return;
		}

		snd_mixer_selem_id_set_name(sid, "Line Out Source");
		elem = snd_mixer_find_selem(handle, sid);

		snd_mixer_selem_get_enum_item(elem, channel, &setting);

		// if both headphone and speakers are off, return 0 in volume to tell the screen that we need 
		// to setup the radio
		*volume = setting == 1;
	}
}
