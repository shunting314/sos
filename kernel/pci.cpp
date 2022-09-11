#include <kernel/pci.h>
#include <kernel/nic/nic.h>
#include <kernel/usb/usb.h>

Port32Bit pci_addr_port(PORT_CONFIG_ADDRESS);
Port32Bit pci_data_port(PORT_CONFIG_DATA);

typedef void visit_fn(const PCIFunction& func);

/*
 * Visit all functions for the device.
 */
static void walkdevice(uint32_t bus_id, uint32_t dev_id, visit_fn* visitor) {
  PCIFunction func0(bus_id, dev_id, 0);

  // the device should not have any functions if function 0 does not exist
  if (!func0) {
    return;
  }

  visitor(func0);

  auto header_type = func0.header_type();
  // the most significant bit of header_type is 1 for multi-function device
  if (header_type & 0x80) {
    for (uint32_t func_id = 1; func_id < MAX_NUM_FUNCTIONS_PER_DEVICE; ++func_id) {
      PCIFunction func_other(bus_id, dev_id, func_id);
      if (func_other) {
        visitor(func_other);
      }
    }
  }
}

/*
 * TODO: this method enumerate thru all possible bus id and device id.
 * A more efficient method is do a DFS search. Check the PCI osdev wiki for more details.
 */
static void walkpci(visit_fn* visitor) {
  uint32_t bus_id;
  uint32_t dev_id;
  for (bus_id = 0; bus_id < MAX_NUM_BUS; ++bus_id) {
    for (dev_id = 0; dev_id < MAX_NUM_DEVICES_PER_BUS; ++dev_id) {
      walkdevice(bus_id, dev_id, visitor);
    }
  }
}

void lspci() {
  walkpci([](const PCIFunction& func) { func.dump(); });
}

// TODO: support multiple devices of the same class.
void collect_pci_devices() {
  ethernet_controller_pci_func = PCIFunction(); // reset
  walkpci([](const PCIFunction& func) {
    auto _full_class_code = (FullClassCode) func.full_class_code();
    if (_full_class_code == FullClassCode::ETHERNET_CONTROLLER) {
      assert(!ethernet_controller_pci_func && "found multiple ethernet controller");
      ethernet_controller_pci_func = func;
    } else if (_full_class_code == FullClassCode::USB_CONTROLLER) {
      switch (func.prog_if()) {
      case 0x00:
        uhci_func = func;
        break;
      case 0x10:
        ohci_func = func;
        break;
      case 0x20:
        ehci_func = func;
        break;
      case 0x30:
        xhci_func = func;
        break;
      }
    }
  });
  assert(ethernet_controller_pci_func && "ethernet controller not found");
}

void PCIFunction::dump() const {
  printf("%d:%d.%d", bus_id_, dev_id_, func_id_);

  auto _vendor_id = vendor_id();
  printf(" [vendor] %s (0x%x)", get_vendor_desc(_vendor_id), _vendor_id);

  // device_id
  auto _device_id = device_id();
  printf(" [device] (0x%x)", _device_id);

  // class & subclass code
  auto _full_class_code = full_class_code();
  printf("\n  [class] %s (0x%x)", get_class_desc(_full_class_code), _full_class_code);
  printf("\n");
}

Bar PCIFunction::getBar(int idx) const {
  Bar ret;
  ret.set_idx(idx);
  ret.set_raw(bar(idx));
  if (ret.get_raw() == 0) {
    return ret;
  }

  // set all 1
  set_bar(ret.get_idx(), 0xFFFFFFFF);

  // read size
  ret.set_size(bar(ret.get_idx()));

  // recover original value
  set_bar(ret.get_idx(), ret.get_raw());
  return ret;
}

Bar PCIFunction::getSoleBar() const {
  Bar retbar;
  for (int i = 0; i < 6; ++i) {
    auto curbar = getBar(i);
    if (curbar) {
      assert(!retbar);
      retbar = curbar;
    }
  }
  assert(retbar);
  return retbar;
}
