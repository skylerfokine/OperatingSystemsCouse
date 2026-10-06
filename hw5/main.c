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

/* ===================================================================
 * Loading the input file
 * =================================================================== */

/* Reads the input file into tasks[]. The first line is the number of
 * tasks, then one "pid arrival burst" line per task.
 * Returns the number of tasks read, or -1 on any error. */
int load_tasks(const char *path) {
  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
    perror(path);
    return -1;
  }

  /* fscanf returns how many values it read, so anything other than 1
   * means the count line is bad. The count must also fit in tasks[]. */
  int n;
  if (fscanf(fp, "%d", &n) != 1 || n < 1 || n > MAX_TASKS) {
    fprintf(stderr, "Error: first line must be a task count from 1 to %d\n",
            MAX_TASKS);
    fclose(fp);
    return -1;
  }

  /* Read each task's three fields. The & passes each field's address so
   * fscanf can write into it. */
  for (int i = 0; i < n; i++) {
    if (fscanf(fp, "%d %d %d", &tasks[i].pid, &tasks[i].arrival,
               &tasks[i].burst) != 3) {
      fprintf(stderr, "Error: could not read task line %d\n", i + 1);
      fclose(fp);
      return -1;
    }

    /* Simulator bookkeeping: remaining counts down from the full burst,
     * and -1 marks start/end as "hasn't happened yet". */
    tasks[i].remaining = tasks[i].burst;
    tasks[i].start = -1;
    tasks[i].end = -1;
  }

  fclose(fp);
  return n;
}

/* ===================================================================
 * Helpers shared by all three algorithms
 * =================================================================== */

/* Adds a task index to the back of the ready queue. Back-of-the-line
 * order is what gives FCFS and RR their first-come, first-served behavior. */
void enqueue(int i) { ready[ready_count++] = i; }

/* Removes the entry at position k of the ready queue and returns the task
 * index it held. Everything after k shifts left so the order is kept. */
int remove_at(int k) {
  int i = ready[k];
  for (int j = k; j < ready_count - 1; j++)
    ready[j] = ready[j + 1];
  ready_count--;
  return i;
}

/* Puts every task arriving at time t into the ready queue. Checking in
 * file order means simultaneous arrivals are queued in file order. */
void add_arrivals(int t) {
  for (int i = 0; i < n_tasks; i++) {
    if (tasks[i].arrival == t) {
      enqueue(i);
      printf("Time %d: P%d arrives\n", t, tasks[i].pid);
    }
  }
}

/* Gives the CPU to task i at time t. Records its first start time and
 * opens a new Segment so the RR table can list every run separately. */
void dispatch(int i, int t) {
  if (tasks[i].start == -1)
    tasks[i].start = t;
  segs[n_segs].pid = tasks[i].pid;
  segs[n_segs].start = t;
  segs[n_segs].end = t;
  n_segs++;
  printf("Time %d: P%d starts running\n", t, tasks[i].pid);
}

/* Runs task i for the single time unit starting at t. Returns 1 if that
 * unit finished the task, 0 if it still needs more CPU time. */
int run_one_unit(int i, int t) {
  tasks[i].remaining--;
  segs[n_segs - 1].end = t + 1; /* current segment now covers this slot */

  if (tasks[i].remaining == 0) {
    tasks[i].end = t + 1;
    printf("Time %d: P%d finishes\n", t + 1, tasks[i].pid);
    return 1;
  }
  return 0;
}

/* Prints the idle message once when the CPU first goes idle. The idle flag
 * is passed by pointer so this function can update the caller's copy. */
void report_idle(int t, int *idle) {
  if (!*idle) {
    printf("Time %d: idle\n", t);
    *idle = 1;
  }
}

/* Waiting time = time in the system minus time actually running.
 * This counts every wait, including the separate gaps a task gets in RR. */
int waiting_time(const Task *x) { return x->end - x->arrival - x->burst; }

/* ===================================================================
 * Output
 * =================================================================== */

/* Prints the average waiting time across all tasks. total is a double so
 * the division keeps decimals like 7.75. */
void print_average(void) {
  double total = 0;
  for (int i = 0; i < n_tasks; i++)
    total += waiting_time(&tasks[i]);
  printf("\nAverage Waiting Time: %.2f\n", total / n_tasks);
}

/* FCFS and SJF table: one row per task, in the order they finished
 * (which is the order they ran, since neither algorithm preempts). */
void print_table(const char *name) {
  /* Sort an array of indexes by end time so tasks[] stays untouched. */
  int order[MAX_TASKS];
  for (int i = 0; i < n_tasks; i++)
    order[i] = i;
  for (int i = 1; i < n_tasks; i++) {
    int key = order[i], j = i - 1;
    while (j >= 0 && tasks[order[j]].end > tasks[key].end) {
      order[j + 1] = order[j];
      j--;
    }
    order[j + 1] = key;
  }

  /* %-Nd left-aligns each value in an N-character column. */
  printf("\n%s:\n", name);
  printf("%-5s %-13s %-11s %-9s %-13s %-12s\n", "PID", "Arrival Time",
         "Start Time", "End Time", "Running Time", "Waiting Time");
  for (int o = 0; o < n_tasks; o++) {
    Task *x = &tasks[order[o]];
    printf("%-5d %-13d %-11d %-9d %-13d %-12d\n", x->pid, x->arrival, x->start,
           x->end, x->burst, waiting_time(x));
  }
  print_average();
}

