/*
 * airhockey - physics calculation
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#ifndef PHYSICS_H
#define PHYSICS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#include "log.h"
#include "mathw.h"

struct phys_body;
struct phys_world;

extern struct phys_world *phys_world_new(struct ulog_dev *log);
extern void phys_world_free(struct phys_world *world);
extern int phys_world_step(struct phys_world *world, int64_t step);
extern void phys_world_add(struct phys_world *world, struct phys_body *body);
extern void phys_world_remove(struct phys_world *world, struct phys_body *body);

extern struct phys_body *phys_body_new();
extern struct phys_body *phys_body_ref(struct phys_body *body);
extern void phys_body_unref(struct phys_body *body);
extern void phys_body_get_transform(struct phys_body *body, math_v3 origin);

extern struct phys_body *phys_body_new_ground();
extern struct phys_body *phys_body_new_sphere();

#ifdef __cplusplus
}
#endif
#endif /* PHYSICS_H */
