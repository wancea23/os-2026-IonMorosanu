# Lab 9. Watching System Calls with strace

Ion Moroșanu, FAF-242. Ubuntu 26.04 VM (kernel 7.0.0-38), folder `~/os-lab9`, strace 6.19, gcc 15.2.0.
On this system `ls` is uutils coreutils 0.8.0 (the Rust rewrite). The full report with the screenshots is
`Lab9_IonMorosanu_FAF-242.docx` in this folder.

Files: `syscalls.c` (the program of Part 2, built with `gcc -Wall -Wextra`, no warnings).

## Part 1. The calls of a real command

`strace -f -e trace=openat,read,write,close ls -l` starts with the dynamic loader opening `/etc/ld.so.cache` and
then the libraries (`libselinux.so.1`, `libgcc_s.so.1`, `libm.so.6`, `libc.so.6`, `libpcre2-8.so.0`), each library as
`openat = 3`, `read(3, "\177ELF...", 832)`, `close(3)`. Then ls reads the directory, walks the folders of
`/usr/share/zoneinfo` and reads `Etc/UTC`, the local time zone it needs for the dates, and at the end prints the whole
listing at once:

```
openat(AT_FDCWD, "/usr/share/zoneinfo/Etc/UTC", O_RDONLY|O_CLOEXEC) = 4
read(4, "TZif2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"..., 114) = 114
read(4, "", 32)                         = 0
close(4)                                = 0
close(3)                                = 0
write(1, "total 4\n-rw-r--r-- 1 wancea wanc"..., 64total 4
-rw-r--r-- 1 wancea wancea 1054 Nov  8 20:54 syscalls.c
) = 64
+++ exited with 0 +++
```

How many calls: `strace -f -o /tmp/ls_all.txt ls -l` wrote 261 lines, one of them `+++ exited with 0 +++`, so one
`ls -l` made **260 system calls**. Only 86 of them are the four of the handout (34 close, 31 openat, 20 read, 1 write).
The most frequent call overall is `getdents64` (34 times), which reads the entries of a directory.

**Observe.** ls prints with the system call `write`, on descriptor **1** (standard output, here the terminal).
`strace -e trace=write ls -l` shows a single `write(1, "total 4\n-rw-r--r-- ...", 64) = 64`: this ls collects the
two output lines in a buffer and hands all 64 bytes to the kernel in one call.

## Part 2. A program that calls the kernel directly

```
$ gcc -Wall -Wextra -o syscalls syscalls.c; echo gcc exit=$?
gcc exit=0
$ ./syscalls
open returned descriptor 3
write sent 14 bytes
read got 14 bytes: hello, kernel
$ cat note.txt; ls -l note.txt
hello, kernel
-rw-r--r-- 1 wancea wancea 14 Nov  8 20:57 note.txt
```

Under `strace -e trace=openat,write,read,close ./syscalls`, after the loader lines (`/etc/ld.so.cache` and
`libc.so.6`), the calls of the program appear in the same order as in the source:

```
openat(AT_FDCWD, "note.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 3
write(1, "open returned descriptor 3\n", 27open returned descriptor 3
) = 27
write(3, "hello, kernel\n", 14)         = 14
write(1, "write sent 14 bytes\n", 20write sent 14 bytes
)   = 20
close(3)                                = 0
openat(AT_FDCWD, "note.txt", O_RDONLY)  = 3
read(3, "hello, kernel\n", 63)          = 14
write(1, "read got 14 bytes: hello, kernel"..., 33read got 14 bytes: hello, kernel
) = 33
close(3)                                = 0
+++ exited with 0 +++
```

Besides the six lines of the handout, every `printf` also shows up as a `write(1, ...)`: printf is a library function,
not a system call, and in the end it calls write on descriptor 1. read asked for 63 bytes (`sizeof(buf) - 1`) and got
14, because the file has only 14.

**Observe.** 0, 1 and 2 are taken before `main` starts: they are standard input, standard output and standard error.
The shell opened them on the terminal and the program inherited them through fork and exec. The kernel always hands out
the lowest free number, so the first file the program opens gets 3. `ls -l /proc/self/fd` shows this:

```
0 -> /dev/pts/0
1 -> /dev/pts/0
2 -> /dev/pts/0
3 -> /proc/5171/fd
```

(3 there is the folder that ls itself opened to list it.) After the first `close(3)` the number 3 is free again, so the
second `openat` also returns 3.

## Part 3. Counting one kind of call

`strace -c ./syscalls` counts the calls instead of printing them: **42 calls** of 18 kinds, among them 8 `mmap`,
4 `openat`, 4 `close`, 4 `write`, 2 `read`, 1 `execve`, and 1 error, `access("/etc/ld.so.preload")` = ENOENT, the
loader checking for a file that does not exist. A full trace (`strace -o`) shows the order: the first 30 calls are the
loader and the start-up code of the C library (execve, mapping libc.so.6, mprotect, getrandom...), and only the last
12 belong to main. They are the six calls of the source, the three writes of printf, and `fstat(1)` plus two `brk`
that printf makes the first time, to see that descriptor 1 is a terminal and to get memory for its buffer. The final
`exit_group(0)` never returns, so `-c` does not count it.

With `-e trace=write` only one kind is counted:

```
$ strace -c -e trace=write ./syscalls
...
100.00    0.002899         724         4           write
$ strace -c -e trace=write ./syscalls > out.txt
...
100.00    0.000077          38         2           write
```

To the terminal the program makes **4** writes: one into note.txt and one for each printf line. When standard output
is a file it makes only **2**: the three printf lines wait in the buffer of the C library and go out together in one
write when the program exits. The C library flushes after every newline when the output is a terminal (line
buffering), and only when the buffer fills up or the program ends when it is a file (full buffering). `cat out.txt`
shows that the three lines are all there, so the output is the same; only the number of trips into the kernel changes.
