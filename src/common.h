struct archive;
void fspec_archive(struct archive *a, char *input);
void *reallocarray(void *p, size_t n, size_t m);
noreturn void fatal(const char *, ...);
void parse(FILE *file, void (*fspec)(char *, size_t));