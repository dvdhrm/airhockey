/*
 * airhockey - 3D engine - window and GL context handling
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <SFML/OpenGL.h>
#include <SFML/Window.h>

#include "engine3d.h"
#include "log.h"
#include "main.h"
#include "mathw.h"

struct e3d_window {
	sfWindow *ctx;
	uint64_t frames;
	int64_t last_frame;

	int64_t fps_secs;
	uint64_t fps_count;

};

static void event_close(struct e3d_window *wnd)
{
	ulog_flog(e3d_log, ULOG_DEBUG, "Window: Received close event on "
							"window %p\n", wnd);
	sfWindow_Close(wnd->ctx);
}

static void event_resize(struct e3d_window *wnd, const sfEvent *ev)
{
	ulog_flog(e3d_log, ULOG_DEBUG, "Window: Received resize event on "
							"window %p\n", wnd);
	glViewport(0, 0, ev->Size.Width, ev->Size.Height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(52.0f, ev->Size.Width / ev->Size.Height, 1.0f, 100.0f);
}

struct e3d_window *e3d_window_new()
{
	struct e3d_window *wnd;
	sfContextSettings set;
	sfVideoMode mode;
	sfEvent ev;

	memset(&set, 0, sizeof(set));
	set.StencilBits = 8;
	set.AntialiasingLevel = 4;

	memset(&mode, 0, sizeof(mode));
	mode.Width = 200;
	mode.Height = 200;
	mode.BitsPerPixel = 32;

	wnd = malloc(sizeof(*wnd));
	if (!wnd) {
		ulog_flog(e3d_log, ULOG_ERROR,
					"Window: Cannot allocate memory\n");
		return NULL;
	}

	memset(wnd, 0, sizeof(*wnd));
	wnd->last_frame = misc_now();

	wnd->ctx = sfWindow_Create(mode, "Test Window", sfTitlebar | sfClose |
								sfResize, &set);
	if (!wnd->ctx) {
		ulog_flog(e3d_log, ULOG_ERROR,
				"Window: Cannot create window context\n");
		goto err_wnd;
	}

	if (!sfWindow_SetActive(wnd->ctx, true)) {
		ulog_flog(e3d_log, ULOG_ERROR,
					"Window: Cannot activate window\n");
		goto err_ctx;
	}

	ulog_flog(e3d_log, ULOG_DEBUG,
		"Window: Creating window %p (frames %lu)\n", wnd, wnd->frames);

	ev.Size.Width = mode.Width;
	ev.Size.Height = mode.Height;
	ev.Type = sfEvtResized;
	event_resize(wnd, &ev);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	sfWindow_Display(wnd->ctx);
	sfWindow_SetFramerateLimit(wnd->ctx, 0);
	sfWindow_Show(wnd->ctx, true);

	return wnd;

err_ctx:
	sfWindow_Destroy(wnd->ctx);
err_wnd:
	free(wnd);
	return NULL;
}

void e3d_window_free(struct e3d_window *wnd)
{
	ulog_flog(e3d_log, ULOG_DEBUG, "Window: Destroying window %p "
					"(frames %lu)\n", wnd, wnd->frames);
	sfWindow_Destroy(wnd->ctx);
	free(wnd);
}

void e3d_window_activate(struct e3d_window *wnd)
{
	sfWindow_SetActive(wnd->ctx, true);
}

static int convert_event(const sfEvent *event, struct e3d_event *out)
{
	if (event->Type == sfEvtKeyPressed) {
		out->type = E3D_EV_KEY;
		out->v.key.code = event->Key.Code;
		out->v.key.value = 1;
		return 1;
	} else if (event->Type == sfEvtKeyReleased) {
		out->type = E3D_EV_KEY;
		out->v.key.code = event->Key.Code;
		out->v.key.value = 0;
		return 1;
	} else {
		return -EAGAIN;
	}
}

/*
 * Polls for window events. Returns 0 if the window got closed. Returns negative
 * error on failures and -EAGAIN if no new event can be read.
 * Returns 1 if an event was stored into \out.
 */
int e3d_window_poll(struct e3d_window *wnd, struct e3d_event *out)
{
	sfEvent ev;
	int ret;

	if (!sfWindow_IsOpened(wnd->ctx))
		return 0;

	while (sfWindow_PollEvent(wnd->ctx, &ev)) {
		if (ev.Type == sfEvtClosed) {
			event_close(wnd);
		} else if (ev.Type == sfEvtResized) {
			event_resize(wnd, &ev);
		} else {
			ret = convert_event(&ev, out);
			if (ret != -EAGAIN)
				return ret;
		}
	}

	if (!sfWindow_IsOpened(wnd->ctx))
		return 0;

	return -EAGAIN;
}

int64_t e3d_window_elapsed(struct e3d_window *wnd)
{
	return misc_now() - wnd->last_frame;
}

void e3d_window_frame(struct e3d_window *wnd)
{
	int64_t now;

	sfWindow_Display(wnd->ctx);

	now = misc_now();
	wnd->fps_secs += now - wnd->last_frame;
	wnd->fps_count++;
	wnd->frames++;

	if (wnd->fps_secs > 1000000) {
		ulog_flog(e3d_log, ULOG_DEBUG, "Window: fps %lu frames %lu\n",
						wnd->fps_count, wnd->frames);
		wnd->fps_count = 0;
		wnd->fps_secs -= 1000000;
	}

	wnd->last_frame = misc_now();
}

void e3d_window_projection(const struct e3d_window *wnd, math_m4 m)
{
	glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat*)m);
}
