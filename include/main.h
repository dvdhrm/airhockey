/*
 * airhockey - main header
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdlib.h>

extern int64_t misc_now();
extern int misc_load_file(const char *file, char **buf, size_t *size);

#endif /* MAIN_H */
