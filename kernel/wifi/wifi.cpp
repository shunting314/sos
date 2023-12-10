#include <kernel/wifi/wifi.h>
#include <kernel/wifi/rtl8188ee.h>
#include <kernel/phys_page.h>
#include <kernel/paging.h>
#include <kernel/sleep.h>
#include <stdlib.h>

// each rx ring contains this many descriptors
#define RTL_PCI_MAX_RX_COUNT 512
uint32_t rxbuffersize = 4096; // linux define this as 9100. I define it as 4096 for now so a single page suffice.

PCIFunction wifi_nic_pci_func;

// specific to rtl88ee
// for each RxDesc, here are the fields that may be set in Linux
// - HW_DESC_RXBUFF_ADDR
// - HW_DESC_RXPKT_LEN
// - HW_DESC_RXOWN
// - HW_DESC_RXERO
struct RxDesc {
  uint32_t pkt_len : 14;
  uint32_t dummy_fields: 16;
  uint32_t eor: 1; // end of ring?
  uint32_t own: 1;
  uint32_t dummy[5];
  uint32_t buff_addr;
  uint32_t dummy2;
};

RxDesc rx_ring[RTL_PCI_MAX_RX_COUNT] __attribute__((aligned(32)));

static_assert(sizeof(RxDesc) == 32);

// For each TxDesc, here are the fields that may be set in Linux
// - HW_DESC_OWN
// - HW_DESC_TX_NEXTDESC_ADDR
struct TxDesc {
  uint32_t dummy0: 31;
  uint32_t own: 1;
  uint32_t dummy1[9];
  uint32_t next_desc_address;
  uint32_t dummy2[5];
};

// linux defines 9 tx rings
// - BEACON_QUEUE has size 2;
// - BE_QUEUE has size 256;
// - all others have size 128
TxDesc tx_ring_beacon[2];
TxDesc tx_ring_mgmt[128];
TxDesc tx_ring_voq[128];
TxDesc tx_ring_viq[128];
TxDesc tx_ring_beq[256];
TxDesc tx_ring_bkq[128];
TxDesc tx_ring_hq[128];

static_assert(sizeof(TxDesc) == 64);

class Rtl88eeDriver {
 public:
  Rtl88eeDriver(PCIFunction func, Bar membar) : func_(func), membar_(membar) { }

  void write_nic_reg(uint32_t off, uint32_t val) {
    *get_nic_reg_ptr(off) = val;
  }

  void write_nic_reg8(uint32_t off, uint8_t val) {
    *((uint8_t*) (membar_.get_addr() + off)) = val;
  }

  uint32_t read_nic_reg(uint32_t off) {
    return *((uint32_t*) (membar_.get_addr() + off));
  }

  uint8_t read_nic_reg8(uint32_t off) {
    return *((uint8_t*) (membar_.get_addr() + off));
  }

  uint16_t read_nic_reg16(uint32_t off) {
    return *((uint16_t*) (membar_.get_addr() + off));
  }

  uint32_t* get_nic_reg_ptr(uint32_t off) {
    return (uint32_t*) (membar_.get_addr() + off);
  }

  uint32_t initializeRxRing();
  uint32_t initializeTxRing(TxDesc* ring, int size);

  uint32_t rxidx = 0;

  void enable_interrupt();
 private:
  PCIFunction func_;
  Bar membar_;
};

uint32_t Rtl88eeDriver::initializeRxRing() {
  uint32_t ret = (uint32_t) rx_ring;
  assert((ret & 31) == 0);

  RxDesc* desc = nullptr;

  for (int i = 0; i < RTL_PCI_MAX_RX_COUNT; ++i) {
    desc = &rx_ring[i];
    desc->pkt_len = rxbuffersize;
    assert(rxbuffersize <= 4096);
    desc->own = 1;
    desc->buff_addr = alloc_phys_page();
  }

  // set eor for the last desc
  desc->eor = 1;

  return ret;
}

uint32_t Rtl88eeDriver::initializeTxRing(TxDesc* ring, int size) {
  uint32_t ret = (uint32_t) ring;
  uint32_t nextdescaddress;
  assert((ret & 31) == 0);

  for (int i = 0; i < size; ++i) {
    nextdescaddress = ret + (i + 1) % size * sizeof(*ring);
    ring[i].next_desc_address = nextdescaddress;
  }
  return ret;
}

