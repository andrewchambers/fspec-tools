.POSIX:

CFLAGS+=-Wall -Wpedantic
LDLIBS=-l archive

BIN=\
	src/fspec-tar\
	src/fspec-cpio\
	src/fspec-fromdir

.PHONY: all
all: $(BIN)

src/fspec-tar.o: src/fspec-archive.c
	$(CC) $(CFLAGS) -D OUT_FORMAT_TAR=1 -c -o $@ src/fspec-archive.c

src/fspec-cpio.o: src/fspec-archive.c
	$(CC) $(CFLAGS) -D OUT_FORMAT_CPIO=1 -c -o $@ src/fspec-archive.c

src/fspec-fromdir.o: src/fspec-fromdir.c
	$(CC) $(CFLAGS) -c -o $@ src/fspec-fromdir.c

.o:
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

.PHONY: test
test: $(BIN)
	./test/run-all

.PHONY: clean
clean:
	rm -f $(BIN) src/fspec-tar.o src/fspec-cpio.o src/fspec-fromdir.o
