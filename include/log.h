/*
 * airhockey - log and error handling
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#ifndef ULOG_LOG_H
#define ULOG_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

struct ulog_dev;

enum ulog_severity {
	ULOG_INTERNAL,
	ULOG_FATAL,
	ULOG_ERROR,
	ULOG_WARN,
	ULOG_INFO,
	ULOG_DEBUG,
	ULOG_SEVERITY_NUM
};

struct ulog_target {
	struct ulog_target *next;
	int severity;
	void *extra;

	int (*init) (struct ulog_target *t);
	void (*destroy) (struct ulog_target *t);
	void (*log) (struct ulog_target *t, const char *m);
	void (*vlog) (struct ulog_target *t, const char *f, va_list l);
};

extern struct ulog_dev *ulog_new(const char *prefix);
extern void ulog_free(struct ulog_dev *log);
extern int ulog_add_target(struct ulog_dev *log, struct ulog_target *target);
extern void ulog_remove_target(struct ulog_dev *log,
						struct ulog_target *target);

extern void ulog_log(struct ulog_dev *log, int sev, const char *msg);
extern void ulog_flog(struct ulog_dev *log, int sev, const char *format, ...);
extern void ulog_vlog(struct ulog_dev *log, int sev, const char *format,
								va_list list);

extern int ulog_t_file_init(struct ulog_target *t);
extern void ulog_t_file_destroy(struct ulog_target *t);
extern void ulog_t_file_log(struct ulog_target *t, const char *m);
extern void ulog_t_file_vlog(struct ulog_target *t, const char *f, va_list l);

extern struct ulog_target ulog_t_stderr;

#endif /* ULOG_LOG_H */
