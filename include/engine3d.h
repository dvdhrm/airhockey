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
#include <SFML/Window.h>

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

enum e3d_keys {
	E3D_KEY_A = sfKeyA,
	E3D_KEY_B = sfKeyB,
	E3D_KEY_C = sfKeyC,
	E3D_KEY_D = sfKeyD,
	E3D_KEY_E = sfKeyE,
	E3D_KEY_F = sfKeyF,
	E3D_KEY_G = sfKeyG,
	E3D_KEY_H = sfKeyH,
	E3D_KEY_I = sfKeyI,
	E3D_KEY_J = sfKeyJ,
	E3D_KEY_K = sfKeyK,
	E3D_KEY_L = sfKeyL,
	E3D_KEY_M = sfKeyM,
	E3D_KEY_N = sfKeyN,
	E3D_KEY_O = sfKeyO,
	E3D_KEY_P = sfKeyP,
	E3D_KEY_Q = sfKeyQ,
	E3D_KEY_R = sfKeyR,
	E3D_KEY_S = sfKeyS,
	E3D_KEY_T = sfKeyT,
	E3D_KEY_U = sfKeyU,
	E3D_KEY_V = sfKeyV,
	E3D_KEY_W = sfKeyW,
	E3D_KEY_X = sfKeyX,
	E3D_KEY_Y = sfKeyY,
	E3D_KEY_Z = sfKeyZ,
	E3D_KEY_NUM0 = sfKeyNum0,
	E3D_KEY_NUM1 = sfKeyNum1,
	E3D_KEY_NUM2 = sfKeyNum2,
	E3D_KEY_NUM3 = sfKeyNum3,
	E3D_KEY_NUM4 = sfKeyNum4,
	E3D_KEY_NUM5 = sfKeyNum5,
	E3D_KEY_NUM6 = sfKeyNum6,
	E3D_KEY_NUM7 = sfKeyNum7,
	E3D_KEY_NUM8 = sfKeyNum8,
	E3D_KEY_NUM9 = sfKeyNum9,
	E3D_KEY_ESCAPE = sfKeyEscape,
	E3D_KEY_LCONTROL = sfKeyLControl,
	E3D_KEY_LSHIFT = sfKeyLShift,
	E3D_KEY_LALT = sfKeyLAlt,
	E3D_KEY_LSYSTEM = sfKeyLSystem,
	E3D_KEY_RCONTROL = sfKeyRControl,
	E3D_KEY_RSHIFT = sfKeyRShift,
	E3D_KEY_RALT = sfKeyRAlt,
	E3D_KEY_RSYSTEM = sfKeyRSystem,
	E3D_KEY_MENU = sfKeyMenu,
	E3D_KEY_LBRACKET = sfKeyLBracket,
	E3D_KEY_RBRACKET = sfKeyRBracket,
	E3D_KEY_SEMICOLON = sfKeySemiColon,
	E3D_KEY_COMMA = sfKeyComma,
	E3D_KEY_PERIOD = sfKeyPeriod,
	E3D_KEY_QUOTE = sfKeyQuote,
	E3D_KEY_SLASH = sfKeySlash,
	E3D_KEY_BACKSLASH = sfKeyBackSlash,
	E3D_KEY_TILDE = sfKeyTilde,
	E3D_KEY_EQUAL = sfKeyEqual,
	E3D_KEY_DASH = sfKeyDash,
	E3D_KEY_SPACE = sfKeySpace,
	E3D_KEY_RETURN = sfKeyReturn,
	E3D_KEY_BACK = sfKeyBack,
	E3D_KEY_TAB = sfKeyTab,
	E3D_KEY_PAGEUP = sfKeyPageUp,
	E3D_KEY_PAGEDOWN = sfKeyPageDown,
	E3D_KEY_END = sfKeyEnd,
	E3D_KEY_HOME = sfKeyHome,
	E3D_KEY_INSERT = sfKeyInsert,
	E3D_KEY_DELETE = sfKeyDelete,
	E3D_KEY_ADD = sfKeyAdd,
	E3D_KEY_SUBTRACT = sfKeySubtract,
	E3D_KEY_MULTIPLY = sfKeyMultiply,
	E3D_KEY_DIVIDE = sfKeyDivide,
	E3D_KEY_LEFT = sfKeyLeft,
	E3D_KEY_RIGHT = sfKeyRight,
	E3D_KEY_UP = sfKeyUp,
	E3D_KEY_DOWN = sfKeyDown,
	E3D_KEY_NUMPAD0 = sfKeyNumpad0,
	E3D_KEY_NUMPAD1 = sfKeyNumpad1,
	E3D_KEY_NUMPAD2 = sfKeyNumpad2,
	E3D_KEY_NUMPAD3 = sfKeyNumpad3,
	E3D_KEY_NUMPAD4 = sfKeyNumpad4,
	E3D_KEY_NUMPAD5 = sfKeyNumpad5,
	E3D_KEY_NUMPAD6 = sfKeyNumpad6,
	E3D_KEY_NUMPAD7 = sfKeyNumpad7,
	E3D_KEY_NUMPAD8 = sfKeyNumpad8,
	E3D_KEY_NUMPAD9 = sfKeyNumpad9,
	E3D_KEY_F1 = sfKeyF1,
	E3D_KEY_F2 = sfKeyF2,
	E3D_KEY_F3 = sfKeyF3,
	E3D_KEY_F4 = sfKeyF4,
	E3D_KEY_F5 = sfKeyF5,
	E3D_KEY_F6 = sfKeyF6,
	E3D_KEY_F7 = sfKeyF7,
	E3D_KEY_F8 = sfKeyF8,
	E3D_KEY_F9 = sfKeyF9,
	E3D_KEY_F10 = sfKeyF10,
	E3D_KEY_F11 = sfKeyF11,
	E3D_KEY_F12 = sfKeyF12,
	E3D_KEY_F13 = sfKeyF13,
	E3D_KEY_F14 = sfKeyF14,
	E3D_KEY_F15 = sfKeyF15,
	E3D_KEY_PAUSE = sfKeyPause,
	E3D_KEY_NUM = sfKeyCount
};

enum e3d_event_type {
	E3D_EV_KEY,
	E3D_EV_NUM
};

struct e3d_event_key {
	unsigned int code;
	unsigned int value;
};

struct e3d_event {
	unsigned int type;
	union e3d_event_union {
		struct e3d_event_key key;
	} v;
};

extern struct e3d_window *e3d_window_new();
extern void e3d_window_free(struct e3d_window *wnd);
extern void e3d_window_activate(struct e3d_window *wnd);

extern int e3d_window_poll(struct e3d_window *wnd, struct e3d_event *out);
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
	E3D_SHADER_SIMPLE
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
 * For fast rotations and conversions we keep a transformation matrix with all
 * transformations applied and a separate position and rotation variable.
 * We could get both out of the matrix, but this conversion is expensive.
 * Furthermore, there is probably only one eye in the whole application so we
 * do not care for memory consumption here.
 */

struct e3d_eye {
	math_v4 position;
	math_m4 matrix;
};

extern void e3d_eye_init(struct e3d_eye *eye);
extern void e3d_eye_destroy(struct e3d_eye *eye);
extern void e3d_eye_reset(struct e3d_eye *eye);
extern void e3d_eye_rotate(struct e3d_eye *eye, float angle, math_v3 axis);
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
