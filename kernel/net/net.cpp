#include <kernel/net/net.h>
#include <ctype.h>
#include <stdlib.h>

IPAddr self_ip("10.0.2.15");
IPAddr gateway_ip("10.0.2.2");
MACAddr self_mac, gateway_mac;
MACAddr allzero_mac("0:0:0:0:0:0");
MACAddr allone_mac("FF:FF:FF:FF:FF:FF");

// NOTE: to simply the code a bit, we allow leading zeros. Leading zeros
// are just ignored and does not indicated the number is an octal number.
// Numbers are always decimal.
// E.g. '012' represets 12 rather than 10.
IPAddr::IPAddr(const char* ipstr) {
  const char* p = ipstr;
  int idx = 0;
  while (*p) {
    assert(idx < 4);
    assert(isdigit(*p));
    int num = 0;
    while (isdigit(*p)) {
      num = num * 10 + (*p - '0');
      ++p;
    }
    assert(num <= 255);
    addr_[idx++] = num;
    assert(*p == '\0' || *p == '.');
    if (*p == '.') {
      ++p;
    }
  }
  assert(idx == 4);
}

void IPAddr::print() const {
  for (int i = 0; i < 4; ++i) {
    printf("%d%c", addr_[i], i == 3 ? '\n' : '.');
  }
}

MACAddr::MACAddr(const char* macstr) {
  const char* p = macstr;
  int idx = 0;
  while (*p) {
    assert(idx < 6);
    assert(isxdigit(*p));
    int num = 0;
    while (isxdigit(*p)) {
      num = num * 16 + hexchar2int(*p++);
    }
    assert(num <= 255);
    addr_[idx++] = num;
    assert(*p == '\0' || *p == ':');
    if (*p == ':') {
      ++p;
    }
  }
  assert(idx == 6);
}
