#include <kernel/wifi/wifi.h>
#include <kernel/wifi/rtl8188ee.h>
#include <kernel/phys_page.h>
#include <kernel/paging.h>
#include <kernel/sleep.h>
#include <kernel/simfs.h>
#include <stdlib.h>

#define MAIN_ANT 0

#define RFREG_OFFSET_MASK 0xfffff
#define MASKDWORD 0xffffffff
#define MASK12BITS 0xfff
#define MASKLWORD 0x0000ffff
#define ETH_ALEN 6
#define IQK_ADDA_REG_NUM 16
#define IQK_MAC_REG_NUM 4
#define IQK_BB_REG_NUM 9

#define MAX_TOLERANCE 5

enum _ANT_DIV_TYPE {
  CGCS_RX_HW_ANTDIV = 0x02,
  // skip the rest for now
};

enum rt_oem_id {
  RT_CID_DEFAULT = 0,
  // skip others for now
};

enum radio_path {
  RF90_PATH_A = 0,
  RF90_PATH_B = 1,
  RF90_PATH_C = 2,
  RF90_PATH_D = 3,
};

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

// follow the definition in linux rtlwifi/wifi.h
struct bb_reg_def {
  uint32_t rfintfs;
  uint32_t rfintfi;
  uint32_t rfintfo;
  uint32_t rfintfe;
  uint32_t rf3wire_offset;
  uint32_t rflssi_select;
  uint32_t rftxgain_stage;
  uint32_t rfhssi_para1;
  uint32_t rfhssi_para2;
  uint32_t rfsw_ctrl;
  uint32_t rfagc_control1;
  uint32_t rfagc_control2;
  uint32_t rfrxiq_imbal;
  uint32_t rfrx_afe;
  uint32_t rftxiq_imbal;
  uint32_t rftx_afe;
  uint32_t rf_rb;  /* rflssi_readback */
  uint32_t rf_rbpi;  /* rflssi_readbackpi */
};

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
 public:
  // keep this field here since it's initialized and used in set_trx_config and being
  // used again in hw_init
  uint32_t receive_config;
  uint8_t eeprom_oemid;
  uint16_t eeprom_did;
  uint16_t eeprom_svid;
  uint16_t eeprom_smid;
  uint32_t oem_id;
  struct bb_reg_def path_a_phyreg;
  uint8_t mac_addr[ETH_ALEN];
  uint8_t antenna_div_type;
  bool hal_state = false; // stopped state
  uint32_t adda_backup[IQK_ADDA_REG_NUM];
  uint32_t iqk_mac_backup[IQK_MAC_REG_NUM];
  uint32_t iqk_bb_backup[IQK_BB_REG_NUM];
  uint8_t rfpi_enable;
  uint8_t reg_bcn_ctrl_val = 0;
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

// follow efuse_read_1byte in linux
// not sure why linux does not consolidate efuse_read_1byte and read_efuse_byte
uint8_t efuse_read_1byte(Rtl88eeDriver& driver, uint16_t address) {
  const uint32_t efuse_len = 256; // EFUSE_REAL_CONTENT_LEN
  uint32_t k = 0;
  if (address < efuse_len) {
    uint8_t temp = address & 0xFF;
    driver.write_nic_reg8(REG_EFUSE_CTRL + 1, temp);
    uint8_t bytetemp = driver.read_nic_reg8(REG_EFUSE_CTRL + 2);
    temp = ((address >> 8) & 0x03) | (bytetemp & 0xFC);
    driver.write_nic_reg8(REG_EFUSE_CTRL + 2, temp);

    bytetemp = driver.read_nic_reg8(REG_EFUSE_CTRL + 3);
    temp = bytetemp & 0x7F;
    driver.write_nic_reg8(REG_EFUSE_CTRL + 3, temp);

    bytetemp = driver.read_nic_reg8(REG_EFUSE_CTRL + 3);
    while (!(bytetemp & 0x80)) {
      bytetemp = driver.read_nic_reg8(REG_EFUSE_CTRL + 3);
      ++k;
      if (k == 1000) {
        break;
      }
    }
    return driver.read_nic_reg8(REG_EFUSE_CTRL);
  } else {
    return 0xFF;
  }
}

// follow read_efuse_byte in linux
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

// follow read_efuse in linux
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
    driver.mac_addr[i] = efuse_tbl[EEPROM_MAC_ADDR + i];
  }
  printf("\n");
  uint16_t eeprom_id = *(uint16_t*) efuse_tbl;
  printf("eeprom_id is 0x%x (should equal to 0x8129)\n", eeprom_id);
  assert(eeprom_id == 0x8129);

  #define EEPROM_CUSTOMER_ID 0xC5
  driver.eeprom_oemid = efuse_tbl[EEPROM_CUSTOMER_ID];
  printf("eeprom_oemid 0x%x\n", driver.eeprom_oemid);
  #define EEPROM_DID 0xD8
  driver.eeprom_did = *(uint16_t*) &efuse_tbl[EEPROM_DID];
  printf("eeprom_did 0x%x\n", driver.eeprom_did);
  #define EEPROM_SVID 0xDA
  driver.eeprom_svid = *(uint16_t*) &efuse_tbl[EEPROM_SVID];
  printf("eeprom_svid 0x%x\n", driver.eeprom_svid);
  #define EEPROM_SMID 0xDC
  driver.eeprom_smid = *(uint16_t*) &efuse_tbl[EEPROM_SMID];
  printf("eeprom_smid 0x%x\n", driver.eeprom_smid);

  assert(driver.eeprom_did == 0x8179); // specific to my toshiba wireless nic for now
  assert(driver.eeprom_svid == 0x10ec);
  assert(driver.eeprom_smid == 0x181);
  driver.oem_id = RT_CID_DEFAULT;
  driver.antenna_div_type = efuse_tbl[EEPROM_RF_ANTENNA_OPT_88E];
  if (driver.antenna_div_type == 0xff) {
    driver.antenna_div_type = 1;
  }
  printf("antenna_div_type 0x%x\n", driver.antenna_div_type);
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
  driver.receive_config = RCR_APPFCS
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

  driver.write_nic_reg(REG_RCR, driver.receive_config);
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

#include "rtl8188ee_table.h"

