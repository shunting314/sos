#include <kernel/wifi/wifi.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/config.h>

bss_meta bss_meta_list[32];
int nbss_meta = 0;

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

int n_too_small_frame = 0;
int n_probe_request = 0;
int n_management_action = 0;

int n_data_frame_data = 0;

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
    while (left > 0) {
      if (ptr[0] == ELEMENT_ID_SSID) {
        // SSID
        // register_found_bss(hdr->addr2, (char*) ptr + 2, ptr[1]);

        if (ptr[1] == strlen(target_net_name) && strncmp((char*) ptr + 2, target_net_name, ptr[1]) == 0) {
          if (register_found_bss(hdr->addr2, (char*) ptr + 2, ptr[1])) {
            #if 0
            printf("beacon frame length %d\n", buflen);
            // hexdump(frame_buf, buflen);
            hexdump(frame_buf - 16, buflen + 16);
            #endif
          }
        }

        // if not break here the while loop will eventually fail on
        //   assert(left >= ptr[1] + 2);
        // Don't know why yet.
        break;
      }
      // printf("left %d, type %d, len %d\n", left, ptr[0], ptr[1]);
      assert(left >= ptr[1] + 2);
      int toskip = ptr[1] + 2;
      ptr += toskip;
      left -= toskip;
    }
    assert(left >= 0); // TODO: change to 'assert(left == 0)' eventually
  } else if (hdr->type == FRAME_TYPE_MANAGEMENT && hdr->subtype == MANAGEMENT_FRAME_PROBE_REQUEST) {
    #if 1
    if (n_probe_request < 1) {
      printf("Received a probe request. #%d\n", n_probe_request++);
      hexdump(frame_buf, buflen);
    }
    #endif
  } else if (hdr->type == FRAME_TYPE_MANAGEMENT && hdr->subtype == MANAGEMENT_FRAME_ACTION) {
    if (n_management_action < 5) {
      printf("Received a management action frame. #%d\n", n_management_action++);
    }
  } else if (hdr->type == FRAME_TYPE_DATA && hdr->subtype == DATA_FRAME_DATA) {
    #if 0
    if (n_data_frame_data < 5) {
      printf("Received a data frame data subtype. #%d\n", n_data_frame_data++);
    }
    #endif
  } else {
    printf("Received a non-beacon frame type %d, subtype %d\n", hdr->type, hdr->subtype);
  }
  // hexdump(frame_buf, min(buflen, 256));
}

/*
 * Compose the probe request manually for now. But maybe we can build some utilities
 * to help composing an 802.11 frame.
 */
int create_probe_request(uint8_t* buf, int capability) {
  int len = 0;
#define ADD_BYTE(val) do { \
  assert(len < capability); \
  buf[len++] = val; \
} while(0)

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
  ADD_BYTE(ELEMENT_ID_SSID);
  const char* target_net_name = get_wireless_network_name();
  ADD_BYTE(strlen(target_net_name));
  for (int i = 0; target_net_name[i]; ++i) {
    ADD_BYTE(target_net_name[i]);
  }

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

  // XXX The hardware will add the FCS?
  return len;
#undef ADD_BYTE
}
