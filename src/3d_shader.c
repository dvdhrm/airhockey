/*
 * airhockey - 3D engine - shader handler
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <SFML/Window/OpenGL.h>
#include "engine3d.h"

struct e3d_shader *e3d_shader_new()
{
	struct e3d_shader *shader;
	GLuint program;

	program = glCreateProgram();
	if (!program || e3d_etest())
		return NULL;

	shader = malloc(sizeof(*shader));
	if (!shader) {
		glDeleteProgram(program);
		return NULL;
	}

	shader->program = program;
	return shader;
}

void e3d_shader_free(struct e3d_shader *shader)
{
	glDeleteProgram(shader->program);
	free(shader);
}

/* loads a file into a string, returns string length or 0 on failure */
static GLint load_file(const char *file, GLchar **out) {
	FILE *ffile;
	long int size, lsize;
	char *buf;

	ffile = fopen(file, "rb");
	if(!ffile)
		return 0;

	fseek(ffile, 0, SEEK_END);
	size = ftell(ffile);
	if (size < 1) {
		fclose(ffile);
		return 0;
	}
	rewind(ffile);

	buf = malloc(size + 1);
	if (!buf) {
		fclose(ffile);
		return 0;
	}
	if((lsize = fread(buf, 1, size, ffile)) != size) {
		free(buf);
		fclose(ffile);
		return 0;
	}
	buf[size] = 0;

	*out = buf;

	fclose(ffile);
	return size;
}

bool e3d_shader_compile(struct e3d_shader *shader, enum e3d_shader_type type, const char *path)
{
	GLint result;
	GLuint shaderobj;
	GLchar *vsh;
	GLint vnum;

	switch (type) {
		case E3D_SHADER_VERT:
			shaderobj = glCreateShader(GL_VERTEX_SHADER);
			break;
		case E3D_SHADER_FRAG:
			shaderobj = glCreateShader(GL_FRAGMENT_SHADER);
			break;
		default:
			return false;
	}

	if (!shaderobj || e3d_etest())
		return false;

	vnum = load_file(path, &vsh);
	if (vnum < 1) {
		glDeleteShader(shaderobj);
		return false;
	}

	glShaderSource(shaderobj, 1, &vsh, &vnum);
	glCompileShader(shaderobj);
	glGetShaderiv(shaderobj, GL_COMPILE_STATUS, &result);
	if(result == GL_FALSE || e3d_etest()) {
		free(vsh);
		glDeleteShader(shaderobj);
		return false;
	}
	free(vsh);

	/* Attach shader to program file and then mark shader for deletion. It gets
	 * deleted when it is detached from every program.
	 */
	glAttachShader(shader->program, shaderobj);
	glDeleteShader(shaderobj);
	if (e3d_etest())
		return false;

	return true;
}

bool e3d_shader_link(struct e3d_shader *shader)
{
	GLint result;

	glLinkProgram(shader->program);
	glGetProgramiv(shader->program, GL_LINK_STATUS, &result);
	if(result == GL_FALSE || e3d_etest()) {
		return false;
	}
	return true;
}
