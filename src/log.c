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

struct ulog_dev {
	struct ulog_target *targets;
	char *prefix;
};

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
	return log;
}

void ulog_free(struct ulog_dev *log)
{
	while (log->targets)
		ulog_remove_target(log, log->targets);

	free(log->prefix);
	free(log);
}

int ulog_add_target(struct ulog_dev *log, struct ulog_target *target)
{
	struct ulog_target *t;
	int ret;

	t = malloc(sizeof(*t));
	if (!t)
		return -ENOMEM;

	t->next = NULL;
	t->severity = target->severity;
	t->extra = target->extra;
	t->init = target->init;
	t->destroy = target->destroy;
	t->log = target->log;
	t->vlog = target->vlog;

	ret = t->init(t);
	if (ret) {
		free(t);
		return ret;
	}

	t->next = log->targets;
	log->targets = t;

	return 0;
}

void ulog_remove_target(struct ulog_dev *log, struct ulog_target *target)
{
	struct ulog_target *iter;

	assert(log->targets);

	if (log->targets == target) {
		log->targets = target->next;
	} else {
		iter = log->targets;
		while (iter->next) {
			if (iter->next == target) {
				iter->next = target->next;
				break;
			}
		}
	}

	target->next = NULL;
	target->destroy(target);
	free(target);
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
	struct ulog_target *iter;

	iter = log->targets;
	while (iter) {
		if (sev <= iter->severity) {
			sev_print(iter, sev);
			iter->log(iter, log->prefix);
			iter->log(iter, msg);
		}
	}

	sev_react(log, sev);
}

void ulog_flog(struct ulog_dev *log, int sev, const char *format, ...)
{
	va_list list;

	va_start(list, format);
	ulog_vlog(log, sev, format, list);
	va_end(list);
}

void ulog_vlog(struct ulog_dev *log, int sev, const char *format, va_list list)
{
	struct ulog_target *iter;

	iter = log->targets;
	while (iter) {
		if (sev <= iter->severity) {
			sev_print(iter, sev);
			iter->log(iter, log->prefix);
			iter->vlog(iter, format, list);
		}
	}

	sev_react(log, sev);
}

int ulog_t_file_init(struct ulog_target *target)
{
	return 0;
}

void ulog_t_file_destroy(struct ulog_target *target)
{
	return;
}

void ulog_t_file_log(struct ulog_target *target, const char *msg)
{
	fprintf(target->extra, "%s", msg);
}

void ulog_t_file_vlog(struct ulog_target *target, const char *f, va_list list)
{
	vfprintf(target->extra, f, list);
}
