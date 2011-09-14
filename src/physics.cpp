/*
 * airhockey - physics simulation
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <btBulletDynamicsCommon.h>

extern "C" {
	#include "log.h"
	#include "main.h"
	#include "physics.h"
}

struct phys_world {
	struct ulog_dev *log;

	btBroadphaseInterface *broadphase;
	btDefaultCollisionConfiguration *coll_conf;
	btCollisionDispatcher *coll_disp;
	btSequentialImpulseConstraintSolver *solver;
	btDiscreteDynamicsWorld *world;
};

struct phys_world *phys_world_new(struct ulog_dev *log)
{
	struct phys_world *world;

	world = (struct phys_world*)malloc(sizeof(*world));
	if (!world)
		return NULL;

	memset(world, 0, sizeof(*world));

	world->log = ulog_ref(log);
	world->broadphase = new btDbvtBroadphase();
	world->coll_conf = new btDefaultCollisionConfiguration();
	world->coll_disp = new btCollisionDispatcher(world->coll_conf);
	world->solver = new btSequentialImpulseConstraintSolver();
	world->world = new btDiscreteDynamicsWorld(world->coll_disp,
			world->broadphase, world->solver, world->coll_conf);
	world->world->setGravity(btVector3(0, 0, -10));

	return world;
}

void phys_world_free(struct phys_world *world)
{
	delete world->world;
	delete world->solver;
	delete world->coll_disp;
	delete world->coll_conf;
	delete world->broadphase;
	free(world);
}

int phys_world_step(struct phys_world *world, int64_t step)
{
	world->world->stepSimulation(step / 1000000.0, 10, 1 / 60.0);
	return 0;
}
