/*
 * airhockey - config file loader
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <libcstr.h>
#include <libuconf.h>

#include "engine3d.h"
#include "log.h"
#include "main.h"
#include "mathw.h"

int config_load_size(const struct uconf_entry *e, size_t *out)
{
	size_t tmp;

	if (e->type != UCONF_ENTRY_QINT)
		return -EINVAL;

	tmp = e->v.qint;
	if (tmp < 0)
		return -EINVAL;

	*out = tmp;
	return 0;
}

/*
 * Load \e into \out. \e is interpreted as a floating point value, that is, it
 * must either be a QFLOAT or QINT.
 * Returns 0 on success or an error code on parsing failure.
 */
int config_load_float(const struct uconf_entry *e, float *out)
{
	if (e->type == UCONF_ENTRY_QFLOAT)
		*out = e->v.qfloat;
	else if (e->type == UCONF_ENTRY_QINT)
		*out = e->v.qint;
	else
		return -EINVAL;

	return 0;
}

static int load_vec(const struct uconf_entry *e, float *out, size_t min,
								size_t max)
{
	const struct uconf_entry *iter;
	int ret;
	size_t i;

	if (e->type != UCONF_ENTRY_LIST || e->v.list.num < min ||
							e->v.list.num > max)
		return -EINVAL;

	i = 0;
	UCONF_ENTRY_FOR(e, iter) {
		ret = config_load_float(iter, &out[i++]);
		if (ret)
			return ret;
	}

	return 0;
}

int config_load_v3(const struct uconf_entry *e, math_v3 out)
{
	return load_vec(e, (void*)out, 3, 3);
}

int config_load_v4(const struct uconf_entry *e, math_v4 out)
{
	return load_vec(e, (void*)out, 4, 4);
}

int config_load_v34(const struct uconf_entry *e, math_v4 out, float d4)
{
	out[3] = d4;
	return load_vec(e, (void*)out, 3, 4);
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
