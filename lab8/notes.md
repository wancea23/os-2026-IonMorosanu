# Lab 8. The Same Calls, Inside Your Own OS

Ion Moroșanu, FAF-242. `~/xv6-riscv` updated to the current xv6 (commit f780d78, 15 Oct 2026), run with `make qemu CPUS=1`
(the VM had 2 virtual CPUs). The full report with the screenshots is `Lab8_IonMorosanu_FAF-242.docx` in this folder.

Files: `run.c`, `pingpong.c`, the optional `primes.c` (all `user/`), and `Makefile` with `$U/_run\`, `$U/_pingpong\`,
`$U/_primes\` in UPROGS. `run.c` is in its final form (the child becomes echo).

## Part 1. run, inside xv6

```
$ run
parent: my pid is 3
child: my pid is 4
hello from the child
parent: child 4 ended with status 0
```

With `{ "ls", 0 }` and `exec("ls", args)` the child printed the listing of `/` and the status was 0. With
`exec("nosuchprog", args)`:

```
$ run
parent: my pid is 3
child: my pid is 4
exec failed
parent: child 4 ended with status 1
```

1. xv6: `fork()`, `exec("echo", args)`, `wait(&status)`. Linux (Lab 7): `fork()`, `execlp("ls", "ls", "-l", NULL)`,
   `waitpid(pid, &status, 0)`. The same three steps with the same rules: fork returns 0 in the child and the child's PID in
   the parent, exec replaces the program and returns only on error, wait blocks until the child ends and gives its status.
   The differences come from the C library of Linux: execlp searches PATH and takes a list, waitpid waits for one chosen child
   and needs `WEXITSTATUS`; the xv6 exec takes the name as given and an argv array ending in 0, and wait returns the plain number.
2. `echo` prints "hello from the child". After exec the child runs echo's code; `args` became echo's argv. run itself prints
   only the `parent:` and `child:` lines.
3. The status is just the number the child passes to exit after the failed exec: `exit(1)` in the xv6 run.c, `exit(127)` in
   the Linux run.c.

## Part 2. pingpong

```
$ pingpong
4: received ping
3: received pong
```

1. A pipe goes in one direction, and whoever holds its read end may read from it. With one pipe the parent could read back its
   own byte before the child takes it. Two pipes give each direction its own channel (parent writes only p2c, reads only c2p).
2. `read(c2p[0], &b, 1)` in the parent: read on an empty pipe blocks until the child writes the byte back. (The child's read
   waits for the ping in the same way; `wait(0)` only collects the child at the end.)

## Optional: primes

```
$ primes
prime 2
prime 3
prime 5
prime 7
prime 11
prime 13
prime 17
prime 19
prime 23
prime 29
prime 31
```

## The earlier programs on the current xv6

On the same updated tree `sleep 10`, `sleep` (usage message), `args`, `mystat` and `find` all work, and the folders lab2, lab4,
lab6 and lab8 of this repository, each copied with its own Makefile into a fresh clone of the current xv6, build without errors.
