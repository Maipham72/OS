#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define NV 20            /* max number of command tokens */
#define NL 100           /* input buffer size */
char line[NL];           /* command input buffer */

/*
   shell prompt
 */
void prompt(void) {
    fprintf(stdout, "\n msh> ");
    fflush(stdout);
}

/*
   signal handler for background processes
 */
void sigchld_handler(int signo) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Background process %d done\n", pid);
    }
}

int main(int argc, char *argv[], char *envp[]) {
    int frkRtnVal;       /* value returned by fork sys call */
    int wpid;            /* value returned by wait */
    char *v[NV];         /* array of pointers to command line tokens */
    char *sep = " \t\n"; /* command line token separators */
    int i;               /* parse index */
    int background;      /* background process flag */

    // Set up signal handler for background process completion
    signal(SIGCHLD, sigchld_handler);

    /* prompt for and process one command line at a time  */
    while (1) { /* do Forever */
        prompt();
        if (fgets(line, NL, stdin) == NULL) {
            if (feof(stdin)) { /* non-zero on EOF  */
                fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(),
                        feof(stdin), ferror(stdin));
                exit(0);
            } else {
                perror("fgets");
                continue;
            }
        }

        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
            continue; /* to prompt */

        // Parse the input line
        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL)
                break;
        }

        // Check for background process
        background = (i > 1 && strcmp(v[i - 1], "&") == 0);
        if (background) {
            v[i - 1] = NULL; // Remove "&" from the command
        }

        // Handle 'cd' command
        if (strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                fprintf(stderr, "msh: expected argument to \"cd\"\n");
            } else if (chdir(v[1]) != 0) {
                perror("msh: cd");
            }
            continue;
        }

        /* fork a child process to exec the command in v[0] */
        switch (frkRtnVal = fork()) {
        case -1: /* fork returns error to parent process */
            perror("fork");
            break;
        case 0: /* code executed only by child process */
            if (execvp(v[0], v) == -1) {
                perror("msh: execvp");
                exit(EXIT_FAILURE); // Exit if execvp fails
            }
            break;
        default: /* code executed only by parent process */
            if (background) {
                printf("Background process %d started\n", frkRtnVal);
            } else {
                wpid = waitpid(frkRtnVal, NULL, 0);
                if (wpid == -1) {
                    perror("waitpid");
                } else {
                    printf("%s done\n", v[0]);
                }
            }
            break;
        } /* switch */
    }     /* while */
} /* main */
