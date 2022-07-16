#include <kernel/nic/nic.h>
#include <kernel/nic/82540EM.h>
#include <kernel/net/arp.h>
#include <kernel/net/crc.h>
#include <kernel/net/net.h>
#include <kernel/net/arp.h>
#include <assert.h>
#include <stdlib.h>

PCIFunction ethernet_controller_pci_func;

#if !NIC_ENABLE_VIRTUAL
// Without enabling virtual functions, we can not use base class ptr to access
// subclass's methods. Hack here so NICDriver* actually means NICDriver_82540EM*.
// Once we support virtual methods, we should be able to clean up these mess.
#define NICDriver NICDriver_82540EM
#endif

// TODO: implement unique_ptr to simplify this part
NICDriver* nic_driver = nullptr;
NICDriver_82540EM _nic_driver_82540EM;

void nic_init() {
  assert(ethernet_controller_pci_func);
  if (ethernet_controller_pci_func.vendor_id() == 0x8086 &&
    ethernet_controller_pci_func.device_id() == 0x100e) {
    // the 82540EM Gigabit Ethernet Controller
    _nic_driver_82540EM = NICDriver_82540EM(ethernet_controller_pci_func);
    nic_driver = &_nic_driver_82540EM;
  }
  assert(nic_driver);
  self_mac = nic_driver->getMACAddr();
  self_mac.print();

  // TODO: try to send and recv ARP messages
  ARPPacket arp_request = ARPPacket::createRequest(gateway_ip);
  for (int i = 0; i < 12; ++i) { // send more than 8 (RING size) requests for testing
    nic_driver->sync_send(allone_mac, arp_request);
  }
  printf("total packets transmitted %d\n", nic_driver->total_packet_transmitted());
  assert(false && "nic_init");
}
