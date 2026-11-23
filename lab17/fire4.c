/* fire4.c - Part 2: the same program with four threads instead of two.
 * All four add to the same counter, a million times each, WITHOUT a lock.
 * Build: gcc -Wall -Wextra -pthread -o fire4 fire4.c   Run: ./fire4
 */
#include <stdio.h>
#include <pthread.h>

#define N 1000000
#define T 4
long counter = 0;              /* one variable, shared by all the threads */

void *work(void *arg) {
    long id = (long)arg;
    printf("thread %ld sees counter = %ld\n", id, counter);
    for (int i = 0; i < N; i++)
        counter++;             /* NOT atomic: read, add, write */
    return NULL;
}

int main(void) {
    pthread_t t[T];
    for (long i = 0; i < T; i++)
        pthread_create(&t[i], NULL, work, (void *)(i + 1));
    for (int i = 0; i < T; i++)
        pthread_join(t[i], NULL);
    printf("expected: %d\n", T * N);
    printf("counter:  %ld   (lost updates: the race)\n", counter);
    return 0;
}
