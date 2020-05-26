set -xe
CC=${CC:-gcc}
CFLAGS=${CFLAGS:--g -O2}
LDFLAGS=${LDFLAGS:--larchive}
$CC src/fspec-archive.c -DOUT_FORMAT_TAR=1 $CFLAGS $LDFLAGS -o fspec-tar
$CC src/fspec-archive.c -DOUT_FORMAT_CPIO=1 $CFLAGS $LDFLAGS -o fspec-cpio
