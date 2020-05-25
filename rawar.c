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

int datafd = -1;
struct archive *a = NULL;
struct archive_entry *entry = NULL;

int parse_error() {
    fputs("parse error", stderr);
    exit(1);
}

void check(int ok) {
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

#include "parse.inc.c"

int main(int argc, char **argv)
{
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
    while (yyparse()) {
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
    }

    archive_entry_free(entry);
    check(archive_write_close(a) == ARCHIVE_OK);
    archive_write_free(a);
    return 0;
}
