/*
 * airhockey - 3D engine
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#ifndef E3D_ENGINE3D_H
#define E3D_ENGINE3D_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <libcstr.h>
#include <SFML/OpenGL.h>

#include "log.h"
#include "mathw.h"

/*
 * General OpenGL access
 * This provides access to all opengl functions so we do not have to rely on
 * OpenGL extension loading etc.
 *
 * It also allows to set the global log object that is used by all E3D
 * functions.
 */

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
	PFNGLUNIFORM4FVPROC glUniform4fv;
	PFNGLUNIFORM1IPROC glUniform1i;
	PFNGLUNIFORM3FPROC glUniform3f;
	PFNGLUNIFORM4FPROC glUniform4f;
	PFNGLBINDBUFFERPROC glBindBuffer;
	PFNGLBUFFERDATAPROC glBufferData;
	PFNGLGENBUFFERSPROC glGenBuffers;
	PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
	PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
	PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
};

extern struct e3d_functions e3d_gl;
#define E3D(func) (e3d_gl.func)

extern struct ulog_dev *e3d_log;

extern void e3d_init(struct ulog_dev *log);
extern void e3d_destroy();
extern void e3d_etest();

/*
 * OpenGL context and window creation
 * This provides access to window creation, event polling and frame creation.
 * OpenGL API does not allow multiple contexts to be active simultaneously, so
 * you need to activate the window you want to work with and all other OpenGL
 * stuff is directed to this window's context.
 * Creating a new window automatically activates this window.
 * Context between windows is shared so you may switch windows but still draw
 * the same buffers/textures/etc.
 */

struct e3d_window;

extern struct e3d_window *e3d_window_new();
extern void e3d_window_free(struct e3d_window *wnd);
extern void e3d_window_activate(struct e3d_window *wnd);

extern bool e3d_window_poll(struct e3d_window *wnd);
extern int64_t e3d_window_elapsed(struct e3d_window *wnd);
extern void e3d_window_frame(struct e3d_window *wnd);
extern void e3d_window_projection(const struct e3d_window *wnd, math_m4 m);

/*
 * Shader loader
 * This loads special AirHockey shaders and hides the shader compilation/linking
 * behind this API.
 */

struct e3d_shader;

enum e3d_shader_attribute_ids {
	E3D_A_VERTEX,
	E3D_A_COLOR,
	E3D_A_NORMAL,
	E3D_A_NUM
};

enum e3d_shader_uniform_ids {
	E3D_U_M_MAT,
	E3D_U_M_MAT_IT,
	E3D_U_MPE_MAT,
	E3D_U_CAM_POS,

	E3D_U_LIGHT0_ENABLED,
	E3D_U_LIGHT0_COLOR,
	E3D_U_LIGHT0_MAT,
	E3D_U_LIGHT0_MAT_IT,

	E3D_U_COLOR,

	E3D_U_NUM
};

struct e3d_shader_locations {
	GLint attr[E3D_A_NUM];
	GLint uni[E3D_U_NUM];
};

enum e3d_shader_type {
	E3D_SHADER_DEBUG,
	E3D_SHADER_SIMPLE,
	E3D_SHADER_GOOCH
};

extern int e3d_shader_new(struct e3d_shader **shader,
						enum e3d_shader_type type);
extern void e3d_shader_free(struct e3d_shader *shader);

extern const struct e3d_shader_locations *e3d_shader_locations(
					const struct e3d_shader *shader);
extern void e3d_shader_use(struct e3d_shader *shader);

/*
 * Primitive Shapes
 * Primitives can be drawn with one call and thus are very fast. Every other
 * shape is based on primitives. A primitive can contain several buffers, but
 * only the available buffers are sent to the graphics engine.
 * If an indicies buffer is available, the drawing function will use it to draw
 * the primitive. Otherwise, the whole buffer is drawn as if the indicies where
 * consecutive.
 */

#define E3D_BUFFER_VERTEX	0x01
#define E3D_BUFFER_COLOR	0x02
#define E3D_BUFFER_NORMAL	0x04

struct e3d_transform {
	struct math_stack mod_stack;
	struct math_stack proj_stack;
	struct math_stack eye_stack;
};

struct e3d_buffer {
	size_t ref;
	GLsizei num;
	GLfloat (*vertex)[4];
	GLfloat (*color)[4];
	GLfloat (*normal)[4];
	GLfloat buf[];
};

struct e3d_primitive {
	size_t ref;
	GLuint type;

	struct e3d_buffer *buf;
	GLsizei num;
	GLuint ibuf[];
};

