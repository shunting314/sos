#pragma once

enum {
  SC_WRITE = 1,
  SC_EXIT = 2, // exit the process
  SC_DUMBFORK = 3,
  SC_GETPID = 4,
  NUM_SYS_CALL,
};
