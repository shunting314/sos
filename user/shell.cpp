#include <stdio.h>

class RAIICls {
 public:
  RAIICls(const char *msg) : msg_(msg) {
    printf("Hello from %s\n", msg);
  }
  /*
   * TODO: global destructors are not supported for now since enabling them will introduce
   * dependencies to symbols like __dso_handle, __cxa_atexit etc.
   * Linker error messages when enabling it: https://gist.github.com/shunting314/7caae70c30bad65b559a51d7934d39df
   */
#if 0
  ~RAIICls() {
    printf("Destructing %s\n", msg_);
  }
#endif
 private:
  const char* msg_;
};

RAIICls global_obj_first("the first global object");
RAIICls global_obj_second("the second global object");

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
