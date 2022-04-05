#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <archive.h>
#include <archive_entry.h>
#include <stdnoreturn.h>
#include "common.h"


static int
filetype(const char *type)
{
	if (strcmp(type, "reg") == 0)
		return AE_IFREG;
	if (strcmp(type, "dir") == 0)
		return AE_IFDIR;
	if (strcmp(type, "sym") == 0)
		return AE_IFLNK;
	if (strcmp(type, "fifo") == 0)
		return AE_IFIFO;
	if (strcmp(type, "blockdev") == 0)
		return AE_IFBLK;
	if (strcmp(type, "chardev") == 0)
		return AE_IFCHR;
	fatal("unknown file type '%s'", type);
}

static int
defaultmode(const char *type)
{
	if (strcmp(type, "dir") == 0)
		return 0755;
	if (strcmp(type, "sym") == 0)
		return 0777;
	if (strcmp(type, "reg") == 0 || strcmp(type, "fifo") == 0)
		return 0644;
	if (strcmp(type, "blockdev") == 0 || strcmp(type, "chardev") == 0)
		return 0600;
	fatal("unknown file type '%s'", type);
}

static struct archive *out;
static struct archive_entry *entry;

static void
archive_entry(char *buf, size_t len)
{
	int setmode, mode, datafd;
	char *p, *end, *line, *datapath;

	p = buf;
	end = buf+len;
	line = buf;
	datafd = -1;
	setmode = 1;
	mode = 0;
	datapath = NULL;

	for (p = buf; p < end; p++) {
		if (*p != '\n')
			continue;
		*p = 0;

		if (line[0] == '/') {
			datapath = line+1;
			if (line[1])
				archive_entry_set_pathname(entry, line + 1);
			else
				archive_entry_set_pathname(entry, ".");
			archive_entry_set_size(entry, 0);
		} else if (strncmp(line, "uid=", 4) == 0) {
			archive_entry_set_uid(entry, strtol(line + 4, NULL, 10));
		} else if (strncmp(line, "gid=", 4) == 0) {
			archive_entry_set_gid(entry, strtol(line + 4, NULL, 10));
		} else if (strncmp(line, "mode=", 5) == 0) {
			setmode = 0;
			mode = strtol(line + 5, NULL, 8);
		} else if (strncmp(line, "devnum=", 7) == 0) {
			archive_entry_set_rdev(entry, strtol(line + 7, NULL, 10));
		} else if (strncmp(line, "type=", 5) == 0) {
			const char *t = line + 5;
			archive_entry_set_filetype(entry, filetype(t));
			if (setmode)
				mode = defaultmode(t);
		} else if (strncmp(line, "target=", 7) == 0) {
			archive_entry_set_symlink(entry, line + 7);
		} else if (strncmp(line, "source=", 7) == 0) {
			datapath = line + 7;
		}

		line = p+1;
	}

	archive_entry_set_perm(entry, mode);

	if (archive_entry_filetype(entry) == AE_IFREG) {
		datafd = open(datapath, O_RDONLY);
		if (datafd == -1)
			fatal("open %s failed:", datapath);
	}

	if (datafd != -1) {
		struct stat st;
		if (fstat(datafd, &st) != 0)
			fatal("fstat failed:");
		archive_entry_set_size(entry, st.st_size);
	}

	if (archive_write_header(out, entry) != ARCHIVE_OK)
		fatal("archive write header failed: %s", archive_error_string(out));

	if (datafd != -1) {
		char buf[4096];
		for (;;) {
			ssize_t wlen = read(datafd, buf, sizeof(buf));
			if (wlen < 0)
				fatal("read failed:");
			if (wlen == 0)
				break;
			if (archive_write_data(out, buf, wlen) != wlen)
				fatal("archive write failed");
		}
		close(datafd);
		datafd = -1;
	}

	archive_entry_clear(entry);

}

void
fspec_archive(struct archive *a, char *input)
{
	if (input) {
		char *dir;

		if (!freopen(input, "r", stdin))
			fatal("unable to open input %s:", input);

		dir = dirname(input);
		if (chdir(dir) < 0)
			fatal("chdir %s:", dir);
	}

	if (archive_write_open_filename(a, NULL) != ARCHIVE_OK)
		fatal("archive open failed: %s", archive_error_string(a));

	out = a;
	entry = archive_entry_new();
	if (!entry)
		fatal("alloc failure");

	parse(stdin, archive_entry);

	archive_entry_free(entry);

	if (ferror(stdin))
		fatal("io error:");
	
	if (archive_write_close(a) != ARCHIVE_OK)
		fatal("archive close failed: %s", archive_error_string(a));

}
