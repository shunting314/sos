#include <stdio.h>

int main(void) {
  char line[1024];
  printf("Enter the main function of shell\n");
  while (true) {
    printf("$ ");
    if (!fgets(line, sizeof(line), 0)) {
      break;
    }
    printf("Got line %s\n", line);
  }
  printf("bye\n");
  return 0;
}
