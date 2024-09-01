#pragma once

#include <ctype.h>

struct IpAddr {
  uint8_t addr[4];

	IpAddr() {}

	IpAddr(const IpAddr &rhs) {
		memcpy(addr, rhs.addr, sizeof(addr));
	}

	IpAddr(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
		addr[0] = a;
		addr[1] = b;
		addr[2] = c;
		addr[3] = d;
	}

	IpAddr(const char* str) {
		const char *left = str;
		const char *right;
		int cnt = 0;
		while (true) {
			assert(isdigit(*left));
			right = left;
			int val = 0;
			while (isdigit(*right)) {
				val = val * 10 + (*right - '0');
				++right;
			}
			assert(cnt < 4);
			assert(val <= 255);
			addr[cnt++] = val;
			if (*right == '\0') {
				break;
			}
			assert(cnt < 4);
			assert(*right == '.');
			left = right + 1;
		}
		assert(cnt == 4);	
	}

	static IpAddr allZero() {
		IpAddr addr;
		for (int i = 0; i < 4; ++i) {
			addr.addr[i] = 0;
		}
		return addr;
	}

  // dump IpAddr without newline
  void dump() {
    for (int i = 0; i < 4; ++i) {
      printf("%d", addr[i]);
      if (i != 3) {
        printf(".");
      }
    }
  }

  bool operator==(const IpAddr &rhs) {
    for (int i = 0; i < 4; ++i) {
      if (addr[i] != rhs.addr[i]) {
        return false;
      }
    }
    return true;
  }

	bool operator!=(const IpAddr &rhs) {
		return !(*this == rhs);
	}
} __attribute__((packed));

static_assert(sizeof(IpAddr) == 4);

struct MacAddr {
  uint8_t addr[6];

  MacAddr() {}

  MacAddr(char *addr) {
    memcpy(this->addr, addr, sizeof(this->addr));
  }

  MacAddr(const MacAddr &rhs) {
    memcpy(this->addr, rhs.addr, sizeof(this->addr));
  }

	static MacAddr allZero() {
		MacAddr addr;
		for (int i = 0; i < 6; ++i) {
			addr.addr[i] = 0;
		}
		return addr;
	}

	static MacAddr bcastAddr() {
		MacAddr addr;
		for (int i = 0; i < 6; ++i) {
			addr.addr[i] = 0xFF;
		}
		return addr;
	}

  // dump MacAddr without newline
  void dump() {
    for (int i = 0; i < 6; ++i) {
      printf("%02x", addr[i]);
      if (i != 5) {
        printf(":");
      }
    }
  }

  bool operator==(const MacAddr &rhs) {
    for (int i = 0; i < 6; ++i) {
      if (addr[i] != rhs.addr[i]) {
        return false;
      }
    }
    return true;
  }

	bool operator!=(const MacAddr &rhs) {
		return !(*this == rhs);
	}
} __attribute__((packed));

static_assert(sizeof(MacAddr) == 6);

struct EthHeader {
  MacAddr dstAddr;
  MacAddr srcAddr;
  uint16_t proto;

  void dump() {
    printf("DstMac: "); dstAddr.dump(); printf("\n");
    printf("SrcMac: "); srcAddr.dump(); printf("\n");
    printf("Ethernet Proto: 0x%x\n", ntohs(proto));
  }
};

static_assert(sizeof(EthHeader) == 14);

enum EthProto {
  ETH_PROTO_IP = 0x0800,
  ETH_PROTO_ARP = 0x0806,
};

struct ArpHeader {
  uint16_t hardware_address_type;
  uint16_t protocol_address_type;
  uint8_t hardware_address_length;
  uint8_t protocol_address_length;
  uint16_t operation;
  MacAddr src_hardware_address;
  IpAddr src_protocol_address;
  MacAddr dst_hardware_address;
  IpAddr dst_protocol_address;

  void dump() {
    printf("ARP:\n");
    printf("HW Type 0x%x\n", ntohs(hardware_address_type));
    printf("Proto Type 0x%x\n", ntohs(protocol_address_type));
    printf("HW Addr Len %d\n", hardware_address_length);
    printf("PROTO Addr Len %d\n", protocol_address_length);
    printf("Op 0x%x\n", ntohs(operation));
    printf("SRC HW Addr: "); src_hardware_address.dump(); printf("\n");
    printf("SRC PROTO Addr: "); src_protocol_address.dump(); printf("\n");
    printf("DST HW Addr: "); dst_hardware_address.dump(); printf("\n");
    printf("DST PROTO Addr: "); dst_protocol_address.dump(); printf("\n");
  }
} __attribute__((packed));

static_assert(sizeof(struct ArpHeader) == 28);

enum {
  HTYPE_ETHERNET = 1, // indicate an ethernet MAC address
};

enum {
	PTYPE_IP = 0x0800, // indicate an IP address
};
