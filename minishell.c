/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linix/unix/minix command line interpreter
 --------------------------------------------------------------------
   File			: minishell.c
   Compiler/System	: gcc/linux

********************************************************************/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>


#define NV 20  /* max number of command tokens */
#define NL 100 /* input buffer size */
char line[NL]; /* command input buffer */

typedef struct bgProcess {
  int number;
  pid_t pid;
  char command[NL];
  struct bgProcess *next;
} bgProcess;

bgProcess *bg = NULL;
// int bgCount = 0;
int bgNum = 1;

/*
        shell prompt
 */

void prompt(void) {
  // fprintf(stdout, "\n msh> ");
  fflush(stdout);
}

void handleCd(char *path) {
  if (chdir(path) == -1) {
    perror("chdir");
  }
}

void addBgProcess(pid_t pid, char *command[]) {
  bgProcess *newProcess = (bgProcess *)malloc(sizeof(bgProcess));
  newProcess->number = bgNum++;
  newProcess->pid = pid;
  // snprintf(newProcess->command, NL, "%s", command);

  newProcess->command[0] = '\0'; // Initialize the command string
  for (int i = 0; command[i] != NULL; i++) {
    strncat(newProcess->command, command[i], NL - strlen(newProcess->command) - 1);
    if (command[i + 1] != NULL) {
      strncat(newProcess->command, " ", NL - strlen(newProcess->command) - 1);
    }
  }
  // strncpy(newProcess->command, command, NL);
  newProcess->next = bg;
  bg = newProcess;
  printf("[%d] %d\n", newProcess->number,newProcess->pid);
}

void removeBgProcess(pid_t pid) {
  bgProcess *prev = NULL, *curr = bg;

  while (curr != NULL) {
    if (curr->pid == pid) {
      if (prev == NULL) {
        bg = curr->next;
      } else {
        prev->next = curr->next;
      }
      printf("[%d]+ Done %s\n", curr->number, curr->command);
      free(curr);
      return;
    }
    prev = curr;
    curr = curr->next;
  }
}

void handleBgProcess() {
  int status;
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    removeBgProcess(pid);
  }
}

// void printMessages() {
//   bgProcess *prev = NULL, *curr = bg, *tmp;

//   while (curr != NULL) {
//     int status;
//     pid_t result = waitpid(curr->pid, &status, WNOHANG);
//     if (result == 0) {
//       prev = curr;
//       curr = curr->next;
//     } else if (result == curr->pid) {
//       printf("[%d]+ Done  %s\n", curr->number, curr->command);
//       if (prev == NULL) {
//         bg = curr->next;
//       } else {
//         prev->next = curr->next;
//       }
//       tmp = curr;
//       curr = curr->next;
//       free(tmp);
//     } else {
//       perror("waitpid");
//       break;
//     }
//   }
// }

int main(int argk, char *argv[], char *envp[])
/* argk - number of arguments */
/* argv - argument vector from command line */
/* envp - environment pointer */

{
  int frkRtnVal; /* value returned by fork sys call */
  // int wpid;            /* value returned by wait */
  char *v[NV];         /* array of pointers to command line tokens */
  char *sep = " \t\n"; /* command line token separators    */
  int i;               /* parse index */

  /* prompt for and process one command line at a time  */

  while (1) { /* do Forever */
    prompt();
    if (fgets(line, NL, stdin) == NULL) {
      if (feof(stdin)) {
        // fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(),
        // feof(stdin), ferror(stdin));
        exit(0);
      }
      continue;
    }

    if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
      continue; /* to prompt */

    v[0] = strtok(line, sep);
    for (i = 1; i < NV; i++) {
      v[i] = strtok(NULL, sep);
      if (v[i] == NULL) break;
    }

    if (strcmp(v[0], "cd") == 0) {
      handleCd(v[1]);
      // printMessages();
      continue;
    }

    int isBg = (v[i - 1] && strcmp(v[i - 1], "&") == 0);
    if (isBg) {
      v[i - 1] = NULL;
    }
    /* assert i is number of tokens + 1 */

    /* fork a child process to exec the command in v[0] */

    switch (frkRtnVal = fork()) {
      case -1: /* fork returns error to parent process */
      
        perror("fork");
        break;
      
      case 0: /* code executed only by child process */
      
        if (execvp(v[0], v) == -1) {
          perror("execvp");
          exit(errno);
        }
        break;
      
      default: /* code executed only by parent process */
      
        if (isBg) {
          
        //   char command[NL] = "";
        //   for (int j = 0; j < i - 1; j++) {
        //     strcat(command, v[j]);
        //     if (j < i - 2) {
        //       strcat(command, " ");
        //     }
        //   }
        //   addBgProcess(frkRtnVal, command);
        // } else {
        //   if (waitpid(frkRtnVal, NULL, 0) == -1) {
        //     perror("waitpid");
        //   }
          addBgProcess(frkRtnVal, v);
        } else {
          if (waitpid(frkRtnVal, NULL, 0) == -1) {
            perror("waitpid");
          }
        }
        break;
      } /* switch */
    handleBgProcess();
  } /* main */
}