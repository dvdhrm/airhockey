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
#include "main.h"
#include "mathw.h"

void e3d_transform_init(struct e3d_transform *transform)
{
	math_stack_init(&transform->mod_stack);
	math_stack_init(&transform->proj_stack);
	math_stack_init(&transform->eye_stack);
}

void e3d_transform_destroy(struct e3d_transform *transform)
{
	math_stack_destroy(&transform->eye_stack);
	math_stack_destroy(&transform->proj_stack);
	math_stack_destroy(&transform->mod_stack);
}

void e3d_transform_reset(struct e3d_transform *transform)
{
	assert(math_stack_is_root(&transform->mod_stack));
	assert(math_stack_is_root(&transform->proj_stack));
	assert(math_stack_is_root(&transform->eye_stack));

	math_m4_identity(MATH_TIP(&transform->mod_stack));
	math_m4_identity(MATH_TIP(&transform->proj_stack));
	math_m4_identity(MATH_TIP(&transform->eye_stack));
}

struct e3d_buffer *e3d_buffer_new(size_t size, uint8_t type)
{
	struct e3d_buffer *buf;
	size_t s, i;

	assert(size > 0);

	s = sizeof(*buf);

	if (type & E3D_BUFFER_VERTEX)
		s += size * sizeof(GLfloat) * 4;
	if (type & E3D_BUFFER_COLOR)
		s += size * sizeof(GLfloat) * 4;
	if (type & E3D_BUFFER_NORMAL)
		s += size * sizeof(GLfloat) * 4;

	buf = malloc(s);
	if (!buf)
		return NULL;

	memset(buf, 0, s);
	buf->ref = 1;
	buf->num = size;

	i = 0;
	if (type & E3D_BUFFER_VERTEX) {
		buf->vertex = (void*)&buf->buf[i];
		i += size * 4;
	}
	if (type & E3D_BUFFER_COLOR) {
		buf->color = (void*)&buf->buf[i];
		i += size * 4;
	}
	if (type & E3D_BUFFER_NORMAL) {
		buf->normal = (void*)&buf->buf[i];
		i += size * 4;
	}

	return buf;
}

struct e3d_buffer *e3d_buffer_ref(struct e3d_buffer *buf)
{
	buf->ref++;
	assert(buf->ref);
	return buf;
}

void e3d_buffer_unref(struct e3d_buffer *buf)
{
	if (!buf)
		return;

	assert(buf->ref);

	buf->ref--;
	if (!buf->ref)
		free(buf);
}

/*
 * This assumes that the buffer \buf is for triangles and generates the normals
 * for it. This does not work if you use \buf with a primitive that uses an
 * index buffer, if the indices are not consecutive or mix source triangles.
 */
void e3d_buffer_generate_triangle_normals(struct e3d_buffer *buf)
{
	size_t i;
	math_v3 dest, a, b;
	math_v4 tmp = { 0.0f, 0.0f, 0.0f, 0.0f };

	assert(buf->vertex);
	assert(buf->normal);

	for (i = 0; i < buf->num; i += 3) {
		math_v3_sub_dest(a, buf->vertex[i], buf->vertex[i + 1]);
		math_v3_sub_dest(b, buf->vertex[i], buf->vertex[i + 2]);
		math_v3_product_dest(dest, a, b);
		math_v3_normalize(dest);
		math_v3_copy((void*)tmp, dest);

		math_v4_copy(buf->normal[i], tmp);
		math_v4_copy(buf->normal[i + 1], tmp);
		math_v4_copy(buf->normal[i + 2], tmp);
	}
}

struct e3d_primitive *e3d_primitive_new(size_t index_size)
{
	struct e3d_primitive *prim;
	size_t s;

	s = sizeof(*prim) + index_size * sizeof(GLuint);
	prim = malloc(s);
	if (!prim)
		return NULL;

	memset(prim, 0, s);
	prim->ref = 1;
	prim->num = index_size;

	/* check for overflow */
	assert(prim->num >= 0);

	return prim;
}

struct e3d_primitive *e3d_primitive_ref(struct e3d_primitive *prim)
{
	prim->ref++;
	assert(prim->ref);
	return prim;
}

void e3d_primitive_unref(struct e3d_primitive *prim)
{
	if (!prim)
		return;

	assert(prim->ref);

	prim->ref--;
	if (!prim->ref) {
		e3d_buffer_unref(prim->buf);
		free(prim);
	}
}

void e3d_primitive_set_buffer(struct e3d_primitive *prim,
							struct e3d_buffer *buf)
{
	e3d_buffer_unref(prim->buf);
	prim->buf = e3d_buffer_ref(buf);
}

