/*
 * airhockey - object creator
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <SFML/Window/OpenGL.h>

#include "main.h"

static GLfloat vert[] = {
	-0.5f, 0.0f, -0.5f,
	-0.5f, 0.0f, 0.5f,
	0.4f, 0.0f, 0.0f,
};
static GLfloat col[] = {
	0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 1.0f,
};
static GLfloat norm[] = {
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
};
static GLuint indi[] = {
	0, 1, 2,
};
static GLuint num = 3;

static void load_buffer(GLuint type, GLuint buf, GLuint size, void *arr)
{
	glBindBuffer(type, buf);
	glBufferData(type, size, arr, GL_DYNAMIC_DRAW);
}

void obj_grab(struct object *obj)
{
	glGenBuffers(4, obj->bufs);
	load_buffer(GL_ARRAY_BUFFER, obj->bufs[0], sizeof(vert), vert);
	load_buffer(GL_ARRAY_BUFFER, obj->bufs[1], sizeof(col), col);
	load_buffer(GL_ARRAY_BUFFER, obj->bufs[2], sizeof(norm), norm);
	load_buffer(GL_ELEMENT_ARRAY_BUFFER, obj->bufs[3], sizeof(indi), indi);
	obj->num = num;
}

void obj_draw(struct object *obj)
{
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, obj->bufs[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, obj->bufs[1]);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, obj->bufs[2]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->bufs[3]);
	glDrawElements(GL_TRIANGLES, obj->num, GL_UNSIGNED_INT, NULL);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}
