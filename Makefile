.POSIX:

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

LDLIBS=-l archive

-include config.mk

CFLAGS+=-Wall -Wpedantic

BIN=\
	src/fspec-archive\
	src/fspec-fromarchive\
	src/fspec-fromdir\
	src/fspec-filldirs

OBJ=\
	src/fspec-archive.o\
	src/fspec-fromarchive.o\
	src/fspec-fromdir.o

CLEAN=\
	src/fspec-tar\
	src/fspec-cpio\
	src/fspec-archive\
	src/fspec-fromarchive\
	src/fspec-fromdir\
	$(OBJ)

.PHONY: all
all: $(BIN) src/fspec-tar src/fspec-cpio

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.o:
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

src/fspec-tar src/fspec-cpio:
	ln -sf fspec-archive $@

.PHONY: test
test: all
	./test/run-all

.PHONY: install
install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	cp $(BIN) $(DESTDIR)$(BINDIR)
	ln -sf fspec-archive $(DESTDIR)$(BINDIR)/fspec-tar
	ln -sf fspec-archive $(DESTDIR)$(BINDIR)/fspec-cpio

.PHONY: clean
clean:
	rm -f $(CLEAN)
