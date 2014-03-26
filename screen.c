/* 
 * screen.c - Interact with the user
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
#include <SDL.h>
#include <SDL/SDL_ttf.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "radio.h"
#include "data.h"

#define WIDTH 320
#define HEIGHT 240
#define DEPTH 8

#define VOLUME_RECT_WIDTH 20
#define VOLUME_RECT_HEIGHT 7

#define VOLUME_BAR_DISTANCE 7

#define VOLUME_BAR_X_POS \
	WIDTH - VOLUME_RECT_WIDTH

#define HALF(string, wh, size) \
	((size - (strlen(string) * wh)) / 2)

/* All rectangles of volume control */
SDL_Rect rects[32];
int colors[32][3];

TTF_Font *freq_font = NULL;
SDL_Surface *freq_info = NULL;

TTF_Font *shortcut_font = NULL;
SDL_Surface *shortcut_info = NULL;

SDL_Surface *screen;

/* Verify if the user want to listen radio in
 * background, or just finish the app
 */
int end_application = 1;

/* blit to the screen */
void apply_surface(int x, int y, SDL_Surface *font, SDL_Surface *screen)
{
	SDL_Rect tmp;

	tmp.x = x;
	tmp.y = y;

	SDL_BlitSurface(font, NULL, screen, &tmp);
	SDL_Flip(screen);
}

/* Get the center of width and height */
int get_center(int mode, char *freq, int size)
{
	if (mode == WIDTH_CENTER)
		return (VOLUME_BAR_X_POS - strlen(freq)) / 2;
	else if (mode == HEIGHT_CENTER)
		return (HEIGHT - size) / 2;
	return 0;
}

/* Initial position of each rectangle and init the colors */
void setup_volume_bar()
{
	int i, ypos = HEIGHT - 5;

	for (i = 0; i < 32; i++) {
		colors[i][0] = colors[i][2] = 0;
		colors[i][1] = 255;

		rects[i].x = VOLUME_BAR_X_POS;
		rects[i].y = ypos;
		rects[i].w = VOLUME_RECT_WIDTH;
		rects[i].h = VOLUME_RECT_HEIGHT;

		ypos -= VOLUME_BAR_DISTANCE;
	}
}

/* draw the volume bar */
void draw_volume_bar(SDL_Surface *screen, int vol, int mode)
{
	int i = 0;

	Uint32 color = SDL_MapRGB(screen->format
				, colors[vol][0]
				, colors[vol][1]
				, colors[vol][2]);

	switch (mode) {
	case STARTUP:
		while (i <= vol) {
			SDL_FillRect(screen, &rects[i], color);
			i++;
		}
		break;
	case VOLUME_DOWN:
		color = SDL_MapRGB(screen->format
				, 0
				, 0
				, 0);
		SDL_FillRect(screen, &rects[vol], color);
		break;
	case VOLUME_UP:
		SDL_FillRect(screen, &rects[vol], color);
	}

	SDL_Flip(screen);
}

/* free all allocated memory and structs ant turn off the radio */
static void finish_app()
{
	if (end_application) {
		set_down();

		/* Turn off all modes */
		mixer_control(HEADPHONE_TURN_OFF, NULL, NULL, NULL);
		mixer_control(SPEAKER_TURN_OFF, NULL, NULL, NULL);
	}

	TTF_CloseFont(freq_font);
	TTF_CloseFont(shortcut_font);
	TTF_Quit();
	SDL_Quit();

	exit(0);
}

/* Get the force terminator (Power Slide + Select) sequence */
static void sighandle(int id, siginfo_t *siginfo, void *context)
{
	finish_app();
}

/* Will load all ttf fonts that we need */
void load_ttf_font()
{
	freq_font = TTF_OpenFont("Fiery_Turk.ttf", 28);

	if (!freq_font)
		fprintf(stderr, "Cannot find ttf Turk/28!\n");

	shortcut_font = TTF_OpenFont("Fiery_Turk.ttf", 8);

	/* put all available shortcuts in the screen */
	if (!shortcut_font)
		fprintf(stderr, "Cannot find ttf Turk/6!\n");
	else {
		SDL_Color color = {255, 255, 255};

		char *message = "Up: Vol+ | Down: Vol- | L: Seek Prv | R: Seek Next | Start: Exit";
		shortcut_info = TTF_RenderText_Solid(shortcut_font, message, color);
		apply_surface(0, 210, shortcut_info, screen);

		message = "B: Run in background | Y: Turn on Headphone | A: Turn on Speakers";
		shortcut_info = TTF_RenderText_Solid(shortcut_font, message, color);
		apply_surface(0, 220, shortcut_info, screen);
	}
}

/* Show to user what is the current frequency */
void print_freq(float freq)
{
	char freq_char[6];

	if (freq_font) {

		sprintf(freq_char, "%.1f", freq);

		/* Remove the old frequency from the screen */
		SDL_Rect tmp_rect;
		Uint32 color = SDL_MapRGB(screen->format, 0, 0, 0);

		SDL_Color scolor = {255, 255, 255};

		tmp_rect.x = tmp_rect.y = 0;
		tmp_rect.w = VOLUME_BAR_X_POS;
		tmp_rect.h = screen->h - 40; /* Don't clean the shortcut bar */
	
		SDL_FillRect(screen, &tmp_rect, color);

		freq_info = TTF_RenderText_Solid(freq_font, freq_char, scolor);
	
		apply_surface(138 /* the center for 6 chars */
   			    , get_center(HEIGHT_CENTER, freq_char, 28), freq_info, screen);
	}
}

