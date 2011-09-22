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

/*
 * Some comment on creating circles:
 * We always create circles with the TRIANGLES_FAN object.
 * A circle has at least 5 vertices in the horizontal layer:
 *            *
 *
 *       *    *    *
 *
 *            *
 * Increasing this count will render the round object better:
 *          * * *
 *        *       *
 *       *    *    *
 *        *       *
 *          * * *
 * This looks much more like a round object. The number of
 * vertices can be anything greater than or equal to five.
 * When passing the \*_num argument, you pass the number of
 * vertices which are used in the circle.
 */

/*
 * Creating circles.
 * This function is used to create a circle.
 * You can pass the number of vertices used in the circle and
 * you can specify the radius. The object is rendered around
 * [0|0|0] by default.
 * \num must be at least 5.
 * All vertexes are stored in sequential order so you can use an
 * TRIANGLE_FAN to draw the circle.
 */
#if 0
static int create_circle(struct world_shape *shape, GLfloat rad, size_t num,
							const GLfloat *color)
{
	static const GLfloat pi = 3.14159265358;
	GLfloat udeg, factor;
	size_t i;
	GLfloat *v, *c;
	int ret;

	assert(rad > 0);
	assert(num >= 5);

	ret = world_shape_alloc(shape, num + 1, GL_TRIANGLE_FAN);
	if (ret)
		return ret;

	v = shape->vertex;
	c = shape->color;

	/* center is always at 0/0/0 */
	v[0] = 0.0;
	v[1] = 0.0;
	v[2] = 0.0;
	v[3] = 1.0;
	memcpy(c, color, sizeof(GLfloat) * 4);

	factor = 2 * pi / (num - 1);

	for(i = 1; i < num; ++i) {
		udeg = (i - 1) * factor;

		v[i * 4] = cos(udeg) * rad;
		v[i * 4 + 1] = sin(udeg) * rad;
		v[i * 4 + 2] = 0.0;
		v[i * 4 + 3] = 1.0;
		memcpy(&c[i * 4], color, sizeof(GLfloat) * 4);
	}

	/* last vertex is always the start again to close the circle */
	v[i * 4] = v[4];
	v[i * 4 + 1] = v[5];
	v[i * 4 + 2] = v[6];
	v[i * 4 + 3] = v[7];
	memcpy(&c[i * 4], color, sizeof(GLfloat) * 4);

	return 0;
}
#endif

/*
 * Load \e into \out. \e is interpreted as a floating point value, that is, it
 * must either be a QFLOAT or QINT.
 * Returns 0 on success or an error code on parsing failure.
 */
static int load_float(const struct uconf_entry *e, GLfloat *out)
{
	int ret = 0;

	if (e->type == UCONF_ENTRY_QFLOAT)
		*out = e->v.qfloat;
	else if (e->type == UCONF_ENTRY_QINT)
		*out = e->v.qint;
	else
		ret = -EINVAL;

	return ret;
}

/*
 * Load floating point vector. This reads \num values from \e and saves them
 * into the array \out.
 * \e must be a list with exactly \num entries. The entries may be anonymous,
 * that is, their name is ignored.
 * Returns 0 on success or an error code on failure.
 *
 * If \num is 0, nothing is done. If \num is 1, then \e can also be a single
 * value and does not have to be a list.
 */
static int load_vec(const struct uconf_entry *e, GLfloat *out, size_t num)
{
	const struct uconf_entry *iter;
	int ret;

	if (num == 0)
		return 0;

	if (e->type != UCONF_ENTRY_LIST) {
		if (num == 1)
			return load_float(e, out);
		else
			return -EINVAL;
	}

	for (iter = e->v.list.first; iter; iter = iter->next) {
		if (num) {
			ret = load_float(iter, out);
			if (ret)
				return ret;
		} else {
			return -EINVAL;
		}

		++out;
		--num;
	}

	if (num)
		return -EINVAL;

	return 0;
}

/* forward declaration to allow recursion */
static int load_shape_generic(struct e3d_shape *shape,
						const struct uconf_entry *p);

/*
 * This loads \e as circle parameter into \shape. It is added as child entry to
 * \shape.
 * Returns 0 on success.
 */
