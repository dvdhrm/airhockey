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

static int init_debug_shader(struct e3d_shader *shader)
{
	int ret;
	static const cstr vert = CSTR_STATIC("./shader/debug.vert");
	static const cstr frag = CSTR_STATIC("./shader/debug.frag");

	ret = compile_shader(shader, GL_VERTEX_SHADER, &vert);
	if (ret)
		return ret;
	ret = compile_shader(shader, GL_FRAGMENT_SHADER, &frag);
	if (ret)
		return ret;

	E3D(glBindAttribLocation(shader->program, 0, "a_Vertex"));
	shader->loc.attr.vertex = 0;
	E3D(glBindAttribLocation(shader->program, 1, "a_Color"));
	shader->loc.attr.color = 1;
	E3D(glBindAttribLocation(shader->program, 2, "a_Normal"));
	shader->loc.attr.normal = 2;

	ret = link_shader(shader);
	if (ret)
		return ret;

	shader->loc.uni.mod_mat = E3D(glGetUniformLocation(shader->program,
							"modelview_matrix"));
	if (shader->loc.uni.mod_mat == -1)
		ulog_flog(e3d_log, ULOG_WARN, "Shader: Cannot find "
					"modelview_matrix uniform location\n");
	shader->loc.uni.proj_mat = E3D(glGetUniformLocation(shader->program,
							"projection_matrix"));
	if (shader->loc.uni.proj_mat == -1)
		ulog_flog(e3d_log, ULOG_WARN, "Shader: Cannot find "
				"projectionview_matrix uniform location\n");
	shader->loc.uni.nor_mat = E3D(glGetUniformLocation(shader->program,
							"normal_matrix"));
	if (shader->loc.uni.nor_mat == -1)
		ulog_flog(e3d_log, ULOG_WARN, "Shader: Cannot find "
					"normal_matrix uniform location\n");

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
	ulog_flog(e3d_log, ULOG_DEBUG, "Shader: Activating shader %p\n",
									shader);
}
