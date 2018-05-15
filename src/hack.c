#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
  // Process id
  int pid = atoi(argv[1]);

  // Memory file path
  char memfile[64];
  sprintf(memfile, "/proc/%ld/mem", (long)pid);

  // Open memory file
  printf("Open memfile: %s\n", memfile);
  int fd = open(memfile, O_RDWR);

  // Trace process
  ptrace(PTRACE_ATTACH, pid, 0, 0);

  // Wait for sync
  waitpid(pid, NULL, 0);

  // Retreive the string address
  off_t addr = (off_t)strtol(argv[2], NULL, 16);

  // Variable to store the string value
  char value[64];

  // Read memory value
  pread(fd, &value, sizeof(value), addr);
  printf("String value read at 0x%lx: %s\n", addr, value);

  // Copy the new string and write it on memory
  strcpy(value, "It is true magic");
  printf("Change it to: %s\n", value);
  pwrite(fd, &value, sizeof(value), addr);

  // Detache process
  ptrace(PTRACE_DETACH, pid, 0, 0);

  // Close memory file
  close(fd);

  return (EXIT_SUCCESS);
}