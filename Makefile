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
	src/fspec-iso\
	src/fspec-sort\
	src/fspec-fromdir \
	src/fspec-changeset

LIBOBJ=\
	src/util.o\
	src/parse.o\
	src/archive.o

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

src/fspec-iso: src/fspec-iso.o src/libcommon.a
	$(CC) $(LDFLAGS) -o $@ src/fspec-iso.o src/libcommon.a $(LDLIBS)

src/fspec-fromdir: src/fspec-fromdir.o
	$(CC) $(LDFLAGS) -o $@ src/fspec-fromdir.o src/libcommon.a $(LDLIBS)

src/fspec-sort: src/fspec-sort.o
	$(CC) $(LDFLAGS) -o $@ src/fspec-sort.o src/libcommon.a $(LDLIBS)

src/fspec-changeset: src/fspec-changeset.o
	$(CC) $(LDFLAGS) -o $@ src/fspec-changeset.o src/libcommon.a $(LDLIBS)

.PHONY: check
check: all
	./test/run-all

.PHONY: install
install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	cp $(BIN) $(DESTDIR)$(BINDIR)

.PHONY: clean
clean:
	rm -f $(BIN) $(BIN:%=%.o) $(LIBOBJ) libcommon.a
