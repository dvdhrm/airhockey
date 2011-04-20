/*
 * airhockey - main header
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <stdlib.h>

#include <SFML/Window/OpenGL.h>

struct object {
	GLuint bufs[4];
	GLuint num;
};

extern void obj_grab(struct object *obj);
extern void obj_draw(struct object *obj);

extern GLuint shader_compile();
extern void shader_link(GLuint shader);
