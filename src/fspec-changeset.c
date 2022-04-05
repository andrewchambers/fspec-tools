#define _XOPEN_SOURCE 700
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <stdnoreturn.h>
#include "common.h"

static void
usage(char *prog)
{
	fprintf(stderr, "usage: %s [from.fspec to.fspec]\n", prog);
	exit(1);
}

typedef struct fsent {
	char *path;
	char *type;
	char *source;
	char *target;
	char *blake3;
	mode_t mode;
	uid_t uid;
	gid_t gid;
	dev_t devnum;
} fsent;

static fsent *from, *to;
static size_t nfrom, nto, capfrom, capto;

static int
pathcmp(const char *p1, const char *p2, const char **end)
{
	char c1, c2;

	for (; *p1 == *p2 && *p1; ++p1, ++p2)
		;
	if (end)
		*end = p2;
	c1 = *p1, c2 = *p2;
	return c1 && c2 ? (c1 == '/' ? 0 : c1) - (c2 == '/' ? 0 : c2) : !c2 - !c1;
}

static void
changesetchmod(fsent *ent)
{
	printf("chmod %04o %s\n", ent->mode, ent->path);
}

static void
changesetchown(fsent *ent)
{
	printf("chown %d:%d %s\n", ent->uid, ent->gid, ent->path);
}

static void
changesetdelete(fsent *ent)
{
	if (ent->type[0] == 'd')
		printf("rmdir %s\n", ent->path);
	else
		printf("rm %s\n", ent->path);
}	

static void
changesetcreate(fsent *ent)
{
	static char buf[16384];
	struct stat st;
	ssize_t n;
	size_t nwritten;
	FILE *dataf;

	if (ent->type[0] == 'r') {

		if (lstat(ent->source, &st) != 0)
			fatal("stat %s:", ent->source);

		dataf = fopen(ent->source, "rb");
		if (!dataf)
			fatal("open %s:", ent->source);

		printf("head -c %ld > %s\n", st.st_size, ent->path);
		nwritten = 0;
		while (1) {
			n = fread(buf, 1, sizeof(buf), dataf);
			if (n <= 0)
				break;
			if (n + nwritten > st.st_size)
				fatal("%s grew while reading", ent->source);
			if (fwrite(buf, 1, n, stdout) != n)
				fatal("write:");
			nwritten += n;
		}
		if (ferror(dataf))
			fatal("read:");
		if (nwritten != st.st_size)
			fatal("file shrunk while reading", ent->source);
		fclose(dataf);
	} else if (ent->type[0] == 'd') {
		printf("mkdir %s\n", ent->path);
	} else if (ent->type[0] == 's') {
		printf("ln -s %s %s\n", ent->target, ent->path);
	} else if (ent->type[0] == 'f') {
		printf("mkfifo %s\n", ent->path);
	} else if (ent->type[0] == 'c' || ent->type[0] == 'b') {
		printf("mknod %s %c %d %d\n", ent->path, ent->type[0], major(ent->devnum), minor(ent->devnum));
	} else {
		fatal("unhandled entry type %s at %s", ent->type, ent->path);
	}
	changesetchmod(ent);
	changesetchown(ent);
}

static void
changeset(void)
{	
	int cmp;
	ssize_t i, j;
	i = nfrom-1;
	j = nto-1;
	while (i >= 0 && j >= 0) {
		cmp = pathcmp(from[i].path, to[j].path, NULL);
		if (cmp == 0) {
			int needs_recreate = 0;
			int needs_chown = 0;
			int needs_chmod = 0;

			if (strcmp(from[i].type, to[j].type) == 0) {
				if (from[i].mode != to[j].mode)
					needs_chmod = 1;
				if (from[i].uid != to[j].uid)
					needs_chown = 1;
				if (from[i].gid != to[j].gid)
					needs_chown = 1;
				if (from[i].type[0] == 'r') {
					if (strcmp(from[i].blake3, to[j].blake3) != 0)
						needs_recreate = 1;
				} else if (from[i].type[0] == 'c' || from[i].type[0] == 'b') {
					if (from[i].devnum != to[j].devnum)
						needs_recreate = 1;
				} else if (from[i].type[0] == 's') {
					if (strcmp(from[i].target, to[j].target) != 0)
						needs_recreate = 1;
				}
			} else {
				needs_recreate = 1;
			}
			if (needs_recreate) {
				changesetdelete(&from[i]);
				changesetcreate(&to[i]);
			} else {
				if (needs_chmod)
					changesetchmod(&to[i]);
				if (needs_chown)
					changesetchown(&to[i]);
			}
			i -= 1;
			j -= 1;
		} else if (cmp > 0) {
			changesetdelete(&from[i]);
			i -= 1;
		} else {
			changesetcreate(&to[j]);
			j -= 1;
		}
	}
	while (i >= 0) {
		changesetdelete(&from[i]);
		i -= 1;
	}
	while (j >= 0) {
		changesetcreate(&to[j]);
		j -= 1;
	}
}


