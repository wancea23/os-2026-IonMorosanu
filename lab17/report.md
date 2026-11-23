# Lab 17: Threads Share Memory, and Race

Ion Moroșanu, FAF-242. Programs: fire.c (two threads, as in the handout) and fire4.c (four threads, Part 2).
Built with `gcc -Wall -Wextra -pthread`, no -O2, on Ubuntu 26.04 (gcc 15.2.0, 2 virtual processors). The outputs below are copied from my terminal. The full report with screenshots is `Lab17_IonMorosanu_FAF-242.docx`.

## Part 1: shared and racing

```
$ ./fire        (three runs)
thread 2 sees counter = 0
thread 1 sees counter = 0
expected: 2000000
counter:  1272324   (lost updates: the race)
thread 1 sees counter = 0
thread 2 sees counter = 130722
expected: 2000000
counter:  1662432   (lost updates: the race)
thread 1 sees counter = 0
thread 2 sees counter = 4012
expected: 2000000
counter:  1631358   (lost updates: the race)
```

Eight more runs (thread 2 and the result): 446416 / 1935194, 5896 / 1423970, 1000000 / 2000000, 0 / 1801830, 479882 / 2000000, 219840 / 2000000, 431638 / 1589395, 322971 / 1609194. Three of the eight were exactly 2000000, so the result is not always less than two million on this machine. The loss needs the two loops to run at the same time: in the run where thread 2 saw 1000000, thread 1 had already finished everything. When the output goes to a pipe instead of the terminal, all 30 runs were wrong.

The three steps of `counter++` (objdump of `work`, addresses 11ec to 11f7): `mov rax, [counter]`, `add rax, 1`, `mov [counter], rax`.

1. Thread 2 prints a counter that is not zero (130722, 4012, 446416 and others), and that value was written by thread 1. So the two threads use the same memory: one variable, one address, and what one writes the other sees at once, with no copy. Only the stack and the registers of a thread are its own.
2. `counter++` is load, add, store. If counter is 5, thread 1 loads 5, thread 2 loads 5 before thread 1 stores, both store 6, and one of the two additions is lost. It happens many times in two million additions, so the result is below two million, and it is different at every run because the overlap of the two threads depends on the scheduler.

## Part 2: four threads

Three runs of `./fire4`, the line `counter:` of each (every run printed `expected: 4000000` before it):

```
counter:  2173480   (lost updates: the race)
counter:  3298953   (lost updates: the race)
counter:  4000000   (lost updates: the race)
```

The third run is exact: the four loops did not overlap enough in that run, so nothing was lost, although the program prints the same text.

Thirty runs of each program with the output sent to a pipe:

```
fire (2 threads): 30 runs, 30 wrong, average lost 617654 of 2000000 (30.9%), worst 936889
fire4 (4 threads): 30 runs, 30 wrong, average lost 1609882 of 4000000 (40.2%), worst 2906164
```

The loss gets bigger: about 2.6 times more updates lost and a larger share (40.2% against 30.9%), because more threads are between their load and their store at the same time.

3. One idea stops the lost updates: mutual exclusion, so that only one thread at a time is inside `counter++` and its three steps (load, add, store) become one step that the others cannot cut, for example with a lock around the line (or an atomic addition).