#if 0
static int load_circle(struct world_shape *shape, const struct uconf_entry *e)
{
	int ret;
	const struct uconf_entry *iter;
	struct world_shape *news;
	GLfloat radius = 1;
	size_t count = 10;
	GLfloat color[4] = { 1.0, 1.0, 1.0, 1.0 };

	if (e->type != UCONF_ENTRY_LIST)
		return -EINVAL;

	news = world_shape_new();
	if (!news)
		return -ENOMEM;

	for (iter = e->v.list.first; iter; iter = iter->next) {
		if (!iter->name)
			ret = -EINVAL;
		if (cstr_strcmp(iter->name, -1, "radius"))
			ret = load_float(iter, &radius);
		else if (cstr_strcmp(iter->name, -1, "count"))
			ret = load_size(iter, &count);
		else if (cstr_strcmp(iter->name, -1, "color"))
			ret = load_vec(iter, color, 4);
		else
			ret = load_generic(news, iter);

		if (ret)
			goto err;
	}

	if (radius <= 0 || count < 5)
		goto err;

	ret = create_circle(news, radius, count, color);
	if (ret)
		goto err;

	news->next = shape->childs;
	shape->childs = news;

	return 0;

err:
	world_shape_free(news);
	return ret;
}
#endif

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
	const struct uconf_entry *iter;
	GLfloat normal[4];
	int ret;

	if (e->type != UCONF_ENTRY_LIST)
		return -EINVAL;

	/*
	 * If is is only one vector entry, then we simply copy this entry over
	 * the whole buffer.
	 */
	if (e->v.list.first && e->v.list.first->type != UCONF_ENTRY_LIST) {
		ret = load_vec(e, normal, 4);
		if (ret)
			return ret;
		for (i = 0; i < buf->num; ++i)
			memcpy(&buf->normal[i], normal, sizeof(normal));
		return 0;
	}

	i = 0;
	UCONF_ENTRY_FOR(e, iter) {
		if (i >= buf->num)
			return -EINVAL;
		ret = load_vec(iter, (void*)&buf->normal[i++], 4);
		if (ret)
			return -EINVAL;
	}

	return 0;
}

static int load_buffer(struct e3d_buffer **buf, const struct uconf_entry *e)
{
	const struct uconf_entry *iter, *vertex, *color, *normal;
	size_t num;
	int ret = 0;
	uint8_t type = 0;
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

	new = e3d_buffer_new(num, type | E3D_BUFFER_VERTEX | E3D_BUFFER_NORMAL);
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
	} else {
		e3d_buffer_generate_triangle_normals(new);
	}

	*buf = e3d_buffer_ref(new);

err:
	e3d_buffer_unref(new);
	return ret;
}

/*
 * This loads \e as triangle parameter into \shape. It is added as child entry
 * to \shape.
 * Returns 0 on success.
 */
static int load_triangle(struct e3d_shape *shape, const struct uconf_entry *e)
{
	int ret = 0;
	const struct uconf_entry *iter, *buffer = NULL;
	struct e3d_shape *new;
	struct e3d_buffer *buf;
	struct e3d_primitive *prim;

	if (e->type != UCONF_ENTRY_LIST)
		return -EINVAL;

	new = e3d_shape_new();
	if (!new)
		return -ENOMEM;

	for (iter = e->v.list.first; iter; iter = iter->next) {
		if (!iter->name)
			ret = -EINVAL;
		else if (cstr_strcmp(iter->name, -1, "buffer"))
			buffer = iter;
		else
			ret = load_shape_generic(new, iter);

		if (ret)
			goto err;
	}

	if (!buffer) {
		ret = -EINVAL;
		goto err;
	}

	ret = load_buffer(&buf, buffer);
	if (ret)
		goto err;

	prim = e3d_primitive_new(0);
	if (!prim) {
		ret = -ENOMEM;
		goto err_buf;
	}

	prim->type = GL_TRIANGLES;

	e3d_primitive_set_buffer(prim, buf);
	e3d_shape_set_primitive(new, prim);
	e3d_shape_link(shape, new);
	ret = 0;

	e3d_primitive_unref(prim);
err_buf:
	e3d_buffer_unref(buf);
err:
	e3d_shape_unref(new);
	return ret;
}

/*
 * Loads generic shape values into \shape from \e. Returns error if an unknwon
 * entry is found.
 */
static int load_shape_generic(struct e3d_shape *shape,
						const struct uconf_entry *e)
{
	math_v3 v;
	int ret;

	if (!e->name) {
		return -EINVAL;
	} else if (cstr_strcmp(e->name, -1, "triangle")) {
		return load_triangle(shape, e);
	} else if (cstr_strcmp(e->name, -1, "translate")) {
		ret = load_vec(e, v, 3);
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

	v = e3d_shape_new();
	if (!v)
		ret = -ENOMEM;

	UCONF_ENTRY_FOR(e, iter) {
		ret = load_shape_generic(v, iter);
		if (ret)
			break;
	}

	if (!ret)
		*shape = e3d_shape_ref(v);

	e3d_shape_unref(v);
	return ret;
}

/*
 * Loads the config file at \path into \root. \root is created by this function.
 * \root must be destroyed/freed by the caller when no longer needed.
 * Return 0 on success.
 */
int config_load(struct uconf_entry **root, const cstr *path)
{
	int ret = 0;
	struct uconf_file *f;
	struct uconf_entry *v;

	f = uconf_file_new();
	if (!f)
		return -ENOMEM;

	ret = uconf_file_open(f, path, UCONF_FILE_READ);
	if (ret)
		goto err_file;

	v = uconf_entry_new();
	if (!v) {
		ret = -ENOMEM;
		goto err_file;
	}

	ret = uconf_parse(v, f);
	if (!ret)
		*root = uconf_entry_ref(v);

	uconf_entry_unref(v);
err_file:
	uconf_file_free(f);
	return ret;
}
