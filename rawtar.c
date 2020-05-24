#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <archive.h>
#include <assert.h>
#include <string.h>
#include <archive_entry.h>

int datafd = -1;
struct archive *a = NULL;
struct archive_entry *entry = NULL;

int parse_error() {
    fputs("parse error", stderr);
    exit(1);
}

/* returns a borrowed char* */
char *decode_quoted(char * t) {
    static char *s = NULL;
    int len;

    t++;
    len = strlen(t);
    t[len-1] = 0;
    if (s) {
        free(s);
        s = NULL;
    }
    s = malloc(len);
    assert(s);
    int i, j;
    i = j = 0;
    while ( t[i] ) {
        switch ( t[i] ) {
        case '\\':
            switch( t[++i] ) {
            case '\\':
                s[j] = '\\';
                break;
            case '\"':
                s[j] = '\"';
                break;
            default:
                s[j++] = '\\';
                s[j] = t[i];
            }
            break;
        default:
            s[j] = t[i];
        }
        ++i;
        ++j;
    }
    s[j] = t[i];
    t[len-1] = '\"';
    return s;
}

void check(int ok) {
    if (!ok) {
        if (a && archive_error_string(a)) {
            fprintf(stderr, "rawtar failed - %s\n", archive_error_string(a));
        } else {
            fputs("rawtar failed", stderr);
        }
        exit(1);
    }
}

#include "parse.inc.c"

int main()
{
    a = archive_write_new();
    entry = archive_entry_new();
    assert(a);
    assert(entry);

    archive_write_set_format_pax_restricted(a);
    check(archive_write_open_filename(a, NULL) == ARCHIVE_OK);

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
