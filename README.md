# rawtar

Create a tarball from a textual tar description.

## Examples
```
$ cat manifest
type=reg uid=123 gid=123 path=foo content="./README.md"
type=symlink path=bar target="/baz"
$ rawtar < manifest > out.tar
```