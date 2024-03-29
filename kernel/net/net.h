#pragma once

#include <stdint.h>

class IPAddr {
 public:
  explicit IPAddr() = default;
  explicit IPAddr(const char* ipstr);
  void print() const;

  bool operator==(const IPAddr& rhs) const {
    const IPAddr& lhs = *this;
    for (int i = 0; i < 4; ++i) {
      if (lhs.addr_[i] != rhs.addr_[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const IPAddr& rhs) const {
    return !((*this) == rhs);
  }
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

  void print() const {
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

  bool operator==(const MACAddr& rhs) {
    const MACAddr& lhs = *this;
    for (int i = 0; i < 6; ++i) {
      if (lhs.addr_[i] != rhs.addr_[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const MACAddr& rhs) {
    return !((*this) == rhs);
  }
 private:
  uint8_t addr_[6];
};

extern IPAddr self_ip;
extern IPAddr gateway_ip;
extern MACAddr self_mac;
extern MACAddr gateway_mac;
extern MACAddr allzero_mac;
extern MACAddr allone_mac;