extern void e3d_transform_init(struct e3d_transform *transform);
extern void e3d_transform_destroy(struct e3d_transform *transform);
extern void e3d_transform_reset(struct e3d_transform *transform);

extern struct e3d_buffer *e3d_buffer_new(size_t size, uint8_t type);
extern struct e3d_buffer *e3d_buffer_ref(struct e3d_buffer *buf);
extern void e3d_buffer_unref(struct e3d_buffer *buf);
extern void e3d_buffer_generate_triangle_normals(struct e3d_buffer *buf);

extern struct e3d_primitive *e3d_primitive_new(size_t num);
extern struct e3d_primitive *e3d_primitive_ref(struct e3d_primitive *prim);
extern void e3d_primitive_unref(struct e3d_primitive *prim);

extern void e3d_primitive_set_buffer(struct e3d_primitive *prim,
							struct e3d_buffer *buf);
extern void e3d_primitive_draw(struct e3d_primitive *prim,
	const struct e3d_shader_locations *loc, struct e3d_transform *trans);
extern void e3d_primitive_draw_normals(struct e3d_primitive *prim,
	const struct e3d_shader_locations *loc, struct e3d_transform *trans);
extern void e3d_primitive_draw_silhouette(struct e3d_primitive *prim,
	const struct e3d_shader_locations *loc, struct e3d_transform *trans);

/*
 * Shapes
 * Shapes are lists and trees of primitives. All primitives may be translated,
 * scaled etc. relative to the others.
 * Shapes allow to group several primitives together into one object and draw it
 * with one call.
 * Each primitive may be modified, moved, etc. during runtime so shapes are not
 * limited to static objects, however, animations and similar should be handled
 * on a higher level as this structure does not allow fast traversal and may be
 * used multiple times in one scene for the same object.
 * The e3d_alter structure is used for object translations/scaling/etc.
 * Also rendering order of shapes is random, so a shape should always be a small
 * entity which is considered one static and rigid object.
 */

struct e3d_shape {
	size_t ref;
	struct e3d_shape *next;
	struct e3d_shape *childs;

	math_m4 alter;
	struct e3d_primitive *prim;
};

extern struct e3d_shape *e3d_shape_new();
extern struct e3d_shape *e3d_shape_ref(struct e3d_shape *shape);
extern void e3d_shape_unref(struct e3d_shape *shape);
extern void e3d_shape_link(struct e3d_shape *parent, struct e3d_shape *shape);
extern void e3d_shape_set_primitive(struct e3d_shape *shape,
						struct e3d_primitive *prim);

typedef void (*e3d_shape_drawer) (struct e3d_primitive *prim,
	const struct e3d_shader_locations *loc, struct e3d_transform *trans);

extern void e3d_shape_draw(const struct e3d_shape *shape,
	const struct e3d_shader_locations *loc, struct e3d_transform *trans,
						e3d_shape_drawer drawer);

/*
 * Eye position
 * The eye position allows to move the whole geometry and position the viewer
 * inside the world. This could be done by moving the root object of the world,
 * but to have a more structured API, a separate object is used.
 * This also allows to have the world class only operate in world space and not
 * be confused by eye space.
 */

struct e3d_eye {
	math_v4 position;
	math_m4 matrix;
};

static inline void e3d_eye_init(struct e3d_eye *eye)
{
	eye->position[0] = 0.0f;
	eye->position[1] = 0.0f;
	eye->position[2] = 0.0f;
	eye->position[3] = 1.0f;
	math_m4_identity(eye->matrix);
}

static inline void e3d_eye_destroy(struct e3d_eye *eye)
{
}

extern void e3d_eye_look_at(struct e3d_eye *eye, math_v3 pos, math_v3 at,
								math_v3 up);
extern void e3d_eye_apply(const struct e3d_eye *eye, math_m4 m);
extern void e3d_eye_supply(const struct e3d_eye *eye,
					const struct e3d_shader_locations *loc);

/*
 * Lights
 * Each world can contain several lights. The number of positioned lights is
 * limited and most special lights exist only once in a scene.
 */

struct e3d_light {
	math_m4 matrix;
};

extern void e3d_light_init(struct e3d_light *light);

static inline void e3d_light_destroy(struct e3d_light *light)
{
}

extern void e3d_light_look_at(struct e3d_light *light, math_v3 pos, math_v3 at,
								math_v3 up);
extern void e3d_light_supply(const struct e3d_light *light, size_t num,
					const struct e3d_shader_locations *loc);

#endif /* E3D_ENGINE3D_H */
