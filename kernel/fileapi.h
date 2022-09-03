#pragma once

int file_open(const char* path, int oflags);
int file_read(int fd, void *buf, int nbyte);
int file_close(int fd);
int file_write(int fd, const void* buf, int nbyte);
