#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <libgen.h>
#include <archive.h>
#include <archive_entry.h>
#include <stdnoreturn.h>
#include "common.h"

static void
usage(const char *prog)
{
	fprintf(stderr, "usage: %s [fspec]\n", prog);
	exit(1);
}

int
main(int argc, char **argv)
{
	const char *prog;
	struct archive *a;
	int opt;

	prog = argc ? basename(argv[0]) : "fspec-tar";
	while ((opt = getopt(argc, argv, "")) != -1) {
		switch (opt) {
		default:
			usage(prog);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 1)
		usage(prog);

	a = archive_write_new();
	if (!a)
		fatal("alloc failure");
	archive_write_set_format_pax_restricted(a);
	fspec_archive(a, argv[0]);
	archive_write_free(a);

	return 0;
}
