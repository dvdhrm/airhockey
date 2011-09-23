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

#if 0
static int load_buffer_vertex(struct e3d_buffer *buf,
						const struct uconf_entry *e)
{
	size_t i;
	const struct uconf_entry *iter;
	int ret;

	if (e->type != UCONF_ENTRY_LIST)
		return -EINVAL;

	i = 0;
	UCONF_ENTRY_FOR(e, iter) {
		if (i >= buf->num)
			return -EINVAL;
		ret = load_vec(iter, (void*)&buf->vertex[i++], 4);
		if (ret)
			return -EINVAL;
	}

	if (i != buf->num)
		return -EINVAL;

	return 0;
}

static int load_buffer_color(struct e3d_buffer *buf,
						const struct uconf_entry *e)
{
	size_t i;
	GLfloat color[4];
	int ret;

	ret = load_vec(e, color, 4);
	if (ret)
		return ret;
	for (i = 0; i < buf->num; ++i)
		memcpy(&buf->color[i], color, sizeof(color));

	return 0;
}

static int load_buffer_normal(struct e3d_buffer *buf,
						const struct uconf_entry *e)
{
	size_t i;
	GLfloat normal[4];
	int ret;

	if (e->type == UCONF_ENTRY_QSTR) {
		if (cstr_strcmp(e->v.qstr, -1, "triangle"))
			e3d_buffer_generate_triangle_normals(buf);
		else
			return -EINVAL;
	} else {
		ret = load_vec(e, normal, 4);
		if (ret)
			return ret;

		for (i = 0; i < buf->num; ++i)
			memcpy(&buf->normal[i], normal, sizeof(normal));
	}

	return 0;
}

static int load_buffer(struct e3d_buffer **buf, const struct uconf_entry *e)
{
	const struct uconf_entry *iter, *vertex, *color, *normal;
	size_t num;
	int ret = 0;
	uint8_t type = E3D_BUFFER_VERTEX;
	struct e3d_buffer *new;

	if (e->type != UCONF_ENTRY_LIST)
		return -EINVAL;

	vertex = color = normal = NULL;

	UCONF_ENTRY_FOR(e, iter) {
		if (!iter->name) {
			ret = -EINVAL;
		} else if (cstr_strcmp(iter->name, -1, "vertex")) {
			vertex = iter;
		} else if (cstr_strcmp(iter->name, -1, "color")) {
			color = iter;
			type |= E3D_BUFFER_COLOR;
		} else if (cstr_strcmp(iter->name, -1, "normal")) {
			normal = iter;
			type |= E3D_BUFFER_NORMAL;
		} else {
			ret = -EINVAL;
		}

		if (ret)
			return ret;
	}

	if (!vertex || vertex->type != UCONF_ENTRY_LIST)
		return -EINVAL;

	num = vertex->v.list.num;
	if (!num)
		return -EINVAL;

	new = e3d_buffer_new(num, type);
	if (!new)
		return -ENOMEM;

	ret = load_buffer_vertex(new, vertex);
	if (ret)
		goto err;

	if (color) {
		ret = load_buffer_color(new, color);
		if (ret)
			goto err;
	}
	if (normal) {
		ret = load_buffer_normal(new, normal);
		if (ret)
			goto err;
	}

	*buf = e3d_buffer_ref(new);

err:
	e3d_buffer_unref(new);
	return ret;
}
#endif

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
 * This creates a buffer for a cylinder. The buffer contains the vertices for
 * the lower circle and upper circle. The rest of the vertices buffer is not
 * touched.
 * This also fills the normals buffer. First the normals for the bottom circle,
 * then for the upper circle, followed by the normals for the sidewall.
 * \detail specifies how much vertices are used for each circle. It must be at
 * least 5.
 * \buf must have enough space for at least \detail * 2 + (\detail - 1) * 6
 * normals. The vertices count is smaller so you should u
 */
#if 0
static void create_cylinder_buffer(struct e3d_buffer *buf, math_v3 extends,
								size_t detail)
{
	static const GLfloat pi = 3.14159265358;
	GLfloat udeg, factor;
	size_t i;
	GLfloat *v;

	assert(detail >= 5);
	assert(buf->num >= (detail * 2 + (detail - 1) * 6));

	v = (void*)buf->vertex;
	factor = 2 * pi / (detail - 1);
	i = 0;

	v[i * 4] = 0.0;
	v[i * 4 + 1] = 0.0;
	v[i * 4 + 2] = -extends[2];
	v[i * 4 + 3] = 1.0;

	for (i += 1; i < detail; ++i) {
		udeg = (i - 1) * factor;

		v[i * 4] = cos(udeg) * extends[0];
		v[i * 4 + 1] = sin(udeg) * extends[1];
		v[i * 4 + 2] = -extends[2];
		v[i * 4 + 3] = 1.0;
	}

	v[i * 4] = 0.0;
	v[i * 4 + 1] = 0.0;
	v[i * 4 + 2] = extends[2];
	v[i * 4 + 3] = 1.0;

	for (i += 1; i < (detail * 2); ++i) {
		udeg = ((i - detail) - 1) * factor;

		v[i * 4] = cos(udeg) * extends[0];
		v[i * 4 + 1] = sin(udeg) * extends[1];
		v[i * 4 + 2] = extends[2];
		v[i * 4 + 3] = 1.0;
	}
}
#endif

