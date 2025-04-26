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


#define SDN_PACKET_IS_MERGED(packet_ptr) ( ((sdn_header_t *) packet_ptr)->reserved & 0x1 )

#define SDN_PACKET_SET_MERGED(packet_ptr) ( ((sdn_header_t *) packet_ptr)->reserved |= 0x1 )

#define SDN_GET_NUM_SUBPACKETS(packet_ptr, header_size) (SDN_PACKET_IS_MERGED(packet_ptr) ? (packet_ptr)[header_size] : 1)

#define SDN_QUEUE_ACTION_NONE 0

#define SDN_QUEUE_ACTION_REPLACE 1

#define SDN_QUEUE_ACTION_MERGE 2

// typedef enum {
//   SDN_QUEUE_ACTION_REPLACE = 1 << 0, 
//   SDN_QUEUE_ACTION_MERGE   = 1 << 1,
//   SDN_QUEUE_ACTION_BOTH    = SDN_QUEUE_ACTION_REPLACE | SDN_QUEUE_ACTION_MERGE
// } sdn_queue_action_t;

#include "data-flow-table.h"
#include "control-flow-table.h"
#include <stddef.h>

#include "stdint.h"
#include "sdn-packetbuf.h"
#include "sdn-protocol.h"
#include "sdn-queue.h"
#include "sdn-debug.h"
#include "sdn-constants.h"
#include <string.h>
#ifdef SDN_CONTROLLER_PC
#include "sdn-serial-packet.h"
#endif

#ifdef MANAGEMENT
#include "manage-info.h"
#ifdef SDN_ENABLED_NODE
#include "contiki.h"
#endif
#endif

typedef struct {
  uint8_t head;
  uint8_t tail;
  uint8_t max_size;
  uint8_t size;
} sdn_queue_t;

sdn_queue_t sdn_send_queue;
sdn_queue_t sdn_recv_queue;

sdn_send_queue_data_t sdn_send_queue_data[SDN_SEND_QUEUE_SIZE];
sdn_recv_queue_data_t sdn_recv_queue_data[SDN_RECV_QUEUE_SIZE];

void sdn_send_queue_init() {
  int i;
  sdn_send_queue.head = 0;
  sdn_send_queue.tail = 0;
  sdn_send_queue.size = 0;
  sdn_send_queue.max_size = SDN_SEND_QUEUE_SIZE ;
  for (i=0; i<SDN_SEND_QUEUE_SIZE ; i++) {
    sdn_send_queue_data[i].data = NULL;
    sdn_send_queue_data[i].len = 0;

#if defined (MANAGEMENT) && defined(SDN_ENABLED_NODE)
    sdn_send_queue_data[i].time = 0;
#endif
  }
}

uint8_t sdn_send_queue_empty() {
  // SDN_DEBUG ("SDN_QUEUE_EMPTY %d\n", queue->size);
  return (sdn_send_queue.size == 0);
}

uint8_t sdn_send_queue_size() {
  // SDN_DEBUG ("SDN_QUEUE_SIZE %d\n", queue->size);
  return sdn_send_queue.size;
}

uint8_t sdn_send_queue_maxSize() {
  // SDN_DEBUG ("SDN_QUEUE_MAX_SIZE %d\n", queue->max_size);
  return sdn_send_queue.max_size;
}

sdn_send_queue_data_t* sdn_send_queue_head() {

  return &sdn_send_queue_data[sdn_send_queue.head];
}

sdn_send_queue_data_t* sdn_send_queue_tail() {
  if (sdn_send_queue.tail == 0)
    return &sdn_send_queue_data[sdn_send_queue.max_size-1];
  else
    return &sdn_send_queue_data[sdn_send_queue.tail-1];
}

sdn_send_queue_data_t* sdn_send_queue_dequeue() {

  sdn_send_queue_data_t *t = sdn_send_queue_head();

  if (!sdn_send_queue_empty()) {
    sdn_send_queue.head++;
    if (sdn_send_queue.head == sdn_send_queue.max_size) sdn_send_queue.head = 0;
    sdn_send_queue.size--;

    //SDN_DEBUG ("Packet for send, dequeued!\n");
    //sdn_send_queue_print();
  } else {
    SDN_DEBUG ("Send queue is empty!\n");
  }

  return t;
}

