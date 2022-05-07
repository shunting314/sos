#ifndef STDIO_H
#define STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* fmt, ...);
int puts(const char *s);
int putchar(int ch);

#ifdef __cplusplus
}
#endif

#endif
