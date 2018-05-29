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

int findHeapAddress(long pid, unsigned long* heapStart,
                    unsigned long* heapEnd) {
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
  int found = 0;
  while (getline(&line, &len, fp) != -1 && !found) {
    if (strstr(line, heapWord) != NULL) {
      char addr1[16];
      char addr2[16];
      int i = 0;

      while (line[i] != '-') {
        addr1[i] = line[i];
        ++i;
      }
      addr1[i] = '\0';

      // Skip separator
      ++i;
      int off = i;

      while (line[i] != '-') {
        addr2[i - off] = line[i];
        ++i;
      }
      addr2[i - off] = '\0';

      // Parse heap start and end address
      *heapStart = (unsigned long)strtol(addr1, NULL, 16);
      *heapEnd = (unsigned long)strtol(addr2, NULL, 16);

      // Found
      found = 1;
    }
  }

  // Free line
  if (line) {
    free(line);
  }

  // Close file
  fclose(fp);

  return 0;
}

int writeOnStringAddress(long pid, unsigned long addr, char* stringBuf,
                         size_t size) {
  struct iovec local[1];
  struct iovec remote[1];

  local[0].iov_base = stringBuf;
  local[0].iov_len = size;
  remote[0].iov_base = (void*)addr;
  remote[0].iov_len = size;

  return (process_vm_writev(pid, local, 1, remote, 1, 0) == size);
}

int writeOnString(long pid, unsigned long heapStart, unsigned long heapEnd,
                  char* oldString, char* newString, size_t size) {
  struct iovec local[1];
  struct iovec remote[1];
  char* buf = malloc(size * sizeof(char));

  local[0].iov_base = buf;
  local[0].iov_len = 64;
  remote[0].iov_base = (void*)heapStart;
  remote[0].iov_len = size;

  // Iterate through heap
  unsigned long scanZone = heapEnd - heapStart;
  while (scanZone > 0) {
    process_vm_readv(pid, local, 1, remote, 1, 0);
    if (strcmp(oldString, local[0].iov_base) == 0) {
      int writeRes = writeOnStringAddress(
          pid, (unsigned long)remote[0].iov_base, newString, size);
      if (writeRes == 0) {
        free(buf);  // free local buffer
        return 0;
      }
    }
    ++remote[0].iov_base;
    --scanZone;
  }

  free(buf);  // free local buffer
  return 1;
}

int main(int argc, char* argv[]) {
  // Process id
  long pid = findPidByName("stringloop");

  if (pid == -1) {
    fprintf(stderr, "Process id not found\n");
    return EXIT_FAILURE;
  }

  // Find heap start address
  unsigned long heapStart = -1, heapEnd;
  int heapRes = findHeapAddress(pid, &heapStart, &heapEnd);
  if (heapRes == -1) {
    fprintf(stderr, "Heap address not found\n");
    return EXIT_FAILURE;
  }
  printf("HEAP begin: 0x%lx - end: 0x%lx\n", heapStart, heapEnd);

  char* oldString = "Change me please";
  char* newString = "It is true magic";
  size_t stringSize = 16;

  // Write the new string on memory
  printf("Scanning memory...\n");
  int writeRes =
      writeOnString(pid, heapStart, heapEnd, oldString, newString, stringSize);

  if (writeRes) {
    printf("Writing operation successful :)\n");
  } else {
    fprintf(stderr, "Error when writing on memory\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}