#include "kernel/types.h"
#include "user/user.h"

// one stage of the chain: the first number read is a prime; the numbers
// that it does not divide go on to the next stage, through a new pipe
void
stage(int in) __attribute__((noreturn));

void
stage(int in)
{
  int p, n, next[2];

  if(read(in, &p, sizeof(p)) != sizeof(p)){   // the pipe is empty: last stage
    close(in);
    exit(0);
  }
  printf("prime %d\n", p);

  pipe(next);
  if(fork() == 0){
    close(next[1]);
    close(in);
    stage(next[0]);
  }
  close(next[0]);
  while(read(in, &n, sizeof(n)) == sizeof(n))
    if(n % p != 0)
      write(next[1], &n, sizeof(n));
  close(in);
  close(next[1]);                       // the next stage sees the end of its input
  wait(0);
  exit(0);
}

int
main(int argc, char *argv[])
{
  int p[2];

  pipe(p);
  if(fork() == 0){
    close(p[1]);
    stage(p[0]);
  }
  close(p[0]);
  for(int i = 2; i <= 35; i++)
    write(p[1], &i, sizeof(i));
  close(p[1]);
  wait(0);
  exit(0);
}
