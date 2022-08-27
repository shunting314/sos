#pragma once

enum {
  SC_WRITE = 1,
  SC_EXIT = 2, // exit the process
  SC_DUMBFORK = 3,
  SC_FORK = 4,
  SC_GETPID = 5,
	SC_OPEN = 6,
	SC_READ = 7,
  SC_CLOSE = 8,
  NUM_SYS_CALL,
};
