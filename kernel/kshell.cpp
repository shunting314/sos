#include <kernel/kshell.h>
#include <kernel/keyboard.h>
#include <kernel/vga.h>
#include <kernel/user_process.h>
#include <kernel/simfs.h>
#include <kernel/ide.h>
#include <kernel/loader.h>
#include <kernel/pci.h>
#include <kernel/phys_page.h>
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
int cmdLs(char* args[]);
int cmdCat(char* args[]);
int cmdLaunch(char* args[]);
int cmdLspci(char *args[]);
int cmdCheckPhysMem(char *args[]);

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

  // ls and cat should be implemented as user space applications.
  // Implement simple version in kernel mode so we can test file loading
  // from the filesystem. Kernel need this functionality to load an
  // executable.
  { "ls", "List the directory", cmdLs},
  { "cat", "Show the file content", cmdCat},
  { "launch", "Lauch a user process from the program in the file system", cmdLaunch},
  { "lspci", "Enumerate PCI devices.", cmdLspci},
  { "check_phys_mem", "Check the amount of available physical pages.", cmdCheckPhysMem},
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

IDEDevice createIDEDevice(int hda_id) {
  if (hda_id == 0) {
    return createMasterIDE();
  } else if (hda_id == 1) {
    return createSlaveIDE();
  } else {
    return IDEDevice(); // return an invalid one
  }
}

// TODO check that the sector no does not exceed the maximum
int cmdReadSector(char* args[]) {
  int hda_id;
  int sectNo;
  IDEDevice dev;
  uint8_t buf[512];
  if (!args[0]) {
    goto err_out;
  }
  dev = createIDEDevice(atoi(args[0]));
  if (!dev) {
    printf("Invalid hda id %s\n", args[0]);
    goto err_out;
  }

  if (!args[1]) {
    goto err_out;
  }
  sectNo = atoi(args[1]);
  if (sectNo < 0) {
    printf("Sector number can not be negative: %d\n", sectNo);
    goto err_out;
  }
  printf("Read sector %d from dev %s\n", sectNo, args[0]);
  dev.read(buf, sectNo, 1);

  for (int i = 0; i < 512; ++i) {
    printf(" %x", buf[i]);
    if (i % 16 == 15) {
      printf("\n");
    }
  }
  return 0;
err_out:
  printf("Usage: read_sector [hda id] [sector no]\n");
  return -1;
}

int cmdWriteSector(char* args[]) {
  auto usage = []() {
    printf("Usage: write_sector [hda id] [sector no] [fill byte]\n");
    return -1;
  };
  if (!args[0] || !args[1] || !args[2]) {
    return usage();
  }
  int hda_id = atoi(args[0]);
  IDEDevice dev = createIDEDevice(hda_id);
  if (!dev) {
    printf("Invalid hda id %s\n", args[0]);
    return usage();
  }
  int sectNo = atoi(args[1]);
  if (sectNo < 0) {
    printf("Sector number can not be negative: %d\n", sectNo);
    return usage();
  }
  int fillByte = atoi(args[2]);
  if (fillByte < 0 || fillByte > 255) {
    printf("Fill byte must between [0, 255], but got %d\n", fillByte);
    return usage();
  }
  uint8_t buf[512];
  for (int i = 0; i < 512; ++i) {
    buf[i] = (uint8_t) fillByte;
  }
  printf("Write sector %d with byte 0x%x\n", sectNo, fillByte);
  dev.write(buf, sectNo, 1);
  return 0;
}

int cmdLs(char *args[]) {
  char *path = args[0];
  char rootdir[] = "/";
  if (!path) {
    path = rootdir;
  }
  return ls(path);
}

int cmdCat(char *args[]) {
  if (!args[0]) {
    printf("Missing path\n");
    return -1;
  }
  return cat(args[0]);
}

int cmdLaunch(char *args[]) {
  if (!args[0]) {
    printf("Missing path to the program\n");
    return -1;
  }
  return launch(args[0], nullptr, true);
}

int cmdLspci(char *args[]) {
  lspci();
  return 0;
}

int cmdCheckPhysMem(char *args[]) {
  printf("Number of available physical pages %d\n", num_avail_phys_pages());
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

static void handleLine(char *line) {
  char* args[128]; // add a nullptr after the last argument
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

void kshell() {
  printf("enter \033[35mkshell\033[m. May start a predefined user problem like the user shell automatically.\n");
  // enable the interrupt here. It's mainly needed when we enter kshell after
  // terminating the last process. In that case, the user process enters kernel
  // by call the exit syscall. Since we use interrupt gate, interrupt is disabled
  // once we enter kernel.
  // We should reenable interrupt so kshell get keyboard inputs.
  asm("sti");

#ifndef QUICK_TEST
  #define QUICK_TEST 1
#endif
#if QUICK_TEST
  // only execute the predefined command for the first time to avoid it being
  // executed in a dead loop. NOTE that handleLine may not return for some commands.
  // E.g. for 'launch shell', when all the user processes terminate, the kernel
  // calls kshell again. Without this flag, kernel will repeatedly execute the
  // command.
  static int first_time = 1;
  char line[1024] = "launch shell";
  if (first_time) {
    first_time = 0;
    handleLine(line);
  }
#else
  char line[1024];
#endif
  while (1) {
    printf("> ");
    int got = keyboardReadLine(line, sizeof(line));
    if (got > 0 && line[got - 1] == '\n') {
      line[got - 1] = '\0';
    }
    handleLine(line);
  }
}
