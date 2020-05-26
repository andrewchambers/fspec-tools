# fspec-tools

Tools to create filesystem images from an fspec filesystem specification.

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

$ fspec-mktar < fs.fspec > out.tar
$ fspec-mkcpio < fs.fspec > out.cpio
```
