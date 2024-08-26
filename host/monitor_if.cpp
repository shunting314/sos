/*
 * Follow https://stackoverflow.com/questions/16710040/arp-request-and-reply-using-c-socket-programming
 * to monitor a network interface.
 *
 * The code uses some linux header file and does not work on mac.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

struct IpAddr {
  uint8_t addr[4];

  // dump IpAddr without newline
  void dump() {
    for (int i = 0; i < 4; ++i) {
      printf("%d", addr[i]);
      if (i != 3) {
        printf(".");
      }
    }
  }
};

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
} __attribute__((packed));

static_assert(sizeof(MacAddr) == 6);
MacAddr selfMac;

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
  uint16_t hardware_type;
  uint16_t protocol_type;
  uint8_t hardware_address_length;
  uint8_t protocol_address_length;
  uint16_t operation;
  MacAddr src_hardware_address;
  IpAddr src_protocol_address;
  MacAddr dst_hardware_address;
  IpAddr dst_protocol_address;

  void dump() {
    printf("ARP:\n");
    printf("HW Type 0x%x\n", ntohs(hardware_type));
    printf("Proto Type 0x%x\n", ntohs(protocol_type));
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

void hexdump(char *buf, int len) {
  for (int i = 0; i < len; ++i) {
    printf(" %02x", (uint8_t) buf[i]);
    if (i % 16 == 15) {
      printf("\n");
    }
  }
  printf("\n");
}

void usage(int exitCode) {
  printf("Usage...\n");
  exit(exitCode);
}

void handleFrame(char *buffer, int length) {
  printf("length %d\n", length);
  if (length < sizeof(EthHeader)) {
    hexdump(buffer, length);
    printf("Skip the packet since it's shorter than a EthHeader\n"); 
    return;
  }

  EthHeader *ethHdr = (EthHeader *) buffer;
  ethHdr->dump();
  printf("toSelf: %d, fromSelf %d\n", ethHdr->dstAddr == selfMac, ethHdr->srcAddr == selfMac);
  if (ntohs(ethHdr->proto) == ETH_PROTO_ARP) {
    // parse ARP
    printf("Got an ARP frame\n");

    // the buffer does not contain the checksum?
    if (length != sizeof(EthHeader) + sizeof(ArpHeader)) {
      printf("Unexpected length %d, expected %d\n", length, (int) (sizeof(EthHeader) + sizeof(ArpHeader) + 2));
      hexdump(buffer, length);
      return;
    }
    ArpHeader *arp = (ArpHeader*) (buffer + sizeof(EthHeader));
    arp->dump();
    return;
  }
  hexdump(buffer, length);
}

int xdigitToVal(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  } else if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  } else if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  } else {
    assert(false && "invalid xdigit");
  }
}

int main(int argc, char **argv) {
  char buffer[4096];
  const char *interfaceName = nullptr;
  const char *savedFramePath = nullptr;

  for (int i = 1; i + 1 < argc; i += 2) {
    if (argv[i][0] != '-' || strlen(argv[i]) != 2) {
      usage(1);
    }
    switch (argv[i][1]) {
    case 'i':
      interfaceName = argv[i + 1];
      break;
    case 'f':
      savedFramePath = argv[i + 1];
      break;
    default:
      printf("Invalid option '-%c'\n", argv[i][1]);
      return -1;
    }
  }

  assert(interfaceName);

  // requires root
  int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (sock == -1) {
    perror("socket()");
    exit(1);
  }
  printf("Successfully opend socket: %d\n", sock);

  struct ifreq ifr;
  strncpy(ifr.ifr_name, interfaceName, strlen(interfaceName));
  if (ioctl(sock, SIOCGIFHWADDR, &ifr) == -1) {
    perror("SIOCGIFHWADDR");
    exit(1);
  }
  selfMac = MacAddr(ifr.ifr_hwaddr.sa_data);
  printf("Self mac addr: "); selfMac.dump(); printf("\n");

  int length;

  if (savedFramePath) {
    FILE *fp = fopen(savedFramePath, "rb");
    if (!fp) {
      printf("Fail to open file %s\n", savedFramePath);
      return -1;
    }
    length = 0;
    char word[16];
    while (fscanf(fp, "%s", word) != EOF) {
      if (strlen(word) != 2 || !isxdigit(word[0]) || !isxdigit(word[1])) {
        printf("Invalid world %s read from file\n", word);
        return -1;
      }
      uint8_t val = (xdigitToVal(word[0]) << 4) | xdigitToVal(word[1]);
      assert(length < sizeof(buffer));
      buffer[length++] = val;
    }

    fclose(fp);
    printf("Loaded %d bytes from file %s\n", length, savedFramePath);
    handleFrame(buffer, length);
    return 0;
  }

  while (true) {
    length = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
    if (length == -1) {
      perror("recvfrom()");
      exit(1);
    }
    printf("Input newline character to dump the next packet:\n");
    char ch;
    while (true) {
      ch = fgetc(stdin);
      if (ch == '\n') {
        break;
      }
      if (ch == EOF) {
        printf("Got EOF, bye\n");
        return 0;
      }
    }

    handleFrame(buffer, length);
  }
  printf("bye\n");
  return 0;
}
