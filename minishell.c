/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linux/unix/minix command line interpreter
 --------------------------------------------------------------------
   File                : minishell.c
   Compiler/System     : gcc/linux

********************************************************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#define NV 20            /* max number of command tokens */
#define NL 100           /* input buffer size */
char line[NL];   /* command input buffer */

int bg_process_count = 0;

void prompt(void) {
    // fprintf(stdout, "\n msh> ");
    fflush(stdout);
}

void handle_cd(char *path) {
    if (chdir(path) == -1) {
        perror("chdir");
    }
}

void handle_background_processes() {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        bg_process_count--;
        printf("[%d]+ Done pid %d\n", bg_process_count + 1, pid);
    }
}

int main(int argc, char *argv[], char *envp[]) {
    int frkRtnVal;    /* value returned by fork sys call */
    // int wpid;         /* value returned by wait */
    char *v[NV];      /* array of pointers to command line tokens */
    char *sep = " \t\n";/* command line token separators */
    int i;

    while (1) {
        prompt();
        if (fgets(line, NL, stdin) == NULL) {
            if (feof(stdin)) {
                // fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(), feof(stdin), ferror(stdin));
                exit(0);
            }
            continue;
        }

        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
            continue;   /* to prompt */

        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL)
                break;
        }

        if (strcmp(v[0], "cd") == 0) {
            handle_cd(v[1]);
            continue;
        }

        int is_background = (v[i-1] && strcmp(v[i-1], "&") == 0);
        if (is_background) {
            v[i-1] = NULL;  // Remove the '&' from arguments
        }

        switch (frkRtnVal = fork()) {
        case -1:    /* fork error */
            perror("fork");
            break;

        case 0:     /* child process */
            if (execvp(v[0], v) == -1) {
                perror("execvp");
                exit(errno);
            }
            break;

        default:    /* parent process */
            if (is_background) {
                bg_process_count++;
                printf("[%d] %d\n", bg_process_count, frkRtnVal);
            } else {
                if (waitpid(frkRtnVal, NULL, 0) == -1) {
                    perror("waitpid");
                }
            }
            break;
        }

        handle_background_processes();
    }
}
