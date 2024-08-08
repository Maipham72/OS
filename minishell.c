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

typedef struct bg_process {
    int number;
    pid_t pid;
    char command[NL];
    struct bg_process *next;
} bg_process;

bg_process *bg_processes = NULL;
int current_bg_number = 1;

void prompt(void) {
    // fprintf(stdout, "\n msh> ");
    fflush(stdout);
}

void handle_cd(char *path) {
    if (chdir(path) == -1) {
        perror("chdir");
    }
}

void add_bg_process(pid_t pid, char *command) {
    bg_process *new_process = (bg_process *)malloc(sizeof(bg_process));
    new_process->number = current_bg_number++;
    new_process->pid = pid;
    snprintf(new_process->command, NL, "%s", command); // Store the command
    new_process->next = bg_processes;
    bg_processes = new_process;
    printf("[%d] %d\n", new_process->number, new_process->pid);
}

void print_done_messages() {
    bg_process *prev = NULL, *curr = bg_processes, *tmp;

    while (curr != NULL) {
        int status;
        pid_t result = waitpid(curr->pid, &status, WNOHANG);
        if (result == 0) {
            // Process has not finished
            prev = curr;
            curr = curr->next;
        } else if (result == curr->pid) {
            // Process has finished
            printf("[%d]+ Done               %s\n", curr->number, curr->command);
            if (prev == NULL) {
                bg_processes = curr->next;
            } else {
                prev->next = curr->next;
            }
            tmp = curr;
            curr = curr->next;
            free(tmp);
        } else {
            // waitpid error
            perror("waitpid");
            break;
        }
    }
}

int main(int argc, char *argv[], char *envp[]) {
    int frkRtnVal;    /* value returned by fork sys call */
    //int wpid;         /* value returned by wait */
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
            print_done_messages();
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
                char command[NL] = "";
                for (int j = 0; j < i - 1; j++) {
                    strcat(command, v[j]);
                    if (j < i - 2) {
                        strcat(command, " ");
                    }
                }
                add_bg_process(frkRtnVal, command);
            } else {
                if (waitpid(frkRtnVal, NULL, 0) == -1) {
                    perror("waitpid");
                }
            }
            break;
        }

        print_done_messages();
    }
}
