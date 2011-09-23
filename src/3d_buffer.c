/*
 * airhockey - 3D engine - buffers
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "engine3d.h"
#include "log.h"
#include "main.h"
#include "mathw.h"

int e3d_vbo_new(struct e3d_vbo **vbo, unsigned int ele_type, size_t ele_num,
								size_t num)
{
	struct e3d_vbo *v;
	void *d;
	size_t size;

	assert(vbo);
	assert(e3d_tsize[ele_type]);
	assert(ele_num > 0);
	assert(num);

	v = malloc(sizeof(*v));
	if (!v)
		return -ENOMEM;

	size = e3d_tsize[ele_type] * ele_num * num;
	d = malloc(size);
	if (!d) {
		free(v);
		return -ENOMEM;
	}

	memset(v, 0, sizeof(*v));
	memset(d, 0, size);
	v->ref = 1;
	v->ele_type = ele_type;
	v->ele_num = ele_num;
	v->num = num;
	v->data = d;
	v->id = 0;

	*vbo = v;
	return 0;
}

void e3d_vbo_ref(struct e3d_vbo *vbo)
{
	assert(vbo);
	++vbo->ref;
	assert(vbo->ref);
}

void e3d_vbo_unref(struct e3d_vbo *vbo)
{
	if (!vbo)
		return;

	assert(vbo->ref);

	if (--vbo->ref)
		return;

	e3d_vbo_release(vbo);
	if (vbo->id)
		E3D(glDeleteBuffers(1, &vbo->id));
	free(vbo);
}

int e3d_vbo_grab(struct e3d_vbo *vbo, int hint)
{
	size_t size = e3d_tsize[vbo->ele_type] * vbo->ele_num * vbo->num;

	assert(vbo);
	assert(vbo->data);

	if (!vbo->id)
		E3D(glGenBuffers(1, &vbo->id));

	E3D(glBindBuffer(GL_ARRAY_BUFFER, vbo->id));
	E3D(glBufferData(GL_ARRAY_BUFFER, size, vbo->data, hint));

	return 0;
}

void e3d_vbo_release(struct e3d_vbo *vbo)
{
	assert(vbo);

	free(vbo->data);
	vbo->data = NULL;
}

void e3d_vbo_bind(struct e3d_vbo *vbo, GLint attr, size_t off)
{
	size_t offset;

	assert(vbo);
	assert(attr >= 0);

	offset = vbo->ele_num * e3d_tsize[vbo->ele_type] * off;

	if (vbo->id) {
		assert(false);
	} else {
		assert(vbo->data);

		E3D(glBindBuffer(GL_ARRAY_BUFFER, 0));
		E3D(glVertexAttribPointer(attr, vbo->ele_num, vbo->ele_type,
				GL_FALSE, 0, E3D_OFF(vbo->data, offset)));
	}
}

void e3d_vbo_draw(struct e3d_vbo *vbo, GLuint type, size_t num, size_t off)
{
	size_t offset;

	assert(vbo);
	assert(num > 0);

	offset = vbo->ele_num * e3d_tsize[vbo->ele_type] * off;

	if (vbo->id) {
		assert(false);
	} else {
		assert(vbo->data);

		E3D(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		glDrawElements(type, num, vbo->ele_type,
						E3D_OFF(vbo->data, offset));
	}
}

static void vbo_debug_at(struct e3d_vbo *vbo, size_t at)
{
	float *f = E3D_VBO_AT(vbo, at);

	if (vbo->ele_num == 1)
		ulog_flog(e3d_log, ULOG_DEBUG, "VBO: %f\n", f[0]);
	else if (vbo->ele_num == 3)
		ulog_flog(e3d_log, ULOG_DEBUG, "VBO: %f %f %f\n",
							f[0], f[1], f[2]);
	else if (vbo->ele_num == 4)
		ulog_flog(e3d_log, ULOG_DEBUG, "VBO: %f %f %f %f\n",
							f[0], f[1], f[2], f[3]);
}

void e3d_vbo_debug(struct e3d_vbo *vbo)
{
	size_t i;

	ulog_flog(e3d_log, ULOG_DEBUG, "VBO %p debug\n", vbo);
	ulog_flog(e3d_log, ULOG_DEBUG, "VBO %p type %u ele_num %lu num %lu\n",
				vbo, vbo->ele_type, vbo->ele_num, vbo->num);
	ulog_flog(e3d_log, ULOG_DEBUG, "VBO %p id %u data %p\n",
						vbo, vbo->id, vbo->data);

	if (vbo->data)
		for (i = 0; i < vbo->num; ++i)
			vbo_debug_at(vbo, i);

	ulog_flog(e3d_log, ULOG_DEBUG, "End of VBO %p debug\n", vbo);
}

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

int e3d_primitive_new(struct e3d_primitive **prim)
{
	struct e3d_primitive *p;

	p = malloc(sizeof(*p));
	if (!p)
		return -ENOMEM;

	memset(p, 0, sizeof(*p));
	p->ref = 1;

	*prim = p;
	return 0;
}

void e3d_primitive_ref(struct e3d_primitive *prim)
{
	assert(prim);
	++prim->ref;
	assert(prim->ref);
}

void e3d_primitive_unref(struct e3d_primitive *prim)
{
	if (!prim)
		return;

	assert(prim->ref);

	if (--prim->ref)
		return;

	e3d_vbo_unref(prim->vertex);
	e3d_vbo_unref(prim->color);
	e3d_vbo_unref(prim->normal);
	e3d_vbo_unref(prim->index);
	free(prim);
}

void e3d_primitive_set_vertex(struct e3d_primitive *prim, size_t off,
							struct e3d_vbo *vbo)
{
	assert(prim);
	assert(vbo);
	assert(e3d_vbo_is_v4(vbo));

	e3d_vbo_unref(prim->vertex);
	e3d_vbo_ref(vbo);
	prim->voff = off;
	prim->vertex = vbo;
}

void e3d_primitive_set_color(struct e3d_primitive *prim, size_t off,
							struct e3d_vbo *vbo)
{
	assert(prim);
	assert(vbo);
	assert(e3d_vbo_is_v4(vbo));

	e3d_vbo_unref(prim->color);
	e3d_vbo_ref(vbo);
	prim->coff = off;
	prim->color = vbo;
}

void e3d_primitive_set_normal(struct e3d_primitive *prim, size_t off,
							struct e3d_vbo *vbo)
{
	assert(prim);
	assert(vbo);
	assert(e3d_vbo_is_v4(vbo));

	e3d_vbo_unref(prim->normal);
	e3d_vbo_ref(vbo);
	prim->noff = off;
	prim->normal = vbo;
}

void e3d_primitive_set_index(struct e3d_primitive *prim, size_t off,
							struct e3d_vbo *vbo)
{
	assert(prim);
	assert(vbo);
	assert(e3d_vbo_is_idx(vbo));

	e3d_vbo_unref(prim->index);
	e3d_vbo_ref(vbo);
	prim->ioff = off;
	prim->normal = vbo;
}

static void setup_uniforms(int how, const struct e3d_shader_locations *loc,
						struct e3d_transform *trans)
{
	math_m4 tmp;

	/* modelview, projection and eye matrix combined */
	math_m4_mult_dest(tmp, MATH_TIP(&trans->proj_stack),
						MATH_TIP(&trans->eye_stack));
	math_m4_mult(tmp, MATH_TIP(&trans->mod_stack));
	E3D(glUniformMatrix4fv(loc->uni[E3D_U_MPE_MAT], 1, 0, (void*)tmp));

	if (how == E3D_DRAW_FULL) {
		/* modelview matrix */
		E3D(glUniformMatrix4fv(loc->uni[E3D_U_M_MAT], 1, 0,
					(void*)MATH_TIP(&trans->mod_stack)));

		math_m4_invert_dest(tmp, MATH_TIP(&trans->mod_stack));
		E3D(glUniformMatrix4fv(loc->uni[E3D_U_M_MAT_IT], 1, 0,
								(void*)tmp));
	} else if (how == E3D_DRAW_SILHOUETTE) {
		E3D(glUniform4f(loc->uni[E3D_U_COLOR], 0.0, 0.0, 0.0, 1.0));
	} else if (how == E3D_DRAW_NORMALS) {
		E3D(glUniform4f(loc->uni[E3D_U_COLOR], 1.0, 0.1, 0.1, 1.0));
	}
}

