#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <archive.h>
#include <errno.h>
#include <string.h>
#include <archive_entry.h>

static int datafd = -1;
static struct archive *a = NULL;
static struct archive_entry *entry = NULL;

static void
check(int ok) {
    if (!ok) {
        if (a && archive_error_string(a)) {
            fprintf(stderr, "failed - %s\n", archive_error_string(a));
        } else if (errno != 0) {
            fprintf(stderr, "failed - %s\n", strerror(errno));
        } else {
            fputs("failed", stderr);
        }
        exit(1);
    }
}

static int
filetype(const char *type)
{
    if (strcmp(type, "reg") == 0)
        return AE_IFREG;
    if (strcmp(type, "dir") == 0)
        return AE_IFDIR;
    if (strcmp(type, "sym") == 0)
        return AE_IFLNK;
    fprintf(stderr, "unknown file type '%s'\n", type);
    exit(1);
}

int main(int argc, char **argv)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t n;

    a = archive_write_new();
    entry = archive_entry_new();
    check(!!a);
    check(!!entry);
#if defined(OUT_FORMAT_CPIO)
    archive_write_set_format_cpio(a);
#elif defined(OUT_FORMAT_TAR)
    archive_write_set_format_pax_restricted(a);
#else
#error "define OUT_FORMAT_CPIO or OUT_FORMAT_TAR"
#endif
    check(archive_write_open_filename(a, NULL) == ARCHIVE_OK);

    errno = 0;
    do {
        /* skip blank lines */
        while ((n = getline(&line, &len, stdin)) == 1)
            ;
        if (n <= 0)
            break;
        if (line[n - 1] == '\n')
            line[n - 1] = '\0';
        archive_entry_set_pathname(entry, line);
        /* Setting this explicitly once seems necessary for cpio mode. */
        archive_entry_set_size(entry, 0);
        while ((n = getline(&line, &len, stdin)) > 1) {
            if (line[n - 1] == '\n')
                line[n - 1] = '\0';
            if (strncmp(line, "uid=", 4) == 0) {
                archive_entry_set_uid(entry, strtol(line + 4, NULL, 10));
            } else if (strncmp(line, "gid=", 4) == 0) {
                archive_entry_set_gid(entry, strtol(line + 4, NULL, 10));
            } else if (strncmp(line, "perm=", 5) == 0) {
                archive_entry_set_perm(entry, strtol(line + 5, NULL, 8));
            } else if (strncmp(line, "type=", 5) == 0) {
                archive_entry_set_filetype(entry, filetype(line + 5));
            } else if (strncmp(line, "link=", 5) == 0) {
                archive_entry_set_symlink(entry, line + 5);
            } else if (strncmp(line, "source=", 7) == 0) {
                const char *path = line + 7;
                struct stat st;

                check(stat(path, &st) == 0);
                archive_entry_set_size(entry, st.st_size);
                datafd = open(path, O_RDONLY);
                check(datafd != -1);
            } else {
                fprintf(stderr, "unknown attribute line '%s'\n", line);
                exit(1);
            }
        }
        check(archive_write_header(a, entry) == ARCHIVE_OK);
        archive_entry_clear(entry);
        if (datafd != -1) {
            char buff[4096];
            int wlen = read(datafd, buff, sizeof(buff));
            check(wlen >= 0);
            while ( wlen > 0 ) {
                check(archive_write_data(a, buff, wlen) == wlen);
                wlen = read(datafd, buff, sizeof(buff));
                check(wlen >= 0);
            }
            close(datafd);
            datafd = -1;
        }
    } while (n != -1);
    
    check(!ferror(stdin));
    check(archive_write_close(a) == ARCHIVE_OK);
    
    archive_entry_free(entry);
    archive_write_free(a);
    return 0;
}