// follow rtl88e_phy_mac_config in linux
void phy_mac_config(Rtl88eeDriver& driver) {
  uint32_t size = RTL8188EEMAC_1T_ARRAYLEN;
  uint32_t* arr = RTL8188EEMAC_1T_ARRAY;
  for (int i = 0; i < size; i += 2) {
    driver.write_nic_reg8(arr[i], (uint8_t) arr[i + 1]);
  }

  driver.write_nic_reg8(0x04CA, 0x0B);
}

// TODO: does linux use the hardware instruction directly? Check the usage in rtl8188ee/phy.c
uint32_t ffs(uint32_t mask) {
  assert(mask > 0);
  for (int i = 0; i < 32; ++i) {
    if (mask & (1U << i)) {
      return i + 1;
    }
  }
  assert(false && "can not reach here");
}

// follow _rtl88e_phy_calculate_bit_shift in linux
int bit_shift_from_mask(uint32_t mask) {
  assert(mask > 0);

  uint32_t off = ffs(mask); // return value of ffs start from 1 and count from least significant bit
  assert(off > 0);
  return off - 1;
}

// follow rtl_set_bbreg in linux
void rtl_set_bbreg(Rtl88eeDriver& driver, uint32_t addr, uint32_t mask, uint32_t data) {
  if (mask != 0xffffffff) {
    uint32_t original_value = driver.read_nic_reg(addr);
    uint32_t bitshift = bit_shift_from_mask(mask);
    data = (((original_value & ~mask)) | (data << bitshift));
  }
  driver.write_nic_reg(addr, data);
}

uint32_t rtl_get_bbreg(Rtl88eeDriver& driver, uint32_t addr, uint32_t mask) {
  uint32_t orig_val = driver.read_nic_reg(addr);
  uint32_t bitshift = bit_shift_from_mask(mask);
  uint32_t return_val = (orig_val & mask) >> bitshift;
  return return_val;
}

// follow _rtl88e_phy_rf_serial_read in linux
uint32_t phy_rf_serial_read(Rtl88eeDriver& driver, enum radio_path rfpath, uint32_t offset) {
  struct bb_reg_def *pphyreg = &driver.path_a_phyreg;
  uint32_t retvalue;
  offset &= 0xff;
  uint32_t newoffset = offset;
  uint32_t tmplong = rtl_get_bbreg(driver, RFPGA0_XA_HSSIPARAMETER2, MASKDWORD);
  assert(rfpath == RF90_PATH_A);
  uint32_t tmplong2 = tmplong;
  tmplong2 = (tmplong2 & (~BLSSIREADADDRESS)) | (newoffset << 23) | BLSSIREADEDGE;
  rtl_set_bbreg(driver, RFPGA0_XA_HSSIPARAMETER2, MASKDWORD, tmplong &(~BLSSIREADEDGE));
  msleep(1);
  rtl_set_bbreg(driver, pphyreg->rfhssi_para2, MASKDWORD, tmplong2);
  msleep(1);
  assert(rfpath == RF90_PATH_A);
  uint8_t rfpi_enable = (uint8_t) rtl_get_bbreg(driver, RFPGA0_XA_HSSIPARAMETER1, BIT(8));
  if (rfpi_enable) {
    retvalue = rtl_get_bbreg(driver, pphyreg->rf_rbpi, BLSSIREADBACKDATA);
  } else {
    retvalue = rtl_get_bbreg(driver, pphyreg->rf_rb, BLSSIREADBACKDATA);
  }

  return retvalue;
}

// follow rtl88e_phy_set_rf_reg in linux
void phy_set_rf_reg(Rtl88eeDriver& driver, enum radio_path rfpath, uint32_t addr, uint32_t mask, uint32_t data) {
  if (mask != RFREG_OFFSET_MASK) {
    uint32_t original_value = phy_rf_serial_read(driver, rfpath, addr);
    uint32_t bitshift = bit_shift_from_mask(mask);
    data = ((original_value & (~mask)) | (data << bitshift));
  }

  // follow _rtl88e_phy_rf_serial_write
  assert(rfpath == RF90_PATH_A);
  struct bb_reg_def *pphyreg = &driver.path_a_phyreg;
  addr &= 0xff;
  uint32_t data_and_addr = ((addr << 20) | (data & 0x000fffff)) & 0x0fffffff;
  rtl_set_bbreg(driver, pphyreg->rf3wire_offset, MASKDWORD, data_and_addr);
}

void rtl_set_rfreg(Rtl88eeDriver& driver, enum radio_path rfpath, uint32_t addr, uint32_t mask, uint32_t data) {
  phy_set_rf_reg(driver, rfpath, addr, mask, data);
}

uint32_t rtl_get_rfreg(Rtl88eeDriver& driver, enum radio_path rfpath, uint32_t regaddr, uint32_t bitmask) {
  // follow rtl88e_phy_query_rf_reg in linux
  uint32_t original_value = phy_rf_serial_read(driver, rfpath, regaddr);

  uint32_t bitshift = bit_shift_from_mask(bitmask);
  uint32_t readback_value = (original_value & bitmask) >> bitshift;
  return readback_value;
}

// follow _rtl8188e_config_bb_reg in linux
void config_bb_reg(Rtl88eeDriver& driver, uint32_t addr, uint32_t data) {
  if (addr == 0xfe) {
    msleep(50);
  } else if (addr == 0xfd) {
    msleep(5);
  } else if (addr == 0xfc || addr == 0xfb || addr == 0xfa || addr == 0xf9) {
    msleep(1);
  } else {
    rtl_set_bbreg(driver, addr, 0xffffffff, data);
    msleep(1);
  }
}