void e3d_primitive_draw(struct e3d_primitive *prim, int how,
	const struct e3d_shader_locations *loc, struct e3d_transform *trans)
{
	size_t i;
	math_v4 vertex[2];

	assert(prim->num);

	setup_uniforms(how, loc, trans);

	if (how == E3D_DRAW_FULL) {
		assert(prim->vertex);
		assert(prim->color);
		assert(prim->normal);

		E3D(glEnableVertexAttribArray(loc->attr[E3D_A_VERTEX]));
		E3D(glEnableVertexAttribArray(loc->attr[E3D_A_COLOR]));
		E3D(glEnableVertexAttribArray(loc->attr[E3D_A_NORMAL]));
		e3d_vbo_bind(prim->vertex, loc->attr[E3D_A_VERTEX], prim->voff);
		e3d_vbo_bind(prim->normal, loc->attr[E3D_A_NORMAL], prim->noff);
		e3d_vbo_bind(prim->color, loc->attr[E3D_A_COLOR], prim->coff);

		if (prim->index)
			e3d_vbo_draw(prim->index, prim->type, prim->num,
								prim->ioff);
		else
			glDrawArrays(prim->type, 0, prim->num);
	} else if (how == E3D_DRAW_SILHOUETTE) {
		assert(prim->vertex);

		E3D(glEnableVertexAttribArray(loc->attr[E3D_A_VERTEX]));
		e3d_vbo_bind(prim->vertex, loc->attr[E3D_A_VERTEX], prim->voff);

		if (prim->index)
			e3d_vbo_draw(prim->index, prim->type, prim->num,
								prim->ioff);
		else
			glDrawArrays(prim->type, 0, prim->num);
	} else if (how == E3D_DRAW_NORMALS) {
		assert(prim->vertex);
		assert(prim->vertex->data);
		assert(prim->normal);
		assert(prim->normal->data);

		E3D(glBindBuffer(GL_ARRAY_BUFFER, 0));
		E3D(glEnableVertexAttribArray(loc->attr[E3D_A_VERTEX]));

		for (i = 0; i < prim->num; ++i) {
			math_v4_copy(vertex[0], E3D_VBO_AT(prim->vertex, i));
			math_v4_copy(vertex[1], E3D_VBO_AT(prim->vertex, i));
			math_v4_add(vertex[1], E3D_VBO_AT(prim->normal, i));

			E3D(glVertexAttribPointer(loc->attr[E3D_A_VERTEX], 4,
						GL_FLOAT, GL_FALSE, 0, vertex));
			glDrawArrays(GL_LINES, 0, 2);
		}
	}
}

