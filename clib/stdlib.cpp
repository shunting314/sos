// random number related APIs
// A simple linear congruential generator. Check wiki page for more detail:
// https://en.wikipedia.org/wiki/Linear_congruential_generator
// The parameters used here follows glibc.
#include <stdlib.h>

#define M (1 << 31)
#define A 1103515245
#define C 12345
uint32_t _lcg_seed = 41;

uint32_t rand() {
  _lcg_seed = (A * _lcg_seed + C) % M;
  return _lcg_seed & 0x7FFFFFFF;
}

float rand01() {
  return rand() / 2147483648.0f;
}

void srand(int s) {
  _lcg_seed = s;
}
