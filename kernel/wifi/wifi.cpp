#include <kernel/wifi/wifi.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/config.h>

bss_meta bss_meta_list[32];
int nbss_meta = 0;
int assoc_id = -1;

uint8_t self_mac_addr[MAC_ADDR_LEN];

// a helper function to register the seen bss. The network name and addr
// will be printed when it's seen for the first time.
bool register_found_bss(macaddr_t addr, const char* name, int len) {
  for (int i = 0; i < nbss_meta; ++i) {
    if (bss_meta_list[i].bssid == addr) {
      return false; // already registered
    }
  }
  assert(nbss_meta < sizeof(bss_meta_list) / sizeof(bss_meta_list[0]));
  auto& picked_meta = bss_meta_list[nbss_meta++];
  picked_meta.bssid = addr;
  picked_meta.name = strndup(name, len);

  printf("Found BSS %d, name: %s, addr: ", nbss_meta - 1, picked_meta.name);
  picked_meta.bssid.print();
  printf("\n");
  return true;
}

macaddr_t get_wireless_network_mac() {
  static macaddr_t ap_mac;
  if (!ap_mac.is_allzero()) {
    return ap_mac;
  }

  // slow path. find the mac address
  const char* target_net_name = get_wireless_network_name();
  int found_idx = -1;
  for (int i = 0; i < nbss_meta; ++i) {
    if (strcmp(target_net_name, bss_meta_list[i].name) == 0) {
      found_idx = i;
      break;
    }
  }
  assert(found_idx >= 0);
  ap_mac = bss_meta_list[found_idx].bssid;
  return ap_mac;
}

/*
 * beacon and probe response have the same format. Thus share a dump
 * function.
 */
void dump_beacon_or_probe_response(uint8_t *frame_buf, uint32_t len) {
  ieee80211_hdr* hdr = (ieee80211_hdr*) frame_buf;
  // bss id
  printf(" addr1: "); hdr->addr1.print(); printf("\n");
  printf(" addr2: "); hdr->addr2.print(); printf("\n");
  assert(hdr->addr2 == hdr->addr3);
  printf(" retry: %d\n", hdr->retry);

  uint8_t *ptr = frame_buf + sizeof(*hdr) + sizeof(beacon_body);
  int left = len - sizeof(*hdr) - sizeof(beacon_body);

  while (left > FCS_LEN) {
    assert(left - FCS_LEN >= ptr[1] + 2);
    int toskip = ptr[1] + 2;
    ptr += toskip;
    left -= toskip;
  }
  assert(left == FCS_LEN);
  printf("beacon/probe response is valid\n");
}

int n_too_small_frame = 0;
int n_probe_request = 0;
int n_probe_response = 0;
int n_management_action = 0;

int n_data_frame_data = 0;
int n_authentication = 0;
int n_assoc_response = 0;