// follow rtl88e_phy_bb_config in linux
// BB may means baseband since I see BASEBAND_CONFIG_PHY_REG in _rtl88e_phy_bb8188e_config_parafile
// baseband can be modulated into higher frequency for RF (radio frequence).
void phy_bb_config(Rtl88eeDriver& driver) {
  // skip the call to _rtl88e_phy_init_bb_rf_register_definition which just
  // set register definitions in the software and does not directly update
  // hareware status
  
  driver.write_nic_reg16(REG_SYS_FUNC_EN,
    driver.read_nic_reg16(REG_SYS_FUNC_EN) | BIT(13) | BIT(0) | BIT(1));
  driver.write_nic_reg8(REG_RF_CTRL, RF_EN | RF_RSTB | RF_SDMRSTB);
  // this regiser is written as a byte here but as a word above
  driver.write_nic_reg8(REG_SYS_FUNC_EN,
    FEN_PPLL | FEN_PCIEA | FEN_DIO_PCIE | FEN_BB_GLB_RSTN | FEN_BBRSTB);

  driver.write_nic_reg(0x4c,
    driver.read_nic_reg(0x4c) | BIT(23));

  {
    // follow _rtl88e_phy_bb8188e_config_parafile
    {
      // follow phy_config_bb_with_headerfile with config BASEBAND_CONFIG_PHY_REG
      uint32_t* table = RTL8188EEPHY_REG_1TARRAY;
      uint16_t size = RTL8188EEPHY_REG_1TARRAYLEN;

      for (int i = 0; i < size; i += 2) {
        uint32_t v1 = table[i];
        uint32_t v2 = table[i + 1];
        assert(v1 < 0xcdcdcdcd);

        config_bb_reg(driver, v1, v2);
      }
    }

    {
      // ignore phy_config_bb_with_pghdr with config BASEBAND_CONFIG_PHY_REG for now since it does not write any hardware registers
    }

    {
      // follow phy_config_bb_with_headerfile with config BASEBAND_CONFIG_AGC_TAB
      uint32_t* table = RTL8188EEAGCTAB_1TARRAY;
      uint16_t size = RTL8188EEAGCTAB_1TARRAYLEN;
      for (int i = 0; i < size; i += 2) {
        rtl_set_bbreg(driver, table[i], 0xffffffff, table[i + 1]);
        msleep(1);
      }
    }
  }
}

// follow _rtl8188e_config_rf_radio_a in linux
void config_rf_radio_a(Rtl88eeDriver& driver, uint32_t addr, uint32_t data) {
  // follow _rtl8188e_config_rf_reg
  if (addr == 0xffe) { // is this a linux bug and should be 0xfe?
    msleep(50);
  } else if (addr == 0xfd) {
    msleep(5);
  } else if (addr == 0xfc || addr == 0xfb || addr == 0xfa || addr == 0xf9) {
    msleep(1);
  } else {
    phy_set_rf_reg(driver, RF90_PATH_A, addr, RFREG_OFFSET_MASK, data);
    msleep(1);
  }
}

// follow rtl88e_phy_rf_config in linux
void phy_rf_config(Rtl88eeDriver& driver) {
  // assume rf_type is RF_1T1R as set in _rtl88ee_read_chip_version

  // follow _rtl88e_phy_rf6052_config_parafile
  uint32_t num_total_rfpath = 1;
  for (int rfpath = 0; rfpath < num_total_rfpath; ++rfpath) {
    assert(rfpath == RF90_PATH_A);

    // XXX the code below assumes rfpath is RF90_PATH_A. Other rfpath need access different registers
    driver.path_a_phyreg = {
      .rfintfs = RFPGA0_XAB_RFINTERFACESW,
      .rfintfo = RFPGA0_XA_RFINTERFACEOE,
      .rfintfe = RFPGA0_XA_RFINTERFACEOE,
      .rf3wire_offset = RFPGA0_XA_LSSIPARAMETER,
      .rfhssi_para2 = RFPGA0_XA_HSSIPARAMETER2,
      .rf_rb = RFPGA0_XA_LSSIREADBACK,
      .rf_rbpi = TRANSCEIVEA_HSPI_READBACK,
    };
    struct bb_reg_def* pphyreg = &driver.path_a_phyreg;

    uint32_t regval = rtl_get_bbreg(driver, pphyreg->rfintfs, BRFSI_RFENV);

    // need access rtlphy->phyreg_def[rfpath] which is setup in _rtl88e_phy_init_bb_rf_register_definition
    rtl_set_bbreg(driver, pphyreg->rfintfe, BRFSI_RFENV << 16, 1);
    msleep(1);

    rtl_set_bbreg(driver, pphyreg->rfintfo, BRFSI_RFENV, 1);
    msleep(1);

    rtl_set_bbreg(driver, pphyreg->rfhssi_para2, B3WIREADDREAALENGTH, 0);
    msleep(1);
    rtl_set_bbreg(driver, pphyreg->rfhssi_para2, B3WIREDATALENGTH, 0);
    msleep(1);

    { // follow rtl88e_phy_config_rf_with_headerfile in linux
      uint16_t size = RTL8188EE_RADIOA_1TARRAYLEN;
      uint32_t* data = RTL8188EE_RADIOA_1TARRAY;
      // follow process_path_a
      for (int i = 0; i < size; i += 2) {
        uint32_t v1 = data[i];
        uint32_t v2 = data[i + 1];
        assert(v1 < 0xcdcdcdcd);

        config_rf_radio_a(driver, v1, v2);
      }

      // need do something for RT_CID_819X_HP. Skip that since the oem_id is RT_CID_DEFAULT
      assert(driver.oem_id == RT_CID_DEFAULT);
    }

    rtl_set_bbreg(driver, pphyreg->rfintfs, BRFSI_RFENV, regval);
  }
}

// follow _rtl88ee_hw_configure in linux
void hw_configure(Rtl88eeDriver& driver) {
  driver.write_nic_reg(REG_RRSR, RATE_ALL_CCK | RATE_ALL_OFDM_AG);
  driver.write_nic_reg8(REG_HWSEQ_CTRL, 0xFF);
}

// follow rtl_cam_reset_all_entry in linux
void cam_reset_all_entry(Rtl88eeDriver& driver) {
  driver.write_nic_reg(REG_CAMCMD, BIT(31) | BIT(30));
}

// follow rtl88ee_enable_hw_security_config in linux
void enable_hw_security_config(Rtl88eeDriver& driver) {
  // assume rtlpriv->sec.use_defaultkey is false

  uint8_t val = SCR_TXENCENABLE | SCR_RXDECENABLE | SCR_RXBCUSEDK | SCR_TXBCUSEDK;

  driver.write_nic_reg8(REG_CR + 1, 0x02);
  driver.write_nic_reg8(REG_SECCFG, val);
}

// follow _rtl88e_phy_set_rfpath_switch in linux
void phy_set_rfpath_switch(Rtl88eeDriver& driver, bool bmain, bool is2t) {
  assert(bmain);
  assert(!is2t);
  assert(!driver.hal_state); // hal is started after hw_init

  if (!driver.hal_state) {
    driver.update_nic_reg8(REG_LEDCFG0, BIT(7), BIT(7));
    rtl_set_bbreg(driver, RFPGA0_XAB_RFPARAMETER, BIT(13), 0x01);
  }
  rtl_set_bbreg(driver, RFPGA0_XAB_RFINTERFACESW, BIT(8) | BIT(9), 0);
  rtl_set_bbreg(driver, 0x914, MASKLWORD, 0x0201);

  rtl_set_bbreg(driver, RFPGA0_XA_RFINTERFACEOE, BIT(14) | BIT(13) | BIT(12), 0);
  rtl_set_bbreg(driver, RFPGA0_XB_RFINTERFACEOE, BIT(5) | BIT(4) | BIT(3), 0);
  rtl_set_bbreg(driver, RCONFIG_RAM64x16, BIT(31), 0);
}

