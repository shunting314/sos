#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void handleKeyboard();
void keyboardInit();
char keyboardGetChar();
// read len - 1 characters or until a newline is reached. newline is also stored
// in buf. The buf is null terminated. returns the actual number of characters
// read.
int keyboardReadLine(char* buf, int len);

#ifdef __cplusplus
}
#endif
