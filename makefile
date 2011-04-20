#
# airhockey - makefile
# Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
# Dedicated to the Public Domain
#

.PHONY: all clean

all:
	gcc -o airhockey src/*.c -lcsfml-window -lGLU -Iinclude -Wall

clean:
	rm -f airhockey
