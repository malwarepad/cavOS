#include <checksum.h>
#include <icmp.h>
#include <ipv4.h>
#include <linked_list.h>
#include <malloc.h>
#include <system.h>
#include <tcp.h>
#include <udp.h>
#include <util.h>

// IP(v4) layer (https://en.wikipedia.org/wiki/Internet_Protocol_version_4)
// Copyright (C) 2024 Panagiotis

#define IPV4_DEBUG 0

/*
 * The common theme you will see in this file
 * is that despite the sheer simplicity of the IP(v4) layer...

 * ... complexity still exists in the form of fragmentation
 * and re-assembly which does make everything harder
 * than it should be. Remember, no fragment order is guaranteed!
 */

/* IPv4 fragmentation: Linked-list utilities */

IPv4fragmentedPacket *netIPv4FindFragmentedPkg(NIC *nic, uint8_t *ip,
                                               uint16_t id) {
  IPv4fragmentedPacket *curr = nic->firstFragmentedPacket;
  while (curr) {
    if (curr->id == id && memcmp(&curr->source_address[0], ip, 4) == 0)
      break;
    curr = curr->next;
  }
  return curr;
}

bool netIPv4RemoveFragmentedPkg(NIC *nic, uint8_t *ip, uint16_t id) {
  return LinkedListRemove(&nic->firstFragmentedPacket,
                          netIPv4FindFragmentedPkg(nic, ip, id));
}

/* IPv4 fragmentation: Packet fragment cleanup/finalization */

bool netIPv4DiscardFragmentedPkg(NIC *nic, uint8_t *ip, uint16_t id) {
  IPv4fragmentedPacket *currFrag = netIPv4FindFragmentedPkg(nic, ip, id);
  if (!currFrag)
    return false;

  IPv4fragmentedPacketRaw *browse = currFrag->firstPacket;
  while (browse) {
    IPv4fragmentedPacketRaw *next = browse->next;

    free(browse->buffer);
    free(browse);

    browse = next;
  }

  netIPv4RemoveFragmentedPkg(nic, ip, id);
  return true;
}

uint32_t netIPv4FinishFragmentedPkg(NIC *nic, uint8_t *ip, uint16_t id,
                                    void **target, void *insertStart,
                                    uint32_t insertStartSize) {
  IPv4fragmentedPacket *ref = netIPv4FindFragmentedPkg(nic, ip, id);
  if (!ref)
    return 0;

  // (1) we still have the ethernet header to put!
  uint32_t finalSize = ref->max + insertStartSize;
  uint8_t *finalBuffer = (uint8_t *)malloc(finalSize);

  memset(finalBuffer, 0, finalSize);
  memcpy(finalBuffer, insertStart, insertStartSize); // (1)

  IPv4fragmentedPacketRaw *browse = ref->firstPacket;
  while (browse) {
    memcpy((size_t)finalBuffer + (size_t)insertStartSize + browse->index,
           browse->buffer, browse->size);
    browse = browse->next;
  }

  netIPv4DiscardFragmentedPkg(nic, ip, id);
  IPv4header *header = (size_t)finalBuffer + sizeof(netPacketHeader);
  header->length = switch_endian_16(finalSize); // just in case

  *target = finalBuffer;
  return finalSize;
}

/* Standard verification, not related with fragments! */

bool netIPv4Verify(NIC *nic, IPv4header *header, uint32_t size) {
  if (header->version != 0x04) {
    debugf("[networking::ipv4] Bad version, ignoring: %d\n", header->version);
    return false;
  }

  if (switch_endian_16(header->length) < 20) {
    debugf("[networking::ipv4] Too small length, ignoring: %d\n",
           switch_endian_16(header->length));
    return false;
  }

  if (size > nic->mintu &&
      switch_endian_16(header->length) != (size - sizeof(netPacketHeader))) {
    debugf(
        "[networking::ipv4] Bad length, ignoring! header{%d} != actual{%d}\n",
        switch_endian_16(header->length), size - sizeof(netPacketHeader));
    return false;
  }

  if (*(uint32_t *)(&header->destination_address[0]) &&
      *(uint32_t *)(&header->destination_address[0]) != (uint32_t)-1 &&
      memcmp(header->destination_address, nic->ip, 4)) {
    // Exception: Some routers remember past IPs and send DHCP requests using
    // those... We let said requests go through and later check them via the
    // transaction ID
    if (header->protocol == UDP_PROTOCOL &&
        switch_endian_16(((udpHeader *)((size_t)header + sizeof(IPv4header)))
                             ->destination_port) == 68)
      return true;

    debugf("[networking::ipv4] Bad IP, ignoring: %d:%d:%d:%d\n",
           header->destination_address[0], header->destination_address[1],
           header->destination_address[2], header->destination_address[3]);
    return false;
  }

  return true;
}

