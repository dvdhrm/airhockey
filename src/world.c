/*
 * airhockey - world
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <libcstr.h>

#include "engine3d.h"
#include "mathw.h"
#include "physics.h"
#include "world.h"

int world_obj_new(struct world_obj **obj)
{
	int ret;
	struct world_obj *o;

	o = malloc(sizeof(*o));
	if (!o)
		return -ENOMEM;

	memset(o, 0, sizeof(*o));
	math_m4_identity(o->alter);

	o->shape = e3d_shape_new();
	if (!o->shape) {
		ret = -ENOMEM;
		goto err;
	}

	o->body = phys_body_new();
	if (!o->body) {
		ret = -ENOMEM;
		goto err_shape;
	}

	*obj = world_obj_ref(o);
	return 0;

err_shape:
	e3d_shape_unref(o->shape);
err:
	free(o);
	return ret;
}

struct world_obj *world_obj_ref(struct world_obj *obj)
{
	assert(obj);
	++obj->ref;
	assert(obj->ref);
	return obj;
}

void world_obj_unref(struct world_obj *obj)
{
	struct world_obj *iter, *tmp;

	if (!obj)
		return;

	assert(obj->ref);

	if (--obj->ref)
		return;

	/* if refcount drops 0 we cannot be linked! */
	assert(!obj->parent);
	assert(!obj->world);
	assert(!obj->next);
	assert(!obj->prev);

	phys_body_unlink(obj->body);
	phys_body_unref(obj->body);
	e3d_shape_unref(obj->shape);

	/* free our reference of all our childs */
	for (iter = obj->first; iter; ) {
		tmp = iter;
		iter = iter->next;
		world_obj_unlink(tmp);
	}

	free(obj);
}

static void unlink_bodies(struct world_obj *obj)
{
	struct world_obj *iter;

	if (!obj->world)
		return;

	obj->world = NULL;
	phys_body_unlink(obj->body);

	for (iter = obj->first; iter; iter = iter->next)
		unlink_bodies(iter);
}

void world_obj_unlink(struct world_obj *obj)
{
	if (!obj->parent)
		return;

	if (obj->prev)
		obj->prev->next = obj->next;
	else
		obj->parent->first = obj->next;
	if (obj->next)
		obj->next->prev = obj->prev;
	else
		obj->parent->last = obj->prev;

	obj->next = NULL;
	obj->prev = NULL;
	obj->parent = NULL;
	unlink_bodies(obj);
	world_obj_unref(obj);
}

static void link_bodies(struct world_obj *obj)
{
	struct world_obj *iter;

	assert(obj->world);
	phys_world_add(obj->world->phys, obj->body);

	for (iter = obj->first; iter; iter = iter->next) {
		assert(!iter->world);
		iter->world = obj->world;
		link_bodies(iter);
	}
}

void world_obj_link(struct world_obj *prev, struct world_obj *obj)
{
	assert(prev->parent);
	assert(!obj->parent);
	assert(!obj->world);
	assert(!obj->next);
	assert(!obj->prev);

	world_obj_ref(obj);
	obj->parent = prev->parent;

	obj->prev = prev;
	obj->next = prev->next;
	prev->next = obj;
	if (prev->next)
		prev->next->prev = obj;
	else
		prev->parent->last = obj;

	obj->world = prev->world;
	if (obj->world)
		link_bodies(obj);
}

void world_obj_link_first(struct world_obj *parent, struct world_obj *obj)
{
	assert(parent);
	assert(!obj->parent);
	assert(!obj->world);
	assert(!obj->next);
	assert(!obj->prev);

	world_obj_ref(obj);
	obj->parent = parent;

	obj->prev = NULL;
	obj->next = parent->first;
	if (parent->first)
		parent->first->prev = obj;
	else
		parent->last = obj;
	parent->first = obj;

	obj->world = parent->world;
	if (obj->world)
		link_bodies(obj);
}

int world_new(struct world **world)
{
	int ret;
	struct world *w;

	w = malloc(sizeof(*w));
	if (!w)
		return -ENOMEM;

	memset(w, 0, sizeof(*w));

	w->phys = phys_world_new(NULL);
	if (!w->phys) {
		ret = -ENOMEM;
		goto err;
	}

	ret = world_obj_new(&w->root);
	if (ret)
		goto err_phys;
	w->root->world = w;

	e3d_eye_init(&w->eye);
	e3d_light_init(&w->light0);

	*world = w;
	return 0;

err_phys:
	phys_world_free(w->phys);
err:
	free(w);
	return ret;
}

void world_free(struct world *world)
{
	e3d_light_destroy(&world->light0);
	e3d_eye_destroy(&world->eye);
	world->root->world = NULL;
	world_obj_unref(world->root);
	phys_world_free(world->phys);
	free(world);
}
