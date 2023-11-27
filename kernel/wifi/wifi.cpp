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

  uint32_t rxidx = 0;
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

  // follow linux _rtl88ee_init_mac to enable RX DMA
  driver.write_nic_reg8(REG_PCIE_CTRL_REG + 1, 0);

  // TODO these are debugging code that should be removed later
  int idx = 0;
  while (true) {
    printf("idx %d, own %d\n", idx++, rx_ring[driver.rxidx].own);
    dumbsleep(500);
  }

  safe_assert(false && "found wifi nic");
}
