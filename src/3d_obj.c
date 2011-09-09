/*
 * airhockey - object creator
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <SFML/OpenGL.h>

#include "engine3d.h"
#include "main.h"

struct e3d_obj {
	GLuint bufs[4];
	GLuint num;
};

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
	E3D(glBindBuffer(type, buf));
	E3D(glBufferData(type, size, arr, GL_DYNAMIC_DRAW));
}

int e3d_obj_new(struct e3d_obj **obj, const char *f)
{
	*obj = malloc(sizeof(**obj));
	if (!*obj)
		return -ENOMEM;

	return 0;
}

void e3d_obj_free(struct e3d_obj *obj)
{
	free(obj);
}

void e3d_obj_grab(struct e3d_obj *obj)
{
	E3D(glGenBuffers(4, obj->bufs));
	load_buffer(GL_ARRAY_BUFFER, obj->bufs[0], sizeof(vert), vert);
	load_buffer(GL_ARRAY_BUFFER, obj->bufs[1], sizeof(col), col);
	load_buffer(GL_ARRAY_BUFFER, obj->bufs[2], sizeof(norm), norm);
	load_buffer(GL_ELEMENT_ARRAY_BUFFER, obj->bufs[3], sizeof(indi), indi);
	obj->num = num;
}

void e3d_obj_draw(struct e3d_obj *obj)
{
	E3D(glEnableVertexAttribArray(0));
	E3D(glEnableVertexAttribArray(1));
	E3D(glEnableVertexAttribArray(2));

	E3D(glBindBuffer(GL_ARRAY_BUFFER, obj->bufs[0]));
	E3D(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL));
	E3D(glBindBuffer(GL_ARRAY_BUFFER, obj->bufs[1]));
	E3D(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL));
	E3D(glBindBuffer(GL_ARRAY_BUFFER, obj->bufs[2]));
	E3D(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL));

	E3D(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->bufs[3]));
	glDrawElements(GL_TRIANGLES, obj->num, GL_UNSIGNED_INT, NULL);

	E3D(glDisableVertexAttribArray(0));
	E3D(glDisableVertexAttribArray(1));
	E3D(glDisableVertexAttribArray(2));
}