// follow _rtl88e_phy_save_adda_registers in linux
void phy_save_adda_registers(Rtl88eeDriver& driver, uint32_t* reglist, uint32_t* backup, uint32_t num) {
  for (int i = 0; i < num; ++i) {
    backup[i] = rtl_get_bbreg(driver, reglist[i], MASKDWORD);
  }
}

// follow _rtl88e_phy_save_mac_registers in linux
void phy_save_mac_registers(Rtl88eeDriver& driver, uint32_t* macreg, uint32_t* backup) {
  int i;
  for (i = 0; i < (IQK_MAC_REG_NUM - 1); ++i) {
    backup[i] = driver.read_nic_reg8(macreg[i]);
  }
  backup[i] = driver.read_nic_reg(macreg[i]);
}

void phy_path_adda_on(Rtl88eeDriver& driver, uint32_t* addareg, bool is_patha_on, bool is2t) {
  uint32_t pathon = is_patha_on ? 0x04db25a4 : 0x0b1b25a4;
  if (!is2t) {
    pathon = 0x0bdb25a0;
    rtl_set_bbreg(driver, addareg[0], MASKDWORD, 0x0b1b25a0);
  } else {
    rtl_set_bbreg(driver, addareg[0], MASKDWORD, pathon);  
  }
  for (int i = 1; i < IQK_ADDA_REG_NUM; ++i) {
    rtl_set_bbreg(driver, addareg[i], MASKDWORD, pathon);
  }
}

// follow _rtl88e_phy_mac_setting_calibration in linux
void phy_mac_setting_calibration(Rtl88eeDriver& driver, uint32_t* macreg, uint32_t *macbackup) {
  int i = 0;
  driver.write_nic_reg8(macreg[i], 0x3F);
  for (i = 1; i < (IQK_MAC_REG_NUM - 1); ++i) {
    driver.write_nic_reg8(macreg[i], (uint8_t) (macbackup[i] & ~BIT(3)));
  }
  driver.write_nic_reg8(macreg[i], (uint8_t) (macbackup[i] & ~BIT(5)));
}

// follow _rtl88e_phy_path_a_iqk in linux
uint8_t phy_path_a_iqk(Rtl88eeDriver& driver, bool config_pathb) {
  uint8_t result = 0;

  rtl_set_bbreg(driver, 0xe30, MASKDWORD, 0x10008c1c);
  rtl_set_bbreg(driver, 0xe34, MASKDWORD, 0x30008c1c);
  rtl_set_bbreg(driver, 0xe38, MASKDWORD, 0x8214032a);
  rtl_set_bbreg(driver, 0xe3c, MASKDWORD, 0x28160000);

  rtl_set_bbreg(driver, 0xe4c, MASKDWORD, 0x00462911);
  rtl_set_bbreg(driver, 0xe48, MASKDWORD, 0xf9000000);
  rtl_set_bbreg(driver, 0xe48, MASKDWORD, 0xf8000000);
  
  #define IQK_DELAY_TIME 10
  msleep(IQK_DELAY_TIME);

  uint32_t reg_eac = rtl_get_bbreg(driver, 0xeac, MASKDWORD);
  uint32_t reg_e94 = rtl_get_bbreg(driver, 0xe94, MASKDWORD);
  uint32_t reg_e9c = rtl_get_bbreg(driver, 0xe9c, MASKDWORD);
  // not sure why linux does not assign the return value to a variable.
  // maybe reading a hw register has side effect? idk
  rtl_get_bbreg(driver, 0xea4, MASKDWORD);

  if (!(reg_eac & BIT(28)) &&
      (((reg_e94 & 0x03FF0000) >> 16) != 0x142) &&
      (((reg_e9c & 0x03FF0000) >> 16) != 0x42)) {
    result |= 0x01;
  }
  return result;
}

