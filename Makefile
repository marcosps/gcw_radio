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

VERSION=v0.3.1

.PHONY: all bin build clean

all: build bin

build: radio

radio: $(FILES)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf radio radio_player radio_player.opk

bin: build
	mkdir radio_player
	cp radio radio.png Fiery_Turk.ttf README default.gcw0.desktop radio_player
	mksquashfs radio_player radio_player.opk -all-root -noappend -no-exports -no-xattrs
	rm -rf radio_player
