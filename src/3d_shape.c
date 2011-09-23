/*
 * airhockey - 3D engine - shapes
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "engine3d.h"
#include "log.h"
#include "main.h"
#include "mathw.h"

int e3d_shape_new(struct e3d_shape **shape)
{
	struct e3d_shape *s;

	assert(shape);

	s = malloc(sizeof(*s));
	if (!s)
		return -ENOMEM;

	memset(s, 0, sizeof(*s));
	s->ref = 1;
	math_m4_identity(s->alter);

	*shape = s;
	return 0;
}

void e3d_shape_ref(struct e3d_shape *shape)
{
	assert(shape);
	shape->ref++;
	assert(shape->ref);
}

void e3d_shape_unref(struct e3d_shape *shape)
{
	struct e3d_shape *tmp;

	if (!shape)
		return;

	assert(shape->ref);

	if (--shape->ref)
		return;

	while (shape->childs) {
		tmp = shape->childs;
		shape->childs = tmp->next;
		e3d_shape_unref(tmp);
	}

	e3d_primitive_unref(shape->prim);
	free(shape);
}

void e3d_shape_link(struct e3d_shape *parent, struct e3d_shape *shape)
{
	assert(parent);
	assert(shape);
	assert(!shape->next);

	shape->next = parent->childs;
	parent->childs = shape;
	e3d_shape_ref(shape);
}

void e3d_shape_set_primitive(struct e3d_shape *shape,
						struct e3d_primitive *prim)
{
	e3d_primitive_unref(shape->prim);
	shape->prim = prim;
	e3d_primitive_ref(shape->prim);
}

void e3d_shape_draw(const struct e3d_shape *shape, int drawer,
	const struct e3d_shader_locations *loc, struct e3d_transform *trans)
{
	const struct e3d_shape *iter;

	math_stack_push(&trans->mod_stack);
	math_m4_mult(MATH_TIP(&trans->mod_stack), (void*)shape->alter);

	if (shape->prim)
		e3d_primitive_draw(shape->prim, drawer, loc, trans);

	for (iter = shape->childs; iter; iter = iter->next)
		e3d_shape_draw(iter, drawer, loc, trans);

	math_stack_pop(&trans->mod_stack);
}

void e3d_shape_debug(struct e3d_shape *shape)
{
	struct e3d_shape *iter;

	ulog_flog(e3d_log, ULOG_DEBUG, "Debug shape %p\n", shape);

	ulog_flog(e3d_log, ULOG_DEBUG, "Shape %p primitive %p\n", shape,
								shape->prim);
	if (shape->prim)
		e3d_primitive_debug(shape->prim);

	ulog_flog(e3d_log, ULOG_DEBUG, "Shape %p childs:\n", shape);
	for (iter = shape->childs; iter; iter = iter->next)
		e3d_shape_debug(iter);

	ulog_flog(e3d_log, ULOG_DEBUG, "End of shape %p debug\n", shape);
}

/*
 * This is a free implementation of the gluLookAt() function. See the wiki at
 * opengl.org for details on the algorithm.
 * This resets the destination matrix so previous modifications are dropped.
 */
static void look_at(math_m4 m, math_v3 pos, math_v3 at, math_v3 up)
{
	math_v3 f, u, s;

	math_v3_sub_dest(f, at, pos);
	math_v3_normalize(f);

	math_v3_product_dest(s, f, up);
	math_v3_normalize(s);
	math_v3_product_dest(u, s, f);

	m[0][0] = s[0];
	m[1][0] = s[1];
	m[2][0] = s[2];
	m[3][0] = 0.0f;

	m[0][1] = u[0];
	m[1][1] = u[1];
	m[2][1] = u[2];
	m[3][1] = 0.0f;

	m[0][2] = -f[0];
	m[1][2] = -f[1];
	m[2][2] = -f[2];
	m[3][2] = 0.0f;

	m[0][3] = 0.0f;
	m[1][3] = 0.0f;
	m[2][3] = 0.0f;
	m[3][3] = 1.0f;

	math_m4_translate(m, -pos[0], -pos[1], -pos[2]);
}

void e3d_eye_init(struct e3d_eye *eye)
{
	math_v4_copy(eye->position, (math_v4){ 0.0f, 0.0f, 0.0f, 1.0f });
	math_m4_identity(eye->matrix);
}

void e3d_eye_destroy(struct e3d_eye *eye)
{
}

void e3d_eye_reset(struct e3d_eye *eye)
{
	e3d_eye_init(eye);
}

/*
 * Rotates the eye on the given angle and axis. This preserves previous
 * conversions so you should call *_reset() if you do not want this.
 */
void e3d_eye_rotate(struct e3d_eye *eye, float angle, math_v3 axis)
{
	math_m4 m;
	math_q4 q;

	math_q4_rotate(q, angle, axis);
	math_q4_to_m4(q, m);
	math_m4_mult(eye->matrix, m);
}

/*
 * This resets previous transformations. It tries to simluate the gluLookAt()
 * call. Please see its documentation for more information.
 */
void e3d_eye_look_at(struct e3d_eye *eye, math_v3 pos, math_v3 at, math_v3 up)
{
	e3d_eye_init(eye);
	math_v3_copy(eye->position, pos);
	eye->position[3] = 1.0f;
	look_at(eye->matrix, pos, at, up);
}

void e3d_eye_apply(const struct e3d_eye *eye, math_m4 m)
{
	math_m4_mult(m, (void*)eye->matrix);
}

void e3d_eye_supply(const struct e3d_eye *eye,
					const struct e3d_shader_locations *loc)
{
	E3D(glUniform4fv(loc->uni[E3D_U_CAM_POS], 1, (void*)eye->position));
}

void e3d_light_init(struct e3d_light *light)
{
	math_m4_identity(light->matrix);
}

void e3d_light_look_at(struct e3d_light *light, math_v3 pos, math_v3 at,
								math_v3 up)
{
	look_at(light->matrix, pos, at, up);
}

void e3d_light_supply(const struct e3d_light *light, size_t num,
					const struct e3d_shader_locations *loc)
{
	math_m4 t_mat;

	E3D(glUniform1i(loc->uni[E3D_U_LIGHT0_ENABLED], 1));
	E3D(glUniform3f(loc->uni[E3D_U_LIGHT0_COLOR], 1.0, 1.0, 1.0));
	E3D(glUniformMatrix4fv(loc->uni[E3D_U_LIGHT0_MAT], 1, 0,
							(void*)light->matrix));

	math_m4_invert_dest(t_mat, (void*)light->matrix);
	E3D(glUniformMatrix4fv(loc->uni[E3D_U_LIGHT0_MAT_IT], 1, 0,
								(void*)t_mat));
}
