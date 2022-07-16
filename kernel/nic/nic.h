#pragma once

#include <kernel/pci.h>
#include <kernel/net/net.h>
#include <assert.h>

void nic_init();

extern PCIFunction ethernet_controller_pci_func;

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
