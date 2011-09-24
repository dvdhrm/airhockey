/*
 * airhockey - config file loader
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <libcstr.h>
#include <libuconf.h>
#include <SFML/OpenGL.h>

#include "engine3d.h"
#include "log.h"
#include "main.h"
#include "mathw.h"

/* forward declaration to allow recursion */
static int load_generic(const struct uconf_entry *e, struct e3d_shape *shape);

static int load_v34_list(const struct uconf_entry *e, math_v4 *out, float d4)
{
	const struct uconf_entry *iter;
	size_t i;
	int ret;

	assert(uconf_entry_is_list(e));

	i = 0;
	UCONF_ENTRY_FOR(e, iter) {
		ret = config_load_v34(iter, out[i++], d4);
		if (ret)
			return ret;
	}

	return 0;
}

static int load_raw_vertex(struct e3d_vbo **vbo, const struct uconf_entry *e)
{
	int ret;
	struct e3d_vbo *v;
	size_t num;

	assert(e);

	if (!uconf_entry_is_list(e))
		return -EINVAL;

	num = e->v.list.num;
	ret = e3d_vbo_new_v4(&v, num);
	if (ret)
		return ret;

	ret = load_v34_list(e, v->data, 1.0);
	if (ret)
		goto err;

	*vbo = v;
	return 0;

err:
	e3d_vbo_unref(v);
	return ret;
}

static int load_raw_color(struct e3d_vbo **vbo, const struct uconf_entry *e,
								size_t num)
{
	int ret;
	struct e3d_vbo *v;
	math_v4 color = { 1, 1, 1, 1 };
	size_t i;

	if (e) {
		ret = config_load_v34(e, color, 1.0);
		if (ret)
			return ret;
	}

	ret = e3d_vbo_new_v4(&v, num);
	if (ret)
		return ret;

	for (i = 0; i < num; ++i)
		math_v4_copy(E3D_VBO_AT(v, i), color);

	*vbo = v;
	return 0;
}

static int load_raw_normal(struct e3d_vbo **vbo, const struct uconf_entry *e,
								size_t num)
{
	int ret;
	struct e3d_vbo *v;
	math_v4 normal;
	size_t i;

	assert(e);

	ret = e3d_vbo_new_v4(&v, num);
	if (ret)
		return ret;

	ret = config_load_v34(e, normal, 0.0);
	if (ret)
		goto err;

	for (i = 0; i < num; ++i)
		math_v4_copy(E3D_VBO_AT(v, i), normal);

	*vbo = v;
	return 0;

err:
	e3d_vbo_unref(v);
	return ret;
}

static int load_raw_index(struct e3d_vbo **vbo, const struct uconf_entry *e)
{
	return 0;
}

/*
 * This loads \e as raw primitive into \shape. It is added as child entry
 * to \shape.
 * Returns 0 on success.
 */
