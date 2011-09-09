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

static struct ulog_dev *def_log = NULL;

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
	.glBindBuffer = glBindBuffer,
	.glBufferData = glBufferData,
	.glGenBuffers = glGenBuffers,
	.glEnableVertexAttribArray = glEnableVertexAttribArray,
	.glVertexAttribPointer = glVertexAttribPointer,
	.glDisableVertexAttribArray = glDisableVertexAttribArray,
};

/*
 * This checks the OpenGL error queue for errors and prints a warning if an
 * error was encountered.
 */
void e3d_etest() {
	GLenum error;

	error = glGetError();
	if (error == GL_NO_ERROR)
		return;

	e3d_flog(ULOG_WARN, "Warning: OpenGL error %d\n", error);
}

void e3d_set_log(struct ulog_dev *log)
{
	def_log = log;
}

void e3d_flog(int sev, const char *format, ...)
{
	va_list list;

	va_start(list, format);
	ulog_vlog(def_log, sev, format, list);
	va_end(list);
}
