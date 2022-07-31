#pragma once

#include <stdint.h>
#include <kernel/net/net.h>
#include <kernel/net/byteorder.h>
#include <kernel/net/ethernet_frame.h>

enum {
  HTYPE_ETHERNET = 1, // indicate an ethernet MAC address
};

enum {
  PTYPE_IP = 0x0800, // indicate an IP address
};

enum {
  ARP_OP_REQUEST = 1,
  ARP_OP_REPLY = 2,
};

// multi-byte fields are in network byte order.
struct ARPPacket {
  uint16_t hardward_type_ = hton((uint16_t) HTYPE_ETHERNET);
  uint16_t protocol_type_ = hton((uint16_t) PTYPE_IP);
  uint8_t hardware_address_length_ = 6; // 6 for ethernet MAC address
  uint8_t protocol_address_length_ = 4; // 4 for IP address
  uint16_t operation_;

  // src hardware address. Assumes an ethernet MAC address
  MACAddr src_hardware_address;

  // src protocol address. Assumes an IPv4 address
  IPAddr src_protocol_address;

  // dst hardware address
  MACAddr dst_hardware_address; // all 0 for an ARP Request

  // dst protocol address
  IPAddr dst_protocol_address;

  static ARPPacket createRequest(const IPAddr& dst_ip);

  static EtherType getEtherType() {
    return EtherType::ARP;
  }

  uint16_t operation() const {
    return ntoh(operation_);
  }

  const char* typeStr() const {
    switch (operation()) {
      case ARP_OP_REQUEST: return "REQUEST";
      case ARP_OP_REPLY: return "REPLY";
      default:
        break;
    }
    assert(false && "Invalid operation");
    return "";
  }

  void print() {
    printf("ARP '%s'\n", typeStr());
    src_hardware_address.print();
    src_protocol_address.print();
    dst_hardware_address.print();
    dst_protocol_address.print();
  }
};

static_assert(sizeof(ARPPacket) == 28);