// follow _rtl88e_phy_path_a_rx_iqk in linux
uint8_t phy_path_a_rx_iqk(Rtl88eeDriver& driver, bool config_pathb) {
  // Get TXIMR Setting
  // Modify RX IQK mode table
  rtl_set_bbreg(driver, RFPGA0_IQK, MASKDWORD, 0x0);
  rtl_set_rfreg(driver, RF90_PATH_A, RF_WE_LUT, RFREG_OFFSET_MASK, 0x800a0);
  rtl_set_rfreg(driver, RF90_PATH_A, RF_RCK_OS, RFREG_OFFSET_MASK, 0x30000);
  rtl_set_rfreg(driver, RF90_PATH_A, RF_TXPA_G1, RFREG_OFFSET_MASK, 0x0000f);
  rtl_set_rfreg(driver, RF90_PATH_A, RF_TXPA_G2, RFREG_OFFSET_MASK, 0xf117b);
  rtl_set_bbreg(driver, RFPGA0_IQK, MASKDWORD, 0x80800000);

  // IQK setting
  rtl_set_bbreg(driver, RTX_IQK, MASKDWORD, 0x01007c00);
  rtl_set_bbreg(driver, RRX_IQK, MASKDWORD, 0x81004800);

  // path a IQK setting
  rtl_set_bbreg(driver, RTX_IQK_TONE_A, MASKDWORD, 0x10008c1c);
  rtl_set_bbreg(driver, RRX_IQK_TONE_A, MASKDWORD, 0x30008c1c);
  rtl_set_bbreg(driver, RTX_IQK_PI_A, MASKDWORD, 0x82160804);
  rtl_set_bbreg(driver, RRX_IQK_PI_A, MASKDWORD, 0x28160000);

  // LO calibration setting
  rtl_set_bbreg(driver, RIQK_AGC_RSP, MASKDWORD, 0x0046a911);

  // one short, path A LOK & iqk
  rtl_set_bbreg(driver, RIQK_AGC_PTS, MASKDWORD, 0xf9000000);
  rtl_set_bbreg(driver, RIQK_AGC_PTS, MASKDWORD, 0xf8000000);

  msleep(IQK_DELAY_TIME);

  uint32_t reg_eac = rtl_get_bbreg(driver, RRX_POWER_AFTER_IQK_A_2, MASKDWORD);
  uint32_t reg_e94 = rtl_get_bbreg(driver, RTX_POWER_BEFORE_IQK_A, MASKDWORD);
  uint32_t reg_e9c = rtl_get_bbreg(driver, RTX_POWER_AFTER_IQK_A, MASKDWORD);

  uint8_t result = 0;

  if (!(reg_eac & BIT(28)) &&
      (((reg_e94 & 0x03FF0000) >> 16) != 0x142) &&
      (((reg_e9c & 0x03FF0000) >> 16) != 0x42)) {
    result |= 0x1;
  } else {
    return result;
  }

  uint32_t val = 0x80007C00 | (reg_e94 & 0x3FF0000) |
      ((reg_e9c & 0x3FF0000) >> 16);
  rtl_set_bbreg(driver, RTX_IQK, MASKDWORD, val);

  // RX IQK
  // Modify RX IQK mode table
  rtl_set_bbreg(driver, RFPGA0_IQK, MASKDWORD, 0x0);
  rtl_set_rfreg(driver, RF90_PATH_A, RF_WE_LUT, RFREG_OFFSET_MASK, 0x800a0);
  rtl_set_rfreg(driver, RF90_PATH_A, RF_RCK_OS, RFREG_OFFSET_MASK, 0x30000);
  rtl_set_rfreg(driver, RF90_PATH_A, RF_TXPA_G1, RFREG_OFFSET_MASK, 0xf);
  rtl_set_rfreg(driver, RF90_PATH_A, RF_TXPA_G2, RFREG_OFFSET_MASK, 0xf7ffa);
  rtl_set_bbreg(driver, RFPGA0_IQK, MASKDWORD, 0x80800000);

  // IQK setting
  rtl_set_bbreg(driver, RRX_IQK, MASKDWORD, 0x01004800);

  // path a IQK setting
  rtl_set_bbreg(driver, RTX_IQK_TONE_A, MASKDWORD, 0x30008c1c);
  rtl_set_bbreg(driver, RRX_IQK_TONE_A, MASKDWORD, 0x10008c1c);
  rtl_set_bbreg(driver, RTX_IQK_PI_A, MASKDWORD, 0x82160c05);
  rtl_set_bbreg(driver, RRX_IQK_PI_A, MASKDWORD, 0x28160c05);

  // LO calibration setting
  rtl_set_bbreg(driver, RIQK_AGC_RSP, MASKDWORD, 0x0046a911);

  // one short path A LOK & iqk
  rtl_set_bbreg(driver, RIQK_AGC_PTS, MASKDWORD, 0xf9000000);
  rtl_set_bbreg(driver, RIQK_AGC_PTS, MASKDWORD, 0xf8000000);

  msleep(IQK_DELAY_TIME);

  reg_eac = rtl_get_bbreg(driver, RRX_POWER_AFTER_IQK_A_2, MASKDWORD);
  reg_e94 = rtl_get_bbreg(driver, RTX_POWER_BEFORE_IQK_A, MASKDWORD);
  reg_e9c = rtl_get_bbreg(driver, RTX_POWER_AFTER_IQK_A, MASKDWORD);
  uint32_t reg_ea4 = rtl_get_bbreg(driver, RRX_POWER_BEFORE_IQK_A_2, MASKDWORD);

  if (!(reg_eac & BIT(27)) &&
      (((reg_ea4 & 0x03FF0000) >> 16) != 0x132) &&
      (((reg_eac & 0x03FF0000) >> 16) != 0x36)) {
    result |= 0x02;
  }
  return result;
}

// follow _rtl88e_phy_reload_adda_registers in linux
void phy_reload_adda_registers(Rtl88eeDriver& driver, uint32_t* addareg, uint32_t* addabackup, uint32_t num) {
  for (int i = 0; i < num; ++i) {
    rtl_set_bbreg(driver, addareg[i], MASKDWORD, addabackup[i]);
  }
}

// follow _rtl88e_phy_reload_mac_registers in linux
void phy_reload_mac_registers(Rtl88eeDriver& driver, uint32_t* macreg, uint32_t* macbackup) {
  int i;
  for (i = 0; i < (IQK_MAC_REG_NUM - 1); ++i) {
    driver.write_nic_reg8(macreg[i], (uint8_t) macbackup[i]);
  }
  driver.write_nic_reg(macreg[i], macbackup[i]);
}

