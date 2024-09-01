#pragma once

#include <stdio.h>
#include <string.h>

const char *get_interface_name() {
  static char iname[1024]; // not thread safe
  FILE *fp = popen("ip route get 8.8.8.8 | grep dev | sed 's/.*dev \\(\\w*\\) src.*/\\1/'", "r");
  assert(fp);

  if (fgets(iname, sizeof(iname), fp) == NULL) {
    assert(false && "Fail to read the interface name");
  }

  int l = strlen(iname);
  while (l > 0 && iname[l - 1] == '\n') {
    iname[--l] = '\0';
  }

  printf("Interface name is '%s'\n", iname);
  return iname;
}

void hexdump(char *buf, int len) {
  for (int i = 0; i < len; ++i) {
    printf(" %02x", (uint8_t) buf[i]);
    if (i % 16 == 15) {
      printf("\n");
    }
  }
  printf("\n");
}

int xdigitToVal(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  } else if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  } else if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  } else {
    assert(false && "invalid xdigit");
  }
}