void _parse_80211_frame(uint8_t* frame_buf, uint32_t buflen) {
  ieee80211_hdr* hdr = (ieee80211_hdr*) frame_buf;
  const char* target_net_name = get_wireless_network_name();

  if (sizeof(*hdr) > buflen) {
    // Note this may be control frames which can be quite short
    #if 0
    if (n_too_small_frame < 5) {
      printf("Drop a too small frame. buflen %d. #%d\n", buflen, n_too_small_frame++);
    }
    #endif
    return;
  }
  assert(hdr->protocol == 0);
  if (hdr->type == FRAME_TYPE_MANAGEMENT && hdr->subtype == MANAGEMENT_FRAME_BEACON) {
    // printf("Received a beacon\n");
    assert(hdr->addr1.is_broadcast());
    assert(hdr->addr2 == hdr->addr3);
    uint8_t* ptr = frame_buf + sizeof(*hdr) + sizeof(beacon_body);
    int left = buflen - sizeof(*hdr) - sizeof(beacon_body);
    assert(left >= 0);

    // parse the information elements
    while (left > FCS_LEN) {
      if (ptr[0] == ELEMENT_ID_SSID) {
        // SSID
        // register_found_bss(hdr->addr2, (char*) ptr + 2, ptr[1]);

        if (ptr[1] == strlen(target_net_name) && strncmp((char*) ptr + 2, target_net_name, ptr[1]) == 0) {
          if (register_found_bss(hdr->addr2, (char*) ptr + 2, ptr[1])) {
            #if 0
            printf("beacon frame length %d\n", buflen);
            hexdump(frame_buf, buflen);
            #endif
          }
        }
      }
      assert(left - FCS_LEN >= ptr[1] + 2);
      int toskip = ptr[1] + 2;
      ptr += toskip;
      left -= toskip;
    }
    assert(left == FCS_LEN);
  } else if (hdr->type == FRAME_TYPE_MANAGEMENT && hdr->subtype == MANAGEMENT_FRAME_PROBE_REQUEST) {
    #if 0
    if (n_probe_request < 1) {
      printf("Received a probe request. #%d\n", n_probe_request++);
      hexdump(frame_buf, buflen);
    }
    #endif
  } else if (hdr->type == FRAME_TYPE_MANAGEMENT && hdr->subtype == MANAGEMENT_FRAME_PROBE_RESPONSE) {
    #if 0
    if (n_probe_response < 3) {
      printf("+ Received a probe response. #%d, len %d\n", n_probe_response++, buflen);
      dump_beacon_or_probe_response(frame_buf, buflen);
    }
    #endif
  } else if (hdr->type == FRAME_TYPE_MANAGEMENT && hdr->subtype == MANAGEMENT_FRAME_ACTION) {
    if (n_management_action < 5) {
      printf("Received a management action frame. #%d\n", n_management_action++);
    }
  } else if (hdr->type == FRAME_TYPE_MANAGEMENT && hdr->subtype == MANAGEMENT_FRAME_AUTHENTICATION) {
    if (n_authentication < 3) {
      printf("Received an authentication frame. #%d\n", n_authentication++);
      assert(buflen >= sizeof(ieee80211_hdr) + sizeof(authentication_body));
      authentication_body *auth_body = (authentication_body *) (hdr + 1);
      printf("algo %d, seq no %d, status code %d\n", auth_body->algorithm_number, auth_body->transaction_seqno, auth_body->status_code);
    }
  } else if (hdr->type == FRAME_TYPE_MANAGEMENT && hdr->subtype == MANAGEMENT_FRAME_ASSOCIATION_RESPONSE) {
    if (n_assoc_response < 3) {
      printf("Received an association response. #%d\n", n_assoc_response++);
      hexdump(frame_buf, min(buflen, 256));
      int off = sizeof(ieee80211_hdr);
      assert(off + 2 <= buflen);
      uint16_t capability_info = *(uint16_t*) (frame_buf + off);
      off += 2;
      assert(off + 2 <= buflen);
      uint16_t status_code = *(uint16_t*) (frame_buf + off);
      off += 2;
      assert(off + 2 < buflen);
      if (assoc_id < 0) {
        assoc_id = *(uint16_t*) (frame_buf + off);
      } else {
        // don't support re-associate yet.
        assert(assoc_id == *(uint16_t*) (frame_buf + off));
      }
      printf("capa info 0x%x, status code %d, assoc_id 0x%x\n", capability_info, status_code, assoc_id);
    }
  } else if (hdr->type == FRAME_TYPE_DATA && hdr->subtype == DATA_FRAME_DATA) {
    #if 1
    if (n_data_frame_data < 3) {
      printf("Received a data frame data subtype. #%d\n", n_data_frame_data++);
    }
    #endif
  } else {
    printf("Received a unrecognized frame type %d, subtype %d, buflen %d\n", hdr->type, hdr->subtype, buflen);
    hexdump(frame_buf, min(buflen, 256));
    assert(false && "_parse_80211_frame hlt");
  }
  // hexdump(frame_buf, min(buflen, 256));
}

#define ADD_BYTE(val) do { \
  assert(len < capacity); \
  buf[len++] = val; \
} while(0)

int add_ssid(uint8_t *buf, int capacity, int len) {
  ADD_BYTE(ELEMENT_ID_SSID);
  const char* target_net_name = get_wireless_network_name();
  ADD_BYTE(strlen(target_net_name));
  for (int i = 0; target_net_name[i]; ++i) {
    ADD_BYTE(target_net_name[i]);
  }
  return len;
}

int add_supported_rates(uint8_t *buf, int capacity, int len) {
  // supported rates
  // the unit is 500kbps
  uint8_t supported_rates[] = {
    2, // 1mbps
    4, // 2mbps
    11, // 5.5mbps
    22, // 11mbps
  };
  ADD_BYTE(ELEMENT_ID_SUPPORTED_RATES);
  ADD_BYTE(sizeof(supported_rates) / sizeof(*supported_rates));
  for (int i = 0; i < sizeof(supported_rates) / sizeof(*supported_rates); ++i) {
    ADD_BYTE(supported_rates[i]);
  }
  return len;
}

