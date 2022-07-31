#pragma once
#include <kernel/net/arp.h>

class NetCallback {
 public:
  static void on_recv_arp_packet(const ARPPacket& arp_packet);
};
