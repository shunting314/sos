#pragma once

int file_open(const char* path, int oflags);
int file_read(int fd, void *buf, int nbyte);
int file_close(int fd);
