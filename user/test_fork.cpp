#include <stdio.h>
#include <syscall.h>
#include <assert.h>

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
  int exit_code = 12;
  printf("Enter the main function\n");
  int sum = 0;
  for (int i = 1; i <= 100; ++i) {
    sum += i;
  }
  printf("sum from 1 to 100 is %d\n", sum);
  // #define USE_DUMBFORK 1
  #if USE_DUMBFORK
  int r = dumbfork();
  #else
  int r = fork();
  #endif
  if (r < 0) {
    printf("Fail to fork, return %d\n", r);
  } else if (r == 0) { // child process
    for (int i = 0; i < 5; ++i) {
      printf("Message from child process %d [current pid is %d]\n", i, getpid());
    }
  } else {
    // wait for child process to complete first
    int status;
    int rval = waitpid(r, &status, 0);
    assert(status == exit_code);
    assert(rval == r);

    for (int i = 0; i < 5; ++i) {
      printf("Message from parent process %d [r=%d]\n", i, r);
    }
  }
  printf("bye\n");
  return exit_code;
}
