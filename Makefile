.POSIX:

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

CFLAGS+=-Wall -Wpedantic
LDLIBS=-l archive

BIN=\
	src/fspec-archive\
	src/fspec-fromarchive\
	src/fspec-fromdir\
	src/fspec-filldirs

OBJ=\
	src/fspec-archive.o\
	src/fspec-fromarchive.o\
	src/fspec-fromdir.o

LNK=\
	src/fspec-tar\
	src/fspec-initcpio\
	src/fspec-fromtar\
	src/fspec-frominitcpio

CLEAN=\
	src/fspec-archive\
	src/fspec-fromarchive\
	src/fspec-fromdir\
	$(LNK) $(OBJ)

.PHONY: all
all: $(BIN) $(LNK)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.o:
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

src/fspec-tar src/fspec-initcpio: src/fspec-archive
	ln -f src/fspec-archive $@

src/fspec-fromtar src/fspec-frominitcpio: src/fspec-fromarchive
	ln -f src/fspec-fromarchive $@

.PHONY: test
test: all
	./test/run-all

.PHONY: install
install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	cp $(BIN) $(DESTDIR)$(BINDIR)
	cd $(DESTDIR)$(BINDIR) \
	  && ln -f fspec-archive fspec-tar \
	  && ln -f fspec-archive fspec-initcpio \
	  && ln -f fspec-fromarchive fspec-fromtar \
	  && ln -f fspec-fromarchive fspec-frominitcpio \
	  && rm fspec-archive fspec-fromarchive

.PHONY: clean
clean:
	rm -f $(CLEAN)
