#define _XOPEN_SOURCE 700
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>

int root_owned = 0;
int absolute = 0;
char *prefix = "";

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
        err(1, "scandir %s", path);
    if (len == max)
        errx(1, "path is too long");
    path[len++] = '/';
    for (int i = 0; i < dlen; ++i) {
        char *end = memccpy(path + len, d[i]->d_name, '\0', max - len);
        if (!end)
            errx(1, "path is too long");
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

    if (stat(path, &st) != 0)
        err(1, "stat %s", path);

    printf("%s%s\n", prefix, path + 2);

    switch (st.st_mode & S_IFMT) {
    case S_IFREG:
        puts("type=reg");
        if (absolute) {
            if (realpath(path, buf) == NULL)
                err(1, "realpath of %s failed", path);
            if (strchr(buf, '\n'))
                errx(1, "file path contains new line, aborting");
            printf("source=%s\n", buf);
        }
        break;
    case S_IFLNK:
        puts("type=sym");
        buflen = readlink(path, buf, sizeof(buf));
        if (buflen < 0)
            err(1, "readlink of %s failed", path);
        if (buflen >= sizeof(buf))
            errx(1, "link target of %s too long", path);
        buf[buflen] = 0;
        if (strchr(buf, '\n'))
            errx(1, "link target contains new line");
        printf("link=%s\n", buf);
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
    char path[PATH_MAX] = ".";

    while ((opt = getopt(argc, argv, "p:ar")) != -1) {
        switch (opt) {
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
           fprintf(stderr, "Usage: %s [-p PREFIX] [-a] [-r] [PATH]\n", argv[0]);
           exit(EXIT_FAILURE);
        }
    }

    if (optind < argc && chdir(argv[optind]) != 0)
        err(1, "chdir %s", argv[optind]);

    recurse(path, 1, sizeof(path));

    if (ferror(stdout))
        errx(1, "io error");
    return 0;
}
