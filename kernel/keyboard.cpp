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

#define BUFSIZ 4096
class KeyboardInputBuffer {
 public:
  void addChar(char ch) {
    assert(numBuffered() < BUFSIZ);
    buf[(tail++) % BUFSIZ] = ch;
  }

  char getChar() {
    assert(numBuffered() > 0);
    return buf[(head++) % BUFSIZ];
  }

  int numBuffered() {
    return tail - head;
  }

  void debug() {
    printf("head %d, tail %d\n", head, tail);
  }
 private:
  volatile char buf[BUFSIZ];
  volatile int head, tail;
} kbdBuffer;

// TODO: execute init functions in ELF loader so we can do the init in the
// constructor of kdbBuffer global variable.
void keyboardInit() {
  // nothing need to be done so far..
}

/*
 * Sometimes we really want keyboardGetChar to be unblocking. E.g., user mode
 * makes a syscall to read a character from the console. We disable interrupt
 * for syscall. If we are blocking here, then there will be a dead loop.
 */
char keyboardGetChar(bool blocking) {
  if (blocking) {
    while (kbdBuffer.numBuffered() == 0) {
    }
  }
  if (kbdBuffer.numBuffered()) {
    return kbdBuffer.getChar();
  } else {
    return -1;
  }
}

int keyboardReadLine(char* buf, int len) {
  int nread = 0;
  assert(len >= 2); // at least read 1 char that's not '\0'
  while (nread < len - 1) {
    char ch = keyboardGetChar(true); // blocking call
    buf[nread++] = ch;
    if (ch == '\n') {
      break;
    }
  }
  buf[nread] = '\0';
  return nread;
}

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
  // printf("Got an ascii character '%c'(0x%x)\n", ascii, ascii);
  kbdBuffer.addChar(ascii);
  putchar(ascii);
}
