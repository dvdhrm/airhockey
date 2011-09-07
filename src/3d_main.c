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

#include <pthread.h>
#include <SFML/OpenGL.h>

#include "engine3d.h"

/* initialization is protected by a lock and counter */
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static size_t init_num = 0;
struct e3d_functions e3d_gl;

/*
 * Abort the application on error.
 * Also print short message to log.
 */
void e3d_abort()
{
	e3d_log("Fatal Error");
	abort();
}

/*
 * This combines a call to e3d_log and e3d_abort().
 * This is heavily used for fatal errors.
 */
void e3d_die(const char *format, ...)
{
	va_list list;

	va_start(list, format);
	e3d_vlog(format, list);
	va_end(list);
	e3d_abort();
}

/*
 * Prints a message to the log. This is currently just a forward to stderr with
 * a short prefix but may be extended in future to forward it to a logfile.
 */
void e3d_vlog(const char *format, va_list list)
{
	fprintf(stderr, "e3d: ");
	vfprintf(stderr, format, list);
}

/* see e3d_vlog() above */
void e3d_log(const char *format, ...)
{
	va_list list;

	va_start(list, format);
	e3d_vlog(format, list);
	va_end(list);
}

/*
 * Initializes the 3D engine
 * This may be called multiple times for each instance that uses the engine. It
 * keeps an internal counter so the deinit function will only deinitialize the
 * engine when all users have called it.
 */
void e3d_init()
{
	int ret;

	ret = pthread_mutex_lock(&init_lock);
	if (ret)
		e3d_die("Error: Cannot lock mutex\n");

	if (init_num++)
		goto unlock;

	e3d_gl.CreateProgram = glCreateProgram;
	e3d_gl.DeleteProgram = glDeleteProgram;
	e3d_gl.CreateShader = glCreateShader;
	e3d_gl.DeleteShader = glDeleteShader;
	e3d_gl.ShaderSource = glShaderSource;
	e3d_gl.CompileShader = glCompileShader;
	e3d_gl.GetShaderiv = glGetShaderiv;
	e3d_gl.AttachShader = glAttachShader;
	e3d_gl.LinkProgram = glLinkProgram;
	e3d_gl.GetProgramiv = glGetProgramiv;
	e3d_gl.BindAttribLocation = glBindAttribLocation;
	e3d_gl.GetUniformLocation = glGetUniformLocation;
	e3d_gl.UseProgram = glUseProgram;
	e3d_gl.UniformMatrix4fv = glUniformMatrix4fv;
	e3d_gl.BindBuffer = glBindBuffer;
	e3d_gl.BufferData = glBufferData;
	e3d_gl.GenBuffers = glGenBuffers;
	e3d_gl.EnableVertexAttribArray = glEnableVertexAttribArray;
	e3d_gl.VertexAttribPointer = glVertexAttribPointer;
	e3d_gl.DisableVertexAttribArray = glDisableVertexAttribArray;

unlock:
	pthread_mutex_unlock(&init_lock);
}

/* counterpart of e3d_init(), see above */
void e3d_destroy()
{
	int ret;

	ret = pthread_mutex_lock(&init_lock);
	if (ret)
		e3d_die("Error: Cannot lock mutex\n");

	if (--init_num)
		goto unlock;

	memset(&e3d_gl, 0, sizeof(e3d_gl));

unlock:
	pthread_mutex_unlock(&init_lock);
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

	e3d_log("Warning: OpenGL error queue contains err %d\n", error);
}
