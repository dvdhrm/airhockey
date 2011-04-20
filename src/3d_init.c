/*
 * airhockey - 3D engine - initialization
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <stdbool.h>
#include <stdlib.h>
#include <SFML/Window/OpenGL.h>
#include "engine3d.h"

struct e3d_functions e3d_gl;

bool e3d_init()
{
	e3d_gl.test = NULL;
	return true;
}

void e3d_deinit()
{
	e3d_gl.test = NULL;
}
