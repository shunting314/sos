#include <kernel/kshell.h>
#include <kernel/keyboard.h>
#include <kernel/vga.h>
#include <kernel/user_process.h>
#include <kernel/ide.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

UserProcess *_create_process_printloop(char ch) {
  /*
   * nop
   * mov $1, %eax # syscall number 1 for write
   * mov $1, %ebx # file descriptor
   * mov $addr, %ecx # buf 0x40008100
   * mov $1, %edx # cnt
   * mov $0, %esi
   * mov $0, %edi
   * int $0x30
   * 1: jmp 1b
   */
  uint8_t code[512] = {
    0x90, // addr 0x40008000
    0xb8, 0x01, 0x00, 0x00, 0x00,
    0xbb, 0x01, 0x00, 0x00, 0x00,
    0xb9, 0x00, 0x81, 0x00, 0x40,
    0xba, 0x01, 0x00, 0x00, 0x00,
    0xbe, 0x00, 0x00, 0x00, 0x00,
    0xbf, 0x00, 0x00, 0x00, 0x00,
    0xcd, 0x30,
    0xeb, 0xde,
    0xeb, 0xfe,
  };
  code[256] = ch; // data
  return UserProcess::create(code, sizeof(code) / sizeof(code[0]));
}

// return 0 on success
typedef int (*CmdFnPtr)(char* args[]);
int cmdHelp(char* args[]);
int cmdShowPalette(char* args[]);
int cmdShowFlashingDigits(char* args[]);
int cmdShowRunningCurve(char* args[]);
int cmdABProcess(char* args[]);
int cmdReadSector(char* args[]);
int cmdWriteSector(char* args[]);

struct KernelCmd {
  const char* cmdName;
  const char* helpMsg;
  CmdFnPtr cmdFn;
} cmdList[] = {
  {"help", "Print help message", cmdHelp},
  {"show_palette", "Show palette", cmdShowPalette},
  {"show_flashing_digits", "Show flashing digits", cmdShowFlashingDigits},
  {"show_running_curve", "Show running curve", cmdShowRunningCurve},
  {
    "ab_process",
    "Create 2 user processes repeately printing A and B respectively",
    cmdABProcess},
  { "read_sector", "Read sector from hda0", cmdReadSector},
  { "write_sector", "File the specified sector with the specified byte", cmdWriteSector},
  {nullptr, nullptr},
};

int cmdHelp(char* /* args */[]) {
  printf("Here are the available commands:\n");
  for (int i = 0; cmdList[i].cmdName; ++i) {
    printf("  %s - %s\n", cmdList[i].cmdName, cmdList[i].helpMsg);
  }
  return 0;
}

int cmdShowPalette(char* /* args */[]) {
  vga_clear();
  show_palette();
  return 0;
}

int cmdShowFlashingDigits(char* /* args */[]) {
  show_flashing_digits();
  return 0;
}

int cmdShowRunningCurve(char* /* args */[]) {
  show_running_curve();
  return 0;
}

int cmdABProcess(char* /* args */[]) {
  printf("Create process A\n");
  UserProcess *proc_A = _create_process_printloop('A');
  printf("Create process B\n");
  UserProcess *proc_B = _create_process_printloop('B');
  proc_A->resume();
  return 0;
}

// TODO check that the sector no does not exceed the maximum
int cmdReadSector(char* args[]) {
  int sectNo;
  IDEDevice dev = createMasterIDE();
  uint8_t buf[512];
  if (!args[0]) {
    goto err_out;
  }
  sectNo = atoi(args[0]);
  if (sectNo < 0) {
    printf("Sector number can not be negative: %d\n", sectNo);
    goto err_out;
  }
  printf("Read sector %d\n", sectNo);
  dev.read(buf, sectNo, 1);

  for (int i = 0; i < 512; ++i) {
    printf(" %x", buf[i]);
    if (i % 16 == 15) {
      printf("\n");
    }
  }
  return 0;
err_out:
  printf("Usage: read_sector [sector no]\n");
  return -1;
}

int cmdWriteSector(char* args[]) {
  auto usage = []() {
    printf("Usage: write_sector [sector no] [fill byte]\n");
    return -1;
  };
  if (!args[0] || !args[1]) {
    return usage();
  }
  int sectNo = atoi(args[0]);
  if (sectNo < 0) {
    printf("Sector number can not be negative: %d\n", sectNo);
    return usage();
  }
  int fillByte = atoi(args[1]);
  if (fillByte < 0 || fillByte > 255) {
    printf("Fill byte must between [0, 255], but got %d\n", fillByte);
    return usage();
  }
  uint8_t buf[512];
  for (int i = 0; i < 512; ++i) {
    buf[i] = (uint8_t) fillByte;
  }
  IDEDevice dev = createMasterIDE();
  printf("Write sector %d with byte 0x%x\n", sectNo, fillByte);
  dev.write(buf, sectNo, 1);
  return 0;
}

char* parseCmdLine(char* line, char *args[]) {
  char* cmd = nullptr;
  int argIdx = 0;

  char *cur = line;
  while (*cur) {
    // TODO add isspace implementation
    while (*cur == ' ' || *cur == '\t') {
      ++cur;
    }
    if (!*cur) {
      break;
    }
    char *nxt = cur + 1;
    while (*nxt != ' ' && *nxt != '\t' && *nxt != '\0') {
      ++nxt;
    }
    bool end = *nxt == '\0';
    *nxt = '\0';
    if (!cmd) {
      cmd = cur;
    } else {
      args[argIdx++] = cur;
    }
    if (end) {
      break;
    }
    cur = nxt + 1;
  }

  args[argIdx] = nullptr;
  return cmd;
}

void kshell() {
  char line[1024];
  char* args[128]; // add a nullptr after the last argument
  while (1) {
    printf("> ");
    int got = keyboardReadLine(line, sizeof(line));
    if (got > 0 && line[got - 1] == '\n') {
      line[got - 1] = '\0';
    }
    char* cmdName = parseCmdLine(line, args);
    if (cmdName == nullptr) {
      cmdName = (char*) "nullptr";
    }

    int r;
    int i;
    for (i = 0; cmdList[i].cmdName; ++i) {
      if (strcmp(cmdList[i].cmdName, cmdName) == 0) {
        r = cmdList[i].cmdFn(args);
        if (r != 0) {
          printf("Fail to execute command %s, return code %d\n", cmdName, r);
        }
        break;
      }
    }
    // unknown cmd
    if (!cmdList[i].cmdName) {
      printf("Command '%s' is unknown\n", cmdName);
      cmdHelp(args);
    }
  }
}
