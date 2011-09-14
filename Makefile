#
# airhockey - Makefile
# Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
# Dedicated to the Public Domain
#

BINARY=airhockey.bin
HEADERS=include/engine3d.h include/log.h include/main.h include/world.h
HEADERS+=include/mathw.h

SRCS=src/log.c src/main.c src/misc.c src/config.c src/game.c
SRCS+=src/3d_main.c src/3d_shape.c src/3d_shader.c src/3d_window.c
SRCS+=src/mathw.cpp

CFLAGS=-O0 -Wall -g -Iinclude
LFLAGS=-Wall -lGLU -lcsfml-window -luconf -lcstr -lm -lplibsg -lplibul

OBJS=$(addsuffix .o, $(basename $(SRCS)))

.PHONY: all build clean

all: build

build: $(BINARY)

src/%.o: src/%.c
	gcc -o $@ $< -c $(CFLAGS)

src/mathw.o: src/mathw.cpp
	g++ -o $@ $< -c $(CFLAGS) -I/usr/include/plib

$(BINARY): $(OBJS)
	gcc -o $@ $(OBJS) $(LFLAGS)

clean:
	@rm -fv $(BINARY) $(OBJS)

$(OBJS): Makefile
$(OBJS): $(HEADERS)
