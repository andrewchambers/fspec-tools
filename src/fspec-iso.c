#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <libgen.h>
#include <err.h>
#include <string.h>
#include <archive.h>
#include <archive_entry.h>
#include "common.h"

static void
usage(const char *prog)
{
    fprintf(stderr, "usage: %s [manifest]\n", prog);
    exit(1);
}

int
main(int argc, char **argv)
{
    const char *prog;
    struct archive *a;
    int opt;

    a = archive_write_new();
    if (!a)
        errx(1, "alloc failure");
    archive_write_set_format_iso9660(a);

    prog = argc ? basename(argv[0]) : "fspec-iso";

    while ((opt = getopt(argc, argv, "O:")) != -1) {
        switch (opt) {
        case 'O':{
            char *eq = strchr(optarg, '=');
            char *v = eq + 1;
            char *o = optarg;

            if (!eq)
                errx(1, "malformed -O option - '%s'", optarg);

            *eq = 0;
            if (archive_write_set_option(a, "iso9660", o, v) != ARCHIVE_OK)
                errx(1, "setting option failed: %s", archive_error_string(a));
            *eq = '=';
            break;
        }
        default:
            usage(prog);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc > 1)
        usage(prog);

    fspec_archive(a, argv[0]);
    archive_write_free(a);

    return 0;
}
