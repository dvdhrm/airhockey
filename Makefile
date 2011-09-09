#
# airhockey - makefile
# Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
# Dedicated to the Public Domain
#

BINARY=airhockey.bin
HEADERS=include/engine3d.h include/log.h include/main.h include/world.h

SRCS=src/log.c src/main.c src/misc.c
SRCS+=src/3d_main.c src/3d_obj.c src/3d_shader.c src/3d_window.c

CFLAGS=-O0 -Wall -g -Iinclude
LFLAGS=-Wall -lGLU -lcsfml-window

OBJS=$(addsuffix .o, $(basename $(SRCS)))

.PHONY: all build clean

all: build

build: $(BINARY)

src/%.o: src/%.c
	gcc -o $@ $< -c $(CFLAGS)

$(BINARY): $(OBJS)
	gcc -o $@ $(OBJS) $(LFLAGS)

clean:
	@rm -fv $(BINARY) $(OBJS)

$(OBJS): Makefile
$(OBJS): $(HEADERS)
