#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <archive.h>
#include <archive_entry.h>

static struct archive *a = NULL;

static void
usage(char *prog) {
    printf("%s [-d DATADIR]\n", prog);
    exit(1);
}

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

static const char *
filetype(__LA_MODE_T ty)
{
    if (ty == AE_IFREG)
        return "reg";
    if (ty == AE_IFDIR)
        return "dir";
    if (ty == AE_IFLNK)
        return "sym";
    fprintf(stderr, "unknown file type '%d'\n", ty);
    exit(1);
}

int
main (int argc, char **argv)
{
    int r, opt;
    ssize_t size;
    struct archive_entry *entry = NULL;
    char * data_dir = NULL;
    char buff[4096];

    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
        case 'd':
            data_dir = optarg;
            break;
        default:
            usage(argv[0]);
            break;
        }
    }

    errno = 0;
    a = archive_read_new();
    check(!!a);

#if defined(IN_FORMAT_CPIO)
    archive_read_support_format_cpio(a);
#elif defined(IN_FORMAT_TAR)
    archive_read_support_format_tar(a);
    archive_read_support_format_gnutar(a);
#else
#error "define IN_FORMAT_CPIO or IN_FORMAT_TAR"
#endif
    archive_read_support_format_all(a);
    r = archive_read_open_filename(a, NULL, 16384);
    check(r == ARCHIVE_OK);

    while (1) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF)
            break;
        check(r == ARCHIVE_OK);

        const char *path = archive_entry_pathname(entry);
        check(path != NULL);
        if (strchr(path, '\n')) {
            fputs("archive entry path contains new line, aborting", stderr);
            exit(1);
        }

        check(printf("%s\n", path) > 0);
        check(printf("type=%s\n", filetype(archive_entry_filetype(entry))) > 0);
        check(printf("mode=%o\n", archive_entry_perm(entry)) > 0);
        if (archive_entry_filetype(entry) == AE_IFLNK) {
            const char *link = archive_entry_symlink(entry);
            check(link != NULL);
            if (strchr(path, '\n')) {
                fputs("link target contains new line, aborting", stderr);
                exit(1);
            }
            check(printf("link=%s\n", link) > 0);
        }

        if (data_dir) {
            fprintf(stderr, "-d is unimplemented");
            exit(1);
        }
        while (1) {
            size = archive_read_data(a, buff, sizeof(buff));
            check(size >= 0);
            if (size == 0)
                break;
        }

        check(printf("\n") > 0);
    }

    archive_read_free(a);
    check(fflush(stdout) == 0 && !ferror(stdout));
}
