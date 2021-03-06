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

#define VOLUME_BAR_X_POS (WIDTH - VOLUME_RECT_WIDTH)

/* All rectangles of volume control */
SDL_Rect rects[32];
int colors[32][3];

TTF_Font *freq_font = NULL;
SDL_Surface *freq_info = NULL;

TTF_Font *shortcut_font = NULL;
SDL_Surface *shortcut_info = NULL;

TTF_Font *seek_mode_font = NULL;
SDL_Surface *seek_mode_info = NULL;

TTF_Font *fav_rad_font = NULL;
SDL_Surface *fav_rad_info = NULL;

TTF_Font *desc_fav_rad_font = NULL;
SDL_Surface *desc_fav_rad_info = NULL;

SDL_Surface *screen;

/* Verify if the user want to listen radio in
 * background, or just finish the app
 */
int end_application = 1;

/* Default seek mode is auto */
int seek_mode = SEEK_AUTO;

/* Current fav radio selected */
int curr_fav = 0;

/* currect frequency */
float curr_freq = 0;

/* Font color used in all text rendered */
static SDL_Color font_color = {255, 255, 255};

/* All rects to show favorite radios */
static SDL_Rect favrad_rects[5] = {
					{.x = 10, .y = 30, .w = 50, .h = 30},
					{.x = 65, .y = 30, .w = 50, .h = 30},
					{.x = 120, .y = 30, .w = 50, .h = 30},
					{.x = 175, .y = 30, .w = 50, .h = 30},
					{.x = 230, .y = 30, .w = 50, .h = 30}
};

static SDL_Rect favrad_rects_border[5] = {
					{.x = 12, .y = 32, .w = 46, .h = 26},
					{.x = 67, .y = 32, .w = 46, .h = 26},
					{.x = 122, .y = 32, .w = 46, .h = 26},
					{.x = 177, .y = 32, .w = 46, .h = 26},
					{.x = 232, .y = 32, .w = 46, .h = 26}
};

