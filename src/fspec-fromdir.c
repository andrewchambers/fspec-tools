#define _XOPEN_SOURCE 700
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <archive.h>
#include <stdnoreturn.h>
#include "common.h"

int root_owned = 0;
int absolute = 0;
char *prefix = "";

static void
usage(const char *argv0)
{
   fprintf(stderr, "usage: %s [-C dir] [-p prefix] [-a] [-r] [path...]\n", argv0 ? argv0 : "fspec-fromdir");
   exit(1);
}

static int
skipdot(const struct dirent *d)
{
    return strcmp(d->d_name, ".") != 0 && strcmp(d->d_name, "..") != 0;
}

static void printentry(char *, size_t, size_t);

static void
recurse(char *path, size_t len, size_t max)
{
    struct dirent **d;
    int dlen;

    dlen = scandir(path, &d, skipdot, alphasort);
    if (dlen < 0)
        fatal("scandir %s:", path);
    if (len == max)
        fatal("path is too long");
    if (len)
        path[len++] = '/';
    for (int i = 0; i < dlen; ++i) {
        char *end = memccpy(path + len, d[i]->d_name, '\0', max - len);
        if (!end)
            fatal("path is too long");
        printentry(path, end - path - 1, max);
        free(d[i]);
    }
    free(d);
}

static void
printentry(char *path, size_t len, size_t max)
{
    static char buf[PATH_MAX];
    ssize_t buflen;
    struct stat st;

    if (lstat(path, &st) != 0)
        fatal("stat %s:", path);

    printf("%s/%s\n", prefix, path);

    switch (st.st_mode & S_IFMT) {
    case S_IFREG:
        puts("type=reg");
        if (absolute) {
            if (realpath(path, buf) == NULL)
                fatal("realpath of %s failed:", path);
            if (strchr(buf, '\n'))
                fatal("file path contains new line, aborting");
            printf("source=%s\n", buf);
        }
        break;
    case S_IFLNK:
        puts("type=sym");
        buflen = readlink(path, buf, sizeof(buf));
        if (buflen < 0)
            fatal("readlink of %s failed:", path);
        if (buflen >= sizeof(buf))
            fatal("link target of %s too long", path);
        buf[buflen] = 0;
        if (strchr(buf, '\n'))
            fatal("link target contains new line");
        printf("target=%s\n", buf);
        break;
    case S_IFCHR:
    case S_IFBLK:
        printf("type=%s\n", S_ISCHR(st.st_mode) ? "chardev" : "blockdev");
        printf("devnum=%llu\n", (long long unsigned)st.st_rdev);
        break;
    case S_IFDIR:
        puts("type=dir");
        break;
    case S_IFIFO:
        puts("type=fifo");
        break;
    }

    if (!S_ISLNK(st.st_mode))
        printf("mode=%04o\n", st.st_mode & ~S_IFMT);
    if (!root_owned) {
        printf("uid=%d\n", st.st_uid);
        printf("gid=%d\n", st.st_gid);
    }
    putchar('\n');

    if (S_ISDIR(st.st_mode))
        recurse(path, len, max);
}

int
main(int argc, char **argv)
{
    int opt;
    char path[PATH_MAX];

    while ((opt = getopt(argc, argv, "C:p:ar")) != -1) {
        switch (opt) {
        case 'C':
            if (chdir(optarg) != 0)
                fatal("chdir %s:", optarg);
            break;
        case 'p':
           prefix = optarg;
           break;
        case 'a':
           absolute = 1;
           break;
        case 'r':
           root_owned = 1;
           break;
        default:
           usage(argv[0]);
        }
    }

    if (optind < argc) {
        for (; optind < argc; ++optind) {
            char *end = memccpy(path, argv[optind], '\0', sizeof(path));
            if (!end)
                fatal("path is too long");
            if (path[0] == '/')
                fatal("paths must be relative");
            printentry(path, end - path - 1, sizeof(path));
        }
    } else {
        strcpy(path, ".");
        recurse(path, 0, sizeof(path));
    }

    if (fflush(stdout) != 0 || ferror(stdout))
        fatal("io error");
    return 0;
}
