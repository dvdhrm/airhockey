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
#include "log.h"
#include "main.h"

struct e3d_shader {
	GLuint program;
	struct e3d_shader_locations loc;
};

static struct e3d_shader *create_shader()
{
	struct e3d_shader *shader;
	GLuint program;

	program = E3D(glCreateProgram());
	if (!program) {
		ulog_flog(e3d_log, ULOG_ERROR,
					"Shader: Cannot create GL program\n");
		return NULL;
	}

	shader = malloc(sizeof(*shader));
	if (!shader) {
		ulog_flog(e3d_log, ULOG_ERROR,
					"Shader: Cannot allocate memory\n");
		E3D(glDeleteProgram(program));
		return NULL;
	}

	memset(shader, 0, sizeof(*shader));
	shader->program = program;

	return shader;
}

static void free_shader(struct e3d_shader *shader)
{
	E3D(glDeleteProgram(shader->program));
	free(shader);
}

static int compile_shader(struct e3d_shader *shader, GLint type,
							const cstr *path)
{
	int ret;
	char *out;
	size_t size;
	GLuint obj;
	GLint result;
	GLchar *vsh;
	GLint vnum;

	obj = E3D(glCreateShader(type));
	if (!obj) {
		ulog_flog(e3d_log, ULOG_ERROR,
				"Shader: Cannot create GL shader object\n");
		return -EINVAL;
	}

	ret = misc_load_file(path, &out, &size);
	if (ret) {
		ulog_flog(e3d_log, ULOG_ERROR,
				"Shader: Cannot load shader file %s %d\n",
							CSTR_CHAR(path), ret);
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
		ulog_flog(e3d_log, ULOG_ERROR,
					"Shader: Cannot compile shader\n");
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
	if (result == GL_FALSE) {
		ulog_flog(e3d_log, ULOG_ERROR, "Shader: Cannot link shader\n");
		return -EINVAL;
	}

	return 0;
}

static const char *debug_attrs[E3D_A_NUM] = {
	[E3D_A_VERTEX] = "position_in",
	[E3D_A_COLOR] = "color_in",
	[E3D_A_NORMAL] = "normal_in",
};

static const char *debug_unis[E3D_U_NUM] = {
	[E3D_U_M_MAT] = "m_mat",
	[E3D_U_M_MAT_IT] = "m_mat_it",
	[E3D_U_MPE_MAT] = "mpe_mat",
	[E3D_U_CAM_POS] = "cam_pos",

	[E3D_U_LIGHT0_ENABLED] = "lights[0].enabled",
	[E3D_U_LIGHT0_COLOR] = "lights[0].color",
	[E3D_U_LIGHT0_MAT] = "lights[0].mat",
	[E3D_U_LIGHT0_MAT_IT] = "lights[0].mat_it",
};

static int init_debug_shader(struct e3d_shader *shader)
{
	int ret;
	size_t i;
	static const cstr vert = CSTR_STATIC("./shader/debug.vert");
	static const cstr frag = CSTR_STATIC("./shader/debug.frag");

	ret = compile_shader(shader, GL_VERTEX_SHADER, &vert);
	if (ret)
		return ret;
	ret = compile_shader(shader, GL_FRAGMENT_SHADER, &frag);
	if (ret)
		return ret;

	for (i = 0; i < E3D_A_NUM; ++i) {
		if (debug_attrs[i]) {
			E3D(glBindAttribLocation(shader->program, i,
							debug_attrs[i]));
			shader->loc.attr[i] = i;
		} else {
			shader->loc.attr[i] = -1;
		}
	}

	ret = link_shader(shader);
	if (ret)
		return ret;

	for (i = 0; i < E3D_U_NUM; ++i) {
		if (debug_unis[i]) {
			shader->loc.uni[i] = E3D(glGetUniformLocation(
					shader->program, debug_unis[i]));
			if (shader->loc.uni[i] == -1)
				ulog_flog(e3d_log, ULOG_WARN, "Shader: Cannot "
				"find %s uniform location\n", debug_unis[i]);
		} else {
			shader->loc.uni[i] = -1;
		}
	}

	return 0;
}

static const char *simple_attrs[E3D_A_NUM] = {
	[E3D_A_VERTEX] = "position_in",
};

static const char *simple_unis[E3D_U_NUM] = {
	[E3D_U_MPE_MAT] = "mpe_mat",
	[E3D_U_COLOR] = "color",
};

static int init_simple_shader(struct e3d_shader *shader)
{
	int ret;
	size_t i;
	static const cstr vert = CSTR_STATIC("./shader/simple.vert");
	static const cstr frag = CSTR_STATIC("./shader/simple.frag");

	ret = compile_shader(shader, GL_VERTEX_SHADER, &vert);
	if (ret)
		return ret;
	ret = compile_shader(shader, GL_FRAGMENT_SHADER, &frag);
	if (ret)
		return ret;

	for (i = 0; i < E3D_A_NUM; ++i) {
		if (simple_attrs[i]) {
			E3D(glBindAttribLocation(shader->program, i,
							simple_attrs[i]));
			shader->loc.attr[i] = i;
		} else {
			shader->loc.attr[i] = -1;
		}
	}

	ret = link_shader(shader);
	if (ret)
		return ret;

	for (i = 0; i < E3D_U_NUM; ++i) {
		if (simple_unis[i]) {
			shader->loc.uni[i] = E3D(glGetUniformLocation(
					shader->program, simple_unis[i]));
			if (shader->loc.uni[i] == -1)
				ulog_flog(e3d_log, ULOG_WARN, "Shader: Cannot "
				"find %s uniform location\n", simple_unis[i]);
		} else {
			shader->loc.uni[i] = -1;
		}
	}

	return 0;
}

int e3d_shader_new(struct e3d_shader **out, enum e3d_shader_type type)
{
	int ret;
	const char *name = "<invalid>";
	struct e3d_shader *shader;

	shader = create_shader();
	if (!shader)
		return -ENOMEM;

	switch (type) {
		case E3D_SHADER_DEBUG:
			ret = init_debug_shader(shader);
			name = "debug";
			break;
		case E3D_SHADER_SIMPLE:
			ret = init_simple_shader(shader);
			name = "normals";
			break;
		case E3D_SHADER_GOOCH:
			ret = -EINVAL;
			name = "gooch";
			break;
		default:
			ulog_flog(e3d_log, ULOG_ERROR,
					"Shader: Invalid shader type\n");
			ret = -EINVAL;
			break;
	}

	if (ret) {
		free_shader(shader);
	} else {
		*out = shader;
		ulog_flog(e3d_log, ULOG_DEBUG,
				"Shader: Creating shader %p of type %s\n",
								shader, name);
	}

	return ret;
}

void e3d_shader_free(struct e3d_shader *shader)
{
	free_shader(shader);
	ulog_flog(e3d_log, ULOG_DEBUG, "Shader: Destroying shader %p\n",
									shader);
}

const struct e3d_shader_locations *e3d_shader_locations(
						const struct e3d_shader *shader)
{
	return &shader->loc;
}

void e3d_shader_use(struct e3d_shader *shader)
{
	E3D(glUseProgram(shader->program));
}