int main(int argc, char* argv[])
{
	SDL_Event event;
  
	int keypress = 0, lock = 0;
	long vol, min, max, ret = 0;
	float freq = 0;

	char *button_pressed;

	struct sigaction act;
	memset(&act, '\0', sizeof(act));

	act.sa_sigaction = &sighandle;
	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGHUP, &act, NULL) < 0) {
		perror("sigaction");
		return 1;
	}

	/* init home path */
	set_home_path();

	/* get last radio station */
	handle_user_freq(FILE_FREQ_READ, &freq);

	if (freq == 0) {
		fprintf(stdout, "Using default radio 76.5\n");
		freq = 76.5;
		/* save as the default radio */
		handle_user_freq(FILE_FREQ_WRITE, &freq);

	} else {
		if (freq < 76.5 || freq > 108.0) {
			fprintf(stderr, "%s %f %s", "Frequency ", freq,
					" out of range(76.5.9 <> 108.0)! Using the freq 76.5.\n");
			freq = 76.5;
			/* save as the default radio */
			handle_user_freq(FILE_FREQ_WRITE, &freq); 
		}
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Cannot init SDL. Aborting.\n");
		return 1;
   	}

	if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_HWSURFACE | SDL_DOUBLEBUF))) {
		fprintf(stderr, "Cannot SetVideoMode. Aborting.\n");
		SDL_Quit();
		return 1;
	}

	if (TTF_Init() == -1) {
		fprintf(stderr, "Unable to start the TTF. Aborting\n");
		SDL_Quit();
		return 1;
	}

	SDL_ShowCursor(SDL_DISABLE);

	init_controls();

	/* verify if the radio is running in background */
	mixer_control(BYPASS_VERIFICATION, &ret, NULL, NULL);

	/* get the actual volume, the min and max volume range */
	mixer_control(VOLUME_GET, &vol, &min, &max);

	/* if the radio is running in background, don't set the 
	 * same things again
         */
	if (!ret) {
		/* Initialize the radio by the driver */
		setup(freq);

		int mode = HEADPHONE_TURN_ON;

		/* we can get HEADPHONE or SPEAKER from handle */
		handle_mode(MODE_GET, &mode);

		/* Set the flag to turn on the capture line */
		mixer_control(mode, &vol, &min, &max);

		/* if we don't have the last_volume file, use the default */
		handle_sound_level(FILE_VOLUME_READ, &vol);

		/* set the sound volume */
		mixer_control(VOLUME_SET
				, &vol
				, &min
				, &max);
	}

	setup_volume_bar();

	/* Draw the volume bar at the init */
	draw_volume_bar(screen, vol, STARTUP);

	/* Manage the ttf font */
	load_ttf_font();
	print_freq(freq);

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	while(!keypress) {
		while(SDL_WaitEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
			case SDL_KEYDOWN:
				button_pressed = SDL_GetKeyName(event.key.keysym.sym);

				/* lock the screen */
				if (!strcmp(button_pressed, "pause")) {
					lock = 1;
					break;
				}

				/* unlocked screen */
				if (!strcmp(button_pressed, "unknown key"))
					lock = 0;

				/* if the screen is locked, do nothing */
				if (lock)
					break;

				if (!strcmp(button_pressed, "down")) {
					/* avoid negative values */
					if (vol) {
						draw_volume_bar(screen, vol == 1 ? 1 : vol, VOLUME_DOWN);
						vol--;
						mixer_control(VOLUME_SET
								, &vol
								, &min
								, &max);
						handle_sound_level(FILE_VOLUME_WRITE, &vol);
					}

				} else if (!strcmp(button_pressed, "up")) {
					if (vol + 1 <= max) {
						draw_volume_bar(screen, vol == 0 ? 1 : vol + 1, VOLUME_UP);
						vol++;
						mixer_control(VOLUME_SET
								, &vol
								, &min
								, &max);
						handle_sound_level(FILE_VOLUME_WRITE, &vol);
					}

				/* the R button -> Seek Next */
				} else if (!strcmp(button_pressed, "backspace")) {
					freq = seek_radio_station(SEEK_UP);

					print_freq(freq);
					handle_user_freq(FILE_FREQ_WRITE, &freq);
				
				/* the L button -> Seek Previous */
				} else if (!strcmp(button_pressed, "tab")) {
					freq = seek_radio_station(SEEK_DOWN);

					print_freq(freq);
					handle_user_freq(FILE_FREQ_WRITE, &freq);

				/* Y Button -> Turn on Headphone */
				} else if (!strcmp(button_pressed, "space")) {
					mixer_control(SPEAKER_TURN_OFF
							, &vol
							, &min
							, &max);

					mixer_control(HEADPHONE_TURN_ON
							, &vol
							, &min
							, &max);
					int mode = HEADPHONE_TURN_ON;
					handle_mode(MODE_SET, &mode);

				/* A Button - Turn on Speaker */
				} else if (!strcmp(button_pressed, "left ctrl")) {
					mixer_control(HEADPHONE_TURN_OFF
							, &vol
							, &min
							, &max);

					mixer_control(SPEAKER_TURN_ON
							, &vol
							, &min
							, &max);

					int mode = SPEAKER_TURN_ON;
					handle_mode(MODE_SET, &mode);

				/* the B button
				 * Just close the application, and let the radio plays
				 * in background
                                 */
				} else if (!strcmp(button_pressed, "left alt")) {
					end_application = 0;
					keypress = 1;

				/* exit when start button is pressed */
				} else if (!strcmp(button_pressed, "return")) {
					printf("The %s key was pressed!\n",
        	           		SDL_GetKeyName(event.key.keysym.sym));
					keypress = 1;

				} else {
					printf("The %s key was pressed!\n",
					SDL_GetKeyName(event.key.keysym.sym));
				}
			}
			// break the WaitEvent loop
			if (keypress)
				break;
		}
	}

	finish_app();

	return 0;
}
