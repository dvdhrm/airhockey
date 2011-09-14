/*
 * airhockey - main source
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

struct ulog_target_link {
	struct ulog_target_link *next;
	struct ulog_target *target;
};

struct ulog_dev {
	size_t ref;
	struct ulog_target_link *targets;
	char *prefix;
};

static struct ulog_target_link default_log_targets = {
	.next = NULL,
	.target = &ulog_t_stderr,
};

static struct ulog_dev default_log = {
	.ref = 1,
	.targets = &default_log_targets,
	.prefix = "",
};

#define DEF_LOG(log) log = (log ? log : &default_log)

struct ulog_dev *ulog_new(const char *prefix)
{
	struct ulog_dev *log;

	log = malloc(sizeof(*log));
	if (!log)
		return NULL;

	log->prefix = strdup(prefix);
	if (!log->prefix) {
		free(log);
		return NULL;
	}

	log->targets = NULL;
	return ulog_ref(log);
}

struct ulog_dev *ulog_ref(struct ulog_dev *log)
{
	log->ref++;
	assert(log->ref);
	return log;
}

void ulog_unref(struct ulog_dev *log)
{
	if (!log)
		return;

	assert(log->ref);

	if (--log->ref)
		return;

	while (log->targets)
		ulog_remove_target(log, log->targets->target);

	free(log->prefix);
	free(log);
}

int ulog_add_target(struct ulog_dev *log, struct ulog_target *target)
{
	struct ulog_target_link *link;

	link = malloc(sizeof(*link));
	if (!link)
		return -ENOMEM;

	link->target = target->ref(target);
	link->next = log->targets;
	log->targets = link;

	return 0;
}

void ulog_remove_target(struct ulog_dev *log, struct ulog_target *target)
{
	struct ulog_target_link *iter, *tmp;

	assert(log->targets);

	if (log->targets->target == target) {
		tmp = log->targets;
		log->targets = log->targets->next;
	} else {
		iter = log->targets;
		while (iter->next) {
			if (iter->next->target == target)
				break;
		}
		assert(iter->next);
		tmp = iter->next;
		iter->next = iter->next->next;
	}

	tmp->target->unref(target);
	free(tmp);
}

static void sev_react(struct ulog_dev *log, int sev)
{
	if (sev == ULOG_INTERNAL)
		return;

	if (sev <= ULOG_FATAL) {
		ulog_log(log, ULOG_INTERNAL, "Fatal Error\n");
		abort();
	}
}

static void sev_print(struct ulog_target *target, int sev)
{
	const char *str;

	switch (sev) {
		case ULOG_FATAL:
			str = "(FATAL) ";
			break;
		case ULOG_ERROR:
			str = "(ERROR) ";
			break;
		case ULOG_WARN:
			str = "(WARNING) ";
			break;
		case ULOG_INFO:
			str = "(INFO) ";
			break;
		case ULOG_DEBUG:
			str = "(DEBUG) ";
			break;
		default:
			str = "";
	}

	target->log(target, str);
}

void ulog_log(struct ulog_dev *log, int sev, const char *msg)
{
	struct ulog_target_link *iter;

	DEF_LOG(log);

	iter = log->targets;
	while (iter) {
		if (sev <= iter->target->severity) {
			sev_print(iter->target, sev);
			iter->target->log(iter->target, log->prefix);
			iter->target->log(iter->target, msg);
		}
		iter = iter->next;
	}

	sev_react(log, sev);
}

void ulog_flog(struct ulog_dev *log, int sev, const char *format, ...)
{
	va_list list;

	DEF_LOG(log);

	va_start(list, format);
	ulog_vlog(log, sev, format, list);
	va_end(list);
}

void ulog_vlog(struct ulog_dev *log, int sev, const char *format, va_list list)
{
	struct ulog_target_link *iter;

	DEF_LOG(log);

	iter = log->targets;
	while (iter) {
		if (sev <= iter->target->severity) {
			sev_print(iter->target, sev);
			iter->target->log(iter->target, log->prefix);
			iter->target->vlog(iter->target, format, list);
		}
		iter = iter->next;
	}

	sev_react(log, sev);
}

struct ulog_target *t_file_ref(struct ulog_target *target)
{
	return target;
}

void t_file_unref(struct ulog_target *target)
{
	return;
}

void t_file_log(struct ulog_target *target, const char *msg)
{
	fprintf(target->extra ? target->extra : stderr, "%s", msg);
}

void t_file_vlog(struct ulog_target *target, const char *f, va_list list)
{
	vfprintf(target->extra ? target->extra : stderr, f, list);
}

struct ulog_target ulog_t_stderr = {
	.severity = ULOG_INFO,
	.extra = NULL,
	.ref = t_file_ref,
	.unref = t_file_unref,
	.log = t_file_log,
	.vlog = t_file_vlog,
};
