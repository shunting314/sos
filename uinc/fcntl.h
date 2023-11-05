#pragma once

#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR (O_RDONLY | O_WRONLY)
#define O_TRUNC 4

int open(const char* path, int oflags);
