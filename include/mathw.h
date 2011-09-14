/*
 * airhockey - math wrapper
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#ifndef MATHW_H
#define MATHW_H
#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdlib.h>

#include "log.h"

typedef float math_v3[3];
typedef float math_v4[4];
typedef float math_m4[4][4];

extern void math_init(struct ulog_dev *log);
extern void math_destroy();

extern void math_v3_copy(math_v3 dest, math_v3 src);
extern void math_v3_normalize(math_v3 v);
extern void math_v3_product_dest(math_v3 dest, math_v3 a, math_v3 b);
extern void math_v3_sub_dest(math_v3 dest, math_v3 src, math_v3 amount);

extern void math_m4_copy(math_m4 dest, math_m4 src);
extern void math_m4_identity(math_m4 m);

extern void math_m4_translate(math_m4 m, float x, float y, float z);
extern void math_m4_translatev(math_m4 m, math_v3 v);

extern void math_m4_mult_dest(math_m4 dest, math_m4 a, math_m4 b);
extern void math_m4_mult(math_m4 dest, math_m4 src);

extern void math_m4_invert_dest(math_m4 dest, math_m4 src);
extern void math_m4_invert(math_m4 m);

/*
 * Matrix stack
 * The stack is limited to 4 dimensional matrices. It allows to push matrices at
 * the stack, modify them and pop them again.
 */

struct math_entry {
	struct math_entry *next;
	math_m4 matrix;
};

struct math_stack {
	struct math_entry stack;
	struct math_entry *cache;
};

#define MATH_TIP(mstack) ((mstack)->stack.matrix)

extern void math_stack_init(struct math_stack *stack);
extern void math_stack_destroy(struct math_stack *stack);

static inline bool math_stack_is_root(struct math_stack *stack)
{
	return !stack->stack.next;
}

extern int math_stack_push(struct math_stack *stack);
extern void math_stack_pop(struct math_stack *stack);

#ifdef __cplusplus
}
#endif
#endif /* MATHW_H */
