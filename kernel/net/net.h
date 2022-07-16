#pragma once

#include <stdint.h>

class IPAddr {
 public:
  explicit IPAddr() = default;
  explicit IPAddr(const char* ipstr);
  void print() const;
 private:
  uint8_t addr_[4];
};

class MACAddr {
 public:
  MACAddr() { }
  MACAddr(uint16_t w0, uint16_t w1, uint16_t w2) {
    addr_[0] = w0 & 0xFF;
    addr_[1] = w0 >> 8;
    addr_[2] = w1 & 0xFF;
    addr_[3] = w1 >> 8;
    addr_[4] = w2 & 0xFF;
    addr_[5] = w2 >> 8;
  }

  explicit MACAddr(const char* macstr);

  void print() {
    printf("MAC address: ");
    for (int i = 0; i < 6; ++i) {
      printf("%x", addr_[i]);
      if (i == 5) {
        printf("\n");
      } else {
        printf(":");
      }
    }
  }

  const uint8_t* get_addr() const { return addr_; };
 private:
  uint8_t addr_[6];
};

extern IPAddr self_ip;
extern IPAddr gateway_ip;
extern MACAddr self_mac;
extern MACAddr gateway_mac;
extern MACAddr allzero_mac;
extern MACAddr allone_mac;
