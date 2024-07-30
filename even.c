#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

volatile sig_atomic_t stop = 0;

void printEven(int n) {
  for (int i = 0; i <= n; i++) {
    if (i % 2 == 0) {
      printf("%d\n", i);
      sleep(5);
    }
  }
}

void handleSighup(int sig) {
  printf("Ouch!\n");
}

void handleSigint(int sig) {
  printf("Yeah!\n");
  stop = 1;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <number>\n", argv[0]);
    return 1;
  }

  int n = atoi(argv[1]);

  signal(SIGHUP, handleSighup);
  signal(SIGINT, handleSigint);

  printEven(n);
  
  for (int i = 0; i < 30; i++) {
    if (stop) break;
    sleep(1);
  }

  // while (!stop) {
  //   pause();
  // }

  return 0;
}