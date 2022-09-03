#pragma once

int write(int fd, const char *buf, int sz);
extern "C" int exit();
int dumbfork();
int fork(); // COW fork
int getpid();
int open(const char*path, int oflags);
int read(int fd, void *buf, int nbyte);
int close(int fd);
