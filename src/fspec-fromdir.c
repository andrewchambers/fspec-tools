#define _XOPEN_SOURCE 700
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdnoreturn.h>
#include "common.h"

static char *rootdir;
static int want_uid = 0;
static int want_gid = 0;
static int want_mode = 0;
static int want_execute_mode = 0;

static void
usage(const char *argv0)
{
	fprintf(stderr, "usage: %s path\n", argv0 ? argv0 : "fspec-fromdir");
	exit(1);
}

static int
skipdot(const struct dirent *d)
{
	return strcmp(d->d_name, ".") != 0 && strcmp(d->d_name, "..") != 0;
}

static void printentry(char *, size_t, size_t);

static void
recurse(char *path, size_t len, size_t max)
{
	struct dirent **d;
	ssize_t i, namelen, dlen;

	dlen = scandir(path, &d, skipdot, alphasort);
	if (dlen < 0)
		fatal("scandir %s:", path);
	if (len == max)
		fatal("path is too long");
	path[len-1] = '/';
	len += 1;
	for (i = 0; i < dlen; i++) {
		if (strchr(d[i]->d_name, '\n'))
			fatal("path contains newline");
		namelen = strlen(d[i]->d_name);
		if (len+namelen >= max)
			fatal("path is too long");
		memcpy(path+len-1, d[i]->d_name, namelen + 1);
		printentry(path, len+namelen, max);
		free(d[i]);
	}
	free(d);
}

static void
printentry(char *path, size_t len, size_t max)
{
	static char buf[PATH_MAX];
	ssize_t buflen;
	struct stat st;

	if (lstat(path, &st) != 0)
		fatal("stat %s:", path);

	printf("%s%s\n", path[1] ? "" : "/", path+1);

	switch (st.st_mode & S_IFMT) {
	case S_IFREG:
		puts("type=reg");
		printf("source=%s%s\n", rootdir, path+1);
		break;
	case S_IFLNK:
		puts("type=sym");
		buflen = readlink(path, buf, sizeof(buf));
		if (buflen < 0)
			fatal("readlink of %s failed:", path);
		if (buflen >= sizeof(buf))
			fatal("link target of %s too long", path);
		buf[buflen] = 0;
		if (strchr(buf, '\n'))
			fatal("link target contains new line");
		printf("target=%s\n", buf);
		break;
	case S_IFCHR:
	case S_IFBLK:
		printf("type=%s\n", S_ISCHR(st.st_mode) ? "chardev" : "blockdev");
		printf("devnum=%llu\n", (long long unsigned)st.st_rdev);
		break;
	case S_IFDIR:
		puts("type=dir");
		break;
	case S_IFIFO:
		puts("type=fifo");
		break;
	}

	if (!S_ISLNK(st.st_mode) && (want_mode || want_execute_mode)) {
		mode_t mode;

		if (want_mode) {
			mode = st.st_mode & ~S_IFMT;
		} else if (want_execute_mode) {
			switch (st.st_mode & S_IFMT) {
			case S_IFREG:
				mode = 0644 | (st.st_mode&0111);
				break;
			case S_IFLNK:
				mode = 0777;
				break;
			case S_IFCHR:
			case S_IFBLK:
				mode = 0600;
				break;
			case S_IFDIR:
				mode = 0755;
				break;
			case S_IFIFO:
				mode = 0644;
				break;
			default:
				fatal("unsupported file mode");
			}
		}

		printf("mode=%04o\n", mode);
	}
	if (want_uid)
		printf("uid=%d\n", st.st_uid);
	if (want_gid)
		printf("gid=%d\n", st.st_gid);
	putchar('\n');

	if (S_ISDIR(st.st_mode))
		recurse(path, len, max);
}

int
main(int argc, char **argv)
{
	int opt;
	char *prog;
	static char path[PATH_MAX];

	prog = argc ? basename(argv[0]) : "fspec-dir";
	while ((opt = getopt(argc, argv, "ugmx")) != -1) {
		switch (opt) {
		case 'u':
			want_uid = 1;
			break;
		case 'g':
			want_gid = 1;
			break;
		case 'm':
			want_mode = 1;
			break;
		case 'x':
			want_execute_mode = 1;
			break;
		default:
			usage(prog);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage(prog);

	rootdir = argv[0];

	if (strchr(rootdir, '\n'))
		fatal("path contains newline");

	if (chdir(rootdir) != 0)
		fatal("chdir %s:", argv[0]);

	path[0] = '.';
	path[1] = 0;
	printentry(path, 2, sizeof(path));

	if (fflush(stdout) != 0 || ferror(stdout))
		fatal("io error");
	return 0;
}
