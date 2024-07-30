#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

// void printEven(int n) {
//   for (int i = 0; i <= n; i++) {
//     if (i % 2 == 0) {
//       printf("%d\n", i);
//       sleep(5);
//     }
//   }
// }

void handleSighup(int sig) {
  printf("Ouch!\n");
  fflush(stdout);
}

void handleSigint(int sig) {
  printf("Yeah!\n");
  fflush(stdout);
}

int main(int argc, char *argv[]) {
  int n = atoi(argv[1]);

  signal(SIGHUP, handleSighup);
  signal(SIGINT, handleSigint);

  // printEven(n);

  for (int i = 0; i <= n; i++) {
    if (i % 2 == 0) {
      printf("%d\n", i);
      sleep(5);
    }
  }

  // while (!stop) {
  //   pause();
  // }

  return 0;
}