/*
 * airhockey - main source
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "engine3d.h"
#include "log.h"
#include "main.h"

sig_atomic_t terminating = 0;

static struct ulog_dev *debug_log(const char *prefix)
{
	static struct ulog_target target;
	static bool created;
	struct ulog_dev *log;

	log = ulog_new(prefix);
	if (!log)
		return NULL;

	if (!created) {
		memcpy(&target, &ulog_t_stderr, sizeof(target));
		target.severity = ULOG_DEBUG;
		created = true;
	}

	if (ulog_add_target(log, &target)) {
		ulog_unref(log);
		return NULL;
	}

	return log;
}

static void signal_handler(int signum)
{
	terminating = 1;
}

static int signal_setup()
{
	struct sigaction act;
	int ret, i;
	int signals[] = { SIGINT, SIGHUP, SIGTERM, -1 };

	memset(&act, 0, sizeof(act));
	act.sa_handler = signal_handler;

	for (i = 0; signals[i] != -1; ++i) {
		ret = sigaction(signals[i], &act, NULL);
		if (ret)
			return -errno;
	}

	return 0;
}

static int init_subsystems()
{
	struct ulog_dev *log;
	int ret;

	ret = signal_setup();
	if (ret)
		return ret;

	log = debug_log("Math: ");
	if (!log)
		return -ENOMEM;

	math_init(log);
	ulog_unref(log);

	log = debug_log("E3D: ");
	if (!log) {
		math_destroy();
		return -ENOMEM;
	}

	e3d_init(log);
	ulog_unref(log);

	ulog_flog(NULL, ULOG_INFO, "Creating subsystems\n");

	return 0;
}

static void destroy_subsystems()
{
	ulog_flog(NULL, ULOG_INFO, "Destroying subsystems\n");
	e3d_destroy();
	math_destroy();
}

static int init_shaders(struct shaders *shaders)
{
	int ret;

	ret = e3d_shader_new(&shaders->debug, E3D_SHADER_DEBUG);
	if (ret)
		return ret;

	ret = e3d_shader_new(&shaders->normals, E3D_SHADER_NORMALS);
	if (ret) {
		e3d_shader_free(shaders->debug);
		return ret;
	}

	return 0;
}

static void destroy_shaders(struct shaders *shaders)
{
	e3d_shader_free(shaders->normals);
	e3d_shader_free(shaders->debug);
}

int main()
{
	int ret = 0;
	struct e3d_window *wnd;
	struct shaders shaders;
	struct ulog_dev *log;

	ulog_flog(NULL, ULOG_INFO, "Starting\n");

	ret = init_subsystems();
	if (ret)
		goto err;

	wnd = e3d_window_new();
	if (!wnd) {
		ret = -1;
		goto err_subs;
	}

	ret = init_shaders(&shaders);
	if (ret)
		goto err_wnd;

	log = debug_log("Game: ");
	if (!log) {
		ret = -ENOMEM;
		goto err_shader;
	}

	ret = game_run(log, wnd, &shaders);

	ulog_unref(log);
err_shader:
	destroy_shaders(&shaders);
err_wnd:
	e3d_window_free(wnd);
err_subs:
	destroy_subsystems();
err:
	if (ret)
		ulog_flog(NULL, ULOG_ERROR, "Abort\n");
	else
		ulog_flog(NULL, ULOG_INFO, "Exiting\n");
	return -ret;
}
