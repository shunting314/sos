#pragma once

#include <kernel/wifi/legacy/wifi.h>

void _parse_80211_frame(uint8_t* frame_buf, uint32_t buflen);

enum {
  FRAME_TYPE_MANAGEMENT = 0,
  FRAME_TYPE_CONTROL = 1,
  FRAME_TYPE_DATA = 2,
  // 3 is reserved
};

enum {
  MANAGEMENT_FRAME_PROBE_REQUEST = 4,
  MANAGEMENT_FRAME_PROBE_RESPONSE = 5,
  MANAGEMENT_FRAME_BEACON = 8,
  MANAGEMENT_FRAME_ACTION = 13,
};

enum {
  DATA_FRAME_DATA = 0,
};

class macaddr_t {
 public:
  bool is_broadcast() {
    for (int i = 0; i < 6; ++i) {
      if (addr[i] != 0xff) {
        return false;
      }
    }
    return true;
  }

  bool operator==(const macaddr_t& other) {
    for (int i = 0; i < 6; ++i) {
      if (addr[i] != other.addr[i]) {
        return false;
      }
    }
    return true;
  }

  void print() const {
    for (int i = 0; i < 6; ++i) {
      printf("%x", addr[i]);
      if (i != 5) {
        printf(":");
      }
    }
  }
 public: 
  uint8_t addr[6];
};

struct ieee80211_hdr {
  // frame control
  uint16_t protocol : 2;
  uint16_t type : 2;
  uint16_t subtype : 4;
  uint16_t to_ds : 1;
  uint16_t from_ds : 1;
  uint16_t more_fragment : 1;
  uint16_t retry : 1;
  uint16_t power_management : 1;
  uint16_t more_data : 1;
  uint16_t wep : 1; // wired equivalent privacy
  uint16_t order : 1;

  uint16_t duration_id;
  // dest, src, bssid
  macaddr_t addr1, addr2, addr3;
  uint16_t seq_ctl; // sequence control
} __attribute__((packed));

enum {
  MANAGEMENT_FRAME_INFO_ELEMENT_SSID = 0,
};

struct beacon_body {
  uint64_t timestamp;
  uint16_t beacon_interval;
  uint16_t capability_info;
} __attribute__((packed));

enum {
  ELEMENT_ID_SSID = 0,
  ELEMENT_ID_SUPPORTED_RATES = 1,
  ELEMENT_ID_DS_PARAMETER_SET = 3, // use a single byte to encoder the channel number used by the network
  ELEMENT_ID_TIM = 5, // traffic indication map
  ELEMENT_ID_COUNTRY = 7,
  ELEMENT_ID_BSS_LOAD = 11,
  ELEMENT_ID_POWER_CONSTRAINT = 32,
  ELEMENT_ID_TPC_REPORT = 35,
  ELEMENT_ID_ERP = 42,
  ELEMENT_ID_HT_CAPABILITIES = 45,
  ELEMENT_ID_RSN = 48, // security related
  ELEMENT_ID_EXTENDED_SUPPORTED_RATES = 50,  // 0x32
  ELEMENT_ID_SUPPORTED_OPERATING_CLASSES = 59,
  ELEMENT_ID_HT_OPERATION = 61,
  ELEMENT_ID_INTERWORKING = 107,
  ELEMENT_ID_ADVERTISEMENT_PROTOCOL = 108,
  ELEMENT_ID_EXTENDED_CAPABILITIES = 127,
  ELEMENT_ID_VENDOR_SPECIFIC = 221,
};

struct bss_meta {
  macaddr_t bssid;
  char* name; // the name is malloced but never freed
};

bool register_found_bss(macaddr_t addr, const char* name, int len);
// let the caller pass in the buffer instead of returning a allocated buffer
// so we can save a memcpy later on.
int create_probe_request(uint8_t* buf, int capability);

#define MAC_ADDR_LEN 6
extern uint8_t self_mac_addr[MAC_ADDR_LEN];

static bool is_all_zero_mac_addr(uint8_t* addr) {
  for (int i = 0; i < 6; ++i) {
    if (addr[i] != 0) {
      return false;
    }
  }
  return true;
}
