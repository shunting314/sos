// #include <stdio.h>

int printf(const char*, ...);

void bar() {
}

void foo() {
  bar();
}

int main(void) {
  printf("hello\n");
  return 0;
}
