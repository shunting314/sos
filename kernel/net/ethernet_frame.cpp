#include <kernel/net/ethernet_frame.h>
#include <kernel/net/byteorder.h>
#include <stdlib.h>
#include <string.h>

EthernetFrame::EthernetFrame(MACAddr dst_mac_addr, uint16_t ether_type, uint8_t *data, int len, bool should_add_crc) : payload_len_(len) {
  frame_.dst_mac_addr_ = dst_mac_addr;
  frame_.src_mac_addr_ = self_mac;
  frame_.ether_type_ = hton(ether_type);
  memmove(frame_.payload_, data, len);

  // pad the either frame to the minimal of 64 bytes (incuding CRC32) with 0
  if (frame_len() + 4 < 64) {
    pad(60 - frame_len());
  }
  if (should_add_crc) {
    add_crc();
  }
}

void EthernetFrame::print() const {
  hexdump(buf_, frame_len());
}
