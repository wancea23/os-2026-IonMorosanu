# Lab 6. Inside the xv6 File System

Ion Moroșanu, FAF-242. Tree: `~/xv6-riscv` (upstream xv6-riscv). New programs: `user/mystat.c`, `user/find.c`,
added to UPROGS after `$U/_args\` (see `Makefile` in this folder). The full report with screenshots is
`Lab5-6_IonMorosanu_FAF-242.docx` (Labs 5 and 6 combined).

## Part 1. Read what xv6 shows you

```
$ ls
.              1 1 1024
..             1 1 1024
README         2 2 2441
cat            2 3 36728
...
console        3 27 0
$ mkdir a
$ ls a
.              1 28 32
..             1 1 1024
```

1. `kernel/stat.h`: 1 = `T_DIR` (directory), 2 = `T_FILE` (file), 3 = `T_DEVICE` (device).
2. The root has inode 1 (`ROOTINO`). It has no parent, so mkfs makes its `..` point to itself: `.` and `..` are both inode 1.
3. `struct dirent` = `ushort inum` (2 bytes) + `char name[DIRSIZ]` (14 bytes) = 16 bytes. `a` has 32 = 2 x 16 bytes: the entries `.` (its own inode) and `..` (inode 1).

## Part 2. mystat

```
$ mystat README . console
README: file, inode 2, links 1, size 2441 bytes
.: directory, inode 1, links 2, size 1024 bytes
console: device, inode 27, links 1, size 0 bytes
$ ln README r2
$ mystat README r2
README: file, inode 2, links 2, size 2441 bytes
r2: file, inode 2, links 2, size 2441 bytes
$ rm README
$ mystat r2
r2: file, inode 2, links 1, size 2441 bytes
$ cat r2 | wc
48 336 2441
```

(`.` shows 2 links, not 1 as in the handout, because `a` had already been created in Part 1: `create()` in
`kernel/sysfile.c` does `dp->nlink++` for the child's `..` (line 311) and does not count `.` (line 301), so a new
directory in xv6 has 1 link, not 2 as on Linux.)

1. After `ln`: same inode (2) and size (2441) under both names, the link count went from 1 to 2. After `rm`: the name README is gone, the count is back to 1, inode, size and data did not change (`wc` still counts 2441 bytes).
2. `kernel/sysfile.c`, `sys_unlink`, line 244: `ip->nlink--;` (then `iupdate(ip)`). The data blocks are freed later, in `iput()` in `kernel/fs.c` (line 365, `itrunc(ip)`), when the last in-memory reference is dropped and `nlink == 0`. An open file stays readable after its last name is removed.
3. `struct dinode`: `type`, `major`, `minor`, `nlink`, `size`, `addrs[NDIRECT+1]` (12 direct blocks + 1 indirect). 64 bytes, 16 per block. No owner, no mode, no timestamps (so no permissions) and no symlink type.

## Part 3. Measure the limits

```
5:#define BSIZE   1024 // block size
26:#define NDIRECT   12
27:#define NINDIRECT (BSIZE / sizeof(uint))
54:#define DIRSIZ 14
nmeta 47 (boot, super, log blocks 31, inode blocks 13, bitmap blocks 1) blocks 1953 total 2000
$ mkdir averyveryverylongname
$ ls
averyveryveryl 1 29 32
```

1. ls shows `averyveryveryl`: only the first `DIRSIZ` = 14 characters fit in a directory entry, the rest is cut without an error. Lookups also compare only 14 characters, so `mystat averyveryverylongname averyveryveryl` shows the same inode 29. (It also took README's freed slot, the third entry of `/`.)
2. `NINDIRECT` = 1024 / 4 = 256. Largest file = (12 + 256) x 1024 = 268 x 1024 = 274432 bytes (268 KB).
3. Total 2000 blocks: 47 metadata = 1 boot + 1 super + 31 log (header + 30) + 13 inode blocks (200 inodes / 16 + 1) + 1 bitmap; 1953 data blocks.

## Part 4. find

`user/find.c` (based on `user/ls.c`): `find(dir, name)` opens `dir`, checks with `fstat` that it is a directory, and
reads it one `struct dirent` at a time. For each entry it skips free slots (`inum == 0`) and `.` / `..`, builds
`dir/entry` in a 512 byte buffer (the name is copied with `memmove` and ended with a 0, because a 14 character name
has no terminating 0), calls `stat` on it, prints the path if the entry name equals `name` (`strcmp`), and recurses
if it is a directory. The directory is closed before returning, so at most one descriptor per level of depth is open
(16 per process). Optional third argument `-n` prints the inode number next to the path.

Fresh disk (`make clean`, then `make qemu CPUS=1`: the VM has only 2 virtual CPUs, and 3 RISC-V harts on them were
too slow to boot):

```
$ mkdir a
$ mkdir a/b
$ echo x > a/b/r2
$ echo y > a/r2
$ ln cat r2
$ find . r2
./a/b/r2
./a/r2
./r2
$ find a b
a/b
$ find . r2 -n
./a/b/r2 30
./a/r2 31
./r2 3
$ mystat cat r2 a/r2 a/b/r2
cat: file, inode 3, links 2, size 36728 bytes
r2: file, inode 3, links 2, size 36728 bytes
a/r2: file, inode 31, links 1, size 2 bytes
a/b/r2: file, inode 30, links 1, size 2 bytes
$ find nosuch r2
find: cannot open nosuch
$ find .
usage: find <dir> <name> [-n]
```

Stretch: `./r2` has inode 3, the same inode as `cat`, so it is the same file (the hard link made by `ln cat r2`).
`./a/r2` (31) and `./a/b/r2` (30) are separate small files.
