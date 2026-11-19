/*
 * majority_vote.c
 * CS 4420 Assignment 06: Majority Voting with pipes
 *
 */

#include <stdio.h>     /* printf, scanf, fprintf, perror */
#include <stdlib.h>    /* exit, rand, srand */
#include <sys/types.h> /* pid_t */
#include <sys/wait.h>  /* wait */
#include <time.h>      /* time */
#include <unistd.h>    /* pipe, fork, read, write, close, getpid */

int main(void) {
  int n;          /* number of voters (child processes) */
  int max_rounds; /* how many rounds of voting to run */

  printf("Please enter the number of voters followed by a space then the max "
         "rounds: ");

  scanf("%d", &n);
  // re-prompt so a typo doesn't force the user to restart the program
  while (n <= 2) {
    fprintf(stderr, "Error: Need to enter number greater than 2..\n");
    scanf("%d", &n);
  }

  scanf("%d", &max_rounds);
  while (max_rounds < 1) {
    fprintf(stderr,
            "Error: Max_rounds must be greater than 0.. please retry:\n");
    scanf("%d", &max_rounds);
  } /* fd[i][0] = read end of pipe i, fd[i][1] = write end of pipe i */

  int fd[n][2];

  /* ===== Step 2 (1st for loop): judge creates N pipes ===== */
  for (int i = 0; i < n; i++) {
    if (pipe(fd[i]) == -1) {
      perror("Pipe failed");
      return 1;
    }
  }

  int voter_id = -1; /* -1 = judge; 0..n-1 = which voter I am */

  /* ===== Step 3 (2nd for loop): judge forks N voters ===== */
  for (int i = 0; i < n; i++) {
    pid_t pid = fork();
    if (pid == -1) {
      perror("Fork failed");
      return 1;
    }
    if (pid == 0) {
      /* child: remember my voter id (= loop index = pipe id) and break
         out so children never call fork() themselves */
      voter_id = i;
      break;
    }
    /* parent (judge) keeps looping to create the next voter */
  }

  /* ===== Step 3.5: each process closes pipe ends it won't use ===== */
  if (voter_id == -1) {
    /* judge only reads, so close every write end */
    for (int i = 0; i < n; i++) {
      close(fd[i][1]);
    }
  } else {
    /* voter only writes to its own pipe, so close everything else */
    for (int i = 0; i < n; i++) {
      close(fd[i][0]);
      if (i != voter_id) {
        close(fd[i][1]);
      }
    }
    /* seed each voter differently (mix in the PID) so they don't all
       start in the same second and cast identical votes */
    srand(time(NULL) ^ getpid());
  }

  /* ===== Step 4 (3rd for loop): voting rounds ===== */
  for (int round = 1; round <= max_rounds; round++) {
    if (voter_id == -1) {
      /* JUDGE: 4th for loop, read one vote per pipe, decide */
      int yes = 0, no = 0;
      for (int i = 0; i < n; i++) {
        int vote;
        read(fd[i][0], &vote, sizeof(vote)); /* blocks until voter i votes */
        if (vote == 1) {
          yes++;
        } else {
          no++;
        }
      }
      if (yes > no) {
        printf("Round %d: Judge: %d yes, %d no -> final decision: YES\n", round,
               yes, no);
      } else if (no > yes) {
        printf("Round %d: Judge: %d yes, %d no -> final decision: NO\n", round,
               yes, no);
      } else {
        printf("Round %d: Judge: %d yes, %d no -> final decision: TIE\n", round,
               yes, no);
      }
    } else {
      /* VOTER: pick a vote, print it, write it to my pipe (no loop) */
      int vote = rand() % 2; /* 1 = yes, 0 = no */
      printf("Round %d: Voter %d votes %s\n", round, voter_id,
             vote == 1 ? "yes" : "no");
      write(fd[voter_id][1], &vote, sizeof(vote));
    }
  }

  /* ===== Step 5: clean shutdown ===== */
  if (voter_id == -1) {
    /* judge: close read ends, then wait for every voter to exit */
    for (int i = 0; i < n; i++) {
      close(fd[i][0]);
    }
    for (int i = 0; i < n; i++) {
      wait(NULL);
    }
  } else {
    /* voter: close my write end and exit */
    close(fd[voter_id][1]);
    exit(0);
  }

  return 0;
}