uint8_t sdn_send_queue_enqueue(uint8_t *data, uint16_t len, uint32_t time) {

#ifdef SDN_CONTROLLER_PC
  return sdn_send_queue_enqueue_custom(data, len, time, SDN_SERIAL_MSG_TYPE_RADIO);
}

uint8_t sdn_send_queue_enqueue_custom(uint8_t *data, uint16_t len, uint32_t time, uint8_t msg_type) {
    if (msg_type == SDN_SERIAL_MSG_TYPE_FULL_GRAPH) {
        SDN_DEBUG("\tadding SDN_SERIAL_MSG_TYPE_FULL_GRAPH\n");
        SDN_DEBUG("\t\t%u, %u ,%u\n", sdn_send_queue.head, sdn_send_queue.tail, sdn_send_queue.size);
    }
#endif //SDN_CONTROLLER_PC
  if (sdn_send_queue_size() < sdn_send_queue_maxSize()) {

    sdn_send_queue_data[sdn_send_queue.tail].data = data;
    sdn_send_queue_data[sdn_send_queue.tail].len = len;
#ifdef SDN_CONTROLLER_PC
    sdn_send_queue_data[sdn_send_queue.tail].msg_type = msg_type;
#endif
#if defined (MANAGEMENT) && defined (SDN_ENABLED_NODE)
    sdn_send_queue_data[sdn_send_queue.tail].time = time;
#else
    time = time;
#endif
    sdn_send_queue.tail+= 1;
    if (sdn_send_queue.tail == sdn_send_queue.max_size) sdn_send_queue.tail = 0;
    sdn_send_queue.size+= 1;

    //SDN_DEBUG ("Packet for send, enqueued!\n");
#ifdef SDN_CONTROLLER_PC
    sdn_send_queue_print();
#endif //SDN_CONTROLLER_PC
    return SDN_SUCCESS;
  } else {
#ifdef SDN_CONTROLLER_PC
    SDN_DEBUG ("Error on %s!\n", __func__);
#endif //SDN_CONTROLLER_PC
    sdn_send_queue_print();
    return SDN_ERROR;
  }
}

sdn_send_queue_data_t* sdn_send_queue_find_by_type(uint8_t type) {
  uint8_t indexCount, indexPacket;
  for (indexCount = 0, indexPacket = sdn_send_queue.head; indexCount < sdn_send_queue.size; indexCount++) {

    if (sdn_send_queue_data[indexPacket].data != NULL) {
      if (SDN_HEADER(sdn_send_queue_data[indexPacket].data)->type == type && \
          sdnaddr_cmp(&SDN_HEADER(sdn_send_queue_data[indexPacket].data)->source, &sdn_node_addr) == SDN_EQUAL) {
        return &sdn_send_queue_data[indexPacket];
      }
    }

    indexPacket++;
    if (indexPacket == sdn_send_queue.max_size) {
      indexPacket = 0;
    }
  }
  return NULL;
}

void sdn_send_queue_print() {

  uint8_t indexPacket;
  uint8_t indexByte;
  uint8_t indexCount;

  SDN_DEBUG ("Packets on Send Queue (%d):\n", sdn_send_queue.size);

  for (indexCount = 0, indexPacket = sdn_send_queue.head; indexCount < sdn_send_queue.size; indexCount++) {

    SDN_DEBUG("(%d) ",indexPacket);

    if (sdn_send_queue_data[indexPacket].data != NULL) {
#ifdef SDN_CONTROLLER_PC
      SDN_DEBUG ("[%d] ", sdn_send_queue_data[indexPacket].msg_type);
#endif
      SDN_DEBUG ("[ ");

      for (indexByte = 0; indexByte < 6; indexByte++) //sdn_header_t = 6
        SDN_DEBUG("%02X ", sdn_send_queue_data[indexPacket].data[indexByte]);

      SDN_DEBUG ("] [ ");

      for (; indexByte < sdn_send_queue_data[indexPacket].len; indexByte ++)
        SDN_DEBUG("%02X ", sdn_send_queue_data[indexPacket].data[indexByte]);

      SDN_DEBUG ("]");
      SDN_DEBUG("\n");
    } else {
      SDN_DEBUG("data == NULL\n");
    }

    indexPacket++;
    if (indexPacket == sdn_send_queue.max_size) {
      indexPacket = 0;
    }
  }

  SDN_DEBUG ("\n");
}

