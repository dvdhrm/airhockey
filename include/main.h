/*
 * airhockey - main header
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#ifndef MAIN_H
#define MAIN_H

#include <signal.h>
#include <stdint.h>
#include <stdlib.h>

#include <libcstr.h>
#include <libuconf.h>

#include "engine3d.h"
#include "log.h"

extern sig_atomic_t terminating;

extern int64_t misc_now();
extern int misc_load_file(const cstr *file, char **buf, size_t *size);

extern int config_load(struct uconf_entry **root, const cstr *path);
extern int config_load_shape(struct e3d_shape **shape,
						const struct uconf_entry *e);

struct shaders {
	struct e3d_shader *debug;
	struct e3d_shader *simple;
};

extern int game_run(struct ulog_dev *log, struct e3d_window *wnd,
						struct shaders *shaders);

#endif /* MAIN_H */
