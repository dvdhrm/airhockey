/*
 * airhockey - 3D engine - initialization
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

/* this forces glext.h to define all extension prototypes */
#define GL_GLEXT_PROTOTYPES

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <SFML/OpenGL.h>

#include "engine3d.h"
#include "log.h"

struct ulog_dev *e3d_log = NULL;

struct e3d_functions e3d_gl = {
	.glCreateProgram = glCreateProgram,
	.glDeleteProgram = glDeleteProgram,
	.glCreateShader = glCreateShader,
	.glDeleteShader = glDeleteShader,
	.glShaderSource = glShaderSource,
	.glCompileShader = glCompileShader,
	.glGetShaderiv = glGetShaderiv,
	.glAttachShader = glAttachShader,
	.glLinkProgram = glLinkProgram,
	.glGetProgramiv = glGetProgramiv,
	.glBindAttribLocation = glBindAttribLocation,
	.glGetUniformLocation = glGetUniformLocation,
	.glUseProgram = glUseProgram,
	.glUniformMatrix4fv = glUniformMatrix4fv,
	.glUniform4fv = glUniform4fv,
	.glUniform1i = glUniform1i,
	.glUniform3f = glUniform3f,
	.glUniform4f = glUniform4f,
	.glBindBuffer = glBindBuffer,
	.glBufferData = glBufferData,
	.glGenBuffers = glGenBuffers,
	.glEnableVertexAttribArray = glEnableVertexAttribArray,
	.glVertexAttribPointer = glVertexAttribPointer,
	.glDisableVertexAttribArray = glDisableVertexAttribArray,
};

void e3d_init(struct ulog_dev *log)
{
	e3d_log = ulog_ref(log);
}

void e3d_destroy()
{
	ulog_unref(e3d_log);
	e3d_log = NULL;
}

/*
 * This checks the OpenGL error queue for errors and prints a warning if an
 * error was encountered.
 */
void e3d_etest() {
	GLenum error;

	error = glGetError();
	if (error == GL_NO_ERROR)
		return;

	ulog_flog(e3d_log, ULOG_WARN, "OpenGL error %d\n", error);
}