void sdn_recv_queue_init() {
  int i;
  sdn_recv_queue.head = 0;
  sdn_recv_queue.tail = 0;
  sdn_recv_queue.size = 0;
  sdn_recv_queue.max_size = SDN_RECV_QUEUE_SIZE ;
  for (i=0; i<SDN_RECV_QUEUE_SIZE ; i++) {
    sdn_recv_queue_data[i].data = NULL;
    sdn_recv_queue_data[i].len = 0;
#if defined(MANAGEMENT) && defined(SDN_ENABLED_NODE)
    sdn_send_queue_data[i].time = 0;
#endif
  }
}

uint8_t sdn_recv_queue_empty() {
  return (sdn_recv_queue.size == 0);
}

uint8_t sdn_recv_queue_size() {
  return sdn_recv_queue.size;
}

uint8_t sdn_recv_queue_maxSize() {
  return sdn_recv_queue.max_size;
}

sdn_recv_queue_data_t* sdn_recv_queue_head() {

  return &sdn_recv_queue_data[sdn_recv_queue.head];
}

sdn_recv_queue_data_t* sdn_recv_queue_dequeue() {

  sdn_recv_queue_data_t *t = NULL;

  if (!sdn_recv_queue_empty()) {
    t = sdn_recv_queue_head();
    sdn_recv_queue.head++;
    if (sdn_recv_queue.head == sdn_recv_queue.max_size) sdn_recv_queue.head = 0;
    sdn_recv_queue.size--;

    //SDN_DEBUG ("Packet for recv, dequeued!\n");
    //sdn_recv_queue_print();
  } else {
    SDN_DEBUG ("Send recv queue is empty!\n");
  }

  return t;
}

#ifdef ENABLE_SDN_TREATMENT

// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  print packet (including merged ones)
// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void imprimir(uint8_t *p, uint16_t len, uint8_t header_size) {
  printf("Header e flowid: ");
  for (int i = 0; i < header_size; i++){
      printf("%02X ", p[i]);
  }

  printf("\ntamanho completo: %d\n", len);

  
  if (SDN_PACKET_IS_MERGED(p)) {
    uint8_t deslocamento = header_size;
    uint8_t num_sub = p[deslocamento];
    deslocamento++;
    printf("\tSubpacotes: %d\n", num_sub);
    
    for (int i = 0; i < num_sub; i++) {
      uint16_t len_sub = p[deslocamento];
      printf("\tSub pacote #%d (len %d): ", i, len_sub);
      deslocamento++;
      for (int j = 0; j < len_sub; j++) {
        printf("%c ", p[deslocamento + j]);
      }
      deslocamento += len_sub;
      printf("\n");
    }
  } else {
    printf("\tPayload: ");
    
    for (int i = header_size; i < len; i++){
      printf("%c ", p[i]);
    }
    printf("\n");
  }
}

// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  merge
// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

uint16_t sdn_merged_new_length(uint8_t *p, uint8_t header_size) {
  uint16_t new_len = header_size; // header length
  uint8_t offset = header_size;
  uint8_t num_subpackets = p[offset]; 
  new_len += 1;  // byte that stores the number of subpackets
  offset++;

  
  for (int i = 0; i < num_subpackets; i++) {
      uint8_t sub_len = p[offset];  // subpacket length
      new_len += 1 + sub_len;         // byte that stores subpacket len + subpacket len
      offset += 1 + sub_len;
  }
  return new_len;
}

