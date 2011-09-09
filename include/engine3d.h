/*
 * airhockey - 3D engine
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#ifndef E3D_ENGINE3D_H
#define E3D_ENGINE3D_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SFML/OpenGL.h>

#include "log.h"

struct e3d_functions {
	PFNGLCREATEPROGRAMPROC glCreateProgram;
	PFNGLDELETEPROGRAMPROC glDeleteProgram;
	PFNGLCREATESHADERPROC glCreateShader;
	PFNGLDELETESHADERPROC glDeleteShader;
	PFNGLSHADERSOURCEPROC glShaderSource;
	PFNGLCOMPILESHADERPROC glCompileShader;
	PFNGLGETSHADERIVPROC glGetShaderiv;
	PFNGLATTACHSHADERPROC glAttachShader;
	PFNGLLINKPROGRAMPROC glLinkProgram;
	PFNGLGETPROGRAMIVPROC glGetProgramiv;
	PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
	PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
	PFNGLUSEPROGRAMPROC glUseProgram;
	PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
	PFNGLBINDBUFFERPROC glBindBuffer;
	PFNGLBUFFERDATAPROC glBufferData;
	PFNGLGENBUFFERSPROC glGenBuffers;
	PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
	PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
	PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
};

extern struct e3d_functions e3d_gl;
#define E3D(func) (e3d_gl.func)

extern void e3d_etest();
extern void e3d_set_log(struct ulog_dev *log);
extern void e3d_flog(int sev, const char *format, ...);

struct e3d_window;

extern struct e3d_window *e3d_window_init();
extern void e3d_window_destroy(struct e3d_window *wnd);
extern bool e3d_window_poll(struct e3d_window *wnd);
extern int64_t e3d_window_elapsed(struct e3d_window *wnd);
extern void e3d_window_frame(struct e3d_window *wnd);

struct e3d_shader;

enum e3d_shader_type {
	E3D_SHADER_DEBUG,
	E3D_SHADER_GOOCH
};

extern int e3d_shader_new(struct e3d_shader **shader,
						enum e3d_shader_type type);
extern void e3d_shader_free(struct e3d_shader *shader);
extern GLint e3d_shader_uniform(struct e3d_shader *shader, const char *name);
extern void e3d_shader_use(struct e3d_shader *shader);

struct e3d_obj;

extern int e3d_obj_new(struct e3d_obj **obj, const char *file);
extern void e3d_obj_free(struct e3d_obj *obj);

extern void e3d_obj_grab(struct e3d_obj *obj);
extern void e3d_obj_draw(struct e3d_obj *obj);

#endif /* E3D_ENGINE3D_H */
