/*
 * airhockey - main source
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <SFML/OpenGL.h>

#include "main.h"
#include "engine3d.h"

static int create_shader(struct e3d_shader **shader, GLuint *uni_mod,
							GLuint *uni_proj)
{
	int ret = 0;

	ret = e3d_shader_new(shader, E3D_SHADER_DEBUG);
	if (ret) {
		ulog_flog(NULL, ULOG_DEBUG, "Cannot allocate shader\n");
		goto err;
	}

	*uni_mod = e3d_shader_uniform(*shader, "modelview_matrix");
	if (*uni_mod == -1) {
		ulog_flog(NULL, ULOG_DEBUG, "Cannot bind shader uniform\n");
		ret = -EINVAL;
		goto err_shader;
	}

	*uni_proj = e3d_shader_uniform(*shader, "projection_matrix");
	if (*uni_proj == -1) {
		ulog_flog(NULL, ULOG_DEBUG, "Cannot bind shader uniform\n");
		ret = -EINVAL;
		goto err_shader;
	}

	return 0;

err_shader:
	e3d_shader_free(*shader);
err:
	return ret;
}

static int run_window(struct e3d_window *wnd)
{
	int ret;
	struct e3d_obj *obj;
	struct e3d_shader *shader;
	GLuint uni_mod, uni_proj;

	ret = create_shader(&shader, &uni_mod, &uni_proj);
	if (ret) {
		ulog_flog(NULL, ULOG_ERROR, "Cannot create shader\n");
		return ret;
	}

	e3d_shader_use(shader);

	ret = e3d_obj_new(&obj, NULL);
	if (ret) {
		ulog_flog(NULL, ULOG_ERROR, "Cannot create object\n");
		e3d_shader_free(shader);
		return ret;
	}

	e3d_obj_grab(obj);

	ulog_flog(NULL, ULOG_INFO, "Starting window\n");

	while (e3d_window_poll(wnd)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(8.0, 10.0, 10.0,
				0.0, 0.0, 0.0,
				0.0, 1.0, 0.0);
		GLfloat modelview[16], projection[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
		glGetFloatv(GL_PROJECTION_MATRIX, projection);
		E3D(glUniformMatrix4fv(uni_mod, 1, 0, modelview));
		E3D(glUniformMatrix4fv(uni_proj, 1, 0, projection));
		e3d_obj_draw(obj);

		e3d_window_frame(wnd);
		e3d_etest();
	}

	ulog_flog(NULL, ULOG_INFO, "Exiting window\n");

	e3d_obj_free(obj);
	e3d_shader_free(shader);

	return 0;
}

int main()
{
	int ret = 0;
	struct ulog_dev *e3d_log;
	struct ulog_target target;
	struct e3d_window *wnd;

	ulog_flog(NULL, ULOG_INFO, "Starting\n");

	e3d_log = ulog_new("E3D: ");
	if (!e3d_log) {
		ulog_flog(NULL, ULOG_ERROR, "Cannot allocate log-device\n");
		ret = -ENOMEM;
		goto err;
	}

	memcpy(&target, &ulog_t_stderr, sizeof(target));
	target.severity = ULOG_DEBUG;

	ret = ulog_add_target(e3d_log, &target);
	if (ret) {
		ulog_flog(NULL, ULOG_ERROR, "Cannot add log target\n");
		goto err_log;
	}

	e3d_set_log(e3d_log);

	wnd = e3d_window_init();
	if (!wnd) {
		ulog_flog(NULL, ULOG_ERROR, "Cannot create GL context\n");
		ret = -1;
		goto err_log;
	}

	ret = run_window(wnd);

	e3d_window_destroy(wnd);
err_log:
	ulog_free(e3d_log);
err:
	if (ret)
		ulog_flog(NULL, ULOG_ERROR, "Abort\n");
	else
		ulog_flog(NULL, ULOG_INFO, "Exiting\n");
	return -ret;
}
