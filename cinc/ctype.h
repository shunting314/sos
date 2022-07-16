#pragma once
#include <stdint.h>

bool isdigit(char ch) {
  return ch >= '0' && ch <= '9';
}

bool isxdigit(char ch) {
  return isdigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}
