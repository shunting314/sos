#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include "hdr.h"
#include "util.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: arpreq <ip addr>\n");
    return -1;
  }
  
  IpAddr queryIpAddr(argv[1]);
  printf("Query IpAddr: "); queryIpAddr.dump(); printf("\n");
  int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (sock == -1) {
    perror("socket()");
    exit(1);
  }
  printf("Successfully opened socket: %d\n", sock);

  struct ifreq ifr;
  const char *interface_name = get_interface_name();
  strncpy(ifr.ifr_name, interface_name, strlen(interface_name));
  if (ioctl(sock, SIOCGIFINDEX, &ifr) == -1) {
    perror("SIOCGIFINDEX");
    return 1;
  }
  int ifindex = ifr.ifr_ifindex;

  if (ioctl(sock, SIOCGIFHWADDR, &ifr) == -1) {
    perror("SIOCGIFHWADDR");
    exit(1);
  }

  MacAddr selfMac(ifr.ifr_hwaddr.sa_data);

  struct sockaddr_ll socket_address;
  // prepare sockaddr_ll
  socket_address.sll_family = PF_PACKET;
  socket_address.sll_protocol = htons(ETH_P_IP);
  socket_address.sll_ifindex = ifindex;
  socket_address.sll_hatype = ARPHRD_ETHER;
  socket_address.sll_pkttype = PACKET_OTHERHOST;
  socket_address.sll_halen = 0;
  for (int i = 0; i < 6; ++i) {
    socket_address.sll_addr[i] = 0xFF;
  }
  socket_address.sll_addr[6] = socket_address.sll_addr[7] = 0;

  char buffer[sizeof(EthHeader) + sizeof(ArpHeader)];
  EthHeader *ethHdr = (EthHeader *) buffer;
  ArpHeader *arpHdr = (ArpHeader *) &buffer[sizeof(EthHeader)];

  int r;
  { // send
    // setup ethernet header
    ethHdr->dstAddr = MacAddr::bcastAddr();
    ethHdr->srcAddr = selfMac;
    ethHdr->proto = htons(ETH_PROTO_ARP);

    // setup Arp header
    arpHdr->hardware_address_type = htons(HTYPE_ETHERNET);
    arpHdr->protocol_address_type = htons(PTYPE_IP);
    arpHdr->hardware_address_length = 6;
    arpHdr->protocol_address_length = 4;
    arpHdr->operation = htons(1); // request
    arpHdr->src_hardware_address = selfMac;
    // all zero will be returned in response and will not be cached in the destination host
    arpHdr->src_protocol_address = IpAddr::allZero();
    arpHdr->dst_hardware_address = MacAddr::allZero();
    arpHdr->dst_protocol_address = queryIpAddr;

    printf("======= Request\n");
    ethHdr->dump();
    arpHdr->dump();

    r = sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &socket_address, sizeof(socket_address));
    if (r == -1) {
      perror("sendto");
      exit(1);
    }
    printf("'sendto' returns %d\n", r);
  }

  while (true) { // recv
    r = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
    if (r == -1) {
      perror("recvfrom");
      exit(1);
    }
    if (htons(ethHdr->proto) != ETH_PROTO_ARP) {
      continue;
    }

    if (ntohs(arpHdr->operation) != 2) { // not a reply
      continue;
    }
   
    // only check frames targeting self.
    if (ethHdr->dstAddr != selfMac) {
      continue;
    }

    // not for the asked IP address
    if (arpHdr->src_protocol_address != queryIpAddr) {
      continue;
    }

    assert(ethHdr->srcAddr == arpHdr->src_hardware_address);
    assert(ethHdr->dstAddr == arpHdr->dst_hardware_address);
    assert(arpHdr->hardware_address_type == htons(HTYPE_ETHERNET));
    assert(arpHdr->protocol_address_type == htons(PTYPE_IP));
    assert(arpHdr->hardware_address_length == 6);
    assert(arpHdr->protocol_address_length == 4);

    // this IPAddr in the response matches the corresponding field
    // in the request. Dst host will not put the all zero IpAddr
    // in it's ARP cache. But if we send an arbitrary non all zero 
    // IpAddr, it may pollute the dst host's ARP cache.
    assert(arpHdr->dst_protocol_address == IpAddr::allZero());

    MacAddr ansMac = arpHdr->src_hardware_address;
    assert(ansMac != MacAddr::allZero());
    printf("Mac address for IP "); queryIpAddr.dump(); printf(" is "); ansMac.dump(); printf("\n");
    break;
  }

  printf("bye\n");
  return 0;
}
