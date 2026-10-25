# Lab 2. Meet the OS You Will Build

Ion Moroșanu, FAF-242. xv6 cloned into `~/xv6-riscv` (commit 06aad25, 26 Sep 2026), built with riscv64-linux-gnu-gcc 15.2.0
and QEMU 10.2.1 on the Ubuntu 26.04 VM. The full report with the screenshots is `Lab2_IonMorosanu_FAF-242.docx` in this folder.

Files here: `sleep.c` (the program, `user/sleep.c`) and `Makefile` (UPROGS with `$U/_sleep\` under `$U/_echo\`).

## Part 2. Use xv6 as the Unix it is

1. Three programs xv6 ships with: `cat`, `grep`, `wc` (also `echo`, `ls`, `mkdir`, `rm`, `kill`, the shell `sh`; 20 in total).
2. A pipe needs **processes** (the shell runs `ls` and `grep` at the same time in two processes made with fork + exec) and
   **communication between processes** (the `pipe` system call: a kernel buffer with a write end and a read end, which the
   shell puts in place of the stdout of `ls` and the stdin of `grep` with close + dup, `user/sh.c` lines 103-114).
3. The xv6 shell has the same basic syntax as bash (commands, arguments, `|`, `<`, `>`, `;`, `&`), but it is one file of
   499 lines without history, Tab completion, variables, wildcards or scripts.

## Part 3. Read the source

1. `user/cat.c` uses `open` (open a file, get a descriptor), `read` (copy up to 512 bytes into `buf`), `write` (copy them
   to descriptor 1, stdout), `close` (release the descriptor) and `exit` (end the process with a status). The error
   messages with `fprintf(2, ...)` also end up as `write` on descriptor 2.
2. `sys_read` is in `kernel/sysfile.c`, line 69 (`sys_write` on line 83).
3. `kernel/` runs in the privileged (supervisor) mode and controls the hardware, the memory and all processes; `user/` runs
   in user mode, each program in its own address space, and can reach anything only by asking the kernel with system calls.

## Part 4. sleep

The handout calls `sleep(atoi(argv[1]))`, but the current xv6 renamed that system call to **`pause`**
(`int pause(int);` in `user/user.h`), so the first build failed with `implicit declaration of function 'sleep'`
(xv6 builds with `-Werror`). The program calls `pause(atoi(argv[1]))` and keeps the name `sleep`.

```
$ sleep 10
$ sleep
usage: sleep <ticks>
```

`sleep 10` pauses about one second (one tick is about 0.1 s) and returns to the prompt; without an argument it prints the
usage message and exits with 1.

Checked again on 25 Oct 2026 on the newest xv6 (commit f780d78, 15 Oct 2026): this `sleep.c` and this `Makefile`, copied into
a fresh clone, build without errors.
