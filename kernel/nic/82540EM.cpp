#include <kernel/nic/82540EM.h>
#include <kernel/net/ethernet_frame.h>
#include <kernel/net/arp.h>
#include <kernel/paging.h>
#include <kernel/phys_page.h>
#include <assert.h>
#include <stdlib.h>

static int recv_cnt = 0;

// 82540EM is little endian
struct LegacyTransmitDescriptor {
  uint32_t buffer_address_low;
  uint32_t buffer_address_high;

  uint16_t length;
  uint8_t CSO;

  // CMD byte
  uint8_t CMD_EOP : 1;
  uint8_t CMD_IFCS : 1;
  uint8_t CMD_IC : 1;
  uint8_t CMD_RS : 1;
  uint8_t CMD_RSV : 1;
  uint8_t CMD_DEXT : 1;
  uint8_t CMD_VLE : 1;
  uint8_t CMD_IDE : 1;

  uint8_t STA_DD : 1;
  uint8_t STA_EC : 1;
  uint8_t STA_LC : 1;
  uint8_t STA_RSV : 1;

  uint8_t RSV : 4;
  uint8_t CSS;
  uint16_t SPECIAL;

  void print() {
    printf("TD STA_DD %d, STA_EC %d, STA_LC %d, STA_RSV %d, addr low 0x%x\n", STA_DD, STA_EC, STA_LC, STA_RSV, buffer_address_low);
  }
};
static_assert(sizeof(LegacyTransmitDescriptor) == 16);


struct ReceiveDescriptor {
  uint32_t buffer_address_low;
  uint32_t buffer_address_high;
  uint16_t length;
  uint16_t packet_checksum;
  // status byte
  uint8_t ST_DD : 1;
  uint8_t ST_EOP : 1;
  uint8_t ST_reset : 6;
  uint8_t errors;
  uint16_t special;
  void print() {
    assert(buffer_address_high == 0);
    printf("RD addr 0x%x, length 0x%x, packet chksum 0x%x, errors 0x%x, special 0x%x\n", buffer_address_low, length, packet_checksum, errors, special);
    printf("  DD %d, EOP %d\n", ST_DD, ST_EOP);
  }
};
static_assert(sizeof(ReceiveDescriptor) == 16);

// TODO: NOTE we only use the first 8 transmit/receive descriptor in the allocated physical page for now. Make this more flexible.
#define RING_SIZE 8
LegacyTransmitDescriptor* transmit_descriptor_ring;
ReceiveDescriptor* receive_descriptor_ring;

void NICDriver_82540EM::dump_transmit_descriptor_regs() {
  printf("REG_OFF_TDBAL %x\n", read_nic_register(REG_OFF_TDBAL));
  printf("REG_OFF_TDBAH %x\n", read_nic_register(REG_OFF_TDBAH));
  printf("REG_OFF_TDLEN %x\n", read_nic_register(REG_OFF_TDLEN));
  printf("REG_OFF_TDH %x\n", read_nic_register(REG_OFF_TDH));
  printf("REG_OFF_TDT %x\n", read_nic_register(REG_OFF_TDT));
}

void NICDriver_82540EM::dump_receive_descriptor_regs() {
  printf("REG_OFF_RDLEN 0x%x\n", read_nic_register(REG_OFF_RDLEN));
  printf("REG_OFF_RDH %x\n", read_nic_register(REG_OFF_RDH));
  printf("REG_OFF_RDT %x\n", read_nic_register(REG_OFF_RDT));
}

// check '82540EM SDM: 5.3.1 Software Access' for how to access a word in EEPROM
uint16_t NICDriver_82540EM::read_eeprom_word(int woff) {
  /*
   * It's necessary to set eerd_cont.data to 0 in order to read the correct content
   * from EEPROM.
   * We can do that by 'EERD_struct eerd_cont = {0};' which init all fields to 0;
   * or more specifically set 'eerd_cont.data' to 0 by 'eerd_cont.data = 0;'.
   * The manual does not mention that we need set eerd_cont.data to 0 for reading
   * EEPROM though!
   */
  EERD_struct eerd_cont;
  eerd_cont.data = 0;
  eerd_cont.addr = woff;
  eerd_cont.start = 1;

  write_nic_register(REG_OFF_EERD, regStructToUint32(eerd_cont));
  while (true) {
    volatile uint32_t val = read_nic_register(REG_OFF_EERD);
    eerd_cont = uint32ToRegStruct<EERD_struct>(val);
    if (eerd_cont.done) {
      break;
    }
  }
  return eerd_cont.data;
}

