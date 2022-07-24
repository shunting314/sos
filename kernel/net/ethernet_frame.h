#pragma once

#include <kernel/net/net.h>
#include <kernel/net/crc.h>
#include <kernel/net/byteorder.h>
#include <string.h>

#define PAYLOAD_OFF 14

// the type field in an Ethernet frame
enum class EtherType {
  ARP = 0x806, // host byte order
  IP = 0x800,
};

class EthernetFrame {
 public:
  /*
   * When should_add_crc is false, rely on the NIC to add CRC.
   */
  explicit EthernetFrame(MACAddr dst_mac_addr, uint16_t etherType, uint8_t *data, int len, bool should_add_crc=false);

  void print() const;
  int frame_len() const {
    return PAYLOAD_OFF + payload_len_ + padding_len_ + crc_len_;
  }
  const uint8_t* frame_buf() const {
    return buf_;
  }
  void pad(int len) {
    assert(padding_len_ == 0);
    memset((uint8_t*) frame_buf() + frame_len(), 0, len);
    padding_len_ = len;
  }
  void add_crc() {
    assert(crc_len_ == 0);
    uint32_t crc = crc32(frame_buf(), frame_len());
    *((uint32_t*) (frame_buf() + frame_len())) = crc;
    crc_len_ = 4;
  }

  const uint8_t* payload() const {
    return frame_.payload_;
  }
  
  const MACAddr& dst_mac_addr() const {
    return frame_.dst_mac_addr_;
  }

  const MACAddr& src_mac_addr() const {
    return frame_.src_mac_addr_;
  }

  uint16_t ether_type() const {
    return ntoh(frame_.ether_type_);
  }
 private:
  union {
    struct {
      MACAddr dst_mac_addr_;
      MACAddr src_mac_addr_;
      uint16_t ether_type_; // network byte order
      uint8_t payload_[0];
      // crc32
    } frame_;
    uint8_t buf_[4096]; // TODO how to do zero copy?
  };
  int payload_len_;
  int padding_len_ = 0;
  int crc_len_ = 0;
};
