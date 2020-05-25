# rawtar

Create a tarball from a textual tar description.

## Examples
```
$ cat manifest
foo
type=reg
uid=123
gid=123
source="./README.md"

bar
type=sym
link=bar

$ rawtar < manifest > out.tar
```

/bin
type=dir
mode=755
uid=0

/bin/ksh
type=reg
mode=644
source=path/to/content

/bin/sh
type=sym
link=ksh

/share
type=dir
mode=755
