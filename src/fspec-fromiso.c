#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <err.h>
#include <archive.h>
#include <archive_entry.h>
#include "common.h"

static void
usage(const char *prog) {
    fprintf(stderr, "usage: %s [-d datadir]\n", prog);
    exit(1);
}

int
main (int argc, char **argv)
{
    const char *prog;
    int opt;
    struct archive *a;
    char *data_dir = NULL;

    prog = argc ? basename(argv[0]) : "fspec-fromiso";
    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
        case 'd':
            data_dir = optarg;
            break;
        default:
            usage(prog);
            break;
        }
    }

    a = archive_read_new();
    if (!a)
        errx(1, "alloc fail");
    archive_read_support_format_iso9660(a);
    fspec_fromarchive(a, data_dir);
    archive_read_free(a);

    return 0;
}
