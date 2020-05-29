#define _XOPEN_SOURCE 500
#include <sys/stat.h>
#include <ftw.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <err.h>

int root_owned = 0;
int absolute = 0;
char *prefix = "";

static const char *
filetype(mode_t st_mode)
{
    if (S_ISDIR(st_mode))  return "dir";
    if (S_ISREG(st_mode))  return "reg";
    if (S_ISLNK(st_mode))  return "sym";
    if (S_ISFIFO(st_mode)) return "fifo";
    if (S_ISBLK(st_mode))  return "blockdev";
    if (S_ISCHR(st_mode))  return "chardev";
    errx(1, "unknown file type mode '%o'", st_mode);
}

static int
isdefaultmode(mode_t st_mode)
{
    int masked = st_mode & ~S_IFMT;
    return (S_ISDIR(st_mode)  && masked == 0755)
        || (S_ISLNK(st_mode)  && masked == 0755)
        || (S_ISREG(st_mode)  && masked == 0644)
        || (S_ISFIFO(st_mode) && masked == 0644)
        || (S_ISBLK(st_mode)  && masked == 0600)
        || (S_ISCHR(st_mode)  && masked == 0600);
}

static int
printfspec(const char *fpath, const struct stat *sb,
            int tflag, struct FTW *ftwbuf)
{
    int len;
    char pathbuf[PATH_MAX];

    if (strcmp(fpath, ".") == 0)
        return 0;

    printf("%s%s\n", prefix, fpath+2);
    printf("type=%s\n", filetype(sb->st_mode));

    if (!isdefaultmode(sb->st_mode))
        printf("mode=%04o\n", sb->st_mode & ~S_IFMT);

    if (!root_owned) {
        if (sb->st_uid != 0)
            printf("uid=%d\n", sb->st_uid);
        if (sb->st_gid != 0)
            printf("gid=%d\n", sb->st_gid);
    }

    if (S_ISLNK(sb->st_mode)) {
        len = readlink(fpath, pathbuf, sizeof(pathbuf));
        if (len < 0)
            err(1, "readlink of %s failed", fpath);
        if (len == sizeof(pathbuf))
            errx(1, "link target of %s too long", fpath);
        pathbuf[len] = 0;
        if (strchr(pathbuf, '\n'))
            errx(1, "link target contains new line");
        printf("link=%s\n", pathbuf);
    }

    if (S_ISREG(sb->st_mode) && absolute) {
        if (realpath(fpath, pathbuf) == NULL)
            err(1, "realpath of %s failed", fpath);
        if (strchr(pathbuf, '\n'))
            errx(1, "file path contains new line, aborting");
        printf("source=%s\n", pathbuf);
    }

    if (S_ISBLK(sb->st_mode) || S_ISCHR(sb->st_mode))
        printf("devnum=%llu\n", (long long unsigned)sb->st_rdev);

    puts("");
    return 0;
}

int
main(int argc, char **argv)
{
    int opt;
    const char *dir = ".";

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

    if (optind < argc)
        dir = argv[optind];

    if (chdir(dir) != 0)
        err(1, "chdir failed");

    if (nftw(".", printfspec, 20, FTW_PHYS) < 0)
        err(1, "walk of %s failed", dir);

    if (ferror(stdout))
        errx(1, "io error");
    return 0;
}
