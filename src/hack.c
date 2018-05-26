#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
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

unsigned long findHeapAddress(long pid) {
  // Init heap address to be returned
  unsigned long heapAddr = -1;

  // Open maps file
  char mapsfile[64];
  sprintf(mapsfile, "/proc/%ld/maps", pid);
  FILE* fp = fopen(mapsfile, "r");
  if (fp == NULL) {
    return -1;
  }

  // Read the maps file
  char* line = NULL;
  size_t len = 0;
  char* heapWord = "[heap]";
  while (getline(&line, &len, fp) != -1) {
    if (strstr(line, heapWord) != NULL) {
      // Line is the heap
      char* address = strtok(line, "-");
      heapAddr = (unsigned long)strtol(address, NULL, 16);
    }
  }

  // Free line
  if (line) {
    free(line);
  }

  // Close file
  fclose(fp);

  return heapAddr;
}

unsigned long findStringAddress(long pid, unsigned long heapAddr, char* string,
                                size_t size) {
  struct iovec local[1];
  struct iovec remote[1];
  char buf[64];
  
  local[0].iov_base = buf;
  local[0].iov_len = 64;
  remote[0].iov_base = (void*)heapAddr;
  remote[0].iov_len = size;

  int counter = 0;
  int maxCounter = 1000000; // max iterations
  
  // Iterate through heap
  process_vm_readv(pid, local, 1, remote, 1, 0);
  while (strcmp(string, local[0].iov_base) != 0 && counter < maxCounter) {
    ++remote[0].iov_base;
    process_vm_readv(pid, local, 1, remote, 1, 0);
    ++counter;
  }

  unsigned long result = (unsigned long)remote[0].iov_base;
  return (counter < maxCounter) ? result : -1;
}

int writeOnStringAddress(long pid, unsigned long addr, char* stringBuf, size_t size) {
  struct iovec local[1];
  struct iovec remote[1];
  
  local[0].iov_base = stringBuf;
  local[0].iov_len = size;
  remote[0].iov_base = (void*)addr;
  remote[0].iov_len = size;

  return (process_vm_writev(pid, local, 1, remote, 1, 0) == size);
}

int main(int argc, char* argv[]) {
  // Process id
  long pid = findPidByName("stringloop");

  if (pid == -1) {
    fprintf(stderr, "Process id not found\n");
    return EXIT_FAILURE;
  }

  // Find heap start address
  unsigned long heapAddr = findHeapAddress(pid);
  if (heapAddr == -1) {
    fprintf(stderr, "Heap address not found\n");
    return EXIT_FAILURE;
  }
  printf("Heap address: 0x%lx\n", heapAddr);

  char* oldString = "Change me please";
  char* newString = "It is true magic";
  size_t stringSize = 16;

  // Retreive the string address
  unsigned long addr = findStringAddress(pid, heapAddr, oldString, stringSize);
  if (addr == -1) {
    fprintf(stderr, "String not found\n");
    return EXIT_FAILURE;
  }
  printf("String address: 0x%lx\n", addr);

  // Write the new string on memory
  int writeRes = writeOnStringAddress(pid, addr, newString, stringSize);

  if (writeRes) {
    printf("Writing operation successful :)\n");
  } else {
    fprintf(stderr, "Error when writing memory\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}