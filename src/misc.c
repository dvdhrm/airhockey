/*
 * airhockey - main source
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <unistd.h>

#include "main.h"

/*
 * Return current time in microseconds since UNIX EPOCH. This is a monotonic
 * clock. The clock is not reset if the system clock changes so use it for
 * measuring time lapses but not to display the current time.
 */
int64_t misc_now()
{
	int64_t ret;
	struct timespec val;

	clock_gettime(CLOCK_MONOTONIC, &val);

	ret = val.tv_sec * 1000000;
	ret += val.tv_nsec / 1000;

	return ret;
}

/*
 * Loads a file into a string, returns 0 on success. The pointer to the buffer
 * is stored in \out and the size in \size.
 * On success, the resulting string must be freed with free().
 */
int misc_load_file(const char *file, char **out, size_t *size)
{
	FILE *ffile;
	ssize_t len;
	char *buf;

	ffile = fopen(file, "rb");
	if (!ffile)
		return -errno;

	if (fseek(ffile, 0, SEEK_END) != 0) {
		fclose(ffile);
		return -errno;
	}

	len = ftell(ffile);
	if (len < 0) {
		fclose(ffile);
		return -errno;
	}

	if (len < 1) {
		fclose(ffile);
		return -EINVAL;
	}

	rewind(ffile);

	buf = malloc(len + 1);
	if (!buf) {
		fclose(ffile);
		return -ENOMEM;
	}

	if (len != fread(buf, 1, len, ffile)) {
		free(buf);
		fclose(ffile);
		return -EINVAL;
	}

	buf[len] = 0;
	*out = buf;
	*size = len;
	fclose(ffile);

	return 0;
}
