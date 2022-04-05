// archive.c
struct archive;
void fspec_archive(struct archive *a, char *input);
// util.c
void *reallocarray(void *p, size_t n, size_t m);
noreturn void fatal(const char *, ...);
char *xstrdup(const char *s);
// parse.c
void parse(FILE *file, void (*fspec)(char *, size_t));