static int load_raw(const struct uconf_entry *e, struct e3d_shape *shape)
{
	int ret = 0;
	const struct uconf_entry *vertex, *color, *normal, *index, *iter;
	size_t voff, coff, noff, ioff;
	GLuint type;
	struct e3d_shape *new;
	struct e3d_vbo *vbo;
	struct e3d_primitive *prim;

	if (!uconf_entry_is_list(e))
		return -EINVAL;

	ret = e3d_shape_new(&new);
	if (ret)
		return ret;

	voff = 0;
	vertex = NULL;
	coff = 0;
	color = NULL;
	noff = 0;
	normal = NULL;
	ioff = 0;
	index = NULL;
	type = 0;

	UCONF_ENTRY_FOR(e, iter) {
		if (!iter->name) {
			ret = -EINVAL;
			goto err;
		} else if (cstr_strcmp(iter->name, -1, "type")) {
			if (iter->type != UCONF_ENTRY_QSTR) {
				ret = -EINVAL;
				goto err;
			} else if (cstr_strcmp(iter->v.qstr, -1, "triangle")) {
				if (type) {
					ret = -EINVAL;
					goto err;
				}
				type = GL_TRIANGLES;
			} else {
				ret = -EINVAL;
				goto err;
			}
		} else if (cstr_strcmp(iter->name, -1, "voff")) {
			ret = config_load_size(iter, &voff);
			if (ret)
				goto err;
		} else if (cstr_strcmp(iter->name, -1, "vertex")) {
			if (vertex) {
				ret = -EINVAL;
				goto err;
			}
			vertex = iter;
		} else if (cstr_strcmp(iter->name, -1, "coff")) {
			ret = config_load_size(iter, &coff);
			if (ret)
				goto err;
		} else if (cstr_strcmp(iter->name, -1, "color")) {
			if (color) {
				ret = -EINVAL;
				goto err;
			}
			color = iter;
		} else if (cstr_strcmp(iter->name, -1, "noff")) {
			ret = config_load_size(iter, &noff);
			if (ret)
				goto err;
		} else if (cstr_strcmp(iter->name, -1, "normal")) {
			if (normal) {
				ret = -EINVAL;
				goto err;
			}
			normal = iter;
		} else if (cstr_strcmp(iter->name, -1, "ioff")) {
			ret = config_load_size(iter, &ioff);
			if (ret)
				goto err;
		} else if (cstr_strcmp(iter->name, -1, "index")) {
			if (index) {
				ret = -EINVAL;
				goto err;
			}
			index = iter;
		} else {
			ret = load_generic(iter, new);
			if (ret)
				goto err;
		}
	}

	/* vertex data must be given! */
	if (!type || !vertex) {
		ret = -EINVAL;
		goto err;
	}

	ret = e3d_primitive_new(&prim);
	if (ret)
		goto err;
	prim->type = type;

	ret = load_raw_vertex(&vbo, vertex);
	if (ret)
		goto err_prim;
	prim->num = vbo->num;
	e3d_primitive_set_vertex(prim, voff, vbo);
	e3d_vbo_unref(vbo);

	ret = load_raw_color(&vbo, color, prim->num);
	if (ret)
		goto err_prim;
	e3d_primitive_set_color(prim, coff, vbo);
	e3d_vbo_unref(vbo);

	if (normal) {
		ret = load_raw_normal(&vbo, normal, prim->num);
		if (ret)
			goto err_prim;
		e3d_primitive_set_normal(prim, noff, vbo);
		e3d_vbo_unref(vbo);
	} else {
		ret = e3d_primitive_generate_normals(prim);
		if (ret)
			goto err_prim;
	}

	if (index) {
		ret = load_raw_index(&vbo, index);
		if (ret)
			goto err_prim;
		e3d_primitive_set_index(prim, ioff, vbo);
		e3d_vbo_unref(vbo);
	}

	e3d_shape_set_primitive(new, prim);
	e3d_shape_link(shape, new);

err_prim:
	e3d_primitive_unref(prim);
err:
	e3d_shape_unref(new);
	return ret;
}

/*
 * Creates the vertex buffer for cylinders. The vertex buffer contains
 * the vertices for the lower circle followed by the vertices for the upper
 * circle.
 * \vbo must be of type v4 and have room for at least \detail * 2 vertices.
 * \extents specifies the half extents of the cylinder.
 * \detail specifies how much vertices are used for each circle. It must be at
 * least 5.
 */
static void fill_cylinder(struct e3d_vbo *vbo, math_v3 extents, size_t detail)
{
	static const GLfloat pi = 3.14159265358;
	float udeg, factor;
	size_t i;
	float *v;

	assert(detail >= 5);
	assert(e3d_vbo_is_v4(vbo));
	assert(vbo->num >= detail * 2);

	v = (void*)vbo->data;
	factor = 2 * pi / (detail - 1);
	i = 0;

	v[i * 4] = 0.0;
	v[i * 4 + 1] = 0.0;
	v[i * 4 + 2] = -extents[2];
	v[i * 4 + 3] = 1.0;

	for (i += 1; i < detail; ++i) {
		udeg = (i - 1) * factor;

		v[i * 4] = cos(udeg) * extents[0];
		v[i * 4 + 1] = sin(udeg) * extents[1];
		v[i * 4 + 2] = -extents[2];
		v[i * 4 + 3] = 1.0;
	}

	v[i * 4] = 0.0;
	v[i * 4 + 1] = 0.0;
	v[i * 4 + 2] = extents[2];
	v[i * 4 + 3] = 1.0;

	for (i += 1; i < (detail * 2); ++i) {
		udeg = ((i - detail) - 1) * factor;

		v[i * 4] = cos(udeg) * extents[0];
		v[i * 4 + 1] = sin(udeg) * extents[1];
		v[i * 4 + 2] = extents[2];
		v[i * 4 + 3] = 1.0;
	}
}

/*
 * This creates a new cylinder with the given extends. It creates two circles
 * for the bottom and top with triangle fans and one primitive with triangles
 * for the side wall.
 */
