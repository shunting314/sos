#include <kernel/wifi/wifi.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

bss_meta bss_meta_list[32];
int nbss_meta = 0;

bool register_found_bss(macaddr_t addr, const char* name, int len) {
  for (int i = 0; i < nbss_meta; ++i) {
    if (bss_meta_list[i].bssid == addr) {
      return false; // already registered
    }
  }
  assert(nbss_meta < sizeof(bss_meta_list) / sizeof(bss_meta_list[0]));
  auto& picked_meta = bss_meta_list[nbss_meta++];
  picked_meta.bssid = addr;
  assert(len < sizeof(picked_meta.name)); // need 1 byte for \0
  memmove(picked_meta.name, name, len);
  picked_meta.name[len] = '\0';

  printf("Found BSS %d: %s, addr: ", nbss_meta - 1, picked_meta.name);
  picked_meta.bssid.print();
  printf("\n");
  return true;
}

void _parse_80211_frame(uint8_t* frame_buf, uint32_t buflen) {
  ieee80211_hdr* hdr = (ieee80211_hdr*) frame_buf;
  // assert(sizeof(*hdr) <= buflen);
  if (sizeof(*hdr) > buflen) {
    // printf("Drop a too small frame. buflen %d\n", buflen);
    return;
  }
  if (hdr->type == FRAME_TYPE_MANAGEMENT && hdr->subtype == MANAGEMENT_FRAME_BEACON) {
    // printf("Received a beacon\n");
    assert(hdr->addr1.is_broadcast());
    assert(hdr->addr2 == hdr->addr3);
    uint8_t* ptr = frame_buf + sizeof(*hdr) + sizeof(beacon_body);
    int left = buflen - sizeof(*hdr) - sizeof(beacon_body);
    assert(left >= 0);

    // parse the information elements
    while (left > 0) {
      if (ptr[0] == 0) {
        // SSID
        register_found_bss(hdr->addr2, (char*) ptr + 2, ptr[1]);

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
  }
  // hexdump(frame_buf, min(buflen, 256));
}
