#define _XOPEN_SOURCE 500
#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
    struct stat sb;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pathname>\n", argv[0]);
        exit(1);
    }

    if (stat(argv[1], &sb) == -1) 
        err(1, "stat failed");

    printf("%04o\n", sb.st_mode & ~S_IFMT);
    
    if (fflush(stdout) != 0 || ferror(stdout))
        err(1, "io error");
    
    return 0;
}