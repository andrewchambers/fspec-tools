#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <archive.h>
#include <archive_entry.h>

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
    fprintf(stderr, "unknown file type '%s'\n", type);
    exit(1);
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
    fprintf(stderr, "unknown file type '%s'\n", type);
    exit(1);
}

int
main(int argc, char **argv)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t n = 0;
    int datafd = -1;
    struct archive *a = NULL;
    struct archive_entry *entry = NULL;

    a = archive_write_new();
    entry = archive_entry_new();
    if (!a || !entry)
        errx(1, "alloc failure");
#if defined(OUT_FORMAT_CPIO)
    archive_write_set_format_cpio_newc(a);
#elif defined(OUT_FORMAT_TAR)
    archive_write_set_format_pax_restricted(a);
#else
#error "define OUT_FORMAT_CPIO or OUT_FORMAT_TAR"
#endif
    if (archive_write_open_filename(a, NULL) != ARCHIVE_OK)
        errx(1, "archive open failed: %s", archive_error_string(a));

    do {
        int set_default_mode = 1;
        int mode = 0;

        /* skip blank lines */
        while ((n = getline(&line, &len, stdin)) == 1)
            ;
        if (n <= 0)
            break;
        if (line[n - 1] == '\n')
            line[n - 1] = '\0';

        archive_entry_set_pathname(entry, line);
        archive_entry_set_size(entry, 0);

        while ((n = getline(&line, &len, stdin)) > 1) {
            if (line[n - 1] == '\n')
                line[n - 1] = '\0';
            if (strncmp(line, "uid=", 4) == 0) {
                archive_entry_set_uid(entry, strtol(line + 4, NULL, 10));
            } else if (strncmp(line, "gid=", 4) == 0) {
                archive_entry_set_gid(entry, strtol(line + 4, NULL, 10));
            } else if (strncmp(line, "mode=", 5) == 0) {
                set_default_mode = 0;
                mode = strtol(line + 5, NULL, 8);
            } else if (strncmp(line, "devnum=", 7) == 0) {
                archive_entry_set_rdev(entry, strtol(line + 7, NULL, 10));
            } else if (strncmp(line, "type=", 5) == 0) {
                const char *t = line + 5;

                archive_entry_set_filetype(entry, filetype(t));
                if (set_default_mode)
                    mode = defaultmode(t);
            } else if (strncmp(line, "link=", 5) == 0) {
                archive_entry_set_symlink(entry, line + 5);
            } else if (strncmp(line, "source=", 7) == 0) {
                const char *path = line + 7;
                struct stat st;

                if (stat(path, &st) != 0)
                    err(1, "stat %s failed", path);
                archive_entry_set_size(entry, st.st_size);
                datafd = open(path, O_RDONLY);
                if (datafd == -1)
                    err(1, "open %s failed", path);
            } else {
                errx(1, "unknown attribute line '%s'", line);
            }
        }
        archive_entry_set_perm(entry, mode);

        if(archive_write_header(a, entry) != ARCHIVE_OK)
            errx(1, "archive write header failed: %s", archive_error_string(a));

        if (datafd != -1) {
            char buff[4096];
            for (;;) {
                ssize_t wlen = read(datafd, buff, sizeof(buff));
                if (wlen < 0)
                    err(1, "read failed");
                if (wlen == 0)
                    break;
                if (archive_write_data(a, buff, wlen) != wlen)
                    errx(1, "archive write failed");
            }
            close(datafd);
            datafd = -1;
        }

        archive_entry_clear(entry);
    } while (n != -1);

    if(ferror(stdin))
        err(1, "io error");
    
    if (archive_write_close(a) != ARCHIVE_OK)
        errx(1, "archive close failed: %s", archive_error_string(a));

    archive_entry_free(entry);
    archive_write_free(a);

    return 0;
}
