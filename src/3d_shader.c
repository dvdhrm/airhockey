/*
 * airhockey - 3D engine - shader handler
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SFML/OpenGL.h>

#include "engine3d.h"
#include "main.h"

struct e3d_shader {
	GLuint program;
};

static struct e3d_shader *create_shader()
{
	struct e3d_shader *shader;
	GLuint program;

	program = E3D(glCreateProgram());
	if (!program) {
		e3d_flog(ULOG_DEBUG, "Shader: Cannot create GL program\n");
		return NULL;
	}

	shader = malloc(sizeof(*shader));
	if (!shader) {
		e3d_flog(ULOG_DEBUG, "Shader: Cannot allocate memory\n");
		E3D(glDeleteProgram(program));
		return NULL;
	}

	shader->program = program;
	return shader;
}

static void free_shader(struct e3d_shader *shader)
{
	E3D(glDeleteProgram(shader->program));
	free(shader);
}

static int compile_shader(struct e3d_shader *shader, GLint type,
							const char *path)
{
	int ret;
	char *out;
	size_t size;
	GLuint obj;
	GLint result;
	GLchar *vsh;
	GLint vnum;

	obj = E3D(glCreateShader(type));
	if (!obj)
		return -EINVAL;

	ret = misc_load_file(path, &out, &size);
	if (ret) {
		e3d_flog(ULOG_DEBUG, "Shader: Cannot load shader file\n");
		E3D(glDeleteShader(obj));
		return ret;
	}

	vsh = out;
	vnum = size;

	E3D(glShaderSource(obj, 1, (const GLchar**)&vsh, &vnum));
	E3D(glCompileShader(obj));
	E3D(glGetShaderiv(obj, GL_COMPILE_STATUS, &result));
	free(vsh);

	if (result == GL_FALSE) {
		e3d_flog(ULOG_DEBUG, "Shader: Cannot compile shader\n");
		E3D(glDeleteShader(obj));
		return -EINVAL;
	}

	/*
	 * Attach shader to program file and then mark shader for deletion.
	 * It gets deleted when it is detached from every program.
	 */
	E3D(glAttachShader(shader->program, obj));
	E3D(glDeleteShader(obj));

	return 0;
}

static int link_shader(struct e3d_shader *shader)
{
	GLint result;

	E3D(glLinkProgram(shader->program));
	E3D(glGetProgramiv(shader->program, GL_LINK_STATUS, &result));
	if (result == GL_FALSE)
		return -EINVAL;

	return 0;
}

static int init_debug_shader(struct e3d_shader *shader)
{
	int ret;

	ret = compile_shader(shader, GL_VERTEX_SHADER, "./shader/debug.vert");
	if (ret)
		return ret;
	ret = compile_shader(shader, GL_FRAGMENT_SHADER, "./shader/debug.frag");
	if (ret)
		return ret;

	E3D(glBindAttribLocation(shader->program, 0, "a_Vertex"));
	E3D(glBindAttribLocation(shader->program, 1, "a_Color"));
	E3D(glBindAttribLocation(shader->program, 2, "a_Normal"));

	ret = link_shader(shader);
	if (ret)
		return ret;

	return 0;
}

int e3d_shader_new(struct e3d_shader **shader, enum e3d_shader_type type)
{
	int ret;

	*shader = create_shader();
	if (!*shader)
		return -ENOMEM;

	switch (type) {
		case E3D_SHADER_DEBUG:
			ret = init_debug_shader(*shader);
			break;
		case E3D_SHADER_GOOCH:
			ret = -EINVAL;
			break;
		default:
			e3d_flog(ULOG_FATAL, "Shader: Invalid shader type\n");
	}

	if (ret) {
		e3d_flog(ULOG_ERROR, "Shader: Cannot create shader\n");
		free_shader(*shader);
	}

	return ret;
}

void e3d_shader_free(struct e3d_shader *shader)
{
	free_shader(shader);
}

GLint e3d_shader_uniform(struct e3d_shader *shader, const char *name)
{
	return E3D(glGetUniformLocation(shader->program, name));
}

void e3d_shader_use(struct e3d_shader *shader)
{
	E3D(glUseProgram(shader->program));
}