uint8_t sdn_recv_queue_combine_packets(uint8_t *p1, uint16_t p1_len, uint8_t pos, uint8_t header_size){
  
  uint8_t *p2 = sdn_recv_queue_data[pos].data;
  uint16_t *p2_len = &(sdn_recv_queue_data[pos].len); 
  //printf("max_pack: %d, len1: %d, len2: %d\n", SDN_MAX_PACKET_SIZE, p1_len, *p2_len);

  uint8_t num_sub_p1 = SDN_GET_NUM_SUBPACKETS(p1, header_size);
  uint8_t offset_p1 = header_size; // set offset_p1 to skip the header
  
  uint8_t num_sub_p2 = SDN_GET_NUM_SUBPACKETS(p2, header_size);
  uint8_t offset_p2 = header_size;

    
  if(p1_len + *p2_len > SDN_MAX_PACKET_SIZE){
    printf("Max number of merged packets reached!  p1 subpackets: %d,  p2 subpackets: %d\n", num_sub_p1, num_sub_p2);
    return 0;
  }

  // if p2 is already merged, deslocate to the end of the payload
  if (SDN_PACKET_IS_MERGED(p2)) {
    offset_p2 = *p2_len;
  } 
  else{
    // if p2 is not merged, reajust the p2 structure to match the merged structure
    SDN_PACKET_SET_MERGED(p2);
    uint8_t extra_bytes = 2;
    uint16_t p2_payload_len = *p2_len - header_size;

    //open space to input the extra bytes to store the #subpackets and the first subpacket length
    for (int i = *p2_len + (extra_bytes - 1); i > header_size; i--) {
      p2[i] = p2[i-extra_bytes];
    }

    p2[offset_p2] = 1; // initialize the number of subpacket to 1
    offset_p2++;
    p2[offset_p2] = p2_payload_len; // insert the first subpacket len
    offset_p2++; // payload 1 already inserted
    offset_p2 += p2_payload_len;
  }

  // analize the p1 packet
  
  uint16_t p1_sub_payload_len = p1_len - header_size;

  if (SDN_PACKET_IS_MERGED(p1)){
    offset_p1++;

    // insert the subpackets in p1 to p2, one by one
    for(int i = 0; i < num_sub_p1; i++){
      p1_sub_payload_len = p1[offset_p1];
      offset_p1++;

      //insert p1 subpacket payload len in p2
      p2[offset_p2] = p1_sub_payload_len;
      offset_p2++;

      //insert p1 subpacket payload in p2
      memcpy(&p2[offset_p2], &p1[offset_p1], p1_sub_payload_len);


      offset_p1 += p1_sub_payload_len;
      offset_p2 += p1_sub_payload_len;

      //ajust the number of subpackets in p1 counter
      num_sub_p2++;
    }
  } 
  else{

    //if p1 is not merged, insert the only p1 subpacket in p2
    
    p2[offset_p2] = p1_sub_payload_len;
    offset_p2++;

    memcpy(&p2[offset_p2], &p1[offset_p1], p1_sub_payload_len);


    offset_p1 += p1_sub_payload_len;
    offset_p2 += p1_sub_payload_len;

    num_sub_p2++;
  }
  //ajust the number of subpackets in p1 after inserting all the p2 subpackets in p1
  p2[header_size] = num_sub_p2;
  
  *p2_len = sdn_merged_new_length(p2, header_size);


  //eliminate p1 packet
  sdn_packetbuf_pool_put((sdn_packetbuf *) p1);

  printf("merging packets...\n");
  imprimir(p2, *p2_len, header_size);

  return 1;
}

// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  replace the packet that is already in the queue with the new packet
// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

uint8_t sdn_recv_queue_replace_old_packet(uint8_t *data, uint16_t len, uint8_t pos, uint8_t header_size){

  //eliminate the old packet
  sdn_packetbuf_pool_put((sdn_packetbuf *)sdn_recv_queue_data[pos].data);

  //replace by the new one
  sdn_recv_queue_data[pos].len = len;
  sdn_recv_queue_data[pos].data = data;

#if defined (MANAGEMENT) && defined (SDN_ENABLED_NODE)
  sdn_recv_queue_data[pos].time = clock_time();
#endif

  SDN_DEBUG ("Old duplicate packet has been replaced by the new one\n");
  imprimir(data, len, header_size);
  return 1;

}

// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  get destination from routing type
// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

uint8_t sdn_get_routing_dest(uint8_t *p, uint8_t **dest, size_t *dest_len) {

  if(SDN_ROUTED_BY_ADDR(p)) {
    *dest     = (uint8_t *)&SDN_GET_PACKET_ADDR(p);
    *dest_len = sizeof(sdnaddr_t);
    return 1;
  }
  if(SDN_ROUTED_BY_FLOWID(p)) {
    *dest     = (uint8_t *)&SDN_GET_PACKET_FLOW(p);
    *dest_len = sizeof(flowid_t);
    return 1;
  }
  if(SDN_ROUTED_BY_SRC(p)) {
    *dest     = (uint8_t *)&SDN_GET_PACKET_REAL_DEST(p);
    *dest_len = sizeof(sdnaddr_t);
    return 1;
  }
  return 0;
}

// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  delete a speciffic subpacket from a merged packet
// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

uint8_t sdn_recv_queue_delete_subpacket(uint8_t *p, uint16_t p_len, uint8_t subpacket_num, uint8_t header_size, uint8_t p_pos) {

  uint8_t *num_subpackets_ptr = p + header_size;
  uint8_t num_subpackets = *num_subpackets_ptr;

  if(subpacket_num < 1 || subpacket_num > num_subpackets) return 0;

  uint8_t offset = header_size + 1;
  uint8_t current_subpacket = 1;

  // move to the start of the subpacket that will be removed
  while(current_subpacket < subpacket_num) {
    uint8_t len = p[offset];       // current packet lenght
    offset += 1 + len;             // skip 1 byte (that stores the len) + packet lenght
    current_subpacket++;
  }

  uint8_t len_to_remove = p[offset];
  uint8_t total_remove_len = 1 + len_to_remove;

  uint16_t remaining_bytes = p_len - (offset + total_remove_len);

  // deslocate the content after the removed packet 
  memmove(&p[offset], &p[offset + total_remove_len], remaining_bytes);

  // update the num of subpackets
  (*num_subpackets_ptr)--;

  // update the packet lenght in the queue
  sdn_recv_queue_data[p_pos].len = p_len - total_remove_len;

  return 1;
}

// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  compare a speciffic field after the "header" (header_size)
// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

uint8_t sdn_recv_queue_compare_packets(uint8_t *p1, uint16_t p1_len, uint8_t *p2, uint16_t p2_len, uint8_t header_size, uint8_t compare_size, uint8_t p2_pos) {
  
  // p1 is the new packet and p2 is the packet already in the queue
  uint8_t is_p1_merged = SDN_PACKET_IS_MERGED(p1);
  uint8_t is_p2_merged = SDN_PACKET_IS_MERGED(p2);

  uint8_t num_sub_p1 = SDN_GET_NUM_SUBPACKETS(p1, header_size);
  uint8_t num_sub_p2 = SDN_GET_NUM_SUBPACKETS(p2, header_size);

  uint8_t offset1 = header_size + (is_p1_merged ? 1 : 0);
  uint8_t offset2_start = header_size + (is_p2_merged ? 1 : 0);

  // go through all p1 subpackets
  for (uint8_t i = 0; i < num_sub_p1; i++) {
    uint8_t len1 = is_p1_merged ? p1[offset1++] : (p1_len - header_size);
    uint8_t *payload1 = p1 + offset1;

    uint8_t offset2 = offset2_start;
    uint8_t p2_subpacket_num = 0;

    // compare it to all of the p2 subpackets
    for (uint8_t j = 0; j < num_sub_p2; j++) {
      uint8_t len2 = is_p2_merged ? p2[offset2++] : (p2_len - header_size);
      uint8_t *payload2 = p2 + offset2;
      p2_subpacket_num++;
      
      // if compare_size that was passed == 0, that means that we want to compare the entire payload
      uint8_t cmp_len = compare_size == 0 ? (len1 < len2 ? len1 : len2) : compare_size;

      // if the area that we are interested in are the same:
      if (memcmp(payload1, payload2, cmp_len) == 0) {
        if (is_p1_merged && !is_p2_merged) {
          return SDN_QUEUE_ACTION_REPLACE;
        }
        else if (!is_p1_merged && !is_p2_merged) {
          return SDN_QUEUE_ACTION_REPLACE;
        }
        else {
          // if p1 is not merged and p2 is merged / if p1 and p2 are merged: delete the duplicate subpacket in p2, then merge them
          sdn_recv_queue_delete_subpacket(p2, p2_len, p2_subpacket_num, header_size, p2_pos);
          return SDN_QUEUE_ACTION_MERGE;
        }
      }
      offset2 += len2;
    }
    offset1 += len1;
  }
  return SDN_QUEUE_ACTION_MERGE;
}

// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  determinar acao: merge/replace/nada
// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

uint8_t sdn_recv_queue_determine_pckt_action(uint8_t *p1, uint8_t *p2, uint16_t p1_len, uint16_t p2_len, uint8_t *header_size, uint8_t p2_pos) {
  
  // if types are different dont compare;
  uint8_t type = SDN_HEADER(p1)->type;
  if (type != SDN_HEADER(p2)->type) return SDN_QUEUE_ACTION_NONE;
  
  // verify if the destination of the packets are the same
  uint8_t *dest1, *dest2;
  size_t dest_len;

  if (!sdn_get_routing_dest(p1, &dest1, &dest_len) || !sdn_get_routing_dest(p2, &dest2, &dest_len)) {
    return SDN_QUEUE_ACTION_NONE;
  }

  if (memcmp(dest1, dest2, dest_len) != 0) return SDN_QUEUE_ACTION_NONE;

  // treat specific packet types
  uint16_t compare_size = 0;
  switch(type) {
    case SDN_PACKET_CONTROL_FLOW_SETUP:
      *header_size = sizeof(sdn_header_t) + sizeof(sdnaddr_t);
      compare_size = sizeof(sdnaddr_t); // route_destination
      return sdn_recv_queue_compare_packets(p1, p1_len, p2, p2_len, *header_size, compare_size, p2_pos);

    case SDN_PACKET_DATA_FLOW_SETUP:
      *header_size = sizeof(sdn_header_t) + sizeof(sdnaddr_t);
      compare_size = sizeof(flowid_t); // flowid
      return sdn_recv_queue_compare_packets(p1, p1_len, p2, p2_len, *header_size, compare_size, p2_pos);

    case SDN_PACKET_CONTROL_FLOW_REQUEST:
      *header_size = sizeof(sdn_header_t) + sizeof(flowid_t);
      compare_size = sizeof(sdnaddr_t); // address
      return sdn_recv_queue_compare_packets(p1, p1_len, p2, p2_len, *header_size, compare_size, p2_pos);

    case SDN_PACKET_DATA_FLOW_REQUEST:
      *header_size = sizeof(sdn_header_t) + sizeof(flowid_t);
      compare_size = sizeof(flowid_t); // flowid
      return sdn_recv_queue_compare_packets(p1, p1_len, p2, p2_len, *header_size, compare_size, p2_pos);      

    case SDN_PACKET_REGISTER_FLOWID:
      *header_size = sizeof(sdn_header_t) + sizeof(flowid_t);
      compare_size = sizeof(flowid_t); // flowid
      return sdn_recv_queue_compare_packets(p1, p1_len, p2, p2_len, *header_size, compare_size, p2_pos);

    case SDN_PACKET_ACK_BY_FLOW_ADDRESS:
      *header_size = sizeof(sdn_header_t) + sizeof(sdnaddr_t);
      compare_size = sizeof(uint8_t) * 2; // acked_packed_type and acked_packed_seqno
      return sdn_recv_queue_compare_packets(p1, p1_len, p2, p2_len, *header_size, compare_size, p2_pos);

    case SDN_PACKET_ACK_BY_FLOW_ID:
      *header_size = sizeof(sdn_header_t) + sizeof(flowid_t);
      compare_size = 0; // compare all the payload after the struct
      return sdn_recv_queue_compare_packets(p1, p1_len, p2, p2_len, *header_size, compare_size, p2_pos);

    case SDN_PACKET_DATA:
      *header_size = sizeof(sdn_data_t);
      compare_size = 0; // compare all the payload after the struct
      //return sdn_recv_queue_compare_packets(p1, p1_len, p2, p2_len, *header_size, compare_size, p2_pos);
      return SDN_QUEUE_ACTION_MERGE; // always merge

    case SDN_PACKET_NEIGHBOR_REPORT:
      *header_size = sizeof(sdn_header_t);
      return SDN_QUEUE_ACTION_REPLACE; // always replace

    case SDN_PACKET_SRC_ROUTED_ACK:
      *header_size = sizeof(sdn_header_t) + sizeof(sdnaddr_t);
      compare_size = sizeof(uint8_t) * 2; // acked_packed_type and acked_packed_seqno;
      return sdn_recv_queue_compare_packets(p1, p1_len, p2, p2_len, *header_size, compare_size, p2_pos);

    case SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP:
      *header_size = sizeof(sdn_header_t) + sizeof(sdnaddr_t);
      compare_size = sizeof(sdnaddr_t); // flow_setup.route_destination
      return sdn_recv_queue_compare_packets(p1, p1_len, p2, p2_len, *header_size, compare_size, p2_pos);
    
    case SDN_PACKET_SRC_ROUTED_DATA_FLOW_SETUP:
      *header_size = sizeof(sdn_header_t) + sizeof(sdnaddr_t);
      compare_size = sizeof(flowid_t); // flow_setup.flowid
      return sdn_recv_queue_compare_packets(p1, p1_len, p2, p2_len, *header_size, compare_size, p2_pos);
    
    default:
      return SDN_QUEUE_ACTION_NONE;
  }
}

// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// process new packet and decide one of three actions to the new packet: MERGE, REPLACE or NOTHING 
// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

uint8_t sdn_recv_queue_process_new_packet(uint8_t *new_packet, uint16_t new_len) {

  uint8_t new_type = SDN_HEADER(new_packet)->type;

  // dont treat this type of packets
  switch(new_type) {
    case SDN_PACKET_ND:
    case SDN_PACKET_CD:
    case SDN_PACKET_MULTIPLE_CONTROL_FLOW_SETUP:
    case SDN_PACKET_MULTIPLE_DATA_FLOW_SETUP:
    case SDN_PACKET_MNGT_CONT_SRC_RTD:
    case SDN_PACKET_MNGT_NODE_DATA:
    case SDN_PACKET_GENERIC_FLOW:
    case SDN_PACKET_GENERIC_SRC_ROUTED:
    case SDN_PACKET_GENERIC_ADDR:
    case SDN_PACKET_PHASE_INFO:
      return 0;
    default:
      break;
  }

  uint8_t pos = sdn_recv_queue.head;
  uint8_t header_size;

  while (pos != sdn_recv_queue.tail) {
    uint8_t *old_packet = sdn_recv_queue_data[pos].data;
    uint16_t old_len = sdn_recv_queue_data[pos].len;

    uint8_t action = sdn_recv_queue_determine_pckt_action(new_packet, old_packet, new_len, old_len, &header_size, pos);

    if (action == SDN_QUEUE_ACTION_REPLACE) {
      SDN_DEBUG("Duplicate packet (type: %x) found, replacing the old one...\n", SDN_HEADER(new_packet)->type);
      return sdn_recv_queue_replace_old_packet(new_packet, new_len, pos, header_size);
    }

    if (action == SDN_QUEUE_ACTION_MERGE) {
      SDN_DEBUG("Same type packet (type: %x) found, merging packets...\n", SDN_HEADER(new_packet)->type);
      return sdn_recv_queue_combine_packets(new_packet, new_len, pos, header_size);
    }
    pos = (pos+1) % sdn_recv_queue.max_size;
  }
  return 0;
}
// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// 
// ————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

