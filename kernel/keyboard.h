#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define KEYBOARD_CTRL_D -1

void handleKeyboard();
void keyboardInit();
char keyboardGetChar(bool blocking);
void keyboardPutback(char ch);
// read len - 1 characters or until a newline is reached. newline is also stored
// in buf. The buf is null terminated. returns the actual number of characters
// read.
int keyboardReadLine(char* buf, int len);

#ifdef __cplusplus
}
#endif
