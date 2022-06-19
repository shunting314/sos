#pragma once

#include <kernel/pci.h>
#include <assert.h>

void nic_init();

extern PCIFunction ethernet_controller_pci_func;

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
 private:
  uint8_t addr_[6];
};

// TODO can not build if enabling virtual methods. Revisit later!
#define NIC_ENABLE_VIRTUAL 0

class NICDriver {
 public:
  explicit NICDriver(const PCIFunction& pci_func) : pci_func_(pci_func) { }
#if NIC_ENABLE_VIRTUAL
  // TODO: this depends on 'operator delete(void*, unsigned long)'
  virtual ~NICDriver() = default;
#endif
  MACAddr getMACAddr() const { return mac_addr_; }
 protected:
  PCIFunction pci_func_;
  MACAddr mac_addr_;
};
