#pragma

typedef void init_fn_t(void);

/*
 * The state for initializing an application. Kernel sets this up and
 * app initialization code will process it.
 */
struct AppInitState {
 public:
   // the function ptrs (init_fn_t*) usually reside in the stack space above
   // the AppInitState object.
   init_fn_t** init_fn_table = nullptr;
   int init_fn_table_size = 0;
};
