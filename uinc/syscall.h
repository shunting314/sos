#pragma once

int write(int fd, char *buf, int sz);
extern "C" int exit();
int dumbfork();
int fork(); // COW fork
int getpid();