/* RR tables: first every CPU segment in order (a task can appear several
 * times), then one summary row per task in input order. */
void print_rr_table(int quantum) {
  printf("\nRR (Time quantum = %d):\n", quantum);
  printf("%-5s %-11s %-9s %-12s\n", "PID", "Start Time", "End Time",
         "Running Time");
  for (int s = 0; s < n_segs; s++)
    printf("%-5d %-11d %-9d %-12d\n", segs[s].pid, segs[s].start, segs[s].end,
           segs[s].end - segs[s].start);

  printf("\n%-5s %-13s %-13s %-9s %-12s\n", "PID", "Arrival Time",
         "Running Time", "End Time", "Waiting Time");
  for (int i = 0; i < n_tasks; i++)
    printf("%-5d %-13d %-13d %-9d %-12d\n", tasks[i].pid, tasks[i].arrival,
           tasks[i].burst, tasks[i].end, waiting_time(&tasks[i]));
  print_average();
}

/* ===================================================================
 * Scheduling algorithms
 * Each one is a time-driven loop: one pass = one time unit. Every unit
 * it adds arrivals, picks a task if the CPU is free, runs it for one unit,
 * and prints any state change.
 * =================================================================== */

/* First Come First Serve: always run the task at the front of the queue,
 * and let it run until it finishes (non-preemptive). */
void FCFS(void) {
  int t = 0, finished = 0;
  int running = -1; /* index of the task on the CPU, -1 if the CPU is free */
  int idle = 0;

  while (finished < n_tasks) {
    add_arrivals(t);

    /* CPU is free: take whoever has been waiting longest (the front). */
    if (running == -1 && ready_count > 0) {
      running = remove_at(0);
      dispatch(running, t);
      idle = 0;
    }

    /* Run one unit; if the task finishes, the CPU frees up for next unit. */
    if (running != -1) {
      if (run_one_unit(running, t)) {
        finished++;
        running = -1;
      }
    } else {
      report_idle(t, &idle);
    }

    t++;
  }

  print_table("FCFS");
}

/* Shortest Job First (non-preemptive): when the CPU is free, run the ready
 * task with the smallest burst. Once started, it runs to completion. */
void SJF(void) {
  int t = 0, finished = 0;
  int running = -1;
  int idle = 0;

  while (finished < n_tasks) {
    add_arrivals(t);

    if (running == -1 && ready_count > 0) {
      /* Scan the whole queue for the shortest burst. Ties go to the earlier
       * arrival, then the lower pid, so the choice is always predictable. */
      int best = 0;
      for (int k = 1; k < ready_count; k++) {
        Task *a = &tasks[ready[k]];
        Task *b = &tasks[ready[best]];
        if (a->burst < b->burst ||
            (a->burst == b->burst && a->arrival < b->arrival) ||
            (a->burst == b->burst && a->arrival == b->arrival &&
             a->pid < b->pid))
          best = k;
      }
      running = remove_at(best);
      dispatch(running, t);
      idle = 0;
    }

    if (running != -1) {
      if (run_one_unit(running, t)) {
        finished++;
        running = -1;
      }
    } else {
      report_idle(t, &idle);
    }

    t++;
  }

  print_table("SJF");
}

/* Round Robin: run the front task for at most `quantum` units. If it isn't
 * done by then, it goes to the back of the queue and the next task runs. */
void RR(int quantum) {
  int t = 0, finished = 0;
  int running = -1;
  int used = 0;       /* units the running task has used of its quantum */
  int preempted = -1; /* task whose quantum just expired, waiting to requeue */
  int idle = 0;

  while (finished < n_tasks) {
    add_arrivals(t);

    /* A preempted task rejoins the back of the queue AFTER this unit's
     * arrivals, so a task arriving at the same moment gets in line first. */
    if (preempted != -1) {
      enqueue(preempted);
      preempted = -1;
    }

    /* CPU is free: take the front task and give it a fresh quantum. */
    if (running == -1 && ready_count > 0) {
      running = remove_at(0);
      used = 0;
      dispatch(running, t);
      idle = 0;
    }

    if (running != -1) {
      used++;
      if (run_one_unit(running, t)) {
        /* Finished within its quantum. */
        finished++;
        running = -1;
      } else if (used == quantum) {
        /* Quantum used up but not done: give up the CPU and requeue. */
        printf("Time %d: P%d quantum expires\n", t + 1, tasks[running].pid);
        preempted = running;
        running = -1;
      }
    } else {
      report_idle(t, &idle);
    }

    t++;
  }

  print_rr_table(quantum);
}

/* ===================================================================
 * main: check arguments, load tasks, run the chosen algorithm
 * =================================================================== */
int main(int argc, char *argv[]) {

  // need at least the program name, an input file, and an algorithm
  if (argc < 3) {
    fprintf(stderr, "Usage: %s input_file [FCFS|RR|SJF] [time_quantum]\n",
            argv[0]);
    return 1;
  }

  // store the file and algorithm
  const char *file = argv[1];
  const char *algo = argv[2];
  int quantum = 0;

  // RR is the only algorithm that needs a quantum, and it must be positive
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

  // load the data from the file; stop if it was missing or malformed
  n_tasks = load_tasks(file);
  if (n_tasks < 0) {
    return 1;
  }

  // run the algorithm the user asked for
  if (strcmp(algo, "FCFS") == 0) {
    FCFS();
  } else if (strcmp(algo, "SJF") == 0) {
    SJF();
  } else {
    RR(quantum);
  }

  return 0;
}