// only work for a management frame so far
int create_ieee80211_hdr(uint8_t *buf, int capacity, int len, int subtype) {
  // frame control
  uint8_t protocol = 0;
  uint8_t type = FRAME_TYPE_MANAGEMENT;
  ADD_BYTE(protocol | (type << 2) | (subtype << 4));
  ADD_BYTE(0);

  // duration/id
  ADD_BYTE(0);
  ADD_BYTE(0);

  // dst address
  auto ap_mac = get_wireless_network_mac();
  for (int i = 0; i < 6; ++i) {
    ADD_BYTE(ap_mac.addr[i]);
  }

  // src address
  assert(!is_all_zero_mac_addr(self_mac_addr));
  for (int i = 0; i < 6; ++i) {
    ADD_BYTE(self_mac_addr[i]);
  }

  // BSSID
  for (int i = 0; i < 6; ++i) {
    ADD_BYTE(ap_mac.addr[i]);
  }

  // sequence control
  uint16_t fragment_id = 0;
  uint16_t sequence_id = 0; // TODO: how should we manage sequence number increment?
  uint16_t sequence_control = fragment_id | (sequence_id << 4);
  // little endian
  ADD_BYTE(sequence_control & 0xFF);
  ADD_BYTE(sequence_control >> 8);

  return len;
}

/*
 * Compose the probe request manually for now. But maybe we can build some utilities
 * to help composing an 802.11 frame.
 */
int create_probe_request(uint8_t* buf, int capacity) {
  int len = 0;

  // TODO: make create_ieee80211_hdr more generic and call it here.
  // frame control
  uint8_t protocol = 0;
  uint8_t type = FRAME_TYPE_MANAGEMENT;
  uint8_t subtype = MANAGEMENT_FRAME_PROBE_REQUEST;
  ADD_BYTE(protocol | (type << 2) | (subtype << 4));
  ADD_BYTE(0);

  // duration/id
  ADD_BYTE(0);
  ADD_BYTE(0);

  // dst address. Use the broadcast address
  for (int i = 0; i < 6; ++i) {
    ADD_BYTE(0xFF);
  }

  // src address.
  assert(!is_all_zero_mac_addr(self_mac_addr));
  for (int i = 0; i < 6; ++i) {
    ADD_BYTE(self_mac_addr[i]);
  }

  // BSSID
  for (int i = 0; i < 6; ++i) {
    ADD_BYTE(0xFF);
  }

  // sequence control
  uint16_t fragment_id = 0;
  uint16_t sequence_id = 0; // TODO: how should we manage sequence number increment?
  uint16_t sequence_control = fragment_id | (sequence_id << 4);
  // little endian
  ADD_BYTE(sequence_control & 0xFF);
  ADD_BYTE(sequence_control >> 8);

  // SSID
  len = add_ssid(buf, capacity, len);

  // supported rates
  len = add_supported_rates(buf, capacity, len);

  // NOTE: The hardware will add the FCS. Software can skip it
  return len;
}

int create_authentication_frame(uint8_t *buf, int capacity) {
  int len = 0;

  len = create_ieee80211_hdr(buf, capacity, len, MANAGEMENT_FRAME_AUTHENTICATION);

  uint16_t algo_num = 0; // open system authentication
  ADD_BYTE(algo_num & 0xFF);
  ADD_BYTE(algo_num >> 8);

  uint16_t seq_no = 1;
  ADD_BYTE(seq_no & 0xFF);
  ADD_BYTE(seq_no >> 8);

  uint16_t status = 0; // not needed for authentication 'request'
  ADD_BYTE(status & 0xFF);
  ADD_BYTE(status >> 8);

  return len;
}

int create_association_request(uint8_t *buf, int capacity) {
  int len = 0;

  len = create_ieee80211_hdr(buf, capacity, len, MANAGEMENT_FRAME_ASSOCIATION_REQUEST);

  uint16_t capability_info = 1;
  ADD_BYTE(capability_info & 0xFF);
  ADD_BYTE(capability_info >> 8);

  uint16_t listen_interval = 10;
  ADD_BYTE(listen_interval & 0xFF);
  ADD_BYTE(listen_interval >> 8);

  len = add_ssid(buf, capacity, len);
  len = add_supported_rates(buf, capacity, len);
  return len;
}
#undef ADD_BYTE
