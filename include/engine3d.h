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
	PFNGLCREATEPROGRAMPROC CreateProgram;
	PFNGLDELETEPROGRAMPROC DeleteProgram;
	PFNGLCREATESHADERPROC CreateShader;
	PFNGLDELETESHADERPROC DeleteShader;
	PFNGLSHADERSOURCEPROC ShaderSource;
	PFNGLCOMPILESHADERPROC CompileShader;
	PFNGLGETSHADERIVPROC GetShaderiv;
	PFNGLATTACHSHADERPROC AttachShader;
	PFNGLLINKPROGRAMPROC LinkProgram;
	PFNGLGETPROGRAMIVPROC GetProgramiv;
	PFNGLBINDATTRIBLOCATIONPROC BindAttribLocation;
	PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation;
	PFNGLUSEPROGRAMPROC UseProgram;
	PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv;
	PFNGLBINDBUFFERPROC BindBuffer;
	PFNGLBUFFERDATAPROC BufferData;
	PFNGLGENBUFFERSPROC GenBuffers;
	PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
	PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer;
	PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray;
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

static inline bool e3d_shader_attrib(struct e3d_shader *shader, GLuint loc, const GLchar *name)
{
	e3d_gl.BindAttribLocation(shader->program, loc, name);
	if (e3d_etest())
		return false;
	return true;
}

static inline GLint e3d_shader_uniform(struct e3d_shader *shader, const GLchar *name)
{
	GLint ret;

	ret = e3d_gl.GetUniformLocation(shader->program, name);
	if (ret == -1 || e3d_etest())
		return -1;
	return ret;
}

static inline bool e3d_shader_use(struct e3d_shader *shader)
{
	e3d_gl.UseProgram(shader->program);
	if (e3d_etest())
		return false;
	return true;
}
