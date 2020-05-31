.POSIX:

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

CFLAGS+=-Wall -Wpedantic
LDLIBS=-l archive

BIN=\
	src/fspec-tar\
	src/fspec-cpio\
	src/fspec-fromtar\
	src/fspec-fromcpio\
	src/fspec-fromdir\
	src/fspec-filldirs

OBJ=\
	src/fspec-tar.o\
	src/fspec-cpio.o\
	src/fspec-fromtar.o\
	src/fspec-fromcpio.o\
	src/fspec-fromdir.o

CLEAN=\
	src/fspec-tar\
	src/fspec-cpio\
	src/fspec-fromtar\
	src/fspec-fromcpio\
	src/fspec-fromiso\
	src/fspec-fromdir\
	$(OBJ)

.PHONY: all
all: $(BIN)

src/fspec-tar.o: src/fspec-archive.c
	$(CC) $(CFLAGS) -D OUT_FORMAT_TAR=1 -c -o $@ src/fspec-archive.c

src/fspec-cpio.o: src/fspec-archive.c
	$(CC) $(CFLAGS) -D OUT_FORMAT_CPIO=1 -c -o $@ src/fspec-archive.c

src/fspec-fromtar.o: src/fspec-fromarchive.c
	$(CC) $(CFLAGS) -D IN_FORMAT_TAR=1 -c -o $@ src/fspec-fromarchive.c

src/fspec-fromcpio.o: src/fspec-fromarchive.c
	$(CC) $(CFLAGS) -D IN_FORMAT_CPIO=1 -c -o $@ src/fspec-fromarchive.c

src/fspec-fromdir.o: src/fspec-fromdir.c
	$(CC) $(CFLAGS) -c -o $@ src/fspec-fromdir.c

.o:
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

.PHONY: test
test: $(BIN)
	./test/run-all

.PHONY: install
install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	cp $(BIN) $(DESTDIR)$(BINDIR)

.PHONY: clean
clean:
	rm -f $(CLEAN)