#if 0
/*
 * This creates a new cylinder with the given extends. It creates two circles
 * for the bottom and top with triangle fans and one primitive with triangles
 * for the roundup.
 */
static int create_cylinder(struct e3d_shape *shape, math_v3 extends,
								math_v4 color)
{
	struct e3d_shape *bottom, *top, *round;
	struct e3d_primitive *pbottom, *ptop, *pround;
	struct e3d_buffer *buf;
	int ret = 0;
	static const size_t detail = 10;
	size_t i;

	assert(detail >= 5);

	if (extends[0] <= 0 || extends[1] <= 0 || extends[2] <= 0)
		return -EINVAL;

	buf = e3d_buffer_new(detail * 2 + (detail - 1) * 6,
		E3D_BUFFER_VERTEX | E3D_BUFFER_COLOR | E3D_BUFFER_NORMAL);
	if (!buf)
		return -ENOMEM;

	create_cylinder_buffer(buf, extends, detail);

	for (i = 0; i < buf->num; ++i)
		memcpy(&buf->color[i], color, sizeof(GLfloat) * 4);

	pbottom = e3d_primitive_new(detail + 1);
	if (!pbottom) {
		ret = -ENOMEM;
		goto err_buf;
	}

	ptop = e3d_primitive_new(detail + 1);
	if (!ptop) {
		ret = -ENOMEM;
		goto err_pbottom;
	}

	pround = e3d_primitive_new((detail - 1) * 6);
	if (!pround) {
		ret = -ENOMEM;
		goto err_ptop;
	}

	bottom = e3d_shape_new();
	if (!bottom) {
		ret = -ENOMEM;
		goto err_pround;
	}

	top = e3d_shape_new();
	if (!top) {
		ret = -ENOMEM;
		goto err_bottom;
	}

	round = e3d_shape_new();
	if (!round) {
		ret = -ENOMEM;
		goto err_top;
	}

	/* create triangle fan buffer for bottom circle */
	pbottom->ibuf[0] = 0;
	for (i = 1; i < detail; ++i) {
		/* reverse order as we have to face down */
		pbottom->ibuf[i] = detail - i;
	}
	pbottom->ibuf[i] = detail - 1;

	pbottom->type = GL_TRIANGLE_FAN;
	e3d_primitive_set_buffer(pbottom, buf);
	e3d_shape_set_primitive(bottom, pbottom);

	/* create triangle fan buffer for upper circle */
	ptop->ibuf[0] = detail;
	for (i = 1; i < detail; ++i) {
		ptop->ibuf[i] = detail + i;
	}
	ptop->ibuf[i] = detail + 1;

	ptop->type = GL_TRIANGLE_FAN;
	e3d_primitive_set_buffer(ptop, buf);
	e3d_shape_set_primitive(top, ptop);

	memset(pround->ibuf, 0, sizeof(GLuint) * (detail - 1) * 6);
	/* create triangles buffer for side wall */
	for (i = 0; i < (detail - 1); ++i) {
		pround->ibuf[i * 6] = i + 1;
		pround->ibuf[i * 6 + 1] = i + 2;
		pround->ibuf[i * 6 + 2] = detail + i + 1;
		pround->ibuf[i * 6 + 3] = detail + i + 2;
		pround->ibuf[i * 6 + 4] = detail + i + 1;
		pround->ibuf[i * 6 + 5] = i + 2;
	}
	/* last index must point to first triangle; fix this */
	i = detail - 2;
	pround->ibuf[i * 6] = i + 1;
	pround->ibuf[i * 6 + 1] = 1;
	pround->ibuf[i * 6 + 2] = detail + i + 1;
	pround->ibuf[i * 6 + 3] = detail + 1;
	pround->ibuf[i * 6 + 4] = detail + i + 1;
	pround->ibuf[i * 6 + 5] = 1;

	pround->type = GL_TRIANGLES;
	e3d_primitive_set_buffer(pround, buf);
	e3d_shape_set_primitive(round, pround);

	e3d_shape_link(shape, bottom);
	e3d_shape_link(shape, top);
	e3d_shape_link(shape, round);

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
err_buf:
	e3d_buffer_unref(buf);
	return ret;
}

/*
 * This loads \e as cylinder parameter into \shape. It is added as child entries
 * to \shape.
 * Returns 0 on success.
 */
static int load_cylinder(struct e3d_shape *shape, const struct uconf_entry *e)
{
	int ret = 0;
	const struct uconf_entry *iter;
	struct e3d_shape *new;
	math_v3 extends = { 0.0, 0.0, 0.0 };
	math_v4 color = { 1.0, 1.0, 1.0, 1.0 };

	if (e->type != UCONF_ENTRY_LIST)
		return -EINVAL;

	new = e3d_shape_new();
	if (!new)
		return -ENOMEM;

	UCONF_ENTRY_FOR(e, iter) {
		if (!iter->name)
			goto err;
		else if (cstr_strcmp(iter->name, -1, "extents"))
			ret = load_vec(iter, (void*)extends, 3);
		else if (cstr_strcmp(iter->name, -1, "color"))
			ret = load_vec(iter, (void*)color, 4);
		else
			ret = load_shape_generic(new, iter);

		if (ret)
			goto err;
	}

	ret = create_cylinder(new, extends, color);
	if (ret)
		goto err;

	e3d_shape_link(shape, new);
	ret = 0;

err:
	e3d_shape_unref(new);
	return ret;
}
#endif

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
