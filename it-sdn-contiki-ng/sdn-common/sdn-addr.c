/*
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE SOFTWARE PROVIDER OR DEVELOPERS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "sdn-addr.h"
#include "sdn-constants.h"
#include "string.h"
#include "stdio.h"

#if SDNADDR_SIZE == 2
sdnaddr_t sdnaddr_null = { { 0, 0 } };
#else /*SDNADDR_SIZE == 2*/
#if SDNADDR_SIZE == 8
sdnaddr_t sdnaddr_null = { { 0, 0, 0, 0, 0, 0, 0, 0 } };
#else /*SDNADDR_SIZE == 8*/
#if SDNADDR_SIZE == 6
sdnaddr_t sdnaddr_null = { { 0, 0, 0, 0, 0, 0 } };
#endif /*SDNADDR_SIZE == 6*/
#endif /*SDNADDR_SIZE == 8*/
#endif /*SDNADDR_SIZE == 2*/

sdnaddr_t sdn_node_addr;
sdnaddr_t *sdn_addr_broadcast = &sdnaddr_null;

flowid_t sdn_controller_flow = SDN_CONTROLLER_FLOW;

void sdnaddr_copy(sdnaddr_t *to, sdnaddr_t *from) {
  memcpy(to, from, SDNADDR_SIZE);
}

uint8_t sdnaddr_cmp(sdnaddr_t *addr1, sdnaddr_t *addr2) {

  if (memcmp(addr1, addr2, sizeof(sdnaddr_t)) == 0) {
    return SDN_EQUAL;
  }
  return SDN_NOT_EQUAL;
}

uint8_t sdnaddr_setbyte(sdnaddr_t* addr, uint8_t index, uint8_t value) {
  char *charaddr = (char*)addr;
  if (charaddr != NULL && index < sizeof(sdnaddr_t)) {
    charaddr[index] = value;
    return SDN_SUCCESS;
  }
  return SDN_ERROR;
}

uint8_t sdnaddr_getbyte(sdnaddr_t* addr, uint8_t index, uint8_t *value) {
  char *charaddr = (char*)addr;
  if (charaddr != NULL && index < sizeof(sdnaddr_t)) {
    *value = charaddr[index];
    return SDN_SUCCESS;
  }
  return SDN_ERROR;
}

void sdnaddr_print(sdnaddr_t* addr) {
#ifdef DEBUG_SDN
  sdnaddr_print_nodebug(addr);
#endif
}

void sdnaddr_print_nodebug(sdnaddr_t* addr) {
  uint8_t *charaddr = (uint8_t*)addr;
  uint8_t i;

  printf("[");

  for (i = 0; i < sizeof(sdnaddr_t); i++) {
    printf(" %02X", charaddr[i]);
  }

  printf(" ]");
}

void flowid_copy(flowid_t *to, flowid_t *from) {
  *to = *from;
}

uint8_t flowid_cmp(flowid_t* f1, flowid_t* f2) {
  if (memcmp(f1,f2,sizeof(flowid_t))==0) {
    return SDN_EQUAL;
  }
  return ~SDN_EQUAL;
}

void flowid_print(flowid_t* f) {

  flowid_t flowid;

  memcpy(&flowid, f, sizeof(flowid_t));

  printf("%04u", flowid);
}


size_t sdn_get_src_rtd_merged_subpackets_len(uint8_t *packet) {
  // skip header and destination
  uint8_t *p = packet + sizeof(sdn_header_t) + sizeof(sdnaddr_t);
 
  uint8_t num_sub = *p++;
  size_t total_size = 0;
 
  for(uint8_t i = 0; i < num_sub; i++) {
      uint8_t len = *p++;
      total_size += 1 + len; // byte that stores the length + length
      p += len;
  }
 
  return total_size;
}

sdnaddr_t *sdn_packet_get_next_src_addr(uint8_t *p, size_t pkt_type_size) {
  if (SDN_PACKET_IS_MERGED(p)) {
    size_t sub_len = sdn_get_src_rtd_merged_subpackets_len(p);

    uint8_t *path_len_ptr = p
      + sizeof(sdn_header_t) // header
      + sizeof(sdnaddr_t) // destination
      + 1
      + sub_len
      + sizeof(sdnaddr_t);

    uint8_t path_len = *path_len_ptr;
    return &SDN_GET_ADDR_IN_ARRAY(path_len_ptr + 1, path_len);
  } else {
    uint8_t *path_len_ptr = p + pkt_type_size - sizeof(uint8_t);
    uint8_t path_len = *path_len_ptr;
    return &SDN_GET_ADDR_IN_ARRAY(path_len_ptr + 1, path_len); 
  }
}

sdnaddr_t *sdn_get_real_dest_from_merged_packet(uint8_t *packet) {
  size_t sub_len = sdn_get_src_rtd_merged_subpackets_len(packet);
  uint8_t *real_dest_ptr = packet;

  if (!SDN_PACKET_IS_MERGED(packet)) {
    return SDN_GET_PACKET_REAL_DEST(packet);
  }
  else {
    real_dest_ptr += 
    sizeof(sdn_header_t)
    + sizeof(sdnaddr_t) // destination
    + 1                 // num_subpackets
    + sub_len;          // subpackets
  }
  return (sdnaddr_t*)real_dest_ptr;
}