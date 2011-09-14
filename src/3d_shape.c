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
	const struct e3d_shader_locations *loc, struct math_stack *stack)
{
	math_m4 normal;

	if (!prim->buf)
		return;

	math_m4_invert_dest(normal, MATH_TIP(stack));
	E3D(glUniformMatrix4fv(loc->uni.mod_mat, 1, 0,
						(GLfloat*)MATH_TIP(stack)));
	E3D(glUniformMatrix4fv(loc->uni.nor_mat, 1, 0, (GLfloat*)normal));

	E3D(glBindBuffer(GL_ARRAY_BUFFER, 0));

	if (prim->buf->vertex) {
		E3D(glEnableVertexAttribArray(loc->attr.vertex));
		E3D(glVertexAttribPointer(loc->attr.vertex, 4, GL_FLOAT,
					GL_FALSE, 0, prim->buf->vertex));
	} else {
		E3D(glDisableVertexAttribArray(loc->attr.vertex));
	}

	if (prim->buf->color) {
		E3D(glEnableVertexAttribArray(loc->attr.color));
		E3D(glVertexAttribPointer(loc->attr.color, 4, GL_FLOAT,
						GL_FALSE, 0, prim->buf->color));
	} else {
		E3D(glDisableVertexAttribArray(loc->attr.color));
	}

	if (prim->buf->normal) {
		E3D(glEnableVertexAttribArray(loc->attr.normal));
		E3D(glVertexAttribPointer(loc->attr.normal, 4, GL_FLOAT,
					GL_FALSE, 0, prim->buf->normal));
	} else {
		E3D(glDisableVertexAttribArray(loc->attr.normal));
	}

	if (!prim->num) {
		glDrawArrays(prim->type, 0, prim->buf->num);
	} else {
		E3D(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		glDrawElements(prim->type, prim->num, GL_UNSIGNED_INT,
								prim->ibuf);
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
	const struct e3d_shader_locations *loc, struct math_stack *stack)
{
	const struct e3d_shape *iter;

	math_stack_push(stack);
	math_m4_mult(MATH_TIP(stack), (void*)shape->alter);
	if (shape->prim)
		e3d_primitive_draw(shape->prim, loc, stack);
	for (iter = shape->childs; iter; iter = iter->next)
		e3d_shape_draw(iter, loc, stack);
	math_stack_pop(stack);
}

/*
 * This is a free implementation of the gluLookAt() function. See the wiki at
 * opengl.org for details on the algorithm.
 */
void e3d_eye_look_at(struct e3d_eye *eye, math_v3 pos, math_v3 at, math_v3 up)
{
	math_v3 f, u, s;
	math_m4 *m = &eye->matrix;

	math_v3_sub_dest(f, at, pos);
	math_v3_normalize(f);

	math_v3_product_dest(s, f, up);
	math_v3_normalize(s);
	math_v3_product_dest(u, s, f);

	(*m)[0][0] = s[0];
	(*m)[1][0] = s[1];
	(*m)[2][0] = s[2];
	(*m)[3][0] = 0.0f;

	(*m)[0][1] = u[0];
	(*m)[1][1] = u[1];
	(*m)[2][1] = u[2];
	(*m)[3][1] = 0.0f;

	(*m)[0][2] = -f[0];
	(*m)[1][2] = -f[1];
	(*m)[2][2] = -f[2];
	(*m)[3][2] = 0.0f;

	(*m)[0][3] = 0.0f;
	(*m)[1][3] = 0.0f;
	(*m)[2][3] = 0.0f;
	(*m)[3][3] = 1.0f;

	math_m4_translate(*m, -pos[0], -pos[1], -pos[2]);
}

void e3d_eye_apply(const struct e3d_eye *eye, math_m4 m)
{
	math_m4_mult(m, (void*)eye->matrix);
}
