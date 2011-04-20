/*
 * airhockey - 3D engine
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <stdbool.h>
#include <stdlib.h>
#include <SFML/Window/OpenGL.h>

/*
 * General engine functions
 */

static inline bool e3d_etest() {
	GLenum error;

	error = glGetError();
	if (error == GL_NO_ERROR)
		return false;
	return true;
}

/*
 * Initialization and Deinitialization
 */
extern struct e3d_functions {
	void (*test)(void);
} e3d_gl;

extern bool e3d_init();
extern void e3d_deinit();

/*
 * Shaders
 */

struct e3d_shader {
	GLuint program;
};

enum e3d_shader_type {
	E3D_SHADER_VERT,
	E3D_SHADER_FRAG
};

extern struct e3d_shader *e3d_shader_new();
extern void e3d_shader_free(struct e3d_shader *shader);

extern bool e3d_shader_compile(struct e3d_shader *shader, enum e3d_shader_type type, const char *path);
extern bool e3d_shader_link(struct e3d_shader *shader);
