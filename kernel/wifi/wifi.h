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
  MANAGEMENT_FRAME_BEACON = 8,
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

struct bss_meta {
  macaddr_t bssid;
  char name[256];
};

bool register_found_bss(macaddr_t addr, const char* name, int len);
