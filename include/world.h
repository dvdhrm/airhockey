/*
 * airhockey - world creation
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#ifndef WORLD_H
#define WORLD_H

#include <stdlib.h>
#include "engine3d.h"
#include "mathw.h"
#include "physics.h"

struct world_obj {
	size_t ref;
	struct world *world;
	struct world_obj *parent;
	struct world_obj *first;
	struct world_obj *last;
	struct world_obj *next;
	struct world_obj *prev;

	math_m4 alter;
	struct phys_body *body;
	struct e3d_shape *shape;
};

struct world {
	struct phys_world *phys;
	struct world_obj *root;
};

extern int world_obj_new(struct world_obj **obj);
extern struct world_obj *world_obj_ref(struct world_obj *obj);
extern void world_obj_unref(struct world_obj *obj);

extern void world_obj_unlink(struct world_obj *obj);
extern void world_obj_link(struct world_obj *prev, struct world_obj *obj);
extern void world_obj_link_first(struct world_obj *parent,
							struct world_obj *obj);

extern int world_new(struct world **world);
extern void world_free(struct world *world);

static inline void world_add(struct world *world, struct world_obj *obj)
{
	world_obj_link_first(world->root, obj);
}

#endif /* WORLD_H */
