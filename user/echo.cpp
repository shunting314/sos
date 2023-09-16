#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv) {
  assert(argv[argc] == nullptr);
  printf("Prog name %s:\n", argv[0]);
  for (int i = 1; i < argc; ++i) {
    printf("%s%s", argv[i], i == argc - 1 ? "\n" : " ");
  }
  return 0;
}
