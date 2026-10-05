#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#define MAX_LINE 80
/** setup() reads in the next command line, separating it into
distinct tokens using whitespace as delimiters.
setup() modifies the args parameter so that it holds pointers
to the null-terminated strings that are the tokens in the most
recent user command line as well as a NULL pointer, indicating
the end of the argument list, which comes after the string
pointers that have been assigned to args. */
void setup(char inputBuffer[], char *args[], int *background) {
  int length, /* # of characters in the command line */
      i,      /* loop index for accessing inputBuffer array */
      start,  /* index where beginning of next command parameter is */
      ct;     /* index of where to place the next parameter into args[] */

  ct = 0;

  /* read what the user enters on the command line */
  length = read(STDIN_FILENO, inputBuffer, MAX_LINE);
  start = -1;
  if (length == 0) {
    exit(0); /* ^d was entered, end of user command stream */
  }
  if (length < 0) {
    perror("error reading the command");
    exit(-1); /* terminate with error code of -1 */
  }

  /* examine every character in the inputBuffer */
  for (i = 0; i < length; i++) {
    switch (inputBuffer[i]) {
    case ' ':
    case '\t': /* argument separators */
      if (start != -1) {
        args[ct] = &inputBuffer[start]; /* set up pointer */
        ct++;
      }
      inputBuffer[i] = '\0'; /* add a null char; make a C string */
      start = -1;
      break;

    case '\n': /* should be the final char examined */
      if (start != -1) {
        args[ct] = &inputBuffer[start];
        ct++;
      }
      inputBuffer[i] = '\0';
      args[ct] = NULL; /* no more arguments to this command */
      break;

    case '&':
      *background = 1;
      inputBuffer[i] = '\0';
      break;

    default: /* some other character */
      if (start == -1) {
        start = i;
      }
    }
  }
  args[ct] = NULL; /* just in case the input line was > 80 */
}

int main(void) {
  char inputBuffer[MAX_LINE];   /* buffer to hold command entered */
  int background;               /* equals 1 if a command is followed by '&' */
  char *args[MAX_LINE / 2 + 1]; /* command line arguments */
  while (1) {
    background = 0;
    printf(" COMMAND->");
    fflush(stdout);
    /* setup() calls exit() when Control-D is entered */
    setup(inputBuffer, args, &background);
    /* fork() duplicates this process. Execution continues from this
      same point in BOTH processes, but the return value differs,
      which is how each one learns who it is. */
    pid_t pid = fork();
    int status; /* filled in by waitpid(); holds how the child ended */

    if (pid < 0) {
      /* Negative means no child was created at all. Do not wait for
         a process that does not exist; just re-prompt. */
      perror("fork");

    } else if (pid == 0) {
      /* Zero means we are the child. Replace this process with
         the requested program. execvp() searches PATH, and args is
         already NULL-terminated by setup(). args[0] is passed twice:
         once as the program to locate, once as argv[0] of the new
         program. */
      execvp(args[0], args);

      /* Only reachable if execvp() failed, since a successful call
         never returns. Report which command could not be run and
         terminate this child. Without the exit() the failed child
         would fall back into the while loop*/
      perror(args[0]);
      exit(1);

    } else {
      /* Positive means we are the parent and pid is our child. */
      if (background == 0) {
        /* Foreground command: block until this specific child ends,
           so the next prompt is not printed over its output. */
        waitpid(pid, &status, 0);
      }
      /* Background command: skip the wait entirely and loop straight
         back to the prompt so the user can keep working while the
         child runs. Note that these children are never reaped, so
         each one remains a zombie in the process table until the
         shell exits. */
    }
  }
}
