#pragma

typedef void init_fn_t(void);

/*
 * The state for initializing an application. Kernel sets this up and
 * app initialization code will process it.
 */
struct AppInitState {
 public:
  // argc, argv must be the beginning fields to make sure ulib/entry.cpp works.
  // We assume when an AppInitState is at the stack top, the stack top contains
  // argc and argv in that order.
  int argc = 0;
  const char **argv = nullptr;

  // the function ptrs (init_fn_t*) usually reside in the stack space above
  // the AppInitState object.
  init_fn_t** init_fn_table = nullptr;
  int init_fn_table_size = 0;
};
