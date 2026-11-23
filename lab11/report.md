# Lab 11: The Race and the Semaphore

Ion Moroșanu, FAF-242. Programs: race.c, mutex.c, sem.c, order.c, parking.c (and atomic.c, optional).
Built with `gcc -Wall -pthread`, no -O2, on Ubuntu 26.04 (gcc 15.2.0, 2 virtual processors). The outputs below are copied from my terminal. The full report with screenshots is `Lab11_IonMorosanu_FAF-242.docx`.

## Part 1: the race

```
$ ./race        (three runs)
counter = 1300618 (expected 2000000)
counter = 2000000 (expected 2000000)
counter = 1000000 (expected 2000000)
```

Ten more runs gave 1627695, 1454785, 1555194, 1496380, 2000000, 1524984, 2000000, 2000000, 1896353, 2000000. Six were wrong and four were exact, so on this machine the result is sometimes 2000000, but nothing guarantees it.

1. The three steps of `counter++` (objdump of `add`, address 11be to 11c9): `mov rax, [counter]` loads the value into a register, `add rax, 1` adds 1 in the register, `mov [counter], rax` stores it back.
2. Counter is 5. Thread A loads 5. Thread B loads 5 before A has stored. A stores 6, B stores 6. Two additions were made but the counter grew by one, so one addition is lost. The result 1000000 is what happens when one thread loads 0, is stopped, the other thread does all its million additions, and the first one then stores 1 and adds its remaining 999999.
3. The result depends on where the scheduler stops the threads and how the two processors overlap, and that is different at every run.

## Part 2: mutex and semaphore

mutex (3 runs) and sem (3 runs) all printed `counter = 2000000 (expected 2000000)`.

Time of one run: race 0.027 s (real), mutex 0.098 s, sem 0.116 s. The sys time was 0.011 s, 0.070 s and 0.140 s.

1. Yes, always 2000000 (six runs, plus three more for the time).
2. With `sem_init(&s, 0, 2)` two threads pass `sem_wait` together, so the semaphore is not a lock any more. In eight runs the result was wrong three times: 1967293, 1984717, 1976976 (the other five were 2000000).
3. Each of the two million additions needs a lock and an unlock, and a thread that finds the lock taken sleeps in the kernel and is woken later. That costs far more than one addition (see the sys time).

## Part 3: a semaphore as a signal

```
$ ./order
B: waiting for the message...
A: message written
B: I read "hello"
```

With `sem_init(&ready, 0, 1)` (copy in /tmp):

```
B: waiting for the message...
B: I read ""
A: message written
```

1. B reads an empty string. The semaphore already holds one unit, so `sem_wait` goes on at once and B prints before A has written the message.
2. A semaphore that starts at 0 means that the event has not happened yet, so everyone must wait until someone posts.

## Part 4: the parking lot

```
$ ./parking
car 1 enters  (inside: 1)
car 2 enters  (inside: 2)
car 3 enters  (inside: 3)
car 1 leaves  (inside: 2)
car 4 enters  (inside: 3)
car 3 leaves  (inside: 2)
car 5 enters  (inside: 3)
car 2 leaves  (inside: 2)
car 6 enters  (inside: 3)
car 4 leaves  (inside: 2)
car 5 leaves  (inside: 1)
car 6 leaves  (inside: 0)
```

1. The largest value of `inside` is 3. The line `sem_init(&places, 0, PLACES)` and the `sem_wait(&places)` at the start of `car()` guarantee it: the fourth car finds 0 and waits until a car calls `sem_post`.
2. With PLACES set to 1 the semaphore is only 1 or 0, so it is a lock (a binary semaphore): one car at a time (the cars entered in the order 2, 3, 4, 5, 6, 1).

Optional, `atomic.c`: `counter = 2000000 (expected 2000000)`, real 0.034 s (the mutex took 0.098 s).
