#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 20
#define MAX_SEGS 1000

typedef struct {
  int pid;       /* unique process id from the file */
  int arrival;   /* time the task enters the ready queue */
  int burst;     /* total CPU time the task needs */
  int remaining; /* CPU time still needed; counts down as the task runs */
  int start;     /* first time the task got the CPU; -1 means not yet */
  int end;       /* time the task finished */
} Task;

typedef struct {
  int pid;
  int start;
  int end;
} Segment;

/* Global simulator state. These are global so every function can reach them
 * without passing several arrays and counters around.
 *
 * tasks[]  : every task loaded from the input file
 * ready[]  : the ready queue. It stores INDEXES into tasks[] instead of copies,
 *            so updates to a task (like remaining time) happen in one place.
 * segs[]   : history of every dispatch, used to print the RR table */
Task tasks[MAX_TASKS];
int n_tasks = 0;

int ready[MAX_TASKS];
int ready_count = 0;

Segment segs[MAX_SEGS];
int n_segs = 0;

void FCFS(int *arr, int size) {}

void RR(int *arr, int size) {}

void SJF(int *arr, int size) {}

int main(int argc, char *argv[]) {

  // load the data from the file store the data correctly

  if (argc < 3) {
    fprintf(stderr, "Usage: %s input_file [FCFS|RR|SJF] [time_quantum]\n",
            argv[0]);
    return 1;
  }

  // store the file and algorithm
  const char *file = argv[1];
  const char *algo = argv[2];
  int quantum = 0;

  if (strcmp(algo, "RR") == 0) {
    if (argc < 4) {
      fprintf(stderr, "Error: RR requires a time quantum\n");
      return 1;
    }
    quantum = atoi(argv[3]);
    if (quantum <= 0) {
      fprintf(stderr, "Error: time quantum must be a positive integer\n");
      return 1;
    }
  } else if (strcmp(algo, "FCFS") != 0 && strcmp(algo, "SJF") != 0) {
    fprintf(stderr, "Error: unknown algorithm '%s' (use FCFS, RR, or SJF)\n",
            algo);
    return 1;
  }

  // number of processes
  n_tasks = load_tasks(file);
  if (n_tasks < 0) {
    return 1;
  }
}
