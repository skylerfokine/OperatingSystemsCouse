#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int segment_id;            // identifier of the shared memory segment
  unsigned short mode;       // permissions of the segment
  struct shmid_ds shmbuffer; // struct the kernel fills in with segment info

  // Step 1: create a new shared memory segment
  // <-- you write this
  segment_id = shmget(IPC_PRIVATE, 400, IPC_CREAT | S_IRUSR | S_IWUSR);

  printf("The segment_id is :%d\n", segment_id);

  // Step 2: retrieve the information of the segment
  if (shmctl(segment_id, IPC_STAT, &shmbuffer) == -1) {
    printf("Unable to access segment %d\n", segment_id);
    return 0;
  }

  // Print the table header and separator rows
  printf("ID\tKEY\tMODE\t\tOWNER\tSIZE\tATTACHES\n");
  printf("--\t---\t----\t\t-----\t----\t--------\n");

  // Begin the data row: ID and key, no newline yet
  printf("%d\t%d\t", segment_id, shmbuffer.shm_perm.__key);

  return 0;
}
