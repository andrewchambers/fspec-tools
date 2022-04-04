#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <archive.h>
#include <archive_entry.h>
#include <stdnoreturn.h>
#include "common.h"

static const char *
filetype(__LA_MODE_T ty)
{
    switch (ty) {
    case AE_IFREG: return "reg";
    case AE_IFDIR: return "dir";
    case AE_IFLNK: return "sym";
    case AE_IFIFO: return "fifo";
    case AE_IFBLK: return "blockdev";
    case AE_IFCHR: return "chardev";
    }
    fprintf(stderr, "unknown file type '%d'\n", ty);
    exit(1);
}

void
fspec_fromarchive(struct archive *a, const char *data_dir)
{
    int r;
    ssize_t size;
    struct archive_entry *entry = NULL;
    FILE *data = NULL;
    char buff[4096];

    if (archive_read_open_filename(a, NULL, 16384) != ARCHIVE_OK)
        fatal("archive open failed: %s", archive_error_string(a));

    if (data_dir) {
        if (strchr(data_dir, '\n'))
            fatal("data dir contains new line");

        r = mkdir(data_dir, 0755);
        if (r == -1 && errno != EEXIST)
            fatal("unable to create %s:", data_dir);
    }

    while (1) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF)
            break;
        if (r != ARCHIVE_OK)
            fatal("archive next header failed: %s", archive_error_string(a));

        const char *path = archive_entry_pathname(entry);
        if (path == NULL)
            fatal("archive header missing path");
        if (strchr(path, '\n'))
            fatal("archive entry path contains new line");

        printf("%s%s\n", path[0] == '/' ? "" : "/", path);
        printf("type=%s\n", filetype(archive_entry_filetype(entry)));
        printf("mode=%04o\n", archive_entry_perm(entry));

        if (archive_entry_uid(entry) != 0)
            printf("uid=%lld\n", (long long)archive_entry_uid(entry));

        if (archive_entry_gid(entry) != 0)
            printf("gid=%lld\n", (long long)archive_entry_gid(entry));

        if (archive_entry_filetype(entry) == AE_IFLNK) {
            const char *target = archive_entry_symlink(entry);
            if (target == NULL)
                fatal("archive header link missing target");

            if (strchr(path, '\n'))
                fatal("link target contains new line");

           printf("target=%s\n", target);
        }

        if (archive_entry_filetype(entry) == AE_IFBLK || archive_entry_filetype(entry) == AE_IFCHR)
            printf("devnum=%llu\n", (long long unsigned)archive_entry_rdev(entry));

        if (data_dir && archive_entry_filetype(entry) == AE_IFREG) {
            char tmppath[2048];
            int n = snprintf(tmppath, sizeof(tmppath), "%s/XXXXXX", data_dir);
            if (n < 0 || n == (sizeof(tmppath)))
                fatal("data dir path too long");

            int tmpfd = mkstemp(tmppath);
            if (tmpfd == -1)
                fatal("unable to create temp file:");

            printf("source=%s\n", tmppath);

            data = fdopen(tmpfd, "w+");
            if (!data)
                fatal("unable to open temporary file:");
        }

        while (1) {
            size = archive_read_data(a, buff, sizeof(buff));
            if (size < 0)
                fatal("archive read failed: %s", archive_error_string(a));
            if (size == 0)
                break;

            if (data)
                if (fwrite(buff, 1, size, data) != size)
                    fatal("short write");
        }

        if (data) {
            if (ferror(data) || fclose(data) != 0)
                fatal("io error");
            data = NULL;
        }

        puts("");
    }

    if (fflush(stdout) != 0 || ferror(stdout))
        fatal("io error");
}
