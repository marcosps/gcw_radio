#
# Makefile - main makefile of radio player
#
# Author: Marcos Paulo de Souza <marcos.souza.org@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

CC=mipsel-linux-gcc
SYSROOT=$(shell $(CC) --print-sysroot)
CFLAGS=-Wall -lasound -lSDL_image `$(SYSROOT)/usr/bin/sdl-config --cflags --libs` \
	-lSDL_ttf -O2 -fomit-frame-pointer -ffunction-sections -ffast-math \
	-fsingle-precision-constant -G0 -g
LDFLAGS = -Wl,--gc-sections
FILES=radio_settings.c screen.c data.c

VERSION=v0.2.1

.PHONY: all bin build clean

all: build bin

build: radio.dge

radio.dge: $(FILES)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf radio.dge radio_player radio_player_$(VERSION).tar.gz

bin: build
	mkdir radio_player
	cp radio.dge radio.png Fiery_Turk.ttf README radio_player
	tar -zcf radio_player_$(VERSION).tar.gz radio_player/
	rm -rf radio_player
