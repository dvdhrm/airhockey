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
#include "world.h"

struct game {
	struct ulog_dev *log;
	struct e3d_window *wnd;
	struct shaders *shaders;
	int64_t tick_time;

	struct world *world;
	struct e3d_transform trans;
};

static inline int game_render(struct game *game)
{
	e3d_transform_reset(&game->trans);
	e3d_window_projection(game->wnd, MATH_TIP(&game->trans.proj_stack));

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	world_draw(game->world, &game->trans, game->shaders);

	e3d_window_frame(game->wnd);
	e3d_etest();

	return 0;
}

static inline int game_step_physics(struct game *game, int64_t step)
{
	return world_step_phys(game->world, step);
}

static inline int game_step_world(struct game *game)
{
	struct e3d_event event;
	int ret;

	while (1) {
		ret = e3d_window_poll(game->wnd, &event);
		if (ret == -EAGAIN)
			break;
		if (ret < 0)
			return ret;

		/* handle events here */
	}

	if (e3d_window_get_key(game->wnd, E3D_KEY_LEFT))
		e3d_eye_rotate(&game->world->eye, -2.0,
				(math_v3){ 0.0, 0.0, 1.0 });
	if (e3d_window_get_key(game->wnd, E3D_KEY_RIGHT))
		e3d_eye_rotate(&game->world->eye, 2.0,
				(math_v3){ 0.0, 0.0, 1.0 });
	if (e3d_window_get_key(game->wnd, E3D_KEY_UP))
		e3d_eye_rotate(&game->world->eye, -2.0,
				(math_v3){ 0.0, 1.0, 0.0 });
	if (e3d_window_get_key(game->wnd, E3D_KEY_DOWN))
		e3d_eye_rotate(&game->world->eye, 2.0,
				(math_v3){ 0.0, 1.0, 0.0 });
	if (e3d_window_get_key(game->wnd, E3D_KEY_PAGEUP))
		e3d_eye_rotate(&game->world->eye, -2.0,
				(math_v3){ 1.0, 0.0, 0.0 });
	if (e3d_window_get_key(game->wnd, E3D_KEY_PAGEDOWN))
		e3d_eye_rotate(&game->world->eye, 2.0,
				(math_v3){ 1.0, 0.0, 0.0 });

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
	}

	ulog_flog(game->log, ULOG_INFO, "Exiting mainloop\n");

	return ret;
}

static int setup_obj(struct world_obj **out, const cstr *file)
{
	int ret;
	struct uconf_entry *conf;
	struct e3d_shape *shape;
	struct world_obj *obj;

	ret = config_load(&conf, file);
	if (ret)
		return ret;

	ret = config_load_shape(&shape, conf);
	uconf_entry_unref(conf);
	if (ret)
		return ret;

	ret = world_obj_new(&obj);
	if (ret) {
		e3d_shape_unref(shape);
		return ret;
	}

	e3d_shape_link(obj->shape, shape);
	e3d_shape_unref(shape);

	*out = obj;
	return 0;
}

static int setup_world(struct world **world)
{
	struct world *w;
	struct world_obj *obj;
	int ret;

	ret = world_new(&w);
	if (ret)
		return ret;
	e3d_eye_look_at(&w->eye, (math_v3) { 18.0, 15.0, 15.0 },
						(math_v3) { 0.0, 0.0, 0.0 },
						(math_v3) { 0.0, 0.0, 1.0 });
	e3d_light_look_at(&w->light0, (math_v3) { 0.0, 0.0, 10.0 },
						(math_v3) { 0.0, 0.0, 0.0 },
						(math_v3) { 0.0, 1.0, 0.0 });

	ret = setup_obj(&obj, &CSTR_CS("data/room.conf"));
	if (ret)
		goto err;
	world_add(w, obj);
	world_obj_unref(obj);

	ret = setup_obj(&obj, &CSTR_CS("data/table.conf"));
	if (ret)
		goto err;
	phys_body_set_shape_ground(obj->body);
	world_add(w, obj);
	world_obj_unref(obj);

	ret = setup_obj(&obj, &CSTR_CS("data/test.conf"));
	if (ret)
		goto err;
	phys_body_set_shape_sphere(obj->body);
	world_add(w, obj);
	phys_body_impulse(obj->body, (math_v3){ 0.4, 1.0, 0.0 });
	world_obj_unref(obj);

	*world = w;
	return 0;

err:
	world_free(w);
	return ret;
}

int game_run(struct ulog_dev *log, struct e3d_window *wnd,
							struct shaders *shaders)
{
	int ret;
	struct game game;

	memset(&game, 0, sizeof(game));
	game.log = log;
	game.wnd = wnd;
	game.shaders = shaders;
	game.tick_time = 20000;
	e3d_transform_init(&game.trans);

	ret = setup_world(&game.world);
	if (ret)
		goto err_math;

	ret = game_loop(&game);

	world_free(game.world);

err_math:
	e3d_transform_destroy(&game.trans);

	return ret;
}
