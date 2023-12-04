#include <kernel/wifi/wifi.h>
#include <kernel/wifi/rtl8188ee.h>
#include <kernel/phys_page.h>
#include <kernel/paging.h>
#include <kernel/sleep.h>

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

  // TODO these are debugging code that should be removed later
  int idx = 0;
  while (true) {
    printf("idx %d, own %d\n", idx++, rx_ring[driver.rxidx].own);
    dumbsleep(500);
  }

  safe_assert(false && "found wifi nic");
}
