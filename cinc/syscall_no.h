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
  SC_WAITPID = 9,
  SC_SPAWN = 10,
  SC_READDIR = 11,
  SC_MKDIR = 12,
  SC_GETCWD = 13,
  SC_CHDIR = 14,
  SC_PIPE = 15,
  SC_UNLINK = 16,
  SC_RMDIR = 17,
  NUM_SYS_CALL,
};
