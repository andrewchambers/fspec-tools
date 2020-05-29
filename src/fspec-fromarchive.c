#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <archive.h>
#include <err.h>
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

    a = archive_read_new();
    if(!a)
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
        if(r != ARCHIVE_OK)
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
            const char *link = archive_entry_symlink(entry);
            if (link == NULL)
                errx(1, "archive header link missing target");

            if (strchr(path, '\n'))
                errx(1, "link target contains new line");

           printf("link=%s\n", link);
        }

        if (archive_entry_filetype(entry) == AE_IFBLK || archive_entry_filetype(entry) == AE_IFCHR)
            printf("devnum=%llu\n", (long long unsigned)archive_entry_rdev(entry));

        if (data_dir) {
            fprintf(stderr, "-d is unimplemented");
            exit(1);
        }

        while (1) {
            size = archive_read_data(a, buff, sizeof(buff));
            if(size < 0)
                errx(1, "archive read failed: %s", archive_error_string(a));
            if (size == 0)
                break;
        }

        puts("");
    }

    archive_read_free(a);

    if(fflush(stdout) != 0 || ferror(stdout))
        errx(1, "io error");

    return 0;
}
