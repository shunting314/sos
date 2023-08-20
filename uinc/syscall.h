#pragma once

int write(int fd, const char *buf, int sz);
extern "C" int exit(int status);
int dumbfork();
int fork(); // COW fork
int spawn(const char* path);
int getpid();
int open(const char*path, int oflags);
int read(int fd, void *buf, int nbyte);
int close(int fd);
int waitpid(int pid, int *pstatus, int /* options */);
