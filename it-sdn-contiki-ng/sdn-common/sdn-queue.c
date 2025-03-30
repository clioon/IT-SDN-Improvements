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

#define SDN_GET_NUM_SUBPACKETS(packet_ptr) (SDN_PACKET_IS_MERGED(packet_ptr) ? (packet_ptr)[sizeof(sdn_data_t)] : 1)

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

uint16_t sdn_merged_new_length(uint8_t *p, uint8_t header_size) {
  uint16_t new_len = header_size; // header length
  uint8_t deslocated = header_size;
  uint8_t num_subpackets = p[deslocated]; 
  new_len += 1;  // byte that stores the number of subpackets
  deslocated++;

  
  for (int i = 0; i < num_subpackets; i++) {
      uint8_t sub_len = p[deslocated];  // subpacket length
      new_len += 1 + sub_len;         // byte that stores subpacket len + subpacket len
      deslocated += 1 + sub_len;
  }
  return new_len;
}

uint8_t sdn_recv_queue_combine_packets(uint8_t *p1, uint16_t p1_len, uint8_t pos, uint8_t header_size){
  
  uint8_t *p2 = sdn_recv_queue_data[pos].data;
  uint16_t *p2_len = &(sdn_recv_queue_data[pos].len); 
  printf("max_pack: %d, len1: %d, len2: %d\n", SDN_MAX_PACKET_SIZE, p1_len, *p2_len);

  uint8_t num_sub_p1 = SDN_GET_NUM_SUBPACKETS(p1);
  uint8_t deslocated_p1 = header_size; // set deslocated_p1 to skip the header
  
  uint8_t num_sub_p2 = SDN_GET_NUM_SUBPACKETS(p2);
  uint8_t deslocated_p2 = header_size;

    
  if(p1_len + *p2_len > SDN_MAX_PACKET_SIZE){
    printf("Max number of merged packets reached!  p1 subpackets: %d,  p2 subpackets: %d\n", num_sub_p1, num_sub_p2);
    return 0;
  }

  // if p2 is already merged, deslocate to the end of the payload
  if (SDN_PACKET_IS_MERGED(p2)) {
    deslocated_p2 = *p2_len;
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

    p2[deslocated_p2] = 1; // initialize the number of subpacket to 1
    deslocated_p2++;
    p2[deslocated_p2] = p2_payload_len; // insert the first subpacket len
    deslocated_p2++; // payload 1 already inserted
    deslocated_p2 += p2_payload_len;
  }

  // analize the p1 packet
  
  uint16_t p1_sub_payload_len = p1_len - header_size;

  if (SDN_PACKET_IS_MERGED(p1)){
    deslocated_p1++;

    // insert the subpackets in p1 to p2, one by one
    for(int i = 0; i < num_sub_p1; i++){
      p1_sub_payload_len = p1[deslocated_p1];
      deslocated_p1++;

      //insert p1 subpacket payload len in p2
      p2[deslocated_p2] = p1_sub_payload_len;
      deslocated_p2++;

      //insert p1 subpacket payload in p2
      memcpy(&p2[deslocated_p2], &p1[deslocated_p1], p1_sub_payload_len);


      deslocated_p1 += p1_sub_payload_len;
      deslocated_p2 += p1_sub_payload_len;

      //ajust the number of subpackets in p1 counter
      num_sub_p2++;
    }
  } 
  else{
    //if p1 is not merged, insert the only p1 subpacket in p2
    
    p2[deslocated_p2] = p1_sub_payload_len;
    deslocated_p2++;

    memcpy(&p2[deslocated_p2], &p1[deslocated_p1], p1_sub_payload_len);


    deslocated_p1 += p1_sub_payload_len;
    deslocated_p2 += p1_sub_payload_len;

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

uint8_t sdn_recv_queue_compare_packets(uint8_t *packet1, uint8_t *packet2, uint16_t len1, uint16_t len2){

  if(SDN_HEADER(packet1)->type != SDN_HEADER(packet2)->type) return 0;
  uint16_t header_size = sizeof(sdn_header_t);

  //in order to verify if the destination of the packets are the same, we need to treat by the routing type

  //if routed by address:
  if(SDN_ROUTED_BY_ADDR(packet1)){
    sdnaddr_t *dest1 = &(SDN_GET_PACKET_ADDR(packet1));
    sdnaddr_t *dest2 = &(SDN_GET_PACKET_ADDR(packet2));

    if(memcmp(dest1, dest2, sizeof(sdnaddr_t)) == 0){
      SDN_DEBUG("Routed by address packets with the same destination!\n");

      //verify if the payloads are duplicate, if it is return 1, else if the type are equal but the payloads arent return 2
      if(memcmp(packet1 + header_size, packet2 + header_size, len1 - header_size) == 0) return 1;

      else if(SDN_HEADER(packet1)->type == SDN_HEADER(packet2)->type) return 2;
    }
  }

  //if routed by flowID:
  if(SDN_ROUTED_BY_FLOWID(packet1)){
    flowid_t *dest1 = &(SDN_GET_PACKET_FLOW(packet1));
    flowid_t *dest2 = &(SDN_GET_PACKET_FLOW(packet2));

    if(memcmp(dest1, dest2, sizeof(flowid_t)) == 0){
      SDN_DEBUG("Routed by flowID packets with the same destination!\n");
      if(memcmp(packet1 + header_size, packet2 + header_size, len1 - header_size) == 0) return 1;

      else if(SDN_HEADER(packet1)->type == SDN_HEADER(packet2)->type) return 2;
    }
  }

  //if routed by Source:
  if(SDN_ROUTED_BY_SRC(packet1)){
    sdnaddr_t *dest1 = &(SDN_GET_PACKET_REAL_DEST(packet1));
    sdnaddr_t *dest2 = &(SDN_GET_PACKET_REAL_DEST(packet2));

    if(memcmp(dest1, dest2, sizeof(sdnaddr_t)) == 0){
      SDN_DEBUG("Routed by source packets with the same destination!\n");
      if(memcmp(packet1 + header_size, packet2 + header_size, len1 - header_size) == 0) return 1;

      else if(SDN_HEADER(packet1)->type == SDN_HEADER(packet2)->type) return 2;
    }
  }

  return 0;
}

uint8_t sdn_recv_queue_is_same_type(uint8_t *data, uint16_t len, uint8_t header_size){

  uint8_t pos = sdn_recv_queue.head;

  while(pos != sdn_recv_queue.tail){
    
    //verify if it is just the same type (== 2)
    if(sdn_recv_queue_compare_packets(data, sdn_recv_queue_data[pos].data, len, sdn_recv_queue_data[pos].len) == 2){

      SDN_DEBUG("Duplicate packet type!: type:%x \n", SDN_HEADER(data)->type);

      //if the treatment went well, return 1. Else it returns 0 and the packet is treated normally
      return sdn_recv_queue_combine_packets(data, len, pos, header_size);
    }

    pos = (pos+1) % sdn_recv_queue.max_size;
  }

  return 0;
}

uint8_t sdn_recv_queue_is_duplicate(uint8_t *data, uint16_t len, uint8_t header_size){

  uint8_t pos = sdn_recv_queue.head;

  while(pos != sdn_recv_queue.tail){
    
    //verify if it is duplicate (== 1)
    if(sdn_recv_queue_compare_packets(data, sdn_recv_queue_data[pos].data, len, sdn_recv_queue_data[pos].len) == 1){

      SDN_DEBUG("Duplicate packet!: type:%x \n", SDN_HEADER(data)->type);
      
      //if the treatment went well, return 1. Else it returns 0 and the packet is treated normally
      return sdn_recv_queue_replace_old_packet(data, len, pos, header_size);
    }

    pos = (pos+1) % sdn_recv_queue.max_size;
  }

  return 0;
}

uint8_t sdn_recv_queue_verify_per_packet_type(uint8_t *data, uint16_t len) {
  switch (SDN_HEADER(data)->type) {
    case SDN_PACKET_CONTROL_FLOW_SETUP:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_control_flow_setup_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_control_flow_setup_t));
      return 0;

    case SDN_PACKET_DATA_FLOW_SETUP:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_data_flow_setup_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_data_flow_setup_t));
      return 0;

    case SDN_PACKET_CONTROL_FLOW_REQUEST:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_control_flow_request_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_control_flow_request_t));
      return 0;

    case SDN_PACKET_DATA_FLOW_REQUEST:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_data_flow_request_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_data_flow_request_t));
      return 0;

    case SDN_PACKET_NEIGHBOR_REPORT:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_neighbor_report_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_neighbor_report_t));
      return 0;

    case SDN_PACKET_DATA:
      return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_data_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_data_t));
      //return 0;

    case SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_src_rtd_control_flow_setup_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_src_rtd_control_flow_setup_t));
      return 0;

    case SDN_PACKET_SRC_ROUTED_DATA_FLOW_SETUP:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_src_rtd_data_flow_setup_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_src_rtd_data_flow_setup_t));
      return 0;

    case SDN_PACKET_MULTIPLE_CONTROL_FLOW_SETUP:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_mult_control_flow_setup_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_mult_control_flow_setup_t));
      return 0;

    case SDN_PACKET_MULTIPLE_DATA_FLOW_SETUP:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_mult_data_flow_setup_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_mult_data_flow_setup_t));
      return 0;

    case SDN_PACKET_ACK_BY_FLOW_ID:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_ack_by_flow_id_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_ack_by_flow_id_t));
      return 0;

    case SDN_PACKET_ACK_BY_FLOW_ADDRESS:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_ack_by_flow_address_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_ack_by_flow_address_t));
      return 0;

    case SDN_PACKET_SRC_ROUTED_ACK:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_src_rtd_ack_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_src_rtd_ack_t));
      return 0;

    case SDN_PACKET_MNGT_CONT_SRC_RTD:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_src_rtd_cont_mngt_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_src_rtd_cont_mngt_t));
      return 0;

    case SDN_PACKET_MNGT_NODE_DATA:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_flow_id_mngt_node_data_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_flow_id_mngt_node_data_t));
      return 0;

    case SDN_PACKET_GENERIC_FLOW:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_generic_flow_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_generic_flow_t));
      return 0;

    case SDN_PACKET_GENERIC_SRC_ROUTED:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_generic_src_rtd_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_generic_src_rtd_t));
      return 0;

    case SDN_PACKET_GENERIC_ADDR:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_generic_addr_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_generic_addr_t));
      return 0;

    case SDN_PACKET_PHASE_INFO:
      //return sdn_recv_queue_is_duplicate(data, len, sizeof(sdn_phase_info_flow_t)) || sdn_recv_queue_is_same_type(data, len, sizeof(sdn_phase_info_flow_t));
      return 0;

    default:
      return 0;
  }
}

uint8_t sdn_recv_queue_enqueue(uint8_t *data, uint16_t len) {

  // verify if packet needs treatment, if so, treat it and if the treatment went well, returns success
  if(sdn_recv_queue_verify_per_packet_type(data, len) == 1){
    return SDN_SUCCESS;
  }
  else if (sdn_recv_queue_size() < sdn_recv_queue_maxSize()) {
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
