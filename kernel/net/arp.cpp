#include <kernel/net/arp.h>
#include <kernel/net/net.h>
#include <assert.h>

ARPPacket ARPPacket::createRequest(const IPAddr& dst_ip) {
  ARPPacket packet;
  packet.operation_ = hton((uint16_t) ARP_OP_REQUEST);
  packet.src_hardware_address = self_mac;
  packet.src_protocol_address = self_ip;
  packet.dst_hardware_address = allzero_mac;
  packet.dst_protocol_address = dst_ip;
  return packet;
}