void netIPv4Receive(NIC *nic, void *body, uint32_t size) {
  IPv4header *header = (size_t)body + sizeof(netPacketHeader);
  if (!netIPv4Verify(nic, header, size))
    return;

  // save those seperately, packet might be fragmented!
  void    *finalBody = body;
  uint32_t finalSize = size;

  uint16_t flags = switch_endian_16(header->flags);
  uint16_t id = switch_endian_16(header->id);

  // fm -> FragMentation
  bool     fmMore = flags & IPV4_FLAGS_MORE_FRAGMENTS;
  uint16_t fmOffset = (flags & ~(IPV4_FLAGS_DONT_FRAGMENT |
                                 IPV4_FLAGS_MORE_FRAGMENTS | IPV4_FLAGS_RESV)) *
                      8;
  bool fmFinished = false; // fragmentation required && final packet composed

  if (fmMore || fmOffset) {
    // there is some kind of fragmentation

    IPv4fragmentedPacket *currFrag =
        netIPv4FindFragmentedPkg(nic, header->source_address, id);
    if (!currFrag) {
      // first fragment we received, register the whole struct
      currFrag = (IPv4fragmentedPacket *)LinkedListAllocate(
          &nic->firstFragmentedPacket, sizeof(IPv4fragmentedPacket));
      memcpy(currFrag->source_address, header->source_address, 4);
      currFrag->id = id;
    }

    uint32_t ipBodySize = switch_endian_16(header->length) -
                          sizeof(IPv4header); // minus header (20)
    currFrag->curr += ipBodySize;

    if (currFrag->curr > (UINT16_MAX)) {
      debugf("[net::ipv4] Reassembly issue: Overflowing maximum IPv4 body!\n");
      netIPv4DiscardFragmentedPkg(nic, header->source_address, id);
      return;
    }

    if (!fmMore) {
      // we're on the last fragment
      if (!currFrag->max) {
        // calculate the total length
        currFrag->max = fmOffset + ipBodySize;
      } else if (currFrag->max) {
        debugf(
            "[net::ipv4] Reassembly issue: More than one last fragments??!\n");
        netIPv4DiscardFragmentedPkg(nic, header->source_address, id);
        return;
      }
    }

#if IPV4_DEBUG
    debugf("[%d] %x dontFragment{%d} moreFragments{%d} (%d/%d) %d\n", id, flags,
           !!(flags & IPV4_FLAGS_DONT_FRAGMENT), fmMore, currFrag->curr,
           currFrag->max, fmOffset);
#endif

    // now save the newly received fragment/chunk to a buffer
    IPv4fragmentedPacketRaw *currFragRaw =
        (IPv4fragmentedPacketRaw *)malloc(sizeof(IPv4fragmentedPacketRaw));
    memset(currFragRaw, 0, sizeof(IPv4fragmentedPacketRaw));

    if (!currFrag->firstPacket || !currFrag->lastPacket) {
      currFrag->firstPacket = currFragRaw;
      currFrag->lastPacket = currFragRaw;
    } else {
      currFrag->lastPacket->next = currFragRaw;
      currFrag->lastPacket = currFragRaw; // a form of "caching", lol
    }

    currFragRaw->index = fmOffset;
    currFragRaw->size = ipBodySize;
    currFragRaw->buffer = malloc(ipBodySize);
    memcpy(currFragRaw->buffer, (size_t)header + sizeof(IPv4header),
           ipBodySize);

    if (currFrag->max) {
      // there is a maximum to check against
      if (currFrag->max == currFrag->curr) {
        // we have everything we need! awesome!
        finalSize = netIPv4FinishFragmentedPkg(
            nic, header->source_address, id, &finalBody, body,
            sizeof(netPacketHeader) + sizeof(IPv4header));
        fmFinished = true;
      } else if (currFrag->curr > currFrag->max) {
        debugf("[net::ipv4] Reassembly issue: More fragments following last "
               "one!\n");
        netIPv4DiscardFragmentedPkg(nic, header->source_address, id);
        return;
      }
    }

    // unless we have all fragments, don't process!
    if (!fmFinished)
      return;
  }

  switch (header->protocol) {
  case ICMP_PROTOCOL:
    netICMPreceive(nic, finalBody, finalSize);
    break;

  case UDP_PROTOCOL:
    netUdpReceive(nic, finalBody, finalSize);
    break;

  case TCP_PROTOCOL:
    netTcpReceive(nic, finalBody, finalSize);
    break;

  default:
    debugf("[ipv4] Odd protocol: %d\n", header->protocol);
    break;
  }

  if (fmFinished)
    free(finalBody);
}

