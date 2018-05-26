#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

off_t findHeapAddress(long pid) {
  // Init heap address to be returned
  off_t heapAddr = -1;

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
      heapAddr = (off_t)strtol(address, NULL, 16);
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

off_t findStringAddress(int fd, off_t heapAddr, char* string) {
  // Init first address to be read
  off_t addr = heapAddr;

  char value[64];
  int counter = 0;
  int maxCounter = 1000000;

  // Iterating through the mem file 
  pread(fd, &value, sizeof(value), addr);
  while (strcmp(string, value) != 0 && counter < maxCounter) {
    ++addr;
    pread(fd, &value, sizeof(value), addr);
    ++counter;
  }

  return (counter < maxCounter) ? addr : -1;
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

  // Find heap start address
  off_t heapAddr = findHeapAddress(pid);
  if (heapAddr == -1) {
    fprintf(stderr, "Heap address not found\n");
    return EXIT_FAILURE;
  }
  printf("Heap address: 0x%lx\n", heapAddr);

  // Retreive the string address
  off_t addr = findStringAddress(fd, heapAddr, "Change me please");
  if (addr == -1) {
    fprintf(stderr, "String not found\n");
    return EXIT_FAILURE;
  }
  printf("String address: 0x%lx\n", addr);

  // Read memory value
  char value[64];
  pread(fd, &value, sizeof(value), addr);
  printf("String value read at 0x%lx: %s\n", addr, value);

  // Copy the new string and write it on memory
  strcpy(value, "It is true magic");
  printf("Change it to: %s\n", value);
  pwrite(fd, &value, sizeof(value), addr);

  // Close memory file
  close(fd);

  return EXIT_SUCCESS;
}