// follow _rtl88e_phy_iq_calibrate in linux
void _phy_iq_calibrate(Rtl88eeDriver& driver, long result[][8], uint8_t t, bool is2t) {
  assert(!is2t);
  uint32_t adda_reg[IQK_ADDA_REG_NUM] = {
    0x85c, 0xe6c, 0xe70, 0xe74,
    0xe78, 0xe7c, 0xe80, 0xe84,
    0xe88, 0xe8c, 0xed0, 0xed4,
    0xed8, 0xedc, 0xee0, 0xeec
  };
  uint32_t iqk_mac_reg[IQK_MAC_REG_NUM] = {
    0x522, 0x550, 0x551, 0x040
  };
  uint32_t iqk_bb_reg[IQK_BB_REG_NUM] = {
    ROFDM0_TRXPATHENABLE, ROFDM0_TRMUXPAR,
    RFPGA0_XCD_RFINTERFACESW,
    0xb68, 0xb6c, 0x870, 0x860, 0x864, 0x800
  };
  if (t == 0) {
    phy_save_adda_registers(driver, adda_reg, driver.adda_backup, IQK_ADDA_REG_NUM);
    phy_save_mac_registers(driver, iqk_mac_reg, driver.iqk_mac_backup);
    phy_save_adda_registers(driver, iqk_bb_reg, driver.iqk_bb_backup, IQK_BB_REG_NUM);
  }

  phy_path_adda_on(driver, adda_reg, true, is2t);
  if (t == 0) {
    driver.rfpi_enable = (uint8_t) rtl_get_bbreg(driver, RFPGA0_XA_HSSIPARAMETER1, BIT(8));
  }
  assert(driver.rfpi_enable);

  // BB setting
  rtl_set_bbreg(driver, 0x800, BIT(24), 0);
  rtl_set_bbreg(driver, 0xc04, MASKDWORD, 0x03a05600);
  rtl_set_bbreg(driver, 0xc08, MASKDWORD, 0x000800e4);
  rtl_set_bbreg(driver, 0x874, MASKDWORD, 0x22204000);

  rtl_set_bbreg(driver, 0x870, BIT(10), 0x01);
  rtl_set_bbreg(driver, 0x870, BIT(26), 0x01);
  rtl_set_bbreg(driver, 0x860, BIT(10), 0x00);
  rtl_set_bbreg(driver, 0x864, BIT(10), 0x00);

  assert(!is2t);
  phy_mac_setting_calibration(driver, iqk_mac_reg, driver.iqk_mac_backup);
  rtl_set_bbreg(driver, 0xb68, MASKDWORD, 0x0f600000);
  assert(!is2t);

  rtl_set_bbreg(driver, 0xe28, MASKDWORD, 0x80800000);
  rtl_set_bbreg(driver, 0xe40, MASKDWORD, 0x01007c00);
  rtl_set_bbreg(driver, 0xe44, MASKDWORD, 0x81004800);

  const int retrycount = 2;
  uint8_t patha_ok;
  for (int i = 0; i < retrycount; ++i) {
    patha_ok = phy_path_a_iqk(driver, is2t);
    if (patha_ok == 0x01) {
      result[t][0] = (rtl_get_bbreg(driver, 0xe94, MASKDWORD) & 0x3FF0000) >> 16;
      result[t][1] = (rtl_get_bbreg(driver, 0xe9c, MASKDWORD) & 0x3FF0000) >> 16;
      break;
    }
  }

  for (int i = 0; i < retrycount; ++i) {
    patha_ok = phy_path_a_rx_iqk(driver, is2t);
    if (patha_ok == 0x03) {
      result[t][2] = (rtl_get_bbreg(driver, 0xea4, MASKDWORD) &
        0x3FF0000) >> 16;
      result[t][3] = (rtl_get_bbreg(driver, 0xeac, MASKDWORD) &
        0x3FF0000) >> 16;
      break;
    }
  }

  assert(!is2t);

  rtl_set_bbreg(driver, 0xe28, MASKDWORD, 0);

  if (t != 0) {
    assert(driver.rfpi_enable);

    phy_reload_adda_registers(driver, adda_reg, driver.adda_backup, IQK_ADDA_REG_NUM);
    phy_reload_mac_registers(driver, iqk_mac_reg, driver.iqk_mac_backup);
    phy_reload_adda_registers(driver, iqk_bb_reg, driver.iqk_bb_backup, IQK_BB_REG_NUM);

    rtl_set_bbreg(driver, 0x840, MASKDWORD, 0x00032ed3);
    assert(!is2t);
    rtl_set_bbreg(driver, 0xe30, MASKDWORD, 0x01008c00);
    rtl_set_bbreg(driver, 0xe34, MASKDWORD, 0x01008c00);
  }
}

// follow _rtl88e_phy_simularity_compare in linux
bool phy_simularity_compare(Rtl88eeDriver& driver, long result[][8], uint8_t c1, uint8_t c2) {
  bool is2t = false;
  uint32_t bound = 4;
  uint32_t simularity_bitmap = 0;
  uint8_t final_candidate[2] = { 0xFF, 0xFF };

  for (int i = 0; i < bound; ++i) {
    uint32_t diff = (result[c1][i] > result[c2][i]) ?
      (result[c1][i] - result[c2][i]) :
      (result[c2][i] - result[c1][i]);
    if (diff > MAX_TOLERANCE) {
      if ((i == 2 || i == 6) && !simularity_bitmap) {
        if (result[c1][i] + result[c1][i + 1] == 0) {
          final_candidate[i / 4] = c2;
        } else if (result[c2][i] + result[c2][i + 1] == 0) {
          final_candidate[i / 4] = c1;
        } else {
          simularity_bitmap |= (1 << i);
        }
      } else {
        simularity_bitmap |= (1 << i);
      }
    }
  }

  bool bresult = true;
  if (simularity_bitmap == 0) {
    for (int i = 0; i < (bound / 4); ++i) {
      if (final_candidate[i] != 0xFF) {
        for (int j = i * 4; j < (i + 1) * 4 - 2; ++j) {
          result[3][j] = result[final_candidate[i]][j];
        }
        bresult = false;
      }
    }
    return bresult;
  } else if (!(simularity_bitmap & 0x0F)) {
    for (int i = 0; i < 4; ++i) {
      result[3][i] = result[c1][i];
    }
    return false;
  } else if (!(simularity_bitmap & 0xF0) && is2t) {
    assert(false && "can not reach here since we assume is2t is false");
  } else {
    return false;
  }
}

// follow _rtl88e_phy_path_a_fill_iqk_matrix in linux
void phy_path_a_fill_iqk_matrix(Rtl88eeDriver& driver, bool iqk_ok, long result[][8], uint8_t final_candidate, bool btxonly) {
  if (final_candidate == 0xFF) {
    return;
  } else if (iqk_ok) {
    uint32_t oldval_0 = (rtl_get_bbreg(driver, ROFDM0_XATXIQIMBALANCE, MASKDWORD) >> 22) & 0x3FF;
    uint32_t x = result[final_candidate][0];
    if ((x & 0x200) != 0) {
      x = x | 0xFFFFFC00;
    }
    uint32_t tx0_a = (x * oldval_0) >> 8;
    rtl_set_bbreg(driver, ROFDM0_XATXIQIMBALANCE, 0x3FF, tx0_a);
    rtl_set_bbreg(driver, ROFDM0_ECCATHRESHOLD, BIT(31), ((x * oldval_0 >> 7) & 0x1));
    long y = result[final_candidate][1];
    if ((y & 0x200) != 0) {
      y = y | 0xFFFFFC00;
    }
    long tx0_c = (y * oldval_0) >> 8;
    rtl_set_bbreg(driver, ROFDM0_XCTXAFE, 0xF0000000,
        ((tx0_c & 0x3C0) >> 6));
    rtl_set_bbreg(driver, ROFDM0_XATXIQIMBALANCE, 0x3F0000, (tx0_c & 0x3F));
    rtl_set_bbreg(driver, ROFDM0_ECCATHRESHOLD, BIT(29), ((y * oldval_0 >> 7) & 0x1));
    if (btxonly) {
      return;
    }
    uint32_t reg = result[final_candidate][2];
    rtl_set_bbreg(driver, ROFDM0_XARXIQIMBALANCE, 0x3FF, reg);
    reg = result[final_candidate][3] & 0x3F;
    rtl_set_bbreg(driver, ROFDM0_XARXIQIMBALANCE, 0xFC00, reg);
    reg = (result[final_candidate][3] >> 6) & 0xF;
    rtl_set_bbreg(driver, 0xca0, 0xF0000000, reg);
  }
}

