#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
  // Print the process id
  printf("Process ID : %d\n", getpid());

  // Allocate the string in memory
  char *s = strdup("Change me please");

  if (s == NULL) {
    fprintf(stderr, "Can't allocate mem with malloc\n");
    return EXIT_FAILURE;
  }

  // Infinite loop until we hack the string
  int i = 0;
  while (strcmp(s, "It is true magic") != 0) {
    printf("%lu - %s (%p)\n", i, s, (void*)s);
    sleep(1);
    ++i;
  }

  // Print the hacked string
  printf("%lu - %s (%p)\n", i, s, (void*)s);

  // Free memory
  free(s);

  return EXIT_SUCCESS;
}