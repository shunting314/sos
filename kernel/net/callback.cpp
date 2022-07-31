#include <kernel/net/callback.h>

void NetCallback::on_recv_arp_packet(const ARPPacket& arp_packet) {
  // only handle arp reply regarding gateway IP address for now.
  if (arp_packet.operation() == ARP_OP_REPLY && arp_packet.src_protocol_address == gateway_ip) {
    printf("Set gateway mac.\n");
    gateway_mac = arp_packet.src_hardware_address;
  }
}