uint8_t sdn_is_final_destination(uint8_t* packet) {

#ifdef SDN_CONTROLLER_NODE
  return 0;

#else
  if (SDN_ROUTED_BY_FLOWID(packet)) {
    struct data_flow_entry *dfe = sdn_dataflow_get(SDN_GET_PACKET_FLOW(packet));
    if(dfe == NULL) return 0;
    else if(dfe->action == SDN_ACTION_RECEIVE){
      printf("packet is in the destination, not proceding the verification\n");
      return 1;
    }
  } 
  
  else if (SDN_ROUTED_BY_ADDR(packet)) {
    struct control_flow_entry *cfe = sdn_controlflow_get(SDN_GET_PACKET_ADDR(packet));
    if(cfe == NULL) return 0;
    if(cfe->action == SDN_ACTION_RECEIVE){
      printf("packet is in the destination, not proceding the verification\n");
      return 1;
    }
  }

  else if (SDN_ROUTED_BY_SRC(packet)) {
    sdnaddr_t * next_hop = &SDN_GET_PACKET_ADDR(packet);
    if (sdnaddr_cmp(next_hop, &sdn_node_addr) == SDN_EQUAL){
      printf("packet is in the destination, not proceding the verification\n");
      return 1;
    }
  }

  else if (SDN_ROUTED_NOT(packet)) {
    if (sdnaddr_cmp(&((sdn_header_t*)packet)->source, &sdn_node_addr) != SDN_EQUAL){
      printf("packet is in the destination, not proceding the verification\n");
      return 1;
    }
  }
  
  return 0;
#endif
}

#endif

uint8_t sdn_recv_queue_enqueue(uint8_t *data, uint16_t len) {
#ifdef ENABLE_SDN_TREATMENT
  // verify if packet needs treatment, if so, treat it and if the treatment went well, returns success
  if(!(sdn_is_final_destination(data)) && (sdn_recv_queue_process_new_packet(data, len))){
    return SDN_SUCCESS;
  }
#endif

  if (sdn_recv_queue_size() < sdn_recv_queue_maxSize()) {
    sdn_recv_queue_data[sdn_recv_queue.tail].data = data;
    sdn_recv_queue_data[sdn_recv_queue.tail].len = len;
#if defined (MANAGEMENT) && defined (SDN_ENABLED_NODE)
    sdn_recv_queue_data[sdn_recv_queue.tail].time = clock_time();
#endif
    sdn_recv_queue.tail+= 1;
    if (sdn_recv_queue.tail == sdn_recv_queue.max_size) sdn_recv_queue.tail = 0;
    sdn_recv_queue.size+= 1;

    SDN_DEBUG ("Packet for recv, enqueued!\n");
    sdn_recv_queue_print();

    return SDN_SUCCESS;
  } else {
    return SDN_ERROR;
  }
}

void sdn_recv_queue_print() {

  uint8_t indexPacket;
  uint8_t indexByte;
  uint8_t indexCount;

  SDN_DEBUG ("Packets on Recv Queue (%d):\n", sdn_recv_queue.size);

  for (indexCount = 0, indexPacket = sdn_recv_queue.head; indexCount < sdn_recv_queue.size; indexCount++) {

    SDN_DEBUG("(%d) ",indexPacket);

    if (sdn_recv_queue_data[indexPacket].data != NULL) {
      SDN_DEBUG ("[ ");

      for (indexByte = 0; indexByte < 6; indexByte++) //sdn_header_t = 6
        SDN_DEBUG("%02X ", sdn_recv_queue_data[indexPacket].data[indexByte]);

      SDN_DEBUG ("] [ ");

      for (; indexByte < sdn_recv_queue_data[indexPacket].len; indexByte ++)
        SDN_DEBUG("%02X ", sdn_recv_queue_data[indexPacket].data[indexByte]);

      SDN_DEBUG ("]");
      SDN_DEBUG("\n");
    } else {
      SDN_DEBUG("data == NULL\n");
    }

    indexPacket++;
    if (indexPacket == sdn_recv_queue.max_size) {
      indexPacket = 0;
    }
  }

  SDN_DEBUG ("\n");
}