void Rtl88eeDriver::enable_interrupt() {
  uint32_t irq_mask_0 = (
    IMR_PSTIMEOUT |
    IMR_HSISR_IND_ON_INT |
    IMR_C2HCMD |
    IMR_HIGHDOK |
    IMR_MGNTDOK |
    IMR_BKDOK |
    IMR_BEDOK |
    IMR_VIDOK |
    IMR_VODOK |
    IMR_RDU |
    IMR_ROK |
    0
  );
  uint32_t irq_mask_1 = IMR_RXFOVW;
  uint32_t sys_irq_mask = (HSIMR_RON_INT_EN | HSIMR_PDN_INT_EN);

  write_nic_reg(REG_HIMR, irq_mask_0);
  write_nic_reg(REG_HIMRE, irq_mask_1);
  write_nic_reg8(REG_C2HEVT_CLEAR, 0);
  // enable system interrupt
  write_nic_reg(REG_HSIMR, sys_irq_mask);
}

void efuse_power_switch(Rtl88eeDriver& driver) {
  // write EFUSE_ACCESS
  driver.write_nic_reg8(REG_EFUSE_ACCESS, 0x69);

  // read SYS_FUNC_EN
  uint16_t sys_func_en = driver.read_nic_reg16(REG_SYS_FUNC_EN);
  assert(sys_func_en & FEN_ELDR);

  uint16_t clk = driver.read_nic_reg16(REG_SYS_CLKR);
  assert((clk & LOADER_CLK_EN) && (clk & ANA8M));
}

// follow linux
uint8_t read_efuse_byte(Rtl88eeDriver& driver, uint16_t offset) {
  uint32_t value32;
  uint8_t readbyte;
  uint16_t retry;

  driver.write_nic_reg8(REG_EFUSE_CTRL + 1, offset & 0xff);
  readbyte = driver.read_nic_reg8(REG_EFUSE_CTRL + 2);
  driver.write_nic_reg8(REG_EFUSE_CTRL + 2, ((offset >> 8) & 0x03) | (readbyte & 0xfc));
  readbyte = driver.read_nic_reg8(REG_EFUSE_CTRL + 3);
  driver.write_nic_reg8(REG_EFUSE_CTRL + 3, readbyte & 0x7f);

  retry = 0;
  value32 = driver.read_nic_reg(REG_EFUSE_CTRL);
  while (!(value32 & 0x80000000) && (retry < 10000)) {
    value32 = driver.read_nic_reg(REG_EFUSE_CTRL);
    ++retry;
  }
  msleep(1); // TODO: 1 ms is too much. Support sleeping for microseconds
  value32 = driver.read_nic_reg(REG_EFUSE_CTRL);
  return value32 & 0xff;
}

// follow linux
void read_efuse(Rtl88eeDriver& driver) {
  uint16_t efuse_addr = 0;
  uint32_t efuse_len = 256; // EFUSE_REAL_CONTENT_LEN
  uint8_t ebyte = read_efuse_byte(driver, 0);
  uint8_t offset, wren;
  const uint16_t efuse_max_section = 64;
  #define EFUSE_MAX_WORD_UNIT 4
  uint16_t efuse_word[EFUSE_MAX_WORD_UNIT][efuse_max_section];
  #define HWSET_MAX_SIZE 512
  uint8_t efuse_tbl[HWSET_MAX_SIZE] = {0};
  uint8_t u1temp = 0;

  assert(ebyte != 0xff);
  ++efuse_addr;

  for (int i = 0; i < efuse_max_section; ++i) {
    for (int j = 0; j < EFUSE_MAX_WORD_UNIT; ++j) {
      efuse_word[j][i] = 0xFFFF;
    }
  }

  while (ebyte != 0xFF && efuse_addr < efuse_len) {
    if ((ebyte & 0x1F) == 0x0F) { /* extended header */
      u1temp = ((ebyte & 0xE0) >> 5);
      ebyte = read_efuse_byte(driver, efuse_addr);

      if ((ebyte & 0x0F) == 0x0F) {
        efuse_addr++;
        ebyte = read_efuse_byte(driver, efuse_addr);

        if (ebyte != 0xFF && efuse_addr < efuse_len) {
          ++efuse_addr;
        }
        continue;
      } else {
        offset = ((ebyte & 0xF0) >> 1) | u1temp;
        wren = (ebyte & 0x0F);
        efuse_addr++;
      }
    } else {
      offset = ((ebyte >> 4) & 0x0f);
      wren = (ebyte & 0x0f);
    }

    if (offset < efuse_max_section) {
      for (int i = 0; i < EFUSE_MAX_WORD_UNIT; ++i) {
        if (!(wren & 0x01)) {
          ebyte = read_efuse_byte(driver, efuse_addr);
          ++efuse_addr;
          efuse_word[i][offset] = (ebyte & 0xff);

          if (efuse_addr >= efuse_len) {
            break;
          }

          ebyte = read_efuse_byte(driver, efuse_addr);
          ++efuse_addr;
          efuse_word[i][offset] |= (((uint16_t) ebyte << 8) & 0xff00);

          if (efuse_addr >= efuse_len) {
            break;
          }
        }

        wren >>= 1;
      }
    }
    ebyte = read_efuse_byte(driver, efuse_addr);
    if (ebyte != 0xff && (efuse_addr < efuse_len)) {
      efuse_addr++;
    }
  }

  for (int i = 0; i < efuse_max_section; ++i) {
    for (int j = 0; j < EFUSE_MAX_WORD_UNIT; ++j) {
      efuse_tbl[(i * 8) + (j * 2)] = (efuse_word[j][i] & 0xff);
      efuse_tbl[(i * 8) + ((j * 2) + 1)] = ((efuse_word[j][i] >> 8) & 0xff);
    }
  }

  // dump the first 256 bytes
  hexdump(efuse_tbl, 256);
}

