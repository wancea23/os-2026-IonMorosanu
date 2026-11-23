# Lab 12: Producer, Consumer and the Other Tools

Ion Moroșanu, FAF-242. Programs: prodcons.c, condvar.c, readers.c, deadlock.c (the fixed version).
Built with `gcc -Wall -pthread` on Ubuntu 26.04 (gcc 15.2.0, 2 virtual processors). The outputs below are copied from my terminal. The full report with screenshots is `Lab12_IonMorosanu_FAF-242.docx`. The experiments (lines exchanged, `if` instead of `while`, no sleeps) were made on copies in /tmp.

## Part 1: producer and consumer

```
$ ./prodcons
put  1   (in buffer: 1)
put  2   (in buffer: 2)
put  3   (in buffer: 3)
put  4   (in buffer: 4)
            take 1   (in buffer: 3)
put  5   (in buffer: 4)
            take 2   (in buffer: 3)
put  6   (in buffer: 4)
            take 3   (in buffer: 3)
put  7   (in buffer: 4)
            take 4   (in buffer: 3)
put  8   (in buffer: 4)
            take 5   (in buffer: 3)
            take 6   (in buffer: 2)
            take 7   (in buffer: 1)
            take 8   (in buffer: 0)
```

1. `sem_wait(&empty)` in the producer. It starts at 4, so after four puts it is 0 and the fifth call blocks until the consumer calls `sem_post(&empty)`.
2. The consumer waits at the start, in `sem_wait(&full)`, because full starts at 0 and the buffer is empty. It goes on after the producer's first `sem_post(&full)`.
3. With the mutex locked before `sem_wait(&empty)` the program printed put 1 to put 4 and then hung (timeout killed it, exit code 124). When the buffer is full the producer holds the mutex and waits for a free slot. The consumer needs that mutex to take an item, so it waits for the producer, and the producer waits for a `sem_post` that only the consumer can make: a deadlock.

## Part 2: the condition variable

```
$ ./condvar
B: not ready, I sleep
A: message written
B: I read "hello"
```

1. The variable `ready` remembers it (it is protected by the mutex). A condition variable remembers nothing, so a signal sent before B waits would be lost.
2. With `if (!ready)` the program printed the same three lines, so here it still works: one reader, one writer, one signal.
3. A wake up does not prove that the condition is true. A spurious wake up is allowed by the standard, and with several waiters the first one to get the mutex can take what was written and set the state back, so the next one wakes and finds `ready == 0`.

## Part 3: readers and writers

```
$ ./readers
writer 1 writes 10  (alone)
reader 2 reads 10   (readers inside: 1)
reader 1 reads 10   (readers inside: 3)
reader 3 reads 10   (readers inside: 3)
writer 2 writes 20  (alone)
```

The count is printed after `rlock` is released, so it can already include readers that came later (1, 3, 3 instead of 1, 2, 3).

1. `if (++readers == 1) sem_wait(&room);` and `if (--readers == 0) sem_post(&room);`. The first reader locks out the writers, the last one lets them in.
2. `++readers` is the same load, add, store as in Lab 11. If two readers changed it together, one change could be lost, the first reader to leave would see 0 and open the room while the other still reads, and a writer could write at that moment.
3. Writer 2 may wait for ever (starvation). While some reader is inside, room stays locked, and if new readers keep arriving the last one never leaves, so `sem_post(&room)` is never called.

## Part 4: deadlock

```
$ timeout 5 ./deadlock; echo "exit code: $?"      (original)
one: I have A, I want B
two: I have B, I want A
exit code: 124
```

After the fix (both threads take A first, then B):

```
one: I have A, I want B
one: I have both
two: I have B, I want A
two: I have both
done
exit code: 0
```

The text of the printf in `two()` was left as in the handout, so `two: I have B, I want A` is not exact after the fix.

1. Thread one holds A and waits for B. Thread two holds B and waits for A. So one waits for two, and two waits for one: a circle.
2. Without the two `sleep(1)` lines the original ended with exit code 0 in all of ten runs, but in a loop of 100 runs with a limit of 3 seconds it hung 24 times (an earlier loop of 2000 runs with a limit of 1 second gave 392 hangs). It does not always hang, and that is worse: the error is rare, tests pass, and the program hangs by chance in real use.
3. A deadlock needs a circle of waiting. If everyone takes the locks in the same order, a thread that waits for B already holds A, and the holder of B can only wait for a lock that is later in the order. Waiting always goes forward, so it can never come back to close a circle.