/*
 * This tries to auto-generate normals for the given primitive. Currently only
 * plain triangles are supported.
 */
int e3d_primitive_generate_normals(struct e3d_primitive *prim)
{
	size_t i;
	math_v3 dest, a, b;
	struct e3d_vbo *n, *v;
	int ret;

	assert(prim->vertex);
	assert(prim->vertex->data);

	if (!e3d_vbo_is_v4(prim->vertex) || prim->index)
		return -EINVAL;

	ret = e3d_vbo_new_v4(&n, prim->vertex->num);
	if (ret)
		return ret;

	e3d_primitive_set_normal(prim, 0, n);

	v = prim->vertex;
	for (i = 0; i < (prim->num - 2); i += 3) {
		math_v3_sub_dest(a, E3D_VBO_AT(v, i), E3D_VBO_AT(v, i + 1));
		math_v3_sub_dest(b, E3D_VBO_AT(v, i), E3D_VBO_AT(v, i + 2));
		math_v3_product_dest(dest, a, b);
		math_v3_normalize(dest);

		/*
		 * Normals buffer is by default initialized to 0 so we can
		 * safely copy v3 into v4 buffer.
		 */
		math_v3_copy(E3D_VBO_AT(n, i), dest);
		math_v3_copy(E3D_VBO_AT(n, i + 1), dest);
		math_v3_copy(E3D_VBO_AT(n, i + 2), dest);
	}

	return 0;
}

void e3d_primitive_debug(struct e3d_primitive *prim)
{
	ulog_flog(e3d_log, ULOG_DEBUG, "Debug prim %p\n", prim);
	ulog_flog(e3d_log, ULOG_DEBUG, "Prim %p type %u num %lu\n",
						prim, prim->type, prim->num);
	ulog_flog(e3d_log, ULOG_DEBUG, "Prim %p vertex %p %lu\n",
						prim, prim->vertex, prim->voff);
	if (prim->vertex)
		e3d_vbo_debug(prim->vertex);
	ulog_flog(e3d_log, ULOG_DEBUG, "Prim %p color %p %lu\n",
						prim, prim->color, prim->coff);
	if (prim->color)
		e3d_vbo_debug(prim->color);
	ulog_flog(e3d_log, ULOG_DEBUG, "Prim %p normal %p %lu\n",
						prim, prim->normal, prim->noff);
	if (prim->normal)
		e3d_vbo_debug(prim->normal);
	ulog_flog(e3d_log, ULOG_DEBUG, "Prim %p index %p %lu\n",
						prim, prim->index, prim->ioff);
	if (prim->index)
		e3d_vbo_debug(prim->index);
	ulog_flog(e3d_log, ULOG_DEBUG, "End of prim %p debug\n", prim);
}
