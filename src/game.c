/*
 * airhockey - game play
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <libcstr.h>

#include "engine3d.h"
#include "log.h"
#include "main.h"
#include "mathw.h"
#include "physics.h"

struct game {
	struct ulog_dev *log;
	struct e3d_window *wnd;
	struct shaders *shaders;
	int64_t tick_time;

	struct phys_world *phys;
	struct e3d_transform trans;
	struct e3d_eye eye;
	struct e3d_shape *room;
	struct e3d_shape *test;
};

static inline int game_render(struct game *game)
{
	math_v3 pos = { 8.0, 10.0, 10.0 };
	math_v3 at = { 0.0, 0.0, 0.0 };
	math_v3 ori = { 0.0, 0.0, 1.0 };
	math_v3 l = { 0.0, 0.0, 3.0 };
	math_v3 lup = { 0.0, 1.0, 0.0 };
	const struct e3d_shader_locations *loc;
	struct e3d_light light;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);

	e3d_eye_look_at(&game->eye, pos, at, ori);

	e3d_transform_reset(&game->trans);

	e3d_window_projection(game->wnd, MATH_TIP(&game->trans.proj_stack));
	e3d_eye_apply(&game->eye, MATH_TIP(&game->trans.eye_stack));

	e3d_light_init(&light);
	e3d_light_look_at(&light, l, at, lup);

	e3d_shader_use(game->shaders->debug);
	loc = e3d_shader_locations(game->shaders->debug);
	e3d_eye_supply(&game->eye, loc);
	e3d_light_supply(&light, 0, loc);
	e3d_shape_draw(game->room, loc, &game->trans);
	e3d_shape_draw(game->test, loc, &game->trans);

	e3d_shader_use(game->shaders->simple);
	loc = e3d_shader_locations(game->shaders->simple);

	glLineWidth(5.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDepthFunc(GL_LEQUAL);
	glCullFace(GL_FRONT);

	e3d_shape_draw_silhouette(game->room, loc, &game->trans);
	e3d_shape_draw_silhouette(game->test, loc, &game->trans);

	e3d_shader_use(game->shaders->simple);
	loc = e3d_shader_locations(game->shaders->simple);
	glLineWidth(1.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);
	e3d_shape_draw_normals(game->room, loc, &game->trans);
	e3d_shape_draw_normals(game->test, loc, &game->trans);

	e3d_window_frame(game->wnd);
	e3d_etest();

	return 0;
}

static inline int game_step_physics(struct game *game, int64_t step)
{
	return phys_world_step(game->phys, step);
}

static inline int game_step_world(struct game *game)
{
	if (!e3d_window_poll(game->wnd))
		return -1;

	return 0;
}

static int game_loop(struct game *game)
{
	int ret;
	bool done;
	int64_t phys_time, phys_curr;
	int64_t last_tick, sum;

	assert(game->tick_time > 0);

	ulog_flog(game->log, ULOG_INFO, "Starting mainloop\n");

	phys_time = misc_now();
	last_tick = phys_time;

	struct phys_body *ground = phys_body_new();
	struct phys_body *obj = phys_body_new();
	phys_body_set_shape_ground(ground);
	phys_body_set_shape_sphere(obj);

	phys_world_add(game->phys, ground);
	phys_world_add(game->phys, obj);

	ret = 0;
	done = false;
	while (!done) {
		if (terminating)
			done = true;

		ret = game_render(game);
		if (ret)
			done = true;

		phys_curr = misc_now();
		ret = game_step_physics(game, phys_curr - phys_time);
		if (ret)
			done = true;
		phys_time = phys_curr;

		sum = misc_now() - last_tick;
		while (sum >= game->tick_time) {
			sum -= game->tick_time;
			last_tick += game->tick_time;

			ret = game_step_world(game);
			if (ret) {
				done = true;
				break;
			}
		}

		math_v3 v;
		phys_body_get_transform(obj, v);
		math_m4_identity(game->test->alter);
		math_m4_translatev(game->test->alter, v);
	}

	ulog_flog(game->log, ULOG_INFO, "Exiting mainloop\n");

	return ret;
}

int game_run(struct ulog_dev *log, struct e3d_window *wnd,
							struct shaders *shaders)
{
	int ret;
	struct e3d_shape *room, *test;
	struct uconf_entry *conf;
	struct game game;

	ret = config_load(&conf, cstr_strdup(-1, "data/room.conf"));
	if (ret)
		ulog_flog(log, ULOG_FATAL, "Cannot load room config\n");

	ret = config_load_shape(&room, conf);
	if (ret)
		ulog_flog(log, ULOG_FATAL, "Cannot load room shape\n");

	uconf_entry_unref(conf);

	ret = config_load(&conf, cstr_strdup(-1, "data/test.conf"));
	if (ret)
		ulog_flog(log, ULOG_FATAL, "Cannot load test config\n");

	ret = config_load_shape(&test, conf);
	if (ret)
		ulog_flog(log, ULOG_FATAL, "Cannot load test shape\n");

	uconf_entry_unref(conf);

	memset(&game, 0, sizeof(game));
	game.log = log;
	game.wnd = wnd;
	game.shaders = shaders;
	game.tick_time = 20000;
	e3d_eye_init(&game.eye);
	e3d_transform_init(&game.trans);
	game.room = room;
	game.test = test;

	game.phys = phys_world_new(log);
	if (!game.phys) {
		ret = -ENOMEM;
		goto err_math;
	}

	e3d_shader_use(shaders->debug);

	ret = game_loop(&game);

	phys_world_free(game.phys);

err_math:
	e3d_transform_destroy(&game.trans);
	e3d_eye_destroy(&game.eye);

	e3d_shape_unref(room);

	return ret;
}
