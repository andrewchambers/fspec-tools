# rawtar

Create a tarball from a textual tar description.

## Examples
```
$ cat manifest
foo
type=reg
uid=123
gid=123
source=./README.md

bar
type=sym
link=bar

$ rawtar < manifest > out.tar
```
