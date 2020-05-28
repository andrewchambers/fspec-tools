#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

int root_owned = 0;
int absolute = 0;
char *prefix = "";

static void
check(int ok) {
    if (!ok) {
        if (errno != 0) {
            fprintf(stderr, "failed - %s\n", strerror(errno));
        } else {
            fputs("failed\n", stderr);
        }
        exit(1);
    }
}

static const char *
filetype(int tflag)
{
    if (tflag == FTW_F)
        return "reg";
    if (tflag == FTW_D)
        return "dir";
    if (tflag == FTW_SL)
        return "sym";
    fprintf(stderr, "unknown file type flags '%d'\n", tflag);
    exit(1);
}

static int
printfspec(const char *fpath, const struct stat *sb,
            int tflag, struct FTW *ftwbuf)
{
    int len;
    char pathbuf[PATH_MAX];

    if (strcmp(fpath, ".") == 0)
        return 0;

    check(printf("%s%s\n", prefix, fpath+2) > 0);
    check(printf("type=%s\n", filetype(tflag)) > 0);
    check(printf("mode=%o\n", sb->st_mode & 0777 /* XXX masking too much */ ) > 0);
    if (!root_owned) {
        if (sb->st_uid != 0)
            check(printf("uid=%d\n", sb->st_uid) > 0);
        if (sb->st_gid != 0)
            check(printf("gid=%d\n", sb->st_gid) > 0);
    }
    
    if (tflag == FTW_SL) {
        len = readlink(fpath, pathbuf, sizeof(pathbuf));
        check(len != sizeof(pathbuf) && len > 0);
        pathbuf[len] = 0;
        if (strchr(pathbuf, '\n')) {
            fputs("link target contains new line, aborting\n", stderr);
            exit(1);
        }
        check(printf("link=%s\n", pathbuf) > 0);
    }
    if (tflag == FTW_F && absolute) {
        check(realpath(fpath, pathbuf) != NULL);
        if (strchr(pathbuf, '\n')) {
            fputs("file path contains new line, aborting\n", stderr);
            exit(1);
        }
        check(printf("source=%s\n", pathbuf) > 0);
    }
    check(puts("") != 0);
    return 0;
}

int
main(int argc, char **argv)
{
    int opt;
    const char *dir = ".";
    errno = 0;

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

   check(chdir(dir) == 0);
   check(nftw(".", printfspec, 20, FTW_PHYS) != -1);
   check(!ferror(stdout));
   return 0;
}
