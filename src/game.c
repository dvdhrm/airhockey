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
	struct e3d_shader *shader;
	const struct e3d_shader_locations *loc;
	int64_t tick_time;

	struct phys_world *phys;
	struct math_stack mstack;
	struct e3d_eye eye;
	struct e3d_shape *room;
};

static inline int game_render(struct game *game)
{
	math_m4 projection;
	math_v3 pos = { 8.0, 10.0, 10.0 };
	math_v3 at = { 0.0, 0.0, 0.0 };
	math_v3 ori = { 0.0, 0.0, 1.0 };

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	e3d_window_projection(game->wnd, projection);
	E3D(glUniformMatrix4fv(game->loc->uni.proj_mat, 1, 0,
							(GLfloat*)projection));

	e3d_eye_look_at(&game->eye, pos, at, ori);

	assert(math_stack_is_root(&game->mstack));
	math_m4_identity(MATH_TIP(&game->mstack));
	e3d_eye_apply(&game->eye, MATH_TIP(&game->mstack));

	e3d_shape_draw(game->room, game->loc, &game->mstack);

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

int game_run(struct ulog_dev *log, struct e3d_window *wnd,
						struct e3d_shader *shader)
{
	int ret;
	struct e3d_shape *room;
	struct uconf_entry *conf;
	struct game game;

	ret = config_load(&conf, cstr_strdup(-1, "data/room.conf"));
	if (ret)
		ulog_flog(log, ULOG_FATAL, "Cannot load room config\n");

	ret = config_load_shape(&room, conf);
	if (ret)
		ulog_flog(log, ULOG_FATAL, "Cannot load room shape\n");

	uconf_entry_unref(conf);

	memset(&game, 0, sizeof(game));
	game.log = log;
	game.wnd = wnd;
	game.shader = shader;
	game.loc = e3d_shader_locations(shader);
	game.tick_time = 20000;
	e3d_eye_init(&game.eye);
	math_stack_init(&game.mstack);
	game.room = room;

	game.phys = phys_world_new(log);
	if (!game.phys) {
		ret = -ENOMEM;
		goto err_math;
	}

	e3d_shader_use(shader);

	ret = game_loop(&game);

	phys_world_free(game.phys);

err_math:
	math_stack_destroy(&game.mstack);
	e3d_eye_destroy(&game.eye);

	e3d_shape_unref(room);

	return ret;
}
