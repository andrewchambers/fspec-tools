.POSIX:

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

ARFLAGS=-cr
LDLIBS=-l archive

-include config.mk

CFLAGS+=-Wall -Wpedantic

BIN=\
	src/fspec-tar\
	src/fspec-cpio\
	src/fspec-fromtar\
	src/fspec-fromcpio\
	src/fspec-fromdir

SCRIPT=\
	src/fspec-filldirs

LIBOBJ=\
	src/archive.o\
	src/fromarchive.o

.PHONY: all
all: $(BIN)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

src/libcommon.a: $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $(LIBOBJ)

src/fspec-tar: src/fspec-tar.o src/libcommon.a
	$(CC) $(LDFLAGS) -o $@ src/fspec-tar.o src/libcommon.a $(LDLIBS)

src/fspec-cpio: src/fspec-cpio.o src/libcommon.a
	$(CC) $(LDFLAGS) -o $@ src/fspec-cpio.o src/libcommon.a $(LDLIBS)

src/fspec-fromtar: src/fspec-fromtar.o src/libcommon.a
	$(CC) $(LDFLAGS) -o $@ src/fspec-fromtar.o src/libcommon.a $(LDLIBS)

src/fspec-fromcpio: src/fspec-fromcpio.o src/libcommon.a
	$(CC) $(LDFLAGS) -o $@ src/fspec-fromcpio.o src/libcommon.a $(LDLIBS)

src/fspec-fromdir: src/fspec-fromdir.o
	$(CC) $(LDFLAGS) -o $@ src/fspec-fromdir.o

.PHONY: test
test: all
	./test/run-all

.PHONY: install
install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	cp $(BIN) $(SCRIPT) $(DESTDIR)$(BINDIR)

.PHONY: clean
clean:
	rm -f $(BIN) $(BIN:%=%.o) $(LIBOBJ) libarchive.a