static int create_cylinder(struct e3d_shape *shape, math_v3 extents,
						math_v4 col, size_t detail)
{
	struct e3d_shape *bottom, *top, *round;
	struct e3d_primitive *pbottom, *ptop, *pround;
	struct e3d_vbo *vb, *cb, *nb;
	int ret = 0;
	size_t i;

	if (detail < 5)
		return -EINVAL;

	if (extents[0] <= 0 || extents[1] <= 0 || extents[2] <= 0)
		return -EINVAL;

	/*
	 * Create vertex and color buffers.
	 * Both contain only data for the lower and upper circle. The different
	 * primitives will use index buffers to create both circles and the side
	 * wall.
	 */

	ret = e3d_vbo_new_v4(&vb, detail * 2);
	if (ret)
		return ret;

	fill_cylinder(vb, extents, detail);

	ret = e3d_vbo_new_v4(&cb, vb->num);
	if (ret)
		goto err_vert;

	for (i = 0; i < cb->num; ++i)
		math_v4_copy(E3D_VBO_AT(cb, i), col);

	/* bottom circle normal and index buffer plus primitive */
	ret = e3d_vbo_new_v4(&nb, detail + 1);
	if (ret)
		goto err_color;

	ret = e3d_primitive_new_idx(&pbottom, detail + 1);
	if (ret) {
		e3d_vbo_unref(nb);
		goto err_color;
	}
	e3d_primitive_set_normal(pbottom, 0, nb);
	e3d_vbo_unref(nb);

	/* upper circle normal and index buffer plus primitive */
	ret = e3d_vbo_new_v4(&nb, detail + 1);
	if (ret)
		goto err_pbottom;

	ret = e3d_primitive_new_idx(&ptop, detail + 1);
	if (ret) {
		e3d_vbo_unref(nb);
		goto err_pbottom;
	}
	e3d_primitive_set_normal(ptop, 0, nb);
	e3d_vbo_unref(nb);

	/* side wall normal and index buffer plus primitive */
	ret = e3d_vbo_new_v4(&nb, (detail - 1) * 6);
	if (ret)
		goto err_ptop;

	ret = e3d_primitive_new_idx(&pround, (detail - 1) * 6);
	if (ret) {
		e3d_vbo_unref(nb);
		goto err_ptop;
	}
	e3d_primitive_set_normal(pround, 0, nb);
	e3d_vbo_unref(nb);

	/* shapes */
	ret = e3d_shape_new(&bottom);
	if (ret)
		goto err_pround;

	ret = e3d_shape_new(&top);
	if (ret)
		goto err_bottom;

	ret = e3d_shape_new(&round);
	if (ret)
		goto err_top;

	/* triangle fan buffer for bottom circle with indices and normals */
	/* reverse order as we have to face down */
	*(GLuint*)E3D_VBO_AT(pbottom->index, 0) = 0;
	for (i = 1; i < detail; ++i)
		*(GLuint*)E3D_VBO_AT(pbottom->index, i) = detail - i;
	*(GLuint*)E3D_VBO_AT(pbottom->index, i) = detail - 1;

	for (i = 0; i <= detail; ++i)
		math_v4_copy(E3D_VBO_AT(pbottom->normal, i),
						(math_v4) { 0, 0, -1, 0 });

	pbottom->type = GL_TRIANGLE_FAN;
	e3d_primitive_set_vertex(pbottom, 0, vb);
	e3d_primitive_set_color(pbottom, 0, cb);
	e3d_shape_set_primitive(bottom, pbottom);

	/* triangle fan buffer for upper circle with indices and normals */
	*(GLuint*)E3D_VBO_AT(ptop->index, 0) = 0;
	for (i = 1; i < detail; ++i)
		*(GLuint*)E3D_VBO_AT(ptop->index, i) = i;
	*(GLuint*)E3D_VBO_AT(ptop->index, i) = 1;

	for (i = 0; i <= detail; ++i)
		math_v4_copy(E3D_VBO_AT(ptop->normal, i),
						(math_v4) { 0, 0, 1, 0 });

	ptop->type = GL_TRIANGLE_FAN;
	e3d_primitive_set_vertex(ptop, detail, vb);
	e3d_primitive_set_color(ptop, detail, cb);
	e3d_shape_set_primitive(top, ptop);

	/* create triangles buffer for side wall */
	for (i = 0; i < (detail - 1); ++i) {
		*(GLuint*)E3D_VBO_AT(pround->index, i * 6) = i + 1;
		*(GLuint*)E3D_VBO_AT(pround->index, i * 6 + 1) = i + 2;
		*(GLuint*)E3D_VBO_AT(pround->index, i * 6 + 2) = detail + i + 1;
		*(GLuint*)E3D_VBO_AT(pround->index, i * 6 + 3) = detail + i + 2;
		*(GLuint*)E3D_VBO_AT(pround->index, i * 6 + 4) = detail + i + 1;
		*(GLuint*)E3D_VBO_AT(pround->index, i * 6 + 5) = i + 2;
	}
	/* last index must point to first triangle; fix this */
	--i;
	*(GLuint*)E3D_VBO_AT(pround->index, i * 6) = i + 1;
	*(GLuint*)E3D_VBO_AT(pround->index, i * 6 + 1) = 1;
	*(GLuint*)E3D_VBO_AT(pround->index, i * 6 + 2) = detail + i + 1;
	*(GLuint*)E3D_VBO_AT(pround->index, i * 6 + 3) = detail + 1;
	*(GLuint*)E3D_VBO_AT(pround->index, i * 6 + 4) = detail + i + 1;
	*(GLuint*)E3D_VBO_AT(pround->index, i * 6 + 5) = 1;

	pround->type = GL_TRIANGLES;
	e3d_primitive_set_vertex(pround, 0, vb);
	e3d_primitive_set_color(pround, 0, cb);
	ret = e3d_primitive_generate_normals(pround);
	if (ret)
		goto err_round;
	e3d_shape_set_primitive(round, pround);

	e3d_shape_link(shape, bottom);
	e3d_shape_link(shape, top);
	e3d_shape_link(shape, round);

err_round:
	e3d_shape_unref(round);
err_top:
	e3d_shape_unref(top);
err_bottom:
	e3d_shape_unref(bottom);
err_pround:
	e3d_primitive_unref(pround);
err_ptop:
	e3d_primitive_unref(ptop);
err_pbottom:
	e3d_primitive_unref(pbottom);
err_color:
	e3d_vbo_unref(cb);
err_vert:
	e3d_vbo_unref(vb);
	return ret;
}