uint32_t NICDriver_82540EM::read_nic_register(int off) {
  assert((off & 3) == 0);
  // take advantage of the fact that we setup identity map in NICDriver_82540EM::init
  auto addr = (uint32_t*) (membar_.get_addr() + off);
  return *addr;
}

void NICDriver_82540EM::write_nic_register(int off, uint32_t val) {
  assert((off & 3) == 0);
  // take advantage of the fact that we setup identity map in NICDriver_82540EM::init
  auto addr = (uint32_t*) (membar_.get_addr() + off);
  *addr = val;
}

void NICDriver_82540EM::init_receive_descriptor_ring() {
  // set receive address
  write_nic_register(REG_OFF_RAL0, *((const uint32_t*) mac_addr_.get_addr()));
  write_nic_register(REG_OFF_RAH0, *((const uint16_t*) (mac_addr_.get_addr() + 4)) | 0x80000000);

  receive_descriptor_ring = (ReceiveDescriptor*) alloc_phys_page();
  memset(receive_descriptor_ring, 0, PAGE_SIZE);
  for (int i = 0; i < RING_SIZE; ++i) {
    receive_descriptor_ring[i].buffer_address_low = alloc_phys_page();
  }
  write_nic_register(REG_OFF_RDBAL, (uint32_t) receive_descriptor_ring);
  write_nic_register(REG_OFF_RDBAH, 0);
  write_nic_register(REG_OFF_RDLEN, RING_SIZE * sizeof(ReceiveDescriptor)); // size in bytes

  write_nic_register(REG_OFF_RDH, 0);
  write_nic_register(REG_OFF_RDT, RING_SIZE - 1);

  dump_receive_descriptor_regs();
  printf("RCTL 0x%x\n", read_nic_register(REG_OFF_RCTL));
  write_nic_register(REG_OFF_RCTL, read_nic_register(REG_OFF_RCTL) | 0x2); // EN
  printf("RCTL 0x%x\n", read_nic_register(REG_OFF_RCTL));
}

void NICDriver_82540EM::init_transmit_descriptor_ring() {
  transmit_descriptor_ring = (LegacyTransmitDescriptor*) alloc_phys_page();
  memset((void *) transmit_descriptor_ring, 0 , PAGE_SIZE);
  for (int i = 0; i < RING_SIZE; ++i) {
    transmit_descriptor_ring[i].buffer_address_low = alloc_phys_page();
  }

  write_nic_register(REG_OFF_TDBAL, (uint32_t) transmit_descriptor_ring);
  write_nic_register(REG_OFF_TDBAH, 0);
  write_nic_register(REG_OFF_TDLEN, RING_SIZE * sizeof(LegacyTransmitDescriptor)); // size in bytes

  // TDH/TDT use sizeof(LegacyTransmitDescriptor) as unit according to the
  // formula to compute descriptor address in 3.4 of the SDM.
  write_nic_register(REG_OFF_TDH, 0);
  write_nic_register(REG_OFF_TDT, 0);

  dump_transmit_descriptor_regs();

  printf("TCTL 0x%x\n", read_nic_register(REG_OFF_TCTL));
  write_nic_register(REG_OFF_TCTL,
    read_nic_register(REG_OFF_TCTL) | 0x2); // enable transmit
  printf("TCTL 0x%x\n", read_nic_register(REG_OFF_TCTL));
}

/*
 * According to '82540EM SDM: 4.1 PCI Configuration', 82540EM BAR 0 (if exist)
 * is the memory register, BAR 2 (if exist) is the IO register.
 */
