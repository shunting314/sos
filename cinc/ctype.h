#pragma once
#include <stdint.h>

static bool isdigit(char ch) {
  return ch >= '0' && ch <= '9';
}

static bool isxdigit(char ch) {
  return isdigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

static bool isspace(char ch) {
  return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}
