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

  ARPPacket arp_request = ARPPacket::createRequest(gateway_ip);
  // send more than 8 (transmit RING size) requests for testing. We will succeed to send
  // 12 arp requests out. We are supposed to receive 12 ARP responses.
  // However, because the receive ring size is 8, we can buffer at most 7 packets.
  // The extra 5 packets will be discarded because of buffer overrun.
  for (int i = 0; i < 12; ++i) {
    nic_driver->sync_send(allone_mac, arp_request);
  }
  printf("total packets transmitted %d\n", nic_driver->total_packet_transmitted());

  // wait for an available packet
  while (!nic_driver->unblock_recv()) {
  }
  while (nic_driver->unblock_recv()) {
  }
  assert(gateway_mac != allzero_mac && "gateway mac address uninitialized");
  printf("Gateway MAC address:\n");
  gateway_mac.print();
}