void NICDriver_82540EM::init() {
  assert(pci_func_);
  // enable the bus master bit so the device can do DMA
  pci_func_.enable_bus_master();

  Bar membar = pci_func_.getBar(0);
  Bar iobar = pci_func_.getBar(2);

  printf("82540EM Bars:\n");
  membar.print();
  iobar.print();
  assert(membar && !iobar && "We can only handle membar for now and we assume iobar does not exist");

  membar_ = membar;

  // map the membar to kernel pgdir
  // TODO: the address is in quite high space. In theory the linear address region
  // mapped here may be conflict with user app. Here are a few alternatives
  // 1. reserve some linear address regions in kernel space permanetly for this
  // 2. reserve a few temp virtual pages in kernel. Map on demand and unmap after
  //    done.
  map_region((phys_addr_t) kernel_page_dir, membar.get_addr(), membar.get_addr(), membar.get_size(), MAP_FLAG_WRITE /* kernel RW */);

  mac_addr_ = MACAddr(
    read_eeprom_word(EEPROM_OFF_ENADDR_0),
    read_eeprom_word(EEPROM_OFF_ENADDR_1),
    read_eeprom_word(EEPROM_OFF_ENADDR_2)
  );

  init_transmit_descriptor_ring();
  init_receive_descriptor_ring();
}

/*
 * NIC increment TDH after processed a packet.
 * Software increment TDT after adding a packet to the ring.
 * 'TDH == TDT' indicate an empty ring.
 * TO avoid confusing the case of full and empty ring, we block if RING_SIZE - 1
 * descriptors are in use. That means, there is always at least 1 descriptor
 * not used.
 *
 * TDH, TDT are initialized to 0, 0
 */
void NICDriver_82540EM::sync_send(MACAddr dst_mac_addr, uint16_t etherType, uint8_t *data, int len) {
  EthernetFrame frame(dst_mac_addr, etherType, data, len);
  #if 0
  printf("Dump frame:\n");
  frame.print();
  #endif

  uint32_t tail_off = read_nic_register(REG_OFF_TDT);
  while (true) {
    uint32_t head_off = read_nic_register(REG_OFF_TDH);
    if ((tail_off + 1) % RING_SIZE != head_off) {
      break;
    }
  }
  LegacyTransmitDescriptor* picked_desc = (LegacyTransmitDescriptor*) (transmit_descriptor_ring + tail_off);

  // move the frame
  assert(frame.frame_len() <= PAGE_SIZE);
  memmove((void *) picked_desc->buffer_address_low, frame.frame_buf(), frame.frame_len());
  picked_desc->length = frame.frame_len();
  picked_desc->CMD_RS = 1;
  picked_desc->CMD_EOP = 1;
  picked_desc->CMD_IFCS = 1;

  write_nic_register(REG_OFF_TDT, (tail_off + 1) % RING_SIZE);

  while (!picked_desc->STA_DD) {
  }
}

/*
 * 'RDH == RDT' indicates an empty FIFO and hardware starts dropping packets in this case.
 * Hardward increment RDH after putting an packet to the FIFO;
 * Software increment RDT after consuming an packet from the FIFO.
 *
 * RDH and RDT are intitialized to 0, RING_SIZE - 1
 */
void NICDriver_82540EM::sync_recv() {
  uint32_t tail_off = read_nic_register(REG_OFF_RDT);

  dump_receive_descriptor_regs();
  printf("Waiting for incoming packets...\n");
  while (true) {
    uint32_t head_off = read_nic_register(REG_OFF_RDH);
    if ((tail_off + 1) % RING_SIZE != head_off) {
      break;
    }
  }
  tail_off = (tail_off + 1) % RING_SIZE;
  printf("(recv seq %d) Receive descriptor at %d should have data!\n", recv_cnt++, tail_off);
  ReceiveDescriptor* picked_desc = (ReceiveDescriptor*) (receive_descriptor_ring + tail_off);
  hexdump((uint8_t*) picked_desc->buffer_address_low, 80);
  picked_desc->print();
  if (picked_desc->length < PAYLOAD_OFF) {
    printf("Drop too short packet: %d\n", picked_desc->length);
  } else {
    EthernetFrame* ethframe = (EthernetFrame*) picked_desc->buffer_address_low;
    ethframe->dst_mac_addr().print();
    ethframe->src_mac_addr().print();
    auto ethType = (EtherType) ethframe->ether_type();
    printf("ether type 0x%x\n", ethType);
    int rest_length = picked_desc->length - PAYLOAD_OFF;
    if (ethType == EtherType::ARP) {
      ARPPacket* arp = (ARPPacket *) ethframe->payload();
      if (rest_length < sizeof(ARPPacket)) {
        printf("Not enough payload for ARP packet.\n");
      } else {
        arp->print();
      }
    }
  }
  ((uint64_t*) picked_desc)[1] = 0; // clear the recv descriptor except the address part
  write_nic_register(REG_OFF_RDT, tail_off);
}
