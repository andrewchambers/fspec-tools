#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdnoreturn.h>
#include "common.h"

void
parse(FILE *file, void (*fspec)(char *, size_t))
{
	char bufstack[8192], *bufalloc = NULL;
	char *buf = bufstack, *pos, *end, *rec;
	size_t len, max = sizeof(bufstack);

	rec = buf;
	pos = buf;
	do {
		if (pos - buf > max / 2) {
			len = pos - buf;
			max *= 2;
			buf = bufalloc = realloc(bufalloc, max);
			if (!buf)
				fatal(NULL);
			rec = buf;
			pos = buf + len;
		}
		len = fread(pos, 1, max - (pos - buf), file);
		if (len != max - (pos - buf) && ferror(file))
			fatal("read:");
		if (rec == pos) {
		next:
			while (len > 0 && *pos == '\n')
				++pos, --len;
			if (len == 0) {
				rec = pos = buf;
				continue;
			}
			rec = pos;
			if (*rec != '/')
				fatal("invalid fspec: paths must begin with '/'");
		}
		for (;;) {
			end = memchr(pos, '\n', len);
			if (!end) {
				if (rec > buf) {
					memmove(buf, rec, pos - rec + len);
					pos = buf + (pos - rec) + len;
					rec = buf;
				}
				break;
			}
			len -= end + 1 - pos;
			pos = end + 1;
			if (end > rec && end[-1] == '\n') {
				fspec(rec, end - rec);
				goto next;
			}
		}
	} while (!feof(file));
	if (pos - rec + len > 0) {
		if (pos[len - 1] != '\n')
			fatal("invalid fspec: truncated");
		fspec(rec, pos - rec + len);
	}
	free(bufalloc);
}