/*
 * This loads \e as cylinder parameter into \shape. It is added as child entries
 * to \shape.
 * Returns 0 on success.
 */
static int load_cylinder(const struct uconf_entry *e, struct e3d_shape *shape)
{
	int ret = 0;
	const struct uconf_entry *iter;
	struct e3d_shape *new;
	size_t detail = 0;
	math_v3 extends = { 0.0, 0.0, 0.0 };
	math_v4 color = { 1.0, 1.0, 1.0, 1.0 };

	if (!uconf_entry_is_list(e))
		return -EINVAL;

	ret = e3d_shape_new(&new);
	if (ret)
		return ret;

	UCONF_ENTRY_FOR(e, iter) {
		if (!iter->name)
			ret = -EINVAL;
		else if (cstr_strcmp(iter->name, -1, "extents"))
			ret = config_load_v3(iter, extends);
		else if (cstr_strcmp(iter->name, -1, "color"))
			ret = config_load_v34(iter, color, 1);
		else if (cstr_strcmp(iter->name, -1, "detail"))
			ret = config_load_size(iter, &detail);
		else
			ret = load_generic(iter, new);

		if (ret)
			goto err;
	}

	ret = create_cylinder(new, extends, color, detail);
	if (ret)
		goto err;

	e3d_shape_link(shape, new);

err:
	e3d_shape_unref(new);
	return ret;
}

/*
 * Loads generic shape values into \shape from \e. Returns error if an unknwon
 * entry is found.
 */
static int load_generic(const struct uconf_entry *e, struct e3d_shape *shape)
{
	math_v3 v;
	int ret;

	if (!e->name) {
		return -EINVAL;
	} else if (cstr_strcmp(e->name, -1, "raw")) {
		return load_raw(e, shape);
	} else if (cstr_strcmp(e->name, -1, "cylinder")) {
		return load_cylinder(e, shape);
	} else if (cstr_strcmp(e->name, -1, "translate")) {
		ret = config_load_v3(e, v);
		if (ret)
			return ret;
		math_m4_translatev(shape->alter, v);
	} else {
		return -EINVAL;
	}

	return 0;
}

/*
 * Loads \e as shape into a fresh shape and stores the result into \shape.
 * Returns 0 on success.
 */
int config_load_shape(struct e3d_shape **shape, const struct uconf_entry *e)
{
	const struct uconf_entry *iter;
	struct e3d_shape *v;
	int ret;

	if (!uconf_entry_is_list(e))
		return -EINVAL;

	ret = e3d_shape_new(&v);
	if (ret)
		return ret;

	UCONF_ENTRY_FOR(e, iter) {
		ret = load_generic(iter, v);
		if (ret)
			break;
	}

	if (!ret) {
		*shape = v;
		return 0;
	}

	e3d_shape_unref(v);
	return ret;
}
