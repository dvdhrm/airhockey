/*
 * airhockey - 3D engine - initialization
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

/* this forces glext.h to define all extension prototypes */
#define GL_GLEXT_PROTOTYPES

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <SFML/Window/OpenGL.h>
#include "engine3d.h"

struct e3d_functions e3d_gl;

bool e3d_init()
{
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
	return true;
}

void e3d_deinit()
{
	memset(&e3d_gl, 0, sizeof(e3d_gl));
}
