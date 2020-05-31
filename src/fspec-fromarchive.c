#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <archive.h>
#include <err.h>
#include <errno.h>
#include <archive_entry.h>

static void
usage(char *prog) {
    printf("%s [-d DATADIR]\n", prog);
    exit(1);
}

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

int
main (int argc, char **argv)
{
    int r, opt;
    ssize_t size;
    struct archive *a = NULL;
    struct archive_entry *entry = NULL;
    FILE *data = NULL;
    char *data_dir = NULL;
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

    if (data_dir) {
        if (strchr(data_dir, '\n'))
            errx(1, "data dir contains new line");

        r = mkdir(data_dir, 0755);
        if (r == -1 && errno != EEXIST)
            err(1, "unable to create %s", data_dir);
    }

    a = archive_read_new();
    if (!a)
        errx(1, "alloc fail");

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
    if (r != ARCHIVE_OK)
        errx(1, "archive open failed: %s", archive_error_string(a));

    while (1) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF)
            break;
        if (r != ARCHIVE_OK)
            errx(1, "archive next header failed: %s", archive_error_string(a));

        const char *path = archive_entry_pathname(entry);
        if (path == NULL)
            errx(1, "archive header missing path");
        if (strchr(path, '\n'))
            errx(1, "archive entry path contains new line");

        printf("%s\n", path);
        printf("type=%s\n", filetype(archive_entry_filetype(entry)));
        printf("mode=%04o\n", archive_entry_perm(entry));

        if (archive_entry_uid(entry) != 0)
            printf("uid=%lld\n", (long long)archive_entry_uid(entry));

        if (archive_entry_gid(entry) != 0)
            printf("gid=%lld\n", (long long)archive_entry_gid(entry));

        if (archive_entry_filetype(entry) == AE_IFLNK) {
            const char *target = archive_entry_symlink(entry);
            if (target == NULL)
                errx(1, "archive header link missing target");

            if (strchr(path, '\n'))
                errx(1, "link target contains new line");

           printf("target=%s\n", target);
        }

        if (archive_entry_filetype(entry) == AE_IFBLK || archive_entry_filetype(entry) == AE_IFCHR)
            printf("devnum=%llu\n", (long long unsigned)archive_entry_rdev(entry));

        if (data_dir && archive_entry_filetype(entry) == AE_IFREG) {
            char tmppath[2048];
            int n = snprintf(tmppath, sizeof(tmppath), "%s/XXXXXX", data_dir);
            if (n < 0 || n == (sizeof(tmppath)))
                errx(1, "data dir path too long");

            int tmpfd = mkstemp(tmppath);
            if (tmpfd == -1)
                err(1, "unable to create temp file");

            printf("source=%s\n", tmppath);

            data = fdopen(tmpfd, "w+");
            if (!data)
                err(1, "unable to open temporary file");
        }

        while (1) {
            size = archive_read_data(a, buff, sizeof(buff));
            if (size < 0)
                errx(1, "archive read failed: %s", archive_error_string(a));
            if (size == 0)
                break;

            if (data)
                if (fwrite(buff, 1, size, data) != size)
                    errx(1, "short write");
        }

        if (data) {
            if (ferror(data) || fclose(data) != 0)
                errx(1, "io error");
            data = NULL;
        }

        puts("");
    }

    archive_read_free(a);

    if (fflush(stdout) != 0 || ferror(stdout))
        errx(1, "io error");

    return 0;
}
