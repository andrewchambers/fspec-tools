set -xe
peg parse.peg -o parse.inc.c
${CC:-gcc} rawtar.c ${CFLAGS:--g -O2} ${LDFLAGS:--larchive} -o rawtar