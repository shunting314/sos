#include <stdio.h>

int main(void) {
  printf("Enter the main function of shell\n");
  int sum = 0;
  for (int i = 1; i <= 100; ++i) {
    sum += i;
  }
  printf("sum from 1 to 100 is %d\n", sum);
  printf("bye\n");
  return 0;
}
