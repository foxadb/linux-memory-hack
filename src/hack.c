#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

long findPidByName(char* procname) {
  // Pid to be returned
  long pid = -1;

  // Regex for pid and process name
  regex_t number;
  regex_t name;
  regcomp(&number, "^[0-9]\\+$", 0);
  regcomp(&name, procname, 0);

  // Go to proc dir and open it
  chdir("/proc");
  DIR* proc = opendir("/proc");

  // Iterate through proc dir
  struct dirent* dp;
  char buf[4096];
  while (dp = readdir(proc)) {
    // Only consider dir with pid name
    if (regexec(&number, dp->d_name, 0, 0, 0) == 0) {
      chdir(dp->d_name);

      // Open cmdline file
      int fd = open("cmdline", O_RDONLY);

      // Read file into the buffer
      buf[read(fd, buf, (sizeof buf) - 1)] = '\0';

      if (regexec(&name, buf, 0, 0, 0) == 0) {
        if (pid != -1) {
          fprintf(stderr, "Second process %s found: %s\n", dp->d_name, buf);
          return -1;
        }
        pid = atoi(dp->d_name);
        printf("Process %d found: %s\n", pid, buf);
      }

      // Close cmdline file
      close(fd);

      // Go back to proc dir
      chdir("..");
    }
  }

  // Close proc dir
  closedir(proc);

  return pid;
}

int main(int argc, char* argv[]) {
  // Process id
  long pid = findPidByName("stringloop");

  if (pid == -1) {
    fprintf(stderr, "Process id not found\n");
    return EXIT_FAILURE;
  }

  // Memory file path
  char memfile[64];
  sprintf(memfile, "/proc/%ld/mem", pid);

  // Open memory file
  printf("Open memfile: %s\n", memfile);
  int fd = open(memfile, O_RDWR);

  // Trace process
  ptrace(PTRACE_ATTACH, pid, 0, 0);

  // Wait for sync
  waitpid(pid, NULL, 0);

  // Retreive the string address
  off_t addr = (off_t)strtol(argv[1], NULL, 16);

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

  return EXIT_SUCCESS;
}