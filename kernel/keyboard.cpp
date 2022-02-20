/*
 * The mapping from a scancode to an ascii character is reverse-engineered
 * manually..
 */
#include <kernel/keyboard.h>
#include <kernel/ioport.h>
#include <assert.h>

char scancodeToAscii[128] = {
  //0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
  0x0, 0x0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x0, 0x0,
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']','\n', 0x0, 'a', 's',
  'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'', '`', 0x0,'\\', 'z', 'x', 'c', 'v',
  'b', 'n', 'm', ',', '.', '/', 0x0, 0x0, 0x0, ' ', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};

char shiftIt(char orig) {
  if (orig >= 'a' && orig <= 'z') {
    return orig - 'a' + 'A';
  }
  char shifted = '?';
  switch (orig) {
  case '`': shifted = '~'; break;
  case '1': shifted = '!'; break;
  case '2': shifted = '@'; break;
  case '3': shifted = '#'; break;
  case '4': shifted = '$'; break;
  case '5': shifted = '%'; break;
  case '6': shifted = '^'; break;
  case '7': shifted = '&'; break;
  case '8': shifted = '*'; break;
  case '9': shifted = '('; break;
  case '0': shifted = ')'; break;
  case '-': shifted = '_'; break;
  case '=': shifted = '+'; break;
  case '[': shifted = '{'; break;
  case ']': shifted = '}'; break;
  case '\\': shifted = '|'; break;
  case ';': shifted = ':'; break;
  case '\'': shifted = '"'; break;
  case ',': shifted = '<'; break;
  case '.': shifted = '>'; break;
  case '/': shifted = '?'; break;
  default:
    printf("Don't know how to shift '%c'\n", orig);
    assert(false);
    break;
  }
  return shifted;
}

enum {
  NONE,
  SHIFT,
  TAB_PREFIX,
} meta_key;

#define PRINT_SCAN_CODE_AND_RETURN 0
void handleKeyboard() {
  // Making the Port8Bit static requires symbol __cxa_guard_acquire/__cxa_guard_release
  /* static */ Port8Bit dataPort(0x60);
  uint8_t scancode = dataPort.read();
  if (scancode & 0x80) {
    // ignore the key release event
    return;
  }
#if PRINT_SCAN_CODE_AND_RETURN
  printf("Debug: get a scancode 0x%x\n", scancode);
  return;
#endif

  // special scan code
  switch (scancode) {
  case 0x48: return; // up, ignore for now
  case 0x50: return; // down
  case 0x4b: return; // left
  case 0x4d: return; // right
  case 0x01: return; // ignore escape key
  case 0x38: return; // ignore xxx
  case 0x0E: return; // TODO: support delete key
  case 0x2A: meta_key = SHIFT; return; // shift prefix
  case 0x1d: meta_key = TAB_PREFIX; return; // a tab is encoded as a sequence of scancode 0x1d and scancode for 'i'
  }

  char ascii = scancodeToAscii[scancode];
  switch (meta_key) {
  case TAB_PREFIX:
    if (ascii == 'i') {
      ascii = '\t';
    }
    break;
  case SHIFT:
    // the scancode sequence for shift tab is still considered as a tab
    if (scancode == 0xF) {
      ascii = '\t';
      break;
    }
    if (ascii != 0) {
      ascii = shiftIt(ascii);
      break;
    }
    break;
  }
  meta_key = NONE;

  if (ascii == 0) {
    printf("Got a unrecognized scan code 0x%x\n", scancode);
    assert(false);
  }
  printf("Got an ascii character '%c'(0x%x)\n", ascii, ascii);
}