// follow rtl88e_phy_iq_calibrate in linux
void phy_iq_calibrate(Rtl88eeDriver& driver, bool b_recovery) {
  assert(!b_recovery);
  long result[4][8];
  memset(result, 0, sizeof(result));
  bool is12simular = false, is13simular = false, is23simular = false;
  uint8_t final_candidate = 0xff;
  uint32_t reg_tmp = 0;

  for (int i = 0; i < 3; ++i) {
    // assume rf_type is RF_1T1R as set in _rtl88ee_read_chip_version
    _phy_iq_calibrate(driver, result, i, false); 
    if (i == 1) {
      is12simular = phy_simularity_compare(driver, result, 0, 1);
      if (is12simular) {
        final_candidate = 0;
        break;
      }
    }
    if (i == 2) {
      is13simular = phy_simularity_compare(driver, result, 0, 2);
      if (is13simular) {
        final_candidate = 0;
        break;
      }
      is23simular = phy_simularity_compare(driver, result, 1, 2);
      if (is23simular) {
        final_candidate = 1;
      } else {
        // linux code use the same variable i in the next loop. This is bad
        // but does not cause any damage
        for (int j = 0; j < 8; ++j) {
          reg_tmp += result[3][j];
        }
        if (reg_tmp != 0) {
          final_candidate = 3;
        } else {
          final_candidate = 0xFF;
        }
      }
    }
  }

  uint32_t reg_e94, reg_ea4;
  bool b_patha_ok = false;

  // why linux use a loop here?
  for (int i = 0; i < 4; ++i) {
    reg_e94 = result[i][0];
    reg_ea4 = result[i][2];
  }
  if (final_candidate != 0xff) {
    reg_e94 = result[final_candidate][0];
    reg_ea4 = result[final_candidate][2];
    b_patha_ok = true;
  }
  if (reg_e94 != 0) {
    phy_path_a_fill_iqk_matrix(driver, b_patha_ok, result, final_candidate, (reg_ea4 == 0));
  }
  if (final_candidate != 0xFF) {
    // skip is find
  }
  uint32_t iqk_bb_reg[9] = {
    ROFDM0_XARXIQIMBALANCE,
    ROFDM0_XBRXIQIMBALANCE,
    ROFDM0_ECCATHRESHOLD,
    ROFDM0_AGCRSSITABLE,
    ROFDM0_XATXIQIMBALANCE,
    ROFDM0_XBTXIQIMBALANCE,
    ROFDM0_XCTXAFE,
    ROFDM0_XDTXAFE,
    ROFDM0_RXIQEXTANTA,
  };
  phy_save_adda_registers(driver, iqk_bb_reg, driver.iqk_bb_backup, 9);
}

// follow rtl88e_dm_check_txpower_tracking in linux
// dm means dynamic management according to the comment around 'struct rtl_dm'
void dm_check_txpower_tracking(Rtl88eeDriver& driver) {
  // do noting since txpower_tracking is set to true later in hw_init
}

// follow rtl88e_phy_lc_calibrate in linux
void phy_lc_calibrate(Rtl88eeDriver& driver) {
  // follow _rtl88e_phy_lc_calibrate in linux
  bool is2t = false;
  uint8_t tmpreg = driver.read_nic_reg8(0xd03);
  if ((tmpreg & 0x70) != 0) {
    driver.write_nic_reg8(0xd03, tmpreg & 0x8F);
  } else {
    driver.write_nic_reg8(REG_TXPAUSE, 0xFF);
  }

  uint32_t rf_a_mode = 0;
  if ((tmpreg & 0x70) != 0) {
    rf_a_mode = rtl_get_rfreg(driver, RF90_PATH_A, 0x00, MASK12BITS);

    assert(!is2t);

    rtl_set_rfreg(driver, RF90_PATH_A, 0x00, MASK12BITS, (rf_a_mode & 0x8FFFF) | 0x10000);
    assert(!is2t);
  }

  uint32_t lc_cal = rtl_get_rfreg(driver, RF90_PATH_A, 0x18, MASK12BITS);
  rtl_set_rfreg(driver, RF90_PATH_A, 0x18, MASK12BITS, lc_cal | 0x08000);
  msleep(100);

  if ((tmpreg & 0x70) != 0) {
    driver.write_nic_reg8(0xd03, tmpreg);
    rtl_set_rfreg(driver, RF90_PATH_A, 0x00, MASK12BITS, rf_a_mode);
    assert(!is2t);
  } else {
    driver.write_nic_reg8(REG_TXPAUSE, 0x00);
  }
}

// follow rtl88e_dm_update_rx_idle_ant in linux
void dm_update_rx_idle_ant(Rtl88eeDriver& driver, uint8_t ant) {
  assert(ant == MAIN_ANT);
  // skip since pfat_table->rx_idle_ant should already be MAIN_ANT
}

// follow rtl88e_dm_antenna_div_init in linux
void dm_antenna_div_init(Rtl88eeDriver& driver) {
  assert(driver.antenna_div_type == CGCS_RX_HW_ANTDIV);

  // follow rtl88e_dm_rx_hw_antena_div_init in linux

  // MAC Setting
  uint32_t value32 = rtl_get_bbreg(driver, DM_REG_ANTSEL_PIN_11N, MASKDWORD);
  rtl_set_bbreg(driver, DM_REG_ANTSEL_PIN_11N, MASKDWORD, value32 | (BIT(23) | BIT(25)));
  // Pin Setting
  rtl_set_bbreg(driver, DM_REG_PIN_CTRL_11N, BIT(9) | BIT(8), 0);
  rtl_set_bbreg(driver, DM_REG_RX_ANT_CTRL_11N, BIT(10), 0);
  rtl_set_bbreg(driver, DM_REG_LNA_SWITCH_11N, BIT(22), 1);
  rtl_set_bbreg(driver, DM_REG_LNA_SWITCH_11N, BIT(31), 1);

  // OFDM setting
  rtl_set_bbreg(driver, DM_REG_ANTDIV_PARA1_11N, MASKDWORD, 0xa0);

  // CCK setting
  rtl_set_bbreg(driver, DM_REG_BB_PWR_SAV4_11N, BIT(7), 1);
  rtl_set_bbreg(driver, DM_REG_CCK_ANTDIV_PARA2_11N, BIT(4), 1);
  dm_update_rx_idle_ant(driver, MAIN_ANT);
  rtl_set_bbreg(driver, DM_REG_ANT_MAPPING1_11N, MASKLWORD, 0x0201);
}