void e3d_primitive_draw(struct e3d_primitive *prim,
	const struct e3d_shader_locations *loc, struct e3d_transform *trans)
{
	math_m4 t_mat;

	if (!prim->buf)
		return;

	/* modelview matrix */
	E3D(glUniformMatrix4fv(loc->uni[E3D_U_M_MAT], 1, 0,
					(GLfloat*)MATH_TIP(&trans->mod_stack)));

	math_m4_invert_dest(t_mat, MATH_TIP(&trans->mod_stack));
	E3D(glUniformMatrix4fv(loc->uni[E3D_U_M_MAT_IT], 1, 0,
							(GLfloat*)t_mat));

	/* modelview, projection and eye matrix combined */
	math_m4_mult_dest(t_mat, MATH_TIP(&trans->proj_stack),
						MATH_TIP(&trans->eye_stack));
	math_m4_mult(t_mat, MATH_TIP(&trans->mod_stack));
	E3D(glUniformMatrix4fv(loc->uni[E3D_U_MPE_MAT], 1, 0,
							(GLfloat*)t_mat));

	E3D(glBindBuffer(GL_ARRAY_BUFFER, 0));

	if (prim->buf->vertex) {
		E3D(glEnableVertexAttribArray(loc->attr[E3D_A_VERTEX]));
		E3D(glVertexAttribPointer(loc->attr[E3D_A_VERTEX], 4, GL_FLOAT,
					GL_FALSE, 0, prim->buf->vertex));
	} else {
		E3D(glDisableVertexAttribArray(loc->attr[E3D_A_VERTEX]));
	}

	if (prim->buf->color) {
		E3D(glEnableVertexAttribArray(loc->attr[E3D_A_COLOR]));
		E3D(glVertexAttribPointer(loc->attr[E3D_A_COLOR], 4, GL_FLOAT,
						GL_FALSE, 0, prim->buf->color));
	} else {
		E3D(glDisableVertexAttribArray(loc->attr[E3D_A_COLOR]));
	}

	if (prim->buf->normal) {
		E3D(glEnableVertexAttribArray(loc->attr[E3D_A_NORMAL]));
		E3D(glVertexAttribPointer(loc->attr[E3D_A_NORMAL], 4, GL_FLOAT,
					GL_FALSE, 0, prim->buf->normal));
	} else {
		E3D(glDisableVertexAttribArray(loc->attr[E3D_A_NORMAL]));
	}

	if (!prim->num) {
		glDrawArrays(prim->type, 0, prim->buf->num);
	} else {
		E3D(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		glDrawElements(prim->type, prim->num, GL_UNSIGNED_INT,
								prim->ibuf);
	}
}

void e3d_primitive_draw_normals(struct e3d_primitive *prim,
	const struct e3d_shader_locations *loc, struct e3d_transform *trans)
{
	math_v4 vertex[2];
	math_m4 t_mat;
	size_t i;

	if (!prim->buf || !prim->buf->normal)
		return;

	/* modelview, projection and eye matrix combined */
	math_m4_mult_dest(t_mat, MATH_TIP(&trans->proj_stack),
						MATH_TIP(&trans->eye_stack));
	math_m4_mult(t_mat, MATH_TIP(&trans->mod_stack));
	E3D(glUniformMatrix4fv(loc->uni[E3D_U_MPE_MAT], 1, 0,
							(GLfloat*)t_mat));

	E3D(glBindBuffer(GL_ARRAY_BUFFER, 0));
	E3D(glEnableVertexAttribArray(loc->attr[E3D_A_VERTEX]));

	for (i = 0; i < prim->buf->num; ++i) {
		math_v4_copy(vertex[0], prim->buf->vertex[i]);
		math_v4_copy(vertex[1], prim->buf->vertex[i]);
		math_v4_add(vertex[1], prim->buf->normal[i]);

		E3D(glVertexAttribPointer(loc->attr[E3D_A_VERTEX], 4, GL_FLOAT,
							GL_FALSE, 0, vertex));

		glDrawArrays(GL_LINES, 0, 2);
	}
}

struct e3d_shape *e3d_shape_new()
{
	struct e3d_shape *shape;

	shape = malloc(sizeof(*shape));
	if (!shape)
		return NULL;

	memset(shape, 0, sizeof(*shape));
	math_m4_identity(shape->alter);
	shape->ref = 1;

	return shape;
}

struct e3d_shape *e3d_shape_ref(struct e3d_shape *shape)
{
	shape->ref++;
	assert(shape->ref);
	return shape;
}

void e3d_shape_unref(struct e3d_shape *shape)
{
	struct e3d_shape *tmp;

	assert(shape->ref);
	shape->ref--;
	if (shape->ref)
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
	assert(!shape->next);

	shape->next = parent->childs;
	parent->childs = e3d_shape_ref(shape);
}

void e3d_shape_set_primitive(struct e3d_shape *shape,
						struct e3d_primitive *prim)
{
	e3d_primitive_unref(shape->prim);
	shape->prim = e3d_primitive_ref(prim);
}

void e3d_shape_draw(const struct e3d_shape *shape,
	const struct e3d_shader_locations *loc, struct e3d_transform *trans)
{
	const struct e3d_shape *iter;

	math_stack_push(&trans->mod_stack);
	math_m4_mult(MATH_TIP(&trans->mod_stack), (void*)shape->alter);

	if (shape->prim)
		e3d_primitive_draw(shape->prim, loc, trans);

	for (iter = shape->childs; iter; iter = iter->next)
		e3d_shape_draw(iter, loc, trans);

	math_stack_pop(&trans->mod_stack);
}

void e3d_shape_draw_normals(const struct e3d_shape *shape,
	const struct e3d_shader_locations *loc, struct e3d_transform *trans)
{
	const struct e3d_shape *iter;

	math_stack_push(&trans->mod_stack);
	math_m4_mult(MATH_TIP(&trans->mod_stack), (void*)shape->alter);

	if (shape->prim)
		e3d_primitive_draw_normals(shape->prim, loc, trans);

	for (iter = shape->childs; iter; iter = iter->next)
		e3d_shape_draw_normals(iter, loc, trans);

	math_stack_pop(&trans->mod_stack);
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

void e3d_eye_look_at(struct e3d_eye *eye, math_v3 pos, math_v3 at, math_v3 up)
{
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

void e3d_light_supply(const struct e3d_light *light,
					const struct e3d_shader_locations *loc)
{
	math_m4 t_mat;

	E3D(glUniformMatrix4fv(loc->uni[E3D_U_L_MAT], 1, 0,
							(void*)light->matrix));

	math_m4_invert_dest(t_mat, (void*)light->matrix);
	E3D(glUniformMatrix4fv(loc->uni[E3D_U_L_MAT_IT], 1, 0,
								(void*)t_mat));
}
