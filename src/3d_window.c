/*
 * airhockey - 3D engine - window and GL context handling
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <SFML/OpenGL.h>
#include <SFML/Window.h>

#include "engine3d.h"
#include "main.h"

struct e3d_window {
	sfWindow *ctx;
	uint64_t frames;
	int64_t last_frame;

	int64_t fps_secs;
	uint64_t fps_count;

};

static void event_close(struct e3d_window *wnd)
{
	e3d_flog(ULOG_DEBUG, "Window: Received close event\n");
	sfWindow_Close(wnd->ctx);
}

static void event_resize(struct e3d_window *wnd, const sfEvent *ev)
{
	e3d_flog(ULOG_DEBUG, "Window: Received resize event\n");
	glViewport(0, 0, ev->Size.Width, ev->Size.Height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(52.0f, ev->Size.Width / ev->Size.Height, 1.0f, 100.0f);
}

struct e3d_window *e3d_window_init()
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
		e3d_flog(ULOG_DEBUG, "Window: Cannot allocate memory\n");
		return NULL;
	}

	memset(wnd, 0, sizeof(*wnd));
	wnd->last_frame = misc_now();

	wnd->ctx = sfWindow_Create(mode, "Test Window", sfTitlebar | sfClose |
								sfResize, &set);
	if (!wnd->ctx) {
		e3d_flog(ULOG_DEBUG, "Window: Cannot create window\n");
		goto err_wnd;
	}

	if (!sfWindow_SetActive(wnd->ctx, true)) {
		e3d_flog(ULOG_ERROR, "Window: Cannot activate window\n");
		goto err_ctx;
	}

	ev.Size.Width = mode.Width;
	ev.Size.Height = mode.Height;
	ev.Type = sfEvtResized;
	event_resize(wnd, &ev);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	sfWindow_Display(wnd->ctx);
	sfWindow_SetFramerateLimit(wnd->ctx, 0);
	sfWindow_Show(wnd->ctx, true);

	e3d_flog(ULOG_DEBUG, "Window: Opened (frames: %lu)\n", wnd->frames);
	return wnd;

err_ctx:
	sfWindow_Destroy(wnd->ctx);
err_wnd:
	free(wnd);
	return NULL;
}

void e3d_window_destroy(struct e3d_window *wnd)
{
	e3d_flog(ULOG_DEBUG, "Window: Destroyed (frames: %lu)\n", wnd->frames);
	sfWindow_Destroy(wnd->ctx);
}

bool e3d_window_poll(struct e3d_window *wnd)
{
	sfEvent ev;

	if (!sfWindow_IsOpened(wnd->ctx))
		return false;

	while (sfWindow_PollEvent(wnd->ctx, &ev)) {
		if (ev.Type == sfEvtClosed)
			event_close(wnd);
		else if (ev.Type == sfEvtResized)
			event_resize(wnd, &ev);
	}

	if (!sfWindow_IsOpened(wnd->ctx))
		return false;

	return true;
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
		e3d_flog(ULOG_DEBUG, "Window: fps %lu (frames: %lu)\n",
						wnd->fps_count, wnd->frames);
		wnd->fps_count = 0;
		wnd->fps_secs -= 1000000;
	}

	wnd->last_frame = misc_now();
}
