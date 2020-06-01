# fspec-tools

[![builds.sr.ht status](https://builds.sr.ht/~ach/fspec-tools/commits.svg)](https://builds.sr.ht/~ach/fspec-tools/commits)

Tools to create filesystem images from an fspec filesystem specification.


## Examples


Generate an fspec from the current directory:

```
$ fspec-fromdir

/README.md
type=reg
mode=0644
uid=1000
gid=100

...
```

Generate an fspec from another directory:

```
$ fspec-fromdir -C / dev

/dev/null
type=chardev
devnum=259
mode=0666
uid=0
gid=0

...

```

Create a tarball:

```
$ fspec-tar < fs.fspec | gzip > out.tar.gz
```

Create an initramfs:

```
$ fspec-cpio < fs.fspec | gzip > out.cpio.gz
```

Extract a tarball (no root needed):

```
$ gzip -d < fs.tar.gz | fspec-fromarchive -d ./data > fs.fspec
```

## Format

```
fspec     ::= record ("\n" record)* "\n"?
record    ::= name attribute+
name      ::= "/" [^\n]* "\n"
attribute ::= [a-zA-Z0-9_]+ "=" [^\n]+ "\n"
```

An fspec file consists of a sequence of records separated by blank
lines. Each record describes a node in the filesystem.  The first
line in a record is the file path, and it is followed by one or
more attribute lines. Since newline characters are used to separate
the parts of a record, they are banned from file names and attribute
values.

The following attributes are available:

- **type** (required): Type of filesystem node.
  * `reg`: Regular file
  * `sym`: Symbolic link
  * `dir`: Directory
  * `fifo`: FIFO special file.
  * `chardev`: Character device node
  * `blockdev`: Block device node
- **mode**: File permission bits, represented as exactly four
  octal digits. If not present, the default depends on `type`:
  * `reg`: 0644
  * `sym`: 0777
  * `dir`: 0755
  * `fifo`: 0644
  * `chardev`: 0600
  * `blockdev`: 0600
- **uid**: File owner's ID. If not present, 0 is used.
- **gid**: File group's ID. If not present, 0 is used.
- **source**: Local path to file containing the contents to be
  added to the filesystem image. Only valid for `type=reg`. If not
  present, the entry name is used, relative to the directory
  containing the manifest.
- **target**: Symlink target. Only valid and required for `type=sym`.
- **devnum**: Device number. Only valid and required for
  `type=chardev` or `type=blockdev`.