static void
parseents(fsent **ents, size_t *n, size_t *cap, char *buf, size_t len) {
	fsent *ent;
	char *p, *end, *line;
	int has_mode;
	int has_devnum;

	if (*n == *cap) {
		*cap = *cap ? (*cap)*2 : 512;
		if (!(*ents = reallocarray(*ents, *cap, sizeof(fsent))))
			fatal(NULL);
	}

	ent = &(*ents)[*n];
	memset(ent, 0, sizeof(fsent));

	p = buf;
	end = buf+len;
	line = buf;
	has_mode = 0;
	has_devnum = 0;

	for (p = buf; p < end; p++) {
		if (*p != '\n')
			continue;
		*p = 0;

		if (line[0] == '/') {
			ent->path = xstrdup(line);
		} else if (strncmp(line, "uid=", 4) == 0) {
			ent->uid = atol(line+4);
		} else if (strncmp(line, "gid=", 4) == 0) {
			ent->gid = atol(line+4);
		} else if (strncmp(line, "mode=", 5) == 0) {
			has_mode = 1;
			ent->mode = strtol(line+5, NULL, 8);
		} else if (strncmp(line, "type=", 5) == 0) {
			ent->type = xstrdup(line+5);
		} else if (strncmp(line, "devnum=", 7) == 0) {
			has_devnum = 1;
			ent->devnum = atol(line+7);
		} else if (strncmp(line, "target=", 7) == 0) {
			ent->target = xstrdup(line+7);
		} else if (strncmp(line, "source=", 7) == 0) {
			ent->source = xstrdup(line+7);
		} else if (strncmp(line, "blake3=", 7) == 0) {
			ent->blake3 = xstrdup(line+7);
		} else {
			/* ignore */
		}
		line = p+1;
	}

	if (!ent->type)
		fatal("%s does not have type", ent->path);

	if (strcmp(ent->type, "reg") == 0) {
		if (!has_mode)
			ent->mode = 0644;
		if (!ent->blake3)
			fatal("file %s does not have a blake3 hash", ent->path);
		if (!ent->source)
			ent->source = ent->path;
	} else if (strcmp(ent->type, "dir") == 0) {
		if (!has_mode)
			ent->mode = 0755;
	} else if (strcmp(ent->type, "fifo") == 0) {
		if (!has_mode)
			ent->mode = 0644;
	} else if (strcmp(ent->type, "link") == 0) {
		if (!has_mode)
			ent->mode = 0777;
		if (!ent->target)
			fatal("symlink %s does not have a target", ent->path);
	} else if (strcmp(ent->type, "chardev") == 0 || strcmp(ent->type, "blockdev") == 0) {
		if (!has_mode)
			ent->mode = 0600;
		if (!has_devnum)
			fatal("%s does not have a device number", ent->path);
	} else {
		fatal("%s has an unsupported type: %s", ent->path, ent->type);
	}

	*n += 1;
}

static void
parsefrom(char *buf, size_t len) 
{
	parseents(&from, &nfrom, &capfrom, buf, len);
}

static void
parseto(char *buf, size_t len)
{
	parseents(&to, &nto, &capto, buf, len);
}

int
main(int argc, char *argv[])
{
	int opt;
	char *prog;
	FILE *fromfile, *tofile;

	prog = argc ? basename(argv[0]) : "fspec-changeset";
	while ((opt = getopt(argc, argv, "")) != -1) {
		switch (opt) {
		default:
			usage(prog);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage(prog);

	fromfile = fopen(argv[0], "rb");
	if (!fromfile)
		fatal("unable to open %s:", argv[0]);
	tofile = fopen(argv[1], "rb");
	if (!tofile)
		fatal("unable to open %s:", argv[1]);
	
	parse(fromfile, parsefrom);
	parse(tofile, parseto);

	changeset();

	if (fflush(stdout) != 0 || ferror(stdout))
		fatal("io error:");
}
