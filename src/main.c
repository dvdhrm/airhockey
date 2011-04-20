/*
 * airhockey - main source
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <unistd.h>

#include <SFML/Window.h>

#include "main.h"

static sfWindow *wnd;

typedef int64_t env_time_t;

enum env_clock {
	ENV_CLOCK_REAL,
	ENV_CLOCK_MONO
};

static inline env_time_t env_time_now(enum env_clock clock)
{
	env_time_t ret;
	struct timespec val;

	if (clock == ENV_CLOCK_REAL)
		clock_gettime(CLOCK_REALTIME, &val);
	else
		clock_gettime(CLOCK_MONOTONIC, &val);

	ret = val.tv_sec * 1000000;
	ret += val.tv_nsec / 1000;
	return ret;
}

void checkerror() {
	GLenum error;

	error = glGetError();
	if(error == GL_NO_ERROR) return;

	/* GL error occurred. GL errors should never occur,
	* they either identify memory failures or wrong parameters.
	* Abort the application now.
	*/
	printf("OpenGL error occurred: %d (errno: %d)\n", error, errno);
	abort();
}

static void event_close()
{
	sfWindow_Close(wnd);
}

static void event_resize(const sfEvent *ev)
{
	glViewport(0, 0, ev->Size.Width, ev->Size.Height);
}

int main()
{
	sfWindowSettings set;
	sfVideoMode mode;
	sfEvent ev;
	env_time_t last, now;
	uint64_t frames, fps;
	struct object obj;
	GLuint shader;
	GLuint uni_mod, uni_proj;
	GLint bo;

	set.DepthBits = 8;
	set.StencilBits = 8;
	set.AntialiasingLevel = 4;

	mode.Width = 200;
	mode.Height = 200;
	mode.BitsPerPixel = 32;

	wnd = sfWindow_Create(mode, "Test Window", sfTitlebar | sfClose | sfResize, set);
	if (!wnd) {
		printf("Cannot create window\n");
		abort();
	}

	sfWindow_SetActive(wnd, true);
	sfWindow_Show(wnd, true);
	sfWindow_SetFramerateLimit(wnd, 30);
	sfWindow_UseVerticalSync(wnd, false);
	glViewport(0, 0, 200, 200);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(52.0f, 200 / 200, 1.0f, 100.0f);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader = shader_compile();
	shader_link(shader);
	uni_mod = glGetUniformLocation(shader, "modelview_matrix");
	uni_proj = glGetUniformLocation(shader, "projection_matrix");
	glUseProgram(shader);

	set = sfWindow_GetSettings(wnd);
	printf("%d %d %d\n", set.DepthBits, set.StencilBits, set.AntialiasingLevel);

	last = env_time_now(ENV_CLOCK_MONO);
	frames = 0;
	fps = 0;

	obj_grab(&obj);

	glEnable(GL_MULTISAMPLE);
	glGetIntegerv(GL_MULTISAMPLE, &bo);
	printf("multisample: %i\n", bo);
	bo = 1;
	glGetIntegerv(GL_SAMPLE_BUFFERS, &bo);
	printf("sample buffers: %i\n", bo);

	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);

	checkerror();

	while (sfWindow_IsOpened(wnd)) {
		while (sfWindow_GetEvent(wnd, &ev)) {
			if (ev.Type == sfEvtClosed)
				event_close(wnd);
			if (ev.Type == sfEvtResized)
				event_resize(&ev);
		}

		if (!sfWindow_IsOpened(wnd))
			break;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(8.0, 10.0, 10.0,
				0.0, 0.0, 0.0,
				0.0, 1.0, 0.0);
		GLfloat modelview[16], projection[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
		glGetFloatv(GL_PROJECTION_MATRIX, projection);
		glUniformMatrix4fv(uni_mod, 1, 0, modelview);
		glUniformMatrix4fv(uni_proj, 1, 0, projection);
		obj_draw(&obj);

		sfWindow_Display(wnd);

		++frames;
		++fps;

		now = env_time_now(ENV_CLOCK_MONO);
		if (now > last + 1000000) {
			printf("1s: %u %u frames\n", fps, frames);
			fps = 0;
			last = now;
		}
		checkerror();
	}

	sfWindow_Destroy(wnd);

	return 0;
}
