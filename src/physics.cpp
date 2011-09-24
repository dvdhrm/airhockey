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

void phys_body_get_transform(struct phys_body *body, math_m4 transform)
{
	btTransform trans;

	if (!body->body) {
		math_m4_identity(transform);
		return;
	}

	body->body->getMotionState()->getWorldTransform(trans);
	trans.getOpenGLMatrix((float*)transform);
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
	info.m_friction = 2;
	body->body = new btRigidBody(info);

	if (body->world)
		world_add(body->world, body);
}

void phys_body_set_shape_sphere(struct phys_body *body)
{
	phys_body_set_shape_none(body);

	body->shape = new btSphereShape(0.5);
	body->motion = new btDefaultMotionState(btTransform(
				btQuaternion(0, 0, 0, 1), btVector3(0, 0, 10)));

	btScalar mass = 1;
	btVector3 inertia(0, 0, 0);
	body->shape->calculateLocalInertia(mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(mass, body->motion,
							body->shape, inertia);
	body->body = new btRigidBody(info);

	if (body->world)
		world_add(body->world, body);
}

void phys_body_set_shape_cylinder(struct phys_body *body)
{
	phys_body_set_shape_none(body);

	body->shape = new btCylinderShapeZ(btVector3(1, 1, 0.25));
	body->motion = new btDefaultMotionState(btTransform(
				btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));

	btScalar mass = 100;
	btVector3 inertia(0, 0, 0);
	body->shape->calculateLocalInertia(mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(mass, body->motion,
							body->shape, inertia);
	info.m_friction = 2;
	body->body = new btRigidBody(info);

	if (body->world)
		world_add(body->world, body);
}

void phys_body_set_shape_puk(struct phys_body *body)
{
	phys_body_set_shape_none(body);

	body->shape = new btCylinderShapeZ(btVector3(1, 1, 0.25));
	body->motion = new btDefaultMotionState(btTransform(
				btQuaternion(0, 0, 0, 1), btVector3(3, 3, 1)));

	btScalar mass = 100;
	btVector3 inertia(0, 0, 0);
	body->shape->calculateLocalInertia(mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo info(mass, body->motion,
							body->shape, inertia);
	info.m_friction = 2;
	body->body = new btRigidBody(info);

	if (body->world)
		world_add(body->world, body);
}

void phys_body_set_shape_table(struct phys_body *body)
{
	btCollisionShape *shape;
	btCompoundShape *com;

	phys_body_set_shape_none(body);

	/* TODO: we need to free all childs on body_free() */
	com = new btCompoundShape();

	/* ground */
	shape = new btBoxShape(btVector3(5.5, 10.5, 0.5));
	com->addChildShape(
		btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, -0.5)),
									shape);

	/* sidewall long side 1 */
	shape = new btBoxShape(btVector3(0.25, 10.5, 1));
	com->addChildShape(
		btTransform(btQuaternion(0, 0, 0, 1), btVector3(5.25, 0, 0)),
									shape);

	/* sidewall long side 2 */
	shape = new btBoxShape(btVector3(0.25, 10.5, 1));
	com->addChildShape(
		btTransform(btQuaternion(0, 0, 0, 1), btVector3(-5.25, 0, 0)),
									shape);

	/* sidewall goal 1 */
	shape = new btBoxShape(btVector3(5.0, 0.25, 1));
	com->addChildShape(
		btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -10.5, 0)),
									shape);

	/* sidewall goal 2 */
	shape = new btBoxShape(btVector3(5.0, 0.25, 1));
	com->addChildShape(
		btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 10.5, 0)),
									shape);

	body->shape = com;
	body->motion = new btDefaultMotionState(btTransform(
			btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));

	btRigidBody::btRigidBodyConstructionInfo info(0, body->motion,
						body->shape, btVector3(0,0,0));
	info.m_friction = 1;
	body->body = new btRigidBody(info);

	if (body->world)
		world_add(body->world, body);
}

void phys_body_impulse(struct phys_body *body, math_v3 force)
{
	if (!body->body)
		return;

	body->body->applyCentralImpulse(
				btVector3(force[0], force[1], force[2]));
}

void phys_body_force(struct phys_body *body, math_v3 force)
{
	btVector3 gravity;

	if (!body->body)
		return;

	gravity = body->body->getGravity();
	gravity += btVector3(force[0], force[1], force[2]);
	body->body->setGravity(gravity);
}
