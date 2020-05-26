.POSIX:

CFLAGS+=-Wall -Wpedantic
LDLIBS=-l archive

BIN=\
	src/fspec-tar\
	src/fspec-cpio

.PHONY: all
all: $(BIN)

src/fspec-tar.o: src/fspec-archive.c
	$(CC) $(CFLAGS) -D OUT_FORMAT_TAR=1 -c -o $@ src/fspec-archive.c

src/fspec-cpio.o: src/fspec-archive.c
	$(CC) $(CFLAGS) -D OUT_FORMAT_CPIO=1 -c -o $@ src/fspec-archive.c

.o:
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

.PHONY: clean
clean:
	rm -f $(BIN) src/fspec-tar.o src/fspec-cpio.o
