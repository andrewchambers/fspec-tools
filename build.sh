set -xe

CC=${CC:-gcc}
CFLAGS=${CFLAGS:--g -O2}
LDFLAGS=${LDFLAGS:--larchive}
peg parse.peg -o parse.inc.c

$CC rawar.c -DOUT_FORMAT_TAR=1 $CFLAGS $LDFLAGS -o rawtar
$CC rawar.c -DOUT_FORMAT_CPIO=1 $CFLAGS $LDFLAGS -o rawcpio