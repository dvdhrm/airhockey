/*
 * airhockey - physics simulation
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <assert.h>
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

struct phys_body {
	size_t ref;
	struct phys_world *world;
	struct phys_body *next;
	struct phys_body *prev;

	btCollisionShape *shape;
	btDefaultMotionState *motion;
	btRigidBody *body;
};

struct phys_world {
	struct ulog_dev *log;
	struct phys_body *childs;

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

	if (log)
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
	struct phys_body *iter, *tmp;

	for (iter = world->childs; iter; ) {
		tmp = iter;
		iter = iter->next;
		phys_world_remove(world, tmp);
	}

	delete world->world;
	delete world->solver;
	delete world->coll_disp;
	delete world->coll_conf;
	delete world->broadphase;
	free(world);
}

int phys_world_step(struct phys_world *world, int64_t step)
{
	world->world->stepSimulation(step / 1000000.0, 10);
	return 0;
}

static inline void world_add(struct phys_world *world, struct phys_body *body)
{
	assert(body->world == world);
	assert(body->body);

	world->world->addRigidBody(body->body);
}

void phys_world_add(struct phys_world *world, struct phys_body *body)
{
	assert(!body->world);
	assert(!body->next);
	assert(!body->prev);

	phys_body_ref(body);
	body->world = world;

	body->next = world->childs;
	if (body->next)
		body->next->prev = body;
	world->childs = body;

	if (body->body)
		world_add(world, body);
}

static inline void world_remove(struct phys_world *world,
							struct phys_body *body)
{
	assert(body->world == world);
	assert(body->body);

	world->world->removeRigidBody(body->body);
}

void phys_world_remove(struct phys_world *world, struct phys_body *body)
{
	assert(body->world == world);
	assert(world->childs);

	if (body->body)
		world_remove(world, body);

	if (body->prev)
		body->prev->next = body->next;
	else
		world->childs = body->next;
	if (body->next)
		body->next->prev = body->prev;
	body->next = NULL;
	body->prev = NULL;

	body->world = NULL;
	phys_body_unref(body);
}

struct phys_body *phys_body_new()
{
	struct phys_body *body;

	body = (struct phys_body*)malloc(sizeof(*body));
	if (!body)
		return NULL;

	memset(body, 0, sizeof(*body));

	return phys_body_ref(body);
}

struct phys_body *phys_body_ref(struct phys_body *body)
{
	++body->ref;
	assert(body->ref);
	return body;
}

void phys_body_unref(struct phys_body *body)
{
	if (!body)
		return;

	assert(body->ref);

	if (--body->ref)
		return;

	/* if refcount drops zero we cannot be linked to any world! */
	assert(!body->world);
	assert(!body->next);
	assert(!body->prev);

	phys_body_set_shape_none(body);
	free(body);
}

void phys_body_get_transform(struct phys_body *body, math_v3 origin)
{
	btTransform trans;
	btVector3 o;

	assert(body->body);

	body->body->getMotionState()->getWorldTransform(trans);
	o = trans.getOrigin();
	origin[0] = o.getX();
	origin[1] = o.getY();
	origin[2] = o.getZ();
}

void phys_body_unlink(struct phys_body *body)
{
	if (body->world)
		phys_world_remove(body->world, body);
}

bool phys_body_has_shape(struct phys_body *body)
{
	return !!body->body;
}

void phys_body_set_shape_none(struct phys_body *body)
{
	if (!body->body)
		return;

	if (body->world)
		world_remove(body->world, body);

	if (body->body)
		delete body->body;
	if (body->motion)
		delete body->motion;
	if (body->shape)
		delete body->shape;

	body->body = NULL;
	body->motion = NULL;
	body->shape = NULL;
}

void phys_body_set_shape_ground(struct phys_body *body)
{
	phys_body_set_shape_none(body);

	body->shape = new btStaticPlaneShape(btVector3(0, 0, 1), 0);
	body->motion = new btDefaultMotionState(btTransform(
				btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));

	btRigidBody::btRigidBodyConstructionInfo info(0, body->motion,
						body->shape, btVector3(0,0,0));
	body->body = new btRigidBody(info);

	if (body->world)
		world_add(body->world, body);
}

void phys_body_set_shape_sphere(struct phys_body *body)
{
	phys_body_set_shape_none(body);

	body->shape = new btSphereShape(0.5);
	body->motion = new btDefaultMotionState(btTransform(
				btQuaternion(0, 0, 0, 1), btVector3(0, 0, 50)));

	btScalar mass = 1;
	btVector3 inertia(0, 0, 0);
	body->shape->calculateLocalInertia(mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(mass, body->motion,
							body->shape, inertia);
	body->body = new btRigidBody(info);

	if (body->world)
		world_add(body->world, body);
}