// Don't use this function, it will NOT fragment!
void netIPv4SendInternal(NIC *nic, uint8_t *destination_mac,
                         uint8_t *destination_ip, void *data,
                         uint32_t data_size, uint8_t protocol, uint16_t flags,
                         uint16_t id) {
  uint32_t finalSize = sizeof(IPv4header) + data_size;
  if (finalSize > IPV4_MAX) {
    debugf("[networking::ipv4] Packet excedes limit, aborting: %d/%d\n",
           finalSize, IPV4_MAX);
    return;
  }
  if (!finalSize)
    return;

  uint8_t    *final = malloc(finalSize);
  IPv4header *header = (IPv4header *) final;

  memset(header, 0, sizeof(IPv4header));

  header->version = 0x04;
  header->ihl = 5; // no options required xd
  header->tos = 0;
  header->length = switch_endian_16(finalSize);
  header->id = switch_endian_16(id);
  header->flags = switch_endian_16(flags);
  header->ttl = 64;
  header->protocol = protocol;

  memcpy(header->source_address, nic->ip, 4);
  memcpy(header->destination_address, destination_ip, 4);

  // calculate checksum before request's finalized
  header->checksum = checksum(header, sizeof(IPv4header));

  memcpy((size_t) final + sizeof(IPv4header), data, data_size);

  sendPacket(nic, destination_mac, final, finalSize, 0x0800);

  free(final);
}

#define MAX_FREEID_CYCLES 1000
uint16_t netIPv4GetFreeId(NIC *nic, uint8_t *ip) {
  uint16_t out = rand();
  bool     cycleDone = false;

  // ensure this ID is not used at the time being. even though it's random,
  // you just never know...
  for (int i = 0; !cycleDone || i < MAX_FREEID_CYCLES; i++) {
    if (out == 0) // zero = error
      continue;
    IPv4fragmentedPacket *browse = nic->firstFragmentedPacket;
    if (!browse) // no history at all
      cycleDone = true;

    while (browse) {
      if (memcmp(browse->source_address, ip, 4) == 0 && out == browse->id)
        break;

      browse = browse->next;
      if (!browse) // we're done!
        cycleDone = true;
    }

    out = rand(); // re-generate, re-cycle...
  }

  if (cycleDone)
    return out;
  else
    return 0;
}

void netIPv4Send(NIC *nic, uint8_t *destination_mac, uint8_t *destination_ip,
                 void *data, uint32_t data_size, uint8_t protocol) {
  uint32_t fullSize = sizeof(netPacketHeader) + sizeof(IPv4header) + data_size;
  if (fullSize > IPV4_MAX) {
    debugf("[networking::ipv4] Packet excedes limit, aborting: %d/%d\n",
           fullSize, IPV4_MAX);
    return;
  }

  if (fullSize > nic->mtu) {
    // we will need fragmentation!
    uint16_t headerSizes = sizeof(netPacketHeader) + sizeof(IPv4header);

    // data per each fragment
    uint32_t dataPerEach = ((nic->mtu - headerSizes) / 8) * 8;
    uint32_t totalPerEach = dataPerEach + headerSizes; // + headers

    // block ammount they correspond to
    uint16_t amntBlocks = DivRoundUp(data_size, totalPerEach);
    amntBlocks = DivRoundUp(data_size + headerSizes * amntBlocks, totalPerEach);

    if ((amntBlocks * totalPerEach) > (uint16_t)(-1)) {
      debugf("[networking::ipv4] Fragmentation cannot be done! required{%d} "
             "limit{%d}\n",
             amntBlocks * totalPerEach, (uint16_t)(-1));
      return;
    }

    uint16_t id = netIPv4GetFreeId(nic, destination_ip);
    for (uint16_t i = 0; i < amntBlocks; i++) {
      uint16_t flags = 0;
      bool     last = (i + 1) == amntBlocks;

      // more fragments flag (unless it's the last one)
      if (!last)
        flags |= IPV4_FLAGS_MORE_FRAGMENTS;

      // account for headers! (functionality taken care of by *Internal())
      uint16_t bufferSize = last ? data_size % (dataPerEach) : dataPerEach;
      if (!bufferSize)
        continue;
      flags |= (uint16_t)(i * (dataPerEach)) / 8;
      uint8_t *buffer = malloc(bufferSize);
      memcpy(buffer, (size_t)data + i * (dataPerEach), bufferSize);
      netIPv4SendInternal(nic, destination_mac, destination_ip, buffer,
                          bufferSize, protocol, flags, id);
      free(buffer);
    }

    return;
  }

  netIPv4SendInternal(nic, destination_mac, destination_ip, data, data_size,
                      protocol, 0, 0);
}
