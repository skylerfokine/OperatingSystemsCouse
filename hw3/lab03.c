#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
  int segment_id;            // identifier of the shared memory segment
  unsigned short mode;       // permissions of the segment
  struct shmid_ds shmbuffer; // the shared memory segment

  // Step 1: Create a new shared memory segment using shmget

  /*
   * Ipc_private means give me a brand new private segment, dont look up one by
   * name
   * then the size declares the size of which im allocating
   *
   * finally the flags
   *
   * IPC_CREAT
   *
   * - create it if it doesnt exist
   *
   * S_IRUSR
   * S_IWUSR
   *
   * - read and write permission for the process that owns the segment
   */
  segment_id = shmget(IPC_PRIVATE, 400, IPC_CREAT | S_IRUSR | S_IWUSR);

  printf("New shared memeory segment is created: :%d\n", segment_id);
  ////

  // Step 2: Retrieve the information of the segment
  if (shmctl(segment_id, IPC_STAT, &shmbuffer) == -1) {
    printf("Unable to access segment %d\n", segment_id);
    return 0;
  }

  // Step 3: output information about the segment in the required format
  // Print the table header and separator rows
  printf("ID\tKEY\tMODE\t\tOWNER\tSIZE\tATTACHES\n");
  printf("--\t---\t----\t\t-----\t----\t--------\n");

  // Begin the data row: ID and key, no newline yet
  printf("%d\t%d\t", segment_id, shmbuffer.shm_perm.__key);
  ////

  // Output mode in the right format
  mode = shmbuffer.shm_perm.mode;

  /** OWNER */
  if (mode & 0400)
    printf("r");
  else
    printf("-");
  if (mode & 0200)
    printf("w");
  else
    printf("-");
  if (mode & 0100)
    printf("a");
  else
    printf("-");

  /** GROUP */
  if (mode & 0040)
    printf("r");
  else
    printf("-");
  if (mode & 0020)
    printf("w");
  else
    printf("-");
  if (mode & 0010)
    printf("a");
  else
    printf("-");

  /** WORLD */
  if (mode & 0004)
    printf("r");
  else
    printf("-");
  if (mode & 0002)
    printf("w");
  else
    printf("-");
  if (mode & 0001)
    printf("a");
  else
    printf("-");

  // Finish the data row with owner id, the size and number of attaches
  printf("\t%u\t%zu\t%lu\n", shmbuffer.shm_perm.uid, shmbuffer.shm_segsz,
         shmbuffer.shm_nattch);

  // Step 4: Create a new process using fork
  pid_t pid = fork();

  /* Step 5: The child process sends a message to the parent process via the
   * the shared memory segment created in Step 1 and the parent prints out the
   * message it received from the child process
   */
  if (pid < 0) {
    fprintf(stderr, "Fork failed\n");
    return 1;
  } else if (pid == 0) {
    // CHILD: attach, write a message, detach, exit
    /* Shared memory is now a pointer directly to the section of memory i will
     * be running the processes in
     */
    char *shared_memory = (char *)shmat(segment_id, NULL, 0);
    // This check exsits to see if shared_memory errored out or not
    if (shared_memory == (char *)-1) {
      fprintf(stderr, "shmat failed\n");
      return 1;
    }
    sprintf(shared_memory, "Hello parent process!");
    shmdt(shared_memory);

  } else {
    // PARENT: wait for the child, then attach, read, detach
    // Block until the child exits before reading shared memory. Without this,
    // the parent could read before the child writes — a race condition, since
    // shmat/shmdt provide no synchronization on their own.
    wait(NULL);
    char *shared_memory = (char *)shmat(segment_id, NULL, 0);
    if (shared_memory == (char *)-1) {
      fprintf(stderr, "shmat failed\n");
      return 1;
    }
    printf("\nParent received message from Child: %s\n", shared_memory);
    shmdt(shared_memory);
  }

  return 0;
}
