#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void handleSighup(int sig) {
    printf("Ouch!\n");
    fflush(stdout);
}

void handleSigint(int sig) {
    printf("Yeah!\n");
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);

    if (signal(SIGHUP, handleSighup) == SIG_ERR) {
        perror("Error setting signal handler for SIGHUP");
        return 1;
    }
    if (signal(SIGINT, handleSigint) == SIG_ERR) {
        perror("Error setting signal handler for SIGINT");
        return 1;
    }

    for (int i = 0; i < n; i++) {
        printf("%d\n", 2 * i);
        fflush(stdout);
        sleep(5);
    }

    return 0;
}