// follow rtl88e_dm_init in linux
void dm_init(Rtl88eeDriver& driver) {
  // XXX skip other work in this function since they don't change hardware registers
  dm_antenna_div_init(driver);
}

// follow _rtl88ee_stop_tx_beacon in linux
void stop_tx_beacon(Rtl88eeDriver& driver) {
  uint8_t tmp1byte = driver.read_nic_reg8(REG_FWHW_TXQ_CTRL + 2);
  driver.write_nic_reg8(REG_FWHW_TXQ_CTRL + 2, tmp1byte & (~BIT(6)));
  driver.write_nic_reg8(REG_TBTT_PROHIBIT + 1, 0x64);
  tmp1byte = driver.read_nic_reg8(REG_TBTT_PROHIBIT + 2);
  tmp1byte &= ~(BIT(0));
  driver.write_nic_reg8(REG_TBTT_PROHIBIT + 2, tmp1byte);
}

// follow _rtl88ee_set_bcn_ctrl_reg in linux
void set_bcn_ctrl_reg(Rtl88eeDriver& driver, uint8_t set_bits, uint8_t clear_bits) {
  // XXX should reg_bcn_ctrl_val be BIT(3) set by rtl88ee_set_beacon_related_registers ?
  driver.reg_bcn_ctrl_val |= set_bits;
  driver.reg_bcn_ctrl_val &= ~clear_bits;
  driver.write_nic_reg8(REG_BCN_CTRL, driver.reg_bcn_ctrl_val);
}

// follow _rtl88ee_enable_bcn_sub_func in linux
void enable_bcn_sub_func(Rtl88eeDriver& driver) {
  set_bcn_ctrl_reg(driver, 0, BIT(1));
}

// follow rtl88ee_set_check_bssid in linux
void set_check_bssid(Rtl88eeDriver& driver, bool check_bssid) {
  assert(!check_bssid);
  uint32_t reg_rcr = driver.receive_config;

  reg_rcr &= (~(RCR_CBSSID_DATA | RCR_CBSSID_BCN));
  set_bcn_ctrl_reg(driver, BIT(4), 0);

  // set_hw_reg HW_VAR_RCR
  driver.write_nic_reg(REG_RCR, reg_rcr);
  driver.receive_config = reg_rcr;
}

// follow rtl_op_add_interface in linux
void rtl_op_add_interface(Rtl88eeDriver& driver) {
  // XXX assumes interface type is STATION
  // XXX assumes beacon_enabled is 0
  // XXX skip rtl_ips_nic_on for now.

  {
    // follow rtl88ee_set_network_type in linux

    { // follow _rtl88ee_set_media_status
      // XXX skip led for now
      // XXX assume link_state < MAC80211_LINKED
      uint8_t mode = MSR_NOLINK;
      uint8_t bt_msr = driver.read_nic_reg8(MSR) & 0xfc;
      stop_tx_beacon(driver);
      enable_bcn_sub_func(driver);
      driver.write_nic_reg8(MSR, bt_msr | mode);
      driver.write_nic_reg8(REG_BCNTCFG + 1, 0x66);
    }

    set_check_bssid(driver, false);
  }
  // set mac addr
  for (int i = 0; i < ETH_ALEN; ++i) {
    driver.write_nic_reg8((REG_MACID + i), driver.mac_addr[i]);
  }

  // set retry limit
  uint8_t retry_limit = 0x30;
  driver.write_nic_reg16(REG_RL, (retry_limit << RETRY_LIMIT_SHORT_SHIFT) | (retry_limit << RETRY_LIMIT_LONG_SHIFT));
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
  phy_mac_config(driver);
  
  // need re-adjust receive_config since it's updated by phy_mac_config
  driver.receive_config &= ~(RCR_ACRC32 | RCR_AICV);
  driver.write_nic_reg8(REG_RCR, driver.receive_config);

  phy_bb_config(driver);
  rtl_set_bbreg(driver, RFPGA0_RFMOD, BCCKEN, 1);
  rtl_set_bbreg(driver, RFPGA0_RFMOD, BOFDMEN, 1);

  phy_rf_config(driver);

  // return rfreg RF_CHNLBW is skipped right now

  hw_configure(driver);
  cam_reset_all_entry(driver);
  enable_hw_security_config(driver);

  // set mac addr
  for (int i = 0; i < ETH_ALEN; ++i) {
    driver.write_nic_reg8(REG_MACID + i, driver.mac_addr[i]);
  }

  // TODO skip _rtl88ee_enable_aspm_back_door for now
  // TODO skip rtlpriv->intf_ops->enable_aspm(hw); for now

  assert(driver.antenna_div_type == CGCS_RX_HW_ANTDIV);
  phy_set_rfpath_switch(driver, true, false);

  phy_iq_calibrate(driver, false);
  dm_check_txpower_tracking(driver);
  phy_lc_calibrate(driver);

  uint8_t tmp_u1b = efuse_read_1byte(driver, 0x1FA);
  if (!(tmp_u1b & BIT(0))) {
    rtl_set_rfreg(driver, RF90_PATH_A, 0x15, 0x0F, 0x05);
  }

  if (!(tmp_u1b & BIT(4))) {
    tmp_u1b = driver.read_nic_reg8(0x16);
    tmp_u1b &= 0x0F;
    driver.write_nic_reg8(0x16, tmp_u1b | 0x80);
    msleep(1);
    driver.write_nic_reg8(0x16, tmp_u1b | 0x90);
  }
  driver.write_nic_reg8(REG_NAV_CTRL + 2, ((30000 + 127) / 128));
  dm_init(driver);
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
  driver.hal_state = true;

  rtl_op_add_interface(driver);

  // TODO these are debugging code that should be removed later
  int idx = 0;
  while (idx < 15) {
    printf("idx %d, own %d\n", idx++, rx_ring[driver.rxidx].own);
    msleep(2000);
  }

  safe_assert(false && "found wifi nic");
}
