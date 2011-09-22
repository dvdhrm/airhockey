/*
 * airhockey - math wrapper
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <stdlib.h>

#include <sg.h>

extern "C" {
	#include "log.h"
	#include "mathw.h"
}

static struct ulog_dev *math_log;

static void error_cb(enum ulSeverity sev, char *msg)
{
	int log_sev;

	switch (sev) {
		case UL_DEBUG:
			log_sev = ULOG_DEBUG;
			break;
		case UL_WARNING:
			log_sev = ULOG_WARN;
			break;
		case UL_FATAL:
			log_sev = ULOG_FATAL;
			break;
		default:
			log_sev = ULOG_WARN;
			break;
	}

	ulog_flog(math_log, log_sev, "plib error: %s\n", msg);
}

void math_init(struct ulog_dev *log)
{
	math_log = ulog_ref(log);
	ulSetErrorCallback(error_cb);
}

void math_destroy()
{
	ulSetErrorCallback(NULL);
	ulog_unref(math_log);
	math_log = NULL;
}

void math_v3_copy(math_v3 dest, math_v3 src)
{
	sgCopyVec3(dest, src);
}

void math_v3_normalize(math_v3 v)
{
	sgNormalizeVec3(v);
}

void math_v3_product_dest(math_v3 dest, math_v3 a, math_v3 b)
{
	sgVectorProductVec3(dest, a, b);
}

void math_v3_sub_dest(math_v3 dest, math_v3 src, math_v3 amount)
{
	sgSubVec3(dest, src, amount);
}

void math_v4_copy(math_v4 dest, math_v4 src)
{
	sgCopyVec4(dest, src);
}

void math_v4_add(math_v4 dest, math_v4 src)
{
	sgAddVec4(dest, src);
}

void math_q4_copy(math_q4 dest, math_q4 src)
{
	sgCopyQuat(dest, src);
}

void math_q4_identity(math_q4 q)
{
	sgMakeIdentQuat(q);
}

void math_q4_normalize(math_q4 q)
{
	sgNormalizeQuat(q);
}

void math_q4_to_m4(math_q4 src, math_m4 dest)
{
	sgQuatToMatrix(dest, src);
}

void math_q4_rotate(math_q4 q, float angle, math_v3 axis)
{
	sgAngleAxisToQuat(q, angle, axis);
}

void math_m4_copy(math_m4 dest, math_m4 src)
{
	sgCopyMat4(dest, src);
}

void math_m4_identity(math_m4 m)
{
	sgMakeIdentMat4(m);
}

void math_m4_translate(math_m4 m, float x, float y, float z)
{
	math_m4 t;
	sgMakeTransMat4(t, x, y, z);
	math_m4_mult(m, t);
}

void math_m4_translatev(math_m4 m, math_v3 v)
{
	math_m4 t;
	sgMakeTransMat4(t, v);
	math_m4_mult(m, t);
}

void math_m4_mult_dest(math_m4 dest, math_m4 a, math_m4 b)
{
	sgMultMat4(dest, a, b);
}

void math_m4_mult(math_m4 dest, math_m4 src)
{
	sgPreMultMat4(dest, src);
}

void math_m4_invert_dest(math_m4 dest, math_m4 src)
{
	sgInvertMat4(dest, src);
}

void math_m4_invert(math_m4 m)
{
	sgInvertMat4(m);
}

void math_stack_init(struct math_stack *stack)
{
	stack->stack.next = NULL;
	math_m4_identity(stack->stack.matrix);
	stack->cache = NULL;
}

void math_stack_destroy(struct math_stack *stack)
{
	struct math_entry *tmp;

	while (stack->stack.next) {
		tmp = stack->stack.next;
		stack->stack.next = tmp->next;
		free(tmp);
	}

	while (stack->cache) {
		tmp = stack->cache;
		stack->cache = tmp->next;
		free(tmp);
	}
}

int math_stack_push(struct math_stack *stack)
{
	struct math_entry *entry;

	if (stack->cache) {
		entry = stack->cache;
		stack->cache = entry->next;
	} else {
		entry = (math_entry*)malloc(sizeof(*entry));
		if (!entry)
			return -ENOMEM;
	}

	math_m4_copy(entry->matrix, stack->stack.matrix);
	entry->next = stack->stack.next;
	stack->stack.next = entry;

	return 0;
}

void math_stack_pop(struct math_stack *stack)
{
	struct math_entry *entry;

	assert(stack->stack.next);

	entry = stack->stack.next;
	stack->stack.next = entry->next;

	entry->next = stack->cache;
	stack->cache = entry;

	math_m4_copy(stack->stack.matrix, entry->matrix);
}
