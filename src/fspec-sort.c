#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <archive.h>
#include "common.h"

static char *argv0;
static char **fs;
static size_t fslen;

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-p] [fspec...]\n", argv0);
	exit(1);
}

static void
fspec(char *buf, size_t len)
{
	if ((fslen & (fslen - 1)) == 0) {
		fs = reallocarray(fs, fslen ? fslen * 2 : 1, sizeof(fs[0]));
		if (!fs) {
			perror(NULL);
			exit(1);
		}
	}
	fs[fslen] = malloc(len + 1);
	if (!fs[fslen]) {
		perror(NULL);
		exit(1);
	}
	memcpy(fs[fslen], buf, len);
	fs[fslen][len] = '\0';
	++fslen;
}

static int
cmp(const void *p1, const void *p2)
{
	const char *r1 = *(const char **)p1, *r2 = *(const char **)p2;

	for (; *r1 == *r2 && *r1 != '\n' && *r2 != '\n'; ++r1, ++r2)
		;
	if (*r1 == *r2)
		return 0;
	if (*r1 == '\n')
		return -1;
	if (*r2 == '\n')
		return 1;
	if (*r1 == '/')
		return -1;
	if (*r2 == '/')
		return 1;
	return *r1 - *r2;
}

static void
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

int
main(int argc, char *argv[])
{
	int opt = 0;
	int pflag = 0;
	FILE *file;

	argv0 = argc ? argv[0] : "fspec-sort";
	while ((opt = getopt(argc, argv, "p")) != -1) {
		switch (opt) {
		case 'p':
			pflag = 1;
			break;
		default:
			usage();
		}
	}
	
	if (optind < argc) {
		for (; optind < argc; ++optind) {
			file = fopen(argv[optind], "r");
			if (!file)
				fatal("open %s:", *argv);
			parse(file, fspec);
			fclose(file);
		}
	} else {
		parse(stdin, fspec);
	}

	qsort(fs, fslen, sizeof(fs[0]), cmp);
	for (size_t i = 0; i < fslen; ++i) {
		char *s = fs[i], *p = s, *q;

		if (pflag) {
			if (i) {
				q = fs[i - 1];
				while (*p++ == *q++)
					;
			}
			for (; p[1] != '\n'; ++p) {
				if (*p == '/')
					printf("%.*s\ntype=dir\nmode=0755\n\n", (int)(p - s + (p == s)), s);
			}
		}
		puts(s);
	}
	fflush(stdout);
	if (ferror(stdout))
		fatal("write:");
}