void wifi_init() {
  if (!wifi_nic_pci_func) {
    printf("No wifi nic found\n");
    return;
  }

  // only support RTL88EE right now.
  assert(wifi_nic_pci_func.vendor_id() == PCI_VENDOR_REALTEK);
  assert(wifi_nic_pci_func.device_id() == PCI_DEVICE_RTL88EE);

  // enable bus master
  wifi_nic_pci_func.enable_bus_master();

  // according to linux rtl88ee driver, bar with index 2 is used
  Bar membar = wifi_nic_pci_func.getBar(2);
  map_region((phys_addr_t) kernel_page_dir, membar.get_addr(), membar.get_addr(), membar.get_size(), MAP_FLAG_WRITE);

  Rtl88eeDriver driver(wifi_nic_pci_func, membar);
  driver.write_nic_reg(REG_RX_DESA, driver.initializeRxRing());
#define INIT_TX_RING(tx_ring) driver.initializeTxRing(tx_ring, sizeof(tx_ring) / sizeof(tx_ring[0]))
  driver.write_nic_reg(REG_BCNQ_DESA, INIT_TX_RING(tx_ring_beacon));
  driver.write_nic_reg(REG_MGQ_DESA, INIT_TX_RING(tx_ring_mgmt));
  driver.write_nic_reg(REG_VOQ_DESA, INIT_TX_RING(tx_ring_voq));
  driver.write_nic_reg(REG_VIQ_DESA, INIT_TX_RING(tx_ring_viq));
  driver.write_nic_reg(REG_BEQ_DESA, INIT_TX_RING(tx_ring_beq));
  driver.write_nic_reg(REG_BKQ_DESA, INIT_TX_RING(tx_ring_bkq));
  driver.write_nic_reg(REG_HQ_DESA, INIT_TX_RING(tx_ring_hq));
#undef INIT_TX_RING

  // follow linux _rtl88ee_init_mac to enable RX DMA
  driver.write_nic_reg8(REG_PCIE_CTRL_REG + 1, 0);

  driver.enable_interrupt();

  uint8_t reg_9346cr = driver.read_nic_reg8(REG_9346CR);

  // follow linux rtl88ee_read_eeprom_info
  {
    assert((reg_9346cr & (1 << 4)) == 0 && "Boot from EFUSE rather than EEPROM");
    assert(reg_9346cr & (1 << 5)); // autoload ok.
  }

  efuse_power_switch(driver); // follow linux
  read_efuse(driver);

  // TODO these are debugging code that should be removed later
  int idx = 0;
  while (true) {
    printf("idx %d, own %d\n", idx++, rx_ring[driver.rxidx].own);
    dumbsleep(500);
    break; // TODO
  }

  safe_assert(false && "found wifi nic");
}
