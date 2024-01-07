#include <kernel/wifi/wifi.h>
#include <kernel/wifi/rtl8188ee.h>
#include <kernel/phys_page.h>
#include <kernel/paging.h>
#include <kernel/sleep.h>
#include <kernel/simfs.h>
#include <stdlib.h>

// follow linux code rtlwifi/wifi.h
struct rtlwifi_firmware_header {
  uint16_t signature;
  uint8_t category;
  uint8_t function;
  uint16_t version;
  uint8_t subversion;
  uint8_t rsvd1;
  uint8_t month;
  uint8_t date;
  uint8_t hour;
  uint8_t minute;
  uint16_t ramcodesize;
  uint16_t rsvd2;
  uint32_t svnindex;
  uint32_t rsvd3;
  uint32_t rsvd4;
  uint32_t rsvd5;
};

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

  void write_nic_reg16(uint32_t off, uint16_t val) {
    *((uint16_t*) (membar_.get_addr() + off)) = val;
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

  // read, update, write
  void update_nic_reg8(uint32_t off, uint8_t mask, uint8_t newval) {
    uint8_t val = read_nic_reg8(off);
    val &= ~mask;
    val |= (newval & mask);
    write_nic_reg8(off, val);
  }

  void poll_nic_reg8(uint32_t off, uint8_t mask, uint8_t expected) {
    int maxitr = 5000;
    bool succeed = false;
    for (int i = 0; i < maxitr; ++i) {
      uint8_t val = read_nic_reg8(off);
      if ((val & mask) == (expected & mask)) {
        succeed = true;
        break;
      }
      msleep(1);
    }
    assert(succeed && "poll_nic_reg8 fail");
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

  // print the mac address
  printf("Mac address:");
  for (int i = 0; i < 6; ++i) {
    printf(" %x", efuse_tbl[EEPROM_MAC_ADDR + i]);
  }
  printf("\n");
}

// follow linux rtl_hal_pwrseqcmdparsing called in _rtl88ee_init_mac
void perform_power_seq(Rtl88eeDriver& driver) {
  // RTL8188EE_TRANS_CARDDIS_TO_CARDEMU
  driver.update_nic_reg8(0x0005, (1 << 3) | (1 << 4), 0);
  // RTL8188EE_TRANS_CARDEMU_TO_ACT
  driver.poll_nic_reg8(0x0006, 1 << 1, 1 << 1);
  driver.update_nic_reg8(0x0002, (1 << 0) | (1 << 1), 0);
  driver.update_nic_reg8(0x0026, (1 << 7), (1 << 7));
  driver.update_nic_reg8(0x0005, (1 << 7), 0);
  driver.update_nic_reg8(0x0005, (1 << 4) | (1 << 3), 0);
  driver.update_nic_reg8(0x0005, 1 << 0, 1 << 0);
  driver.poll_nic_reg8(0x0005, 1 << 0, 0);
  driver.update_nic_reg8(0x0023, 1 << 4, 0);
  printf("Done performing the power sequence!\n");
}

// follow _rtl88ee_init_mac in linux but this function only do part of the job
void set_trx_config(Rtl88eeDriver& driver) {
  uint32_t transmit_config = CFENDFORM | (1 << 15);
  uint32_t receive_config = RCR_APPFCS
    | RCR_APP_MIC
    | RCR_APP_ICV
    | RCR_APP_PHYST_RXFF
    | RCR_HTC_LOC_CTRL
    | RCR_AMF
    | RCR_ACF
    | RCR_ADF
    | RCR_AICV
    | RCR_ACRC32
    | RCR_AB
    | RCR_AM
    | RCR_APM;

  driver.write_nic_reg(REG_RCR, receive_config);
  driver.write_nic_reg16(REG_RXFLTMAP2, 0xffff);
  driver.write_nic_reg(REG_TCR, transmit_config);
}

// follow _rtl88ee_llt_write in linux
// return false for failure
static bool llt_write(Rtl88eeDriver& driver, uint32_t address, uint32_t data) {
  uint32_t value = _LLT_INIT_ADDR(address) | _LLT_INIT_DATA(data) | _LLT_OP(_LLT_WRITE_ACCESS);
  driver.write_nic_reg(REG_LLT_INIT, value);
  for (int i = 0; i < POLLING_LLT_THRESHOLD; ++i) {
    value = driver.read_nic_reg(REG_LLT_INIT);
    if (_LLT_NO_ACTIVE == _LLT_OP_VALUE(value)) {
      return true;
    }
  }
  return false;
}

// follow _rtl88ee_llt_table_init in linux
void llt_table_init(Rtl88eeDriver& driver) {
  uint8_t maxpage = 0xAF;
  uint8_t txpktbuf_bndy = 0xAB;
  bool status;
  driver.write_nic_reg8(REG_RQPN_NPQ, 0x01);
  driver.write_nic_reg(REG_RQPN, 0x80730d29);
  driver.write_nic_reg(REG_TRXFF_BNDY, (0x25FF0000 | txpktbuf_bndy));
  driver.write_nic_reg8(REG_TDECTRL + 1, txpktbuf_bndy);

  driver.write_nic_reg8(REG_TXPKTBUF_BCNQ_BDNY, txpktbuf_bndy); // the possible typo is from linux
  driver.write_nic_reg8(REG_TXPKTBUF_MGQ_BDNY, txpktbuf_bndy);

  driver.write_nic_reg8(0x45D, txpktbuf_bndy);
  driver.write_nic_reg8(REG_PBP, 0x11);
  driver.write_nic_reg8(REG_RX_DRVINFO_SZ, 0x4);

  for (int i = 0; i < (txpktbuf_bndy - 1); i++) {
    status = llt_write(driver, i, i + 1);
    if (!status) {
      goto err;
    }
  }
  status = llt_write(driver, txpktbuf_bndy - 1, 0xFF);
  if (!status) {
    goto err;
  }

  for (int i = txpktbuf_bndy; i < maxpage; i++) {
    status = llt_write(driver, i, i + 1);
    if (!status) {
      goto err;
    }
  }

  status = llt_write(driver, maxpage, txpktbuf_bndy);
  if (!status) {
    goto err;
  }
  return;
err:
  assert(false && "llt_table_init fail");
}

// follow _rtl88ee_init_mac in linux
void init_mac(Rtl88eeDriver& driver) {
  driver.update_nic_reg8(REG_XCK_OUT_CTRL, 1 << 0, 0);
  driver.update_nic_reg8(REG_APS_FSMCO + 1, (1 << 7), 0);
  driver.write_nic_reg8(REG_RSV_CTRL, 0);

  perform_power_seq(driver);

  driver.update_nic_reg8(REG_APS_FSMCO, (1 << 4), (1 << 4));
  driver.update_nic_reg8(REG_PCIE_CTRL_REG + 2, 1 << 2, 1 << 2);
  driver.update_nic_reg8(REG_WATCH_DOG + 1, 1 << 7, 1 << 7);
  driver.update_nic_reg8(REG_AFE_XTAL_CTRL_EXT + 1, (1 << 1), (1 << 1));

  driver.update_nic_reg8(REG_TX_RPT_CTRL, 3, 3); // bit 0 and 1
  driver.write_nic_reg8(REG_TX_RPT_CTRL + 1, 2);
  driver.write_nic_reg16(REG_TX_RPT_TIME, 0xcdf0);

  driver.update_nic_reg8(REG_SYS_CLKR, 1 << 3, 1 << 3);
  driver.update_nic_reg8(REG_GPIO_MUXCFG + 1, 1 << 4, 0);
  driver.write_nic_reg8(0x367, 0x80);

  driver.write_nic_reg16(REG_CR, 0x2ff);
  driver.write_nic_reg8(REG_CR + 1, 0x06);
  driver.write_nic_reg8(MSR, 0x00);

  // this assumes rtlhal->mac_func_enable is false
  llt_table_init(driver);

  driver.write_nic_reg(REG_HISR, 0xffffffff);
  driver.write_nic_reg(REG_HISRE, 0xffffffff);

  uint16_t wordtmp = driver.read_nic_reg16(REG_TRXDMA_CTRL);
  wordtmp &= 0xf;
  wordtmp |= 0xE771;
  driver.write_nic_reg16(REG_TRXDMA_CTRL, wordtmp);

  set_trx_config(driver);

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

  driver.write_nic_reg(REG_INT_MIG, 0);
  driver.write_nic_reg(REG_MCUTST_1, 0x0);
  driver.write_nic_reg8(REG_PCIE_CTRL_REG + 1, 0); // enable RX DMA
}

// follow _rtl88e_enable_fw_download in linux
void enable_fw_download(Rtl88eeDriver& driver, bool enable) {
  if (enable) {
    driver.update_nic_reg8(REG_SYS_FUNC_EN + 1, 0x04, 0x04);
    driver.update_nic_reg8(REG_MCUFWDL, 0x01, 0x01);
    driver.update_nic_reg8(REG_MCUFWDL + 2, 0x08, 0); // reg & 0xf7
  } else {
    driver.update_nic_reg8(REG_MCUFWDL, 0x01, 0); // reg & 0xfe
    driver.write_nic_reg8(REG_MCUFWDL + 1, 0);
  }
}

#define START_ADDRESS 0x1000

// follow _rtl88e_write_fw in linux
void write_fw(Rtl88eeDriver& driver, uint16_t version, uint8_t *data, uint32_t size) {
  assert(size % 4 == 0); // if size % 4 == 0 skip rtl_fill_dummy which pad the data ith 0 until the size is a multiple of 4

  assert(size <= 8 * 0x1000); // at most 8 pages
  for (int pageno = 0; pageno * 0x1000 < size; ++pageno) {
    uint8_t *ptr = data + pageno * 0x1000;
    int blocksize = min(0x1000, size - pageno * 0x1000);

    driver.update_nic_reg8(REG_MCUFWDL + 2, 0x7, pageno);

    for (int i = 0; i < blocksize; ++i) {
      driver.write_nic_reg8(START_ADDRESS + i, ptr[i]);
    }
  }
}

// follow _rtl88e_fw_free_to_go in linux
// Check if the firmware is ready to do.
void fw_free_to_go(Rtl88eeDriver& driver) {
  driver.poll_nic_reg8(REG_MCUFWDL, FWDL_CHKSUM_RPT, FWDL_CHKSUM_RPT);
  driver.update_nic_reg8(REG_MCUFWDL, MCUFWDL_RDY | WINTINI_RDY, MCUFWDL_RDY);
  { // firmware_selfreset
    uint8_t byte = driver.read_nic_reg8(REG_SYS_FUNC_EN + 1);
    driver.write_nic_reg8(REG_SYS_FUNC_EN + 1, byte & ~(1 << 2));
    driver.write_nic_reg8(REG_SYS_FUNC_EN + 1, byte | (1 << 2));
  }
  driver.poll_nic_reg8(REG_MCUFWDL, WINTINI_RDY, WINTINI_RDY);
}

// mostly follow linux function rtl88e_download_fw in rtl8188ee/fw.c
void download_fw(Rtl88eeDriver& driver) {
  int firmware_size;
  uint8_t* firmware_buf = SimFs::get().readFile("rtl8188efw.bin", &firmware_size);
  printf("firmware size %d\n", firmware_size);

  struct rtlwifi_firmware_header* pfwheader = (struct rtlwifi_firmware_header*) firmware_buf;
  uint16_t version = pfwheader->version;
  uint8_t subversion = pfwheader->subversion;
  printf("version %d, subversion %d\n", version, subversion);

  // check the signature
  assert(pfwheader->signature == 0x88E1);

  uint8_t* firmware_data = firmware_buf + sizeof(struct rtlwifi_firmware_header);
  firmware_size -= sizeof(struct rtlwifi_firmware_header);

  uint8_t reg_mcufwdl = driver.read_nic_reg8(REG_MCUFWDL);
  assert((reg_mcufwdl & (1 << 7)) == 0); // no need to do selfreset

  enable_fw_download(driver, true);
  write_fw(driver, version, firmware_data, firmware_size);
  enable_fw_download(driver, false);

  fw_free_to_go(driver);

  // free the buffer
  free(firmware_buf);
}

// follow rtl88ee_hw_init in linux
void hw_init(Rtl88eeDriver& driver) {
  uint8_t sys_clkr_p1 = driver.read_nic_reg8(REG_SYS_CLKR + 1);
  uint8_t reg_cr = driver.read_nic_reg8(REG_CR);
  bool mac_func_enable;
  if ((sys_clkr_p1 & (1 << 3)) && (reg_cr != 0 && reg_cr != 0xEA)) {
    mac_func_enable = true;
  } else {
    mac_func_enable = false;
  }
  assert(!mac_func_enable); // it's false for my 8188ee. Only support this case right now.
  init_mac(driver);
  download_fw(driver);

  assert(false && "hw_init nyi");
}

void wifi_init() {
  if (!wifi_nic_pci_func) {
    printf("No wifi nic found\n");
    return;
  }

  // only support RTL88EE right now.
  assert(wifi_nic_pci_func.vendor_id() == PCI_VENDOR_REALTEK);
  assert(wifi_nic_pci_func.device_id() == PCI_DEVICE_RTL88EE);

  printf("RTL88EE interrupt line %d, pin %d\n", wifi_nic_pci_func.interrupt_line(), wifi_nic_pci_func.interrupt_pin());

  // enable bus master
  wifi_nic_pci_func.enable_bus_master();

  // according to linux rtl88ee driver, bar with index 2 is used
  Bar membar = wifi_nic_pci_func.getBar(2);
  map_region((phys_addr_t) kernel_page_dir, membar.get_addr(), membar.get_addr(), membar.get_size(), MAP_FLAG_WRITE);

  Rtl88eeDriver driver(wifi_nic_pci_func, membar);
  driver.enable_interrupt();

  uint8_t reg_9346cr = driver.read_nic_reg8(REG_9346CR);

  // follow linux rtl88ee_read_eeprom_info
  {
    assert((reg_9346cr & (1 << 4)) == 0 && "Boot from EFUSE rather than EEPROM");
    assert(reg_9346cr & (1 << 5)); // autoload ok.
  }

  efuse_power_switch(driver); // follow linux
  read_efuse(driver);

  hw_init(driver);

  // TODO these are debugging code that should be removed later
  int idx = 0;
  while (idx < 15) {
    printf("idx %d, own %d\n", idx++, rx_ring[driver.rxidx].own);
    msleep(2000);
  }

  safe_assert(false && "found wifi nic");
}