/* blit to the screen */
void apply_surface(int x, int y, SDL_Surface *font, SDL_Surface *screen)
{
	SDL_Rect tmp;

	tmp.x = x;
	tmp.y = y;

	SDL_BlitSurface(font, NULL, screen, &tmp);
	SDL_Flip(screen);
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

	Uint32 color = SDL_MapRGB(screen->format, colors[vol][0], colors[vol][1], colors[vol][2]);

	switch (mode) {
	case STARTUP:
		while (i <= vol) {
			SDL_FillRect(screen, &rects[i], color);
			i++;
		}
		break;
	case VOLUME_DOWN:
		color = SDL_MapRGB(screen->format, 0, 0, 0);
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
	TTF_CloseFont(seek_mode_font);
	TTF_CloseFont(fav_rad_font);

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
	seek_mode_font = TTF_OpenFont("Fiery_Turk.ttf", 14);
	fav_rad_font = TTF_OpenFont("Fiery_Turk.ttf", 10);
	desc_fav_rad_font = TTF_OpenFont("Fiery_Turk.ttf", 10);

	/* put all available shortcuts in the screen */
	if (!shortcut_font)
		fprintf(stderr, "Cannot find ttf Turk/6!\n");
	else {
		SDL_Color color = {255, 255, 255};

		char *message = "Up: Vol+ | Down: Vol- | L: Seek Prv | R: Seek Next | Sel+Start: Exit";
		shortcut_info = TTF_RenderText_Solid(shortcut_font, message, color);
		apply_surface(0, 190, shortcut_info, screen);

		message = "B: Run in background | Y: Switch between Headphone or Speakers";
		shortcut_info = TTF_RenderText_Solid(shortcut_font, message, color);
		apply_surface(0, 200, shortcut_info, screen);

		message = "Start: Change seek mode | X: Add favo radio | A: Rem favo radio";
		shortcut_info = TTF_RenderText_Solid(shortcut_font, message, color);
		apply_surface(0, 210, shortcut_info, screen);

		message = "Select: Set favorite radio to play";
		shortcut_info = TTF_RenderText_Solid(shortcut_font, message, color);
		apply_surface(0, 220, shortcut_info, screen);
	}
}

/* Show to user what is the current frequency */
void print_freq(float freq, int searching)
{
	if (freq_font) {

		/* Remove the old frequency from the screen */
		SDL_Rect tmp_rect;

		tmp_rect.x = 80;
		tmp_rect.y = 100;
		tmp_rect.w = 200;
		tmp_rect.h = 50;
	
		Uint32 color = SDL_MapRGB(screen->format, 0, 0, 0);
		SDL_FillRect(screen, &tmp_rect, color);

		int line = 138;
		char freq_char[13];

		sprintf(freq_char, "%.1f", freq);

		if (searching) {
			line = 80;
			strcpy(freq_char, "Searching...");
		}

		freq_info = TTF_RenderText_Solid(freq_font, freq_char, font_color);
	
		apply_surface(line, (HEIGHT - 28) / 2, freq_info, screen);
	}
}

/* Show frequency when seek mode is manual */
static void get_next_frequency(int seek_type)
{
	if (seek_type == SEEK_UP) {
		curr_freq += .10;
		if (curr_freq > 108.0)
			curr_freq = 78.0;
	} else if (seek_type == SEEK_DOWN) {
		curr_freq -= .10;
		if (curr_freq < 76.5)
			curr_freq = 108.0;
	}
}

/* To be able to draw borders, we need to draw a bigger rect, and after a small one
 * filled by black color */
static void draw_favrads_rects()
{
	Uint32 black_color = SDL_MapRGB(screen->format, 0, 0, 0);
	Uint32 green_color = SDL_MapRGB(screen->format, 0, 255, 0);
	Uint32 white_color = SDL_MapRGB(screen->format, 255, 255, 255);

	int i = 0;

	for (i = 0; i < 5; i++) {
		/* Selected favorite radio has green border */
		if (i == curr_fav)
			SDL_FillRect(screen, &favrad_rects[i], green_color);
		else
			SDL_FillRect(screen, &favrad_rects[i], white_color);

		SDL_FillRect(screen, &favrad_rects_border[i], black_color);

		printf("Radio %s\n", favrads.radio[i]);

		int freq_size = strlen(favrads.radio[i]);

		char freq[6];

		if (freq_size == 1)
			strcpy(freq, "-");
		else
			strcpy(freq, favrads.radio[i]);

		/* Draw favorite radio into rect */
		desc_fav_rad_info = TTF_RenderText_Solid(desc_fav_rad_font, freq, font_color);
		apply_surface(favrad_rects[i].x + 10, 35, desc_fav_rad_info, screen);
	}
	SDL_Flip(screen);
}

static void draw_favrads_label()
{
	fav_rad_info = TTF_RenderText_Solid(fav_rad_font, "Favorite Radios", font_color);
	apply_surface(0, 0, fav_rad_info, screen);
}

/* Show the seek mode in the screen */
static void show_seek_mode()
{
	char smode[20];
	int pos;

	if (seek_mode == SEEK_MANUAL) {
		strcpy(smode, "Seek manual");
		pos = 123;
	} else {
		strcpy(smode, "Seek automatic");
		pos = 113;
	}

	/* Remove the old seek mode from the screen */
	SDL_Rect tmp_rect;
	Uint32 color = SDL_MapRGB(screen->format, 0, 0, 0);

	tmp_rect.x = 100;
	tmp_rect.y = 150;
	tmp_rect.w = 140;
	tmp_rect.h = 40;

	SDL_FillRect(screen, &tmp_rect, color);

	seek_mode_info = TTF_RenderText_Solid(seek_mode_font, smode, font_color);
	apply_surface(pos, 150, seek_mode_info, screen);
}

int main(int argc, char* argv[])
{
	SDL_Event event;
  
	int keypress = 0, lock = 0;
	long vol, min, max, ret = 0;

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
	handle_user_freq(FILE_FREQ_READ, &curr_freq);

	if (curr_freq == 0) {
		fprintf(stdout, "Using default radio 76.5\n");
		curr_freq = 76.5;
		/* save as the default radio */
		handle_user_freq(FILE_FREQ_WRITE, &curr_freq);

	} else {
		if (curr_freq < 76.5 || curr_freq > 108.0) {
			fprintf(stderr, "%s %f %s", "Frequency ", curr_freq,
					" out of range(76.5.9 <> 108.0)! Using the freq 76.5.\n");
			curr_freq = 76.5;
			/* save as the default radio */
			handle_user_freq(FILE_FREQ_WRITE, &curr_freq); 
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

	/* get the actual volume, the min and max volume range */
	mixer_control(VOLUME_GET, &vol, &min, &max);

	/* verify if the radio is running in background */
	mixer_control(BYPASS_VERIFICATION, &ret, NULL, NULL);

	int mode = HEADPHONE_TURN_ON;
	/* we can get HEADPHONE or SPEAKER from handle */
	handle_mode(MODE_GET, &mode);

	/* if the radio is running in background, don't set the 
	 * same things again
         */
	if (!ret) {
		/* Initialize the radio by the driver */
		setup(curr_freq);

		/* Set the flag to turn on the capture line */
		mixer_control(mode, &vol, &min, &max);

		/* if we don't have the last_volume file, use the default */
		handle_sound_level(FILE_VOLUME_READ, &vol);

		/* set the sound volume */
		mixer_control(VOLUME_SET, &vol, &min, &max);
	}

	setup_volume_bar();

	/* Draw the volume bar at the init */
	draw_volume_bar(screen, vol, STARTUP);

	handle_fav_radios(FILE_FAVRAD_READ, NULL, 0);

	/* Manage the ttf font */
	load_ttf_font();
	draw_favrads_label();
	print_freq(curr_freq, 0);
	show_seek_mode();
	draw_favrads_rects();

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
						mixer_control(VOLUME_SET, &vol, &min, &max);
						handle_sound_level(FILE_VOLUME_WRITE, &vol);
					}

				} else if (!strcmp(button_pressed, "up")) {
					if (vol + 1 <= max) {
						draw_volume_bar(screen, vol == 0 ? 1 : vol + 1, VOLUME_UP);
						vol++;
						mixer_control(VOLUME_SET, &vol, &min, &max);
						handle_sound_level(FILE_VOLUME_WRITE, &vol);
					}

				/* Change to previous fav radio */
				} else if (!strcmp(button_pressed, "left")) {
					curr_fav = curr_fav > 0 ? curr_fav - 1 : 0;
					draw_favrads_rects();

				/* Change to next fav radio */
				} else if (!strcmp(button_pressed, "right")) {
					curr_fav = curr_fav >= 4 ? curr_fav : curr_fav + 1;
					draw_favrads_rects();

				/* the R button -> Seek Next */
				} else if (!strcmp(button_pressed, "backspace")) {
					if (seek_mode == SEEK_AUTO) {
						print_freq(curr_freq, 1);
						curr_freq = seek_radio_station(SEEK_UP);
					} else {
						get_next_frequency(SEEK_UP);
						set_frequency(curr_freq);
					}
					print_freq(curr_freq, 0);
					show_seek_mode();
					handle_user_freq(FILE_FREQ_WRITE, &curr_freq);
				
				/* the L button -> Seek Previous */
				} else if (!strcmp(button_pressed, "tab")) {
					if (seek_mode == SEEK_AUTO) {
						print_freq(curr_freq, 1);
						curr_freq = seek_radio_station(SEEK_DOWN);
					} else {
						get_next_frequency(SEEK_DOWN);
						set_frequency(curr_freq);
					}
					print_freq(curr_freq, 0);
					show_seek_mode();
					handle_user_freq(FILE_FREQ_WRITE, &curr_freq);

				/* Y Button -> Switch between Headphone and Speaker */
				} else if (!strcmp(button_pressed, "space")) {
					if (mode == SPEAKER_TURN_ON) {
						mixer_control(SPEAKER_TURN_OFF, &vol, &min, &max);
						mixer_control(HEADPHONE_TURN_ON, &vol, &min, &max);

						mode = HEADPHONE_TURN_ON;
						handle_mode(MODE_SET, &mode);
					} else {
						mixer_control(HEADPHONE_TURN_OFF, &vol, &min, &max);
						mixer_control(SPEAKER_TURN_ON, &vol, &min, &max);

						mode = SPEAKER_TURN_ON;
						handle_mode(MODE_SET, &mode);
					}

				/* X Button -> Add favorite radio */
				} else if (!strcmp(button_pressed, "left shift")) {
					char char_freq[6];
					sprintf(char_freq, "%.1f", curr_freq);
					handle_fav_radios(FILE_FAVRAD_WRITE, char_freq, curr_fav);
					draw_favrads_rects();

				/* A Button -> Remove favorite radio */
				} else if (!strcmp(button_pressed, "left ctrl")) {
					handle_fav_radios(FILE_FAVRAD_DELETE, "0", curr_fav);
					draw_favrads_rects();

				/* the B button
				 * Just close the application, and let the radio plays
				 * in background
                                 */
				} else if (!strcmp(button_pressed, "left alt")) {
					end_application = 0;
					keypress = 1;

				/* Choose favorite radio */
				} else if (!strcmp(button_pressed, "escape")) {
					if (strcmp(favrads.radio[curr_fav], "0")) {
						curr_freq = atof(favrads.radio[curr_fav]);
						set_frequency(curr_freq);
						print_freq(curr_freq, 0);
						handle_user_freq(FILE_FREQ_WRITE, &curr_freq);
					}

				/* exit when select + start button are pressed */
				} else if (!strcmp(button_pressed, "return")) {
					Uint8 *keyState = SDL_GetKeyState(NULL);

					// if just start is pressed, we are changing the seek mode
					if (keyState[SDLK_ESCAPE])
						keypress = 1;
					else {
						if (seek_mode == SEEK_AUTO)
							seek_mode = SEEK_MANUAL;
						else
							seek_mode = SEEK_AUTO;
						show_seek_mode();
					}
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
