# Lab 7. Three Small Programs: Processes, Redirection, a Pipe

Ion Moroșanu, FAF-242. Ubuntu 26.04 VM, folder `~/os-lab7`, gcc 15.2.0, every program built with `gcc -Wall -Wextra`
(no warnings). The full report with the screenshots is `Lab7_IonMorosanu_FAF-242.docx` in this folder.

Files: `run.c`, `zombie.c`, `redirect.c`, `pipeline.c` (all in their final, working form) and the optional `pipeline3.c`.

## Part 1. fork + exec + wait

```
$ gcc -Wall -Wextra -o run run.c && ./run
parent: my PID is 4305
child:  my PID is 4306, my parent is 4305
total 40
...
parent: child 4306 ended with status 0
```

With `execlp("date", ...)`: `Sun Oct 25 05:00:58 PM UTC 2026`, status 0. With `execlp("nosuchprog", ...)`:

```
parent: my PID is 4337
child:  my PID is 4338, my parent is 4337
exec: No such file or directory
parent: child 4338 ended with status 127
```

1. A successful exec never returns: it replaces the whole program of the child, so the lines after `execlp` no longer exist
   in that process. `execlp` returns (with -1) only when it fails, and only then do `perror("exec")` and `exit(127)` run.
2. The child prints the PID of its parent, the program `run` itself (4305 = the PID the parent printed), from `getppid()`.
3. Status 0 with `ls` (and `date`), status 127 with `nosuchprog`: the value the child passed to `exit` after the failed exec.

After the tests the execlp line was changed back to `ls -l`.

## Part 2. A zombie, on purpose

```
$ ./zombie > /dev/null &
[1] 4364
$ sleep 2; ps -o pid,ppid,stat,cmd
    PID    PPID STAT CMD
   4034    4002 Ss   /usr/bin/bash
   4364    4034 S    ./zombie
   4366    4364 Z    [ls] <defunct>
   4369    4034 R+   ps -o pid,ppid,stat,cmd
```

After the 20 seconds:

```
    PID    PPID STAT CMD
   4034    4002 Ss   /usr/bin/bash
   4376    4034 R+   ps -o pid,ppid,stat,cmd
[1]+  Done                    ./zombie > /dev/null
```

1. `Z`: zombie (defunct). ls has ended, but its parent has not collected its exit status yet, so the kernel keeps its entry.
2. No, the `[ls] <defunct>` line is gone. It was removed by `waitpid(pid, &status, 0);` in zombie.c, which collects the status.
   (The parent of ./zombie is 4034, the bash of the terminal, not 1 as in the handout's sample.)

## Part 3. Redirection: ls -l > out.txt

```
$ gcc -Wall -Wextra -o redirect redirect.c && ./redirect
open gave descriptor 1
done: the listing is in out.txt
$ head -3 out.txt
total 76
-rw-r--r-- 1 wancea wancea     0 Oct 25 17:05 out.txt
-rw-rw-r-- 1 wancea wancea  1013 Oct 25 16:59 pipeline.c
```

1. Descriptor 1: open always returns the smallest free number, and 1 had just been closed (0 and 2 were still open).
2. ls writes to descriptor 1, which now refers to out.txt; exec keeps the open descriptors, so ls inherited the
   redirection without knowing. The message about the descriptor went to stderr (2), so it still reached the screen.
3. Close descriptor 2 before open (then open returns 2), like `2> err.txt`.

## Part 4. A pipe: ls /etc | wc -l

```
$ gcc -Wall -Wextra -o pipeline pipeline.c && ./pipeline
237
$ ls /etc | wc -l
237
```

With the parent's `close(p[1]);` commented out the program hangs (stopped with Ctrl-C). Run in the background:

```
    PID    PPID STAT CMD
   4034    4002 Ss   /usr/bin/bash
   4443    4034 S    ./pipeline
   4446    4443 S    wc -l
   4449    4034 R+   ps -o pid,ppid,stat,cmd
```

1. Yes, both printed 237.
2. `wc` never ends; `ls` has already finished and is gone.
3. wc waits for end of file on the pipe, which comes only when every write end is closed, and the parent still holds `p[1]` open
   (while it is itself waiting for wc).

The line was restored afterwards and `./pipeline` printed 237 again.

## Optional: pipeline3.c

`ls /usr/bin | grep x | wc -l` with two pipes and three children, every child and the parent closing all four ends:

```
$ ./pipeline3
227
$ ls /usr/bin | grep x | wc -l
227
```
