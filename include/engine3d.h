/*
 * airhockey - 3D engine
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SFML/OpenGL.h>

struct e3d_functions {
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
};

extern struct e3d_functions e3d_gl;

extern void e3d_abort();
extern void e3d_die(const char *format, ...);
extern void e3d_vlog(const char *format, va_list list);
extern void e3d_log(const char *format, ...);
extern void e3d_init();
extern void e3d_destroy();
extern void e3d_etest();

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

static inline void e3d_shader_attrib(struct e3d_shader *shader, GLuint loc, const GLchar *name)
{
	e3d_gl.BindAttribLocation(shader->program, loc, name);
}

static inline GLint e3d_shader_uniform(struct e3d_shader *shader, const GLchar *name)
{
	return e3d_gl.GetUniformLocation(shader->program, name);
}

static inline void e3d_shader_use(struct e3d_shader *shader)
{
	e3d_gl.UseProgram(shader->program);
}
