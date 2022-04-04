#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

void
fatal(const char *fmt, ...)
{
	va_list ap;
	int err = 1;

	if (fmt) {
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		err = fmt[0] && fmt[strlen(fmt) - 1] == ':';
		if (err)
			fputc(' ', stderr);
	}
	if (err)
		perror(NULL);
	else
		fputc('\n', stderr);
	exit(1);
}

void *
reallocarray(void *p, size_t n, size_t m)
{
	if (n > 0 && SIZE_MAX / n < m) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(p, n * m);
}
