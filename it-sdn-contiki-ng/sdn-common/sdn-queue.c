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

#define SDN_QUEUE_ACTION_NONE 0
#define SDN_QUEUE_ACTION_REPLACE 1
#define SDN_QUEUE_ACTION_MERGE 2
#define SDN_SEND_QUEUE 1
#define SDN_RECEIVE_QUEUE 0
#define SDN_MAX_SUBPACKETS 8

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

#ifdef SDN_ENABLED_NODE
#include "sdn-send.h"
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

    // SDN_DEBUG ("Packet for send, dequeued!\n");
    // sdn_send_queue_print();
  } else {
    SDN_DEBUG ("Send queue is empty!\n");
  }

  return t;
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

uint8_t sdn_send_queue_enqueue(uint8_t *data, uint16_t len, uint32_t time) {

#ifdef SDN_CONTROLLER_PC
  return sdn_send_queue_enqueue_custom(data, len, time, SDN_SERIAL_MSG_TYPE_RADIO);
}

uint8_t sdn_send_queue_enqueue_custom(uint8_t *data, uint16_t len, uint32_t time, uint8_t msg_type) {
    if (msg_type == SDN_SERIAL_MSG_TYPE_FULL_GRAPH) {
        SDN_DEBUG("\tadding SDN_SERIAL_MSG_TYPE_FULL_GRAPH\n");
        SDN_DEBUG("\t\t%u, %u ,%u\n", sdn_send_queue.head, sdn_send_queue.tail, sdn_send_queue.size);
    }
#endif // SDN_CONTROLLER_PC
  if (sdn_send_queue_size() < sdn_send_queue_maxSize()) {
#ifdef ENABLE_SDN_TREATMENT
  sdn_processing_send_queue = 1;
  // verify if packet needs treatment, if so, treat it and if the treatment went well, returns success
  if(sdn_queue_process_new_packet(data, len, SDN_SEND_QUEUE)){
    sdn_processing_send_queue = 0;
    SDN_DEBUG("send queue packet enqueued\n");
    return SDN_SUCCESS;
  }
  sdn_processing_send_queue = 0;
#endif

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

    // SDN_DEBUG ("Packet for send, enqueued!\n");
#ifdef SDN_CONTROLLER_PC
    sdn_send_queue_print();
#endif // SDN_CONTROLLER_PC
    return SDN_SUCCESS;
  } else {
#ifdef SDN_CONTROLLER_PC
    SDN_DEBUG ("Error on %s!\n", __func__);
#endif // SDN_CONTROLLER_PC
    sdn_send_queue_print();
    return SDN_ERROR;
  }
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

      for (indexByte = 0; indexByte < 6; indexByte++) // sdn_header_t = 6
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

    // SDN_DEBUG ("Packet for recv, dequeued!\n");
    // sdn_recv_queue_print();
  } else {
    SDN_DEBUG ("Send recv queue is empty!\n");
  }

  return t;
}

uint8_t sdn_recv_queue_enqueue(uint8_t *data, uint16_t len) {
#ifdef ENABLE_SDN_TREATMENT
  // verify if packet needs treatment, if so, treat it and if the treatment went well, returns success
  if(!(sdn_is_final_destination(data)) && (sdn_queue_process_new_packet(data, len, SDN_RECEIVE_QUEUE))){
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

      for (indexByte = 0; indexByte < 6; indexByte++) // sdn_header_t = 6
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

#ifdef ENABLE_SDN_TREATMENT
volatile uint8_t sdn_processing_send_queue = 0;

// void imprimir(uint8_t *p, uint16_t len, uint8_t header_size) {
//   SDN_DEBUG("Header e flowid: ");
//   for (int i = 0; i < header_size; i++){
//       SDN_DEBUG("%02X ", p[i]);
//   }

//   SDN_DEBUG("\ntamanho completo: %d\n", len);

  
//   if (SDN_PACKET_IS_MERGED(p)) {
//     uint8_t deslocamento = header_size;
//     uint8_t num_sub = p[deslocamento];
//     deslocamento++;
//     SDN_DEBUG("\tSubpacotes: %d\n", num_sub);
    
//     for (int i = 0; i < num_sub; i++) {
//       // uint8_t seq_no = p[deslocamento];
//       deslocamento++;
//       deslocamento += LINKADDR_SIZE; // source
//       uint16_t len_sub = p[deslocamento];
//       SDN_DEBUG("\tSub pacote #%d (len %d): ", i, len_sub);
//       deslocamento++;
//       for (int j = 0; j < len_sub; j++) {
//         SDN_DEBUG("%c ", p[deslocamento + j]);
//       }
//       deslocamento += len_sub;
//       SDN_DEBUG("\n");
//     }
//   } else {
//     SDN_DEBUG("\tPayload: ");
    
//     for (int i = header_size; i < len; i++){
//       SDN_DEBUG("%c ", p[i]);
//     }
//     SDN_DEBUG("\n");
//   }
// }

uint16_t sdn_merged_new_length(uint8_t *p, uint8_t header_size) {
  uint16_t new_len = header_size; // header length
  uint8_t offset = header_size;
  uint8_t num_subpackets = p[offset]; 
  new_len += 1;  // byte that stores the number of subpackets
  offset++;

  for (int i = 0; i < num_subpackets; i++) {
    offset ++; // skip the seq no
    offset += LINKADDR_SIZE; // skip the source
    uint8_t sub_len = p[offset];  // subpacket length
    new_len += 2 + LINKADDR_SIZE + sub_len;         // byte that stores the seq no + source  + the subpacket len + subpacket len
    offset += 1 + sub_len;
  }
  return new_len;
}

uint8_t sdn_queue_combine_packets(uint8_t *p1, uint16_t p1_len, uint8_t pos, uint8_t header_size, uint8_t queue) {

  uint8_t *p2 = (queue == SDN_SEND_QUEUE) ? sdn_send_queue_data[pos].data : sdn_recv_queue_data[pos].data;
  uint16_t *p2_len = (queue == SDN_SEND_QUEUE) ? &(sdn_send_queue_data[pos].len) : &(sdn_recv_queue_data[pos].len); 

  uint8_t num_sub_p1 = SDN_GET_NUM_SUBPACKETS(p1, header_size);
  uint8_t seq_no_p1 = SDN_HEADER(p1)->seq_no;
  sdnaddr_t source_p1 = SDN_HEADER(p1)->source;
  uint8_t offset_p1 = header_size; // set offset_p1 to skip the header
  
  uint8_t num_sub_p2 = SDN_GET_NUM_SUBPACKETS(p2, header_size);
  uint8_t seq_no_p2 = SDN_HEADER(p2)->seq_no;
  sdnaddr_t source_p2 = SDN_HEADER(p2)->source;
  uint8_t offset_p2 = header_size;
  
  if (num_sub_p1 > SDN_MAX_SUBPACKETS || num_sub_p2 > SDN_MAX_SUBPACKETS) {
    SDN_DEBUG_ERROR ("Invalid num of subpackets\n");
    return 0;
  }

  if (p1 == NULL || p2 == NULL) {
    SDN_DEBUG_ERROR ("NULL packet\n");
    return 0;
  }

  if (num_sub_p1 + num_sub_p2 > SDN_MAX_SUBPACKETS) {
    SDN_DEBUG_ERROR ("Max number of subpackets reached\n");
    return 0;
  }

  if(p1_len + *p2_len > SDN_MAX_PACKET_SIZE){
    SDN_DEBUG_ERROR ("Packets too large to merge\n");
    return 0;
  }

  // if p2 is already merged, deslocate to the end of the payload
  if (SDN_PACKET_IS_MERGED(p2)) {
    offset_p2 = *p2_len;
  } 
  else{
    // if p2 is not merged, reajust the p2 structure to match the merged structure
    SDN_PACKET_SET_MERGED(p2);
    uint8_t extra_bytes = 3 + LINKADDR_SIZE; // #packtes (1), seqno (1), source(LINKADDR_SIZE), length(1)
    uint16_t p2_payload_len = *p2_len - header_size;

    // open space to input the extra bytes to store the #subpackets, the first seq number, the first source and the first subpacket length
    for (int i = *p2_len + (extra_bytes - 1); i > header_size; i--) {
      p2[i] = p2[i-extra_bytes];
    }

    p2[offset_p2] = 1; // initialize the number of subpacket to 1
    offset_p2++;
    p2[offset_p2] = seq_no_p2; // insert the first subpacket seq number
    offset_p2++;
    memcpy(&p2[offset_p2], source_p2.u8, LINKADDR_SIZE);
    offset_p2 += LINKADDR_SIZE;
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
      uint8_t p1_sub_payload_seq_no = p1[offset_p1];
      offset_p1++;
      sdnaddr_t p1_sub_payload_source;
      memcpy(&p1_sub_payload_source, &p1[offset_p1], LINKADDR_SIZE);
      offset_p1 += LINKADDR_SIZE;

      p1_sub_payload_len = p1[offset_p1];
      offset_p1++;

      // insert p1 subpacket seq number in p2
      p2[offset_p2] = p1_sub_payload_seq_no;
      offset_p2 ++;
      // insert p1 subpacket source in p2
      memcpy(&p2[offset_p2], &p1_sub_payload_source, LINKADDR_SIZE);
      offset_p2 += LINKADDR_SIZE;

      // insert p1 subpacket payload len in p2
      p2[offset_p2] = p1_sub_payload_len;
      offset_p2++;
      // insert p1 subpacket payload in p2
      memcpy(&p2[offset_p2], &p1[offset_p1], p1_sub_payload_len);


      offset_p1 += p1_sub_payload_len;
      offset_p2 += p1_sub_payload_len;

      // ajust the number of subpackets in p1 counter
      num_sub_p2++;
    }
  } 
  else{
    // if p1 is not merged, insert the only p1 subpacket in p2
    p2[offset_p2] = seq_no_p1;
    offset_p2++;
    memcpy(&p2[offset_p2], &source_p1, LINKADDR_SIZE);
    offset_p2 += LINKADDR_SIZE;
    p2[offset_p2] = p1_sub_payload_len;
    offset_p2++;
    memcpy(&p2[offset_p2], &p1[offset_p1], p1_sub_payload_len);


    offset_p1 += p1_sub_payload_len;
    offset_p2 += p1_sub_payload_len;

    num_sub_p2++;
  }
  // ajust the number of subpackets in p2 after inserting all the p1 subpackets in to p2
  p2[header_size] = num_sub_p2;
  
  *p2_len = sdn_merged_new_length(p2, header_size);


  // eliminate p1 packet
  if (queue == SDN_SEND_QUEUE) {
    SDN_METRIC_MERGE_TX(p1, p2);
    sdn_send_queue_print();
  } else {
    SDN_METRIC_MERGE_RX(p1, p2);
    sdn_recv_queue_print();
  }
  sdn_packetbuf_pool_put((sdn_packetbuf *) p1);

  return 1;
}

void sdn_get_src_routing_info(uint8_t *p, uint16_t p_len, uint8_t header_size, sdnaddr_t *real_dest, uint8_t *path_len, sdnaddr_t **path) {

  if (!SDN_PACKET_IS_MERGED(p)) {
    switch (SDN_HEADER(p)->type) {
      case SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP: {
        sdn_src_rtd_control_flow_setup_t *cfs = (sdn_src_rtd_control_flow_setup_t *)p;
        *real_dest = cfs->real_destination;
        *path_len = cfs->path_len;
        *path = (sdnaddr_t *)((uint8_t *)cfs + sizeof(sdn_src_rtd_control_flow_setup_t));
        break;
      }
      case SDN_PACKET_SRC_ROUTED_DATA_FLOW_SETUP: {
        sdn_src_rtd_data_flow_setup_t *dfs = (sdn_src_rtd_data_flow_setup_t *)p;
        *real_dest = dfs->real_destination;
        *path_len = dfs->path_len;
        *path = (sdnaddr_t *)((uint8_t *)dfs + sizeof(sdn_src_rtd_data_flow_setup_t));
        break;
      }
      case SDN_PACKET_SRC_ROUTED_ACK: {
        sdn_src_rtd_ack_t *ack = (sdn_src_rtd_ack_t *)p;
        *real_dest = ack->real_destination;
        *path_len = ack->path_len;
        *path = (sdnaddr_t *)((uint8_t *)ack + sizeof(sdn_src_rtd_ack_t));
        break;
      }
      default:
        return;
    }
    return;
  }  

  // if packet is merged, access by the offset
  uint8_t *offset = p + header_size;
  uint8_t num_subpackets = *offset;
  offset++;

  // go through all the subpackets
  for (int i = 0; i < num_subpackets; i++) {
    offset++; // seq no
    offset += LINKADDR_SIZE; // source
    uint16_t len = *offset;
    offset++;
    offset += len;
  }

  *real_dest = *(sdnaddr_t*)offset;
  offset += sizeof(sdnaddr_t);

  *path_len = *offset;
  offset++;

  *path = (sdnaddr_t *)offset;
}

uint8_t sdn_queue_combine_src_packets(uint8_t *p1, uint16_t p1_len, uint8_t pos, uint8_t header_size, uint8_t queue) {
  
  uint8_t *p2 = (queue == SDN_RECEIVE_QUEUE) ? sdn_recv_queue_data[pos].data : sdn_send_queue_data[pos].data;
  uint16_t *p2_len = (queue == SDN_RECEIVE_QUEUE) ? &(sdn_recv_queue_data[pos].len) : &(sdn_send_queue_data[pos].len);
  
  sdnaddr_t real_dest1, real_dest2;
  uint8_t path_len1, path_len2;
  sdnaddr_t *path1, *path2;

  sdn_get_src_routing_info(p1, p1_len, header_size, &real_dest1, &path_len1, &path1);
  sdn_get_src_routing_info(p2, *p2_len, header_size, &real_dest2, &path_len2, &path2);

  uint16_t tail_len1 = sizeof(sdnaddr_t) + sizeof(uint8_t) + (path_len1 * sizeof(sdnaddr_t));
  uint16_t tail_len2 = sizeof(sdnaddr_t) + sizeof(uint8_t) + (path_len2 * sizeof(sdnaddr_t));

  // verify if after the merge it exceeds max packet size
  uint16_t merged_payload_max = SDN_MAX_PACKET_SIZE - tail_len2;
  if ((p1_len - tail_len1) + (*p2_len - tail_len2) > merged_payload_max) {
    //SDN_DEBUG_ERROR("Merged payload exceeds max size when considering tail\n");
    return 0;
  }

  // copy the path2
  sdnaddr_t temp_path2[path_len2];
  memcpy(temp_path2, path2, path_len2 * sizeof(sdnaddr_t));
  uint8_t temp_path_len2 = path_len2;
  sdnaddr_t temp_real_dest2 = real_dest2;

  // "cut off" the tail and combine packets
  *p2_len = *p2_len - tail_len2;
  if (!sdn_queue_combine_packets(p1, (p1_len - tail_len1), pos, header_size, queue)) {
    //SDN_DEBUG_ERROR("error on combining src routed packets\n");
    return 0;
  }

  // readd the tail to the merged packet
  if (queue == SDN_RECEIVE_QUEUE) {
    p2 = sdn_recv_queue_data[pos].data;
    *p2_len = sdn_recv_queue_data[pos].len;
  } else {
    p2 = sdn_send_queue_data[pos].data;
    *p2_len = sdn_send_queue_data[pos].len;
  }

  
  uint8_t *offset = p2 + *p2_len;

  // adding the real_destination
  memcpy(offset, &temp_real_dest2, sizeof(sdnaddr_t));
  offset += sizeof(sdnaddr_t);

  // adding path_len
  memcpy(offset, &temp_path_len2, sizeof(uint8_t));
  offset += sizeof(uint8_t);

  // adding path
  memcpy(offset, temp_path2, temp_path_len2 * sizeof(sdnaddr_t));
  offset += temp_path_len2 * sizeof(sdnaddr_t);

  // update the final merged packet length
  *p2_len = offset - p2;

  return 1;
}

uint8_t sdn_queue_replace_old_packet(uint8_t *data, uint16_t len, uint8_t pos, uint8_t header_size, uint8_t queue) {
  if (queue == SDN_RECEIVE_QUEUE) {
    // eliminate the old packet
    sdn_packetbuf_pool_put((sdn_packetbuf *)sdn_recv_queue_data[pos].data);

    // replace by the new one
    sdn_recv_queue_data[pos].len = len;
    sdn_recv_queue_data[pos].data = data;

#if defined (MANAGEMENT) && defined (SDN_ENABLED_NODE)
    sdn_recv_queue_data[pos].time = clock_time();
#endif

    SDN_METRIC_REPLACE_RX(data);
  }

  else {
    sdn_packetbuf_pool_put((sdn_packetbuf *)sdn_send_queue_data[pos].data);

    sdn_send_queue_data[pos].len = len;
    sdn_send_queue_data[pos].data = data;

#if defined (MANAGEMENT) && defined (SDN_ENABLED_NODE)
    sdn_send_queue_data[pos].time = clock_time();
#endif

    SDN_METRIC_REPLACE_TX(data);
  }

  return 1;
}

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
    *dest     = (uint8_t *)sdn_get_real_dest_from_merged_packet((uint8_t *)p);
    *dest_len = sizeof(sdnaddr_t);
    return 1;
  }
  return 0;
}

uint8_t  sdn_queue_delete_subpacket(uint8_t *p, uint16_t *p_len, uint8_t subpacket_num, uint8_t header_size, uint8_t p_pos, uint8_t queue) {

  uint8_t *num_subpackets_ptr = p + header_size;
  uint8_t num_subpackets = *num_subpackets_ptr;

  if(subpacket_num < 1 || subpacket_num > num_subpackets) return 0;

  uint8_t offset = header_size + 1;
  uint8_t current_subpacket = 1;

  // move to the start of the subpacket that will be removed
  while(current_subpacket < subpacket_num) {
    offset++;                      // skip seq no
    offset += LINKADDR_SIZE;       // skip source
    uint8_t len = p[offset];       // current packet length
    offset += 1 + len;             // skip 1 byte (that stores the len) + packet len
    current_subpacket++;
  }

  offset++; // skip the seq no
  offset += LINKADDR_SIZE; // skip the source
  uint8_t len_to_remove = p[offset];
  uint8_t total_remove_len = 2 + LINKADDR_SIZE + len_to_remove; // payload len + byte that stores the length and the seq no + LINKADDR_SIZE bytes that stores the source

  uint16_t start_of_subpacket = offset - 1 - LINKADDR_SIZE;
  uint16_t remaining_bytes = *p_len - (start_of_subpacket + total_remove_len); // -1 to go back to the start of the subpacket (in the seq no)

  // deslocate the content after the removed packet 
  memmove(&p[start_of_subpacket], &p[start_of_subpacket + total_remove_len], remaining_bytes);

  // update the num of subpackets
  (*num_subpackets_ptr)--;

  // update the packet lenght in the queue
  *p_len = *p_len - total_remove_len;
  if (queue == SDN_RECEIVE_QUEUE) sdn_recv_queue_data[p_pos].len = *p_len;
  else sdn_send_queue_data[p_pos].len = *p_len;

  return 1;
}

uint8_t sdn_queue_compare_packets(uint8_t *new_packet, uint16_t new_len, uint8_t *old_packet, uint16_t old_len, uint8_t header_size, uint8_t compare_size, uint8_t old_packet_pos, uint8_t queue) {
  
  uint8_t is_new_merged = SDN_PACKET_IS_MERGED(new_packet);
  uint8_t is_old_merged = SDN_PACKET_IS_MERGED(old_packet);

  uint8_t new_subs = SDN_GET_NUM_SUBPACKETS(new_packet, header_size);
  uint8_t old_subs = SDN_GET_NUM_SUBPACKETS(old_packet, header_size);

  uint8_t offset_new = header_size + (is_new_merged ? 1 : 0);

  // array that will store the duplicate subpackets that will be deleted
  uint8_t duplicates[old_subs];
  uint8_t duplicate_count = 0;

  // go through all of the new packet subpackets
  for (uint8_t i = 0; i < new_subs; i++) {
    sdnaddr_t new_src;
    if (is_new_merged) {
      offset_new++; // seq no
      memcpy(&new_src, new_packet + offset_new, LINKADDR_SIZE);
      offset_new += LINKADDR_SIZE; // src
    }
    else{
      new_src = SDN_HEADER(new_packet)->source;
    }

    uint8_t len_new = is_new_merged ? new_packet[offset_new++] : (new_len - header_size);
    uint8_t *payload_new = new_packet + offset_new;

    uint8_t offset_old = header_size + (is_old_merged ? 1 : 0);

    // compare it to all of the old packet subpackets
    for (uint8_t j = 0; j < old_subs; j++) {
      sdnaddr_t old_src;
      if (is_old_merged) {
        offset_old++; // seq no
        memcpy(&old_src, old_packet + offset_old, LINKADDR_SIZE);
        offset_old += LINKADDR_SIZE; // src
      }
      else {
        old_src = SDN_HEADER(old_packet)->source;
      }

      uint8_t same_src = sdnaddr_cmp(&new_src, &old_src);
      uint8_t len_old = is_old_merged ? old_packet[offset_old++] : (old_len - header_size);
      uint8_t *payload_old = old_packet + offset_old;

      if (is_new_merged && is_old_merged && !same_src) {
        offset_old += len_old;
        continue;
      }
      
      // if compare_size that was passed == 0, that means that we want to compare the entire payload
      uint8_t cmp_len = compare_size == 0 ? MIN(len_new, len_old) : compare_size;

      // if the area that we are interested in are the same:
      if (memcmp(payload_new, payload_old, cmp_len) == 0) {
        // case: old is not merged
        if (!is_old_merged) {
          return same_src? SDN_QUEUE_ACTION_REPLACE : SDN_QUEUE_ACTION_NONE;
        }
        // case: old is merged: register the subpacket index that need to be deleted
        if (duplicate_count < old_subs) {
          duplicates[duplicate_count++] = j + 1;
        }
      }
      offset_old += len_old;
    }
    offset_new += len_new;
  }

  // remove all of the old packet duplicate subpackets
  for (uint8_t i = duplicate_count; i > 0; i--) {
    sdn_queue_delete_subpacket(old_packet, &old_len, duplicates[i-1], header_size, old_packet_pos, queue);
  }

  return SDN_QUEUE_ACTION_MERGE;
}

uint8_t sdn_queue_determine_pckt_action(uint8_t *new_packet, uint8_t *old_packet, uint16_t new_len, uint16_t old_len, uint8_t *header_size, uint8_t old_packet_pos, uint8_t queue, uint8_t is_src_routed) {
  
  // if types are different dont compare;
  uint8_t type = SDN_HEADER(new_packet)->type;
  if (type != SDN_HEADER(old_packet)->type) return SDN_QUEUE_ACTION_NONE;

  // verify if the origin of the src rtd packets are the same
  uint8_t same_src = sdnaddr_cmp(&SDN_HEADER(new_packet)->source, &SDN_HEADER(old_packet)->source) == 0;

  // verify if the destination of the packets are the same
  uint8_t *dest1, *dest2;
  size_t dest_len1, dest_len2;

  if (!sdn_get_routing_dest(new_packet, &dest1, &dest_len1) || !sdn_get_routing_dest(old_packet, &dest2, &dest_len2)) return SDN_QUEUE_ACTION_NONE;

  if (dest_len1 != dest_len2) return SDN_QUEUE_ACTION_NONE;
  if (memcmp(dest1, dest2, dest_len1) != 0) return SDN_QUEUE_ACTION_NONE;

  // treat specific packet types
  uint16_t compare_size = 0;
  switch(type) {
    case SDN_PACKET_CONTROL_FLOW_SETUP:
      *header_size = sizeof(sdn_header_t) + sizeof(sdnaddr_t);
      compare_size = sizeof(sdnaddr_t); // route_destination
      return sdn_queue_compare_packets(new_packet, new_len, old_packet, old_len, *header_size, compare_size, old_packet_pos, queue);

    case SDN_PACKET_DATA_FLOW_SETUP:
      *header_size = sizeof(sdn_header_t) + sizeof(sdnaddr_t);
      compare_size = sizeof(flowid_t); // flowid
      return sdn_queue_compare_packets(new_packet, new_len, old_packet, old_len, *header_size, compare_size, old_packet_pos, queue);

    case SDN_PACKET_CONTROL_FLOW_REQUEST:
      *header_size = sizeof(sdn_header_t) + sizeof(flowid_t);
      compare_size = sizeof(sdnaddr_t); // address
      return sdn_queue_compare_packets(new_packet, new_len, old_packet, old_len, *header_size, compare_size, old_packet_pos, queue);

    case SDN_PACKET_DATA_FLOW_REQUEST:
      *header_size = sizeof(sdn_header_t) + sizeof(flowid_t);
      compare_size = sizeof(flowid_t); // flowid
      return sdn_queue_compare_packets(new_packet, new_len, old_packet, old_len, *header_size, compare_size, old_packet_pos, queue);      

    case SDN_PACKET_REGISTER_FLOWID:
      *header_size = sizeof(sdn_header_t) + sizeof(flowid_t);
      compare_size = sizeof(flowid_t); // flowid
      return sdn_queue_compare_packets(new_packet, new_len, old_packet, old_len, *header_size, compare_size, old_packet_pos, queue);

    case SDN_PACKET_ACK_BY_FLOW_ADDRESS:
      *header_size = sizeof(sdn_header_t) + sizeof(sdnaddr_t);
      compare_size = sizeof(uint8_t) * 2; // acked_packed_type and acked_packed_seqno
      return sdn_queue_compare_packets(new_packet, new_len, old_packet, old_len, *header_size, compare_size, old_packet_pos, queue);

    case SDN_PACKET_ACK_BY_FLOW_ID:
      *header_size = sizeof(sdn_header_t) + sizeof(flowid_t);
      compare_size = 0; // compare all the payload after the struct
      return sdn_queue_compare_packets(new_packet, new_len, old_packet, old_len, *header_size, compare_size, old_packet_pos, queue);

    case SDN_PACKET_DATA:
      *header_size = sizeof(sdn_header_t) + sizeof(flowid_t);
      // compare_size = sizeof(flowid_t); // flowid
      // return sdn_queue_compare_packets(new_packet, new_len, old_packet, old_len, *header_size, compare_size, old_packet_pos, queue);
      return SDN_QUEUE_ACTION_MERGE;

    case SDN_PACKET_NEIGHBOR_REPORT:
      if (!same_src) return SDN_QUEUE_ACTION_NONE;
      *header_size = sizeof(sdn_header_t);
      return SDN_QUEUE_ACTION_REPLACE; // always replace

    case SDN_PACKET_SRC_ROUTED_ACK:
      *header_size = sizeof(sdn_header_t) + sizeof(sdnaddr_t);
      compare_size = sizeof(uint8_t) * 2; // acked_packed_type and acked_packed_seqno;
      return sdn_queue_compare_packets(new_packet, new_len, old_packet, old_len, *header_size, compare_size, old_packet_pos, queue);

    case SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP:
      *header_size = sizeof(sdn_header_t) + sizeof(sdnaddr_t);
      compare_size = sizeof(sdnaddr_t); // flow_setup.route_destination
      return sdn_queue_compare_packets(new_packet, new_len, old_packet, old_len, *header_size, compare_size, old_packet_pos, queue);
    
    case SDN_PACKET_SRC_ROUTED_DATA_FLOW_SETUP:
      *header_size = sizeof(sdn_header_t) + sizeof(sdnaddr_t);
      compare_size = sizeof(flowid_t); // flow_setup.flowid
      return sdn_queue_compare_packets(new_packet, new_len, old_packet, old_len, *header_size, compare_size, old_packet_pos, queue);
    
    default:
      return SDN_QUEUE_ACTION_NONE;
  }
}

uint8_t sdn_queue_process_new_packet(uint8_t *new_packet, uint16_t new_len, uint8_t queue) {

  uint8_t new_type = SDN_HEADER(new_packet)->type;
  uint8_t is_src_routed = 0;

  // dont treat these types of packets
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
    
    case SDN_PACKET_SRC_ROUTED_ACK:
    case SDN_PACKET_SRC_ROUTED_DATA_FLOW_SETUP:
    case SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP:
      is_src_routed = 1;

    default:
      break;
  }

  if (queue == SDN_RECEIVE_QUEUE) {
    uint8_t pos = sdn_recv_queue.head;
    uint8_t header_size;

    while (pos != sdn_recv_queue.tail) {
      uint8_t *old_packet = sdn_recv_queue_data[pos].data;
      uint16_t old_len = sdn_recv_queue_data[pos].len;

      uint8_t action = sdn_queue_determine_pckt_action(new_packet, old_packet, new_len, old_len, &header_size, pos, queue, is_src_routed);

      if (action == SDN_QUEUE_ACTION_REPLACE) {
        SDN_DEBUG("RECEIVE QUEUE: Duplicate packet (type: %x) found, replacing the old one...\n", SDN_HEADER(new_packet)->type);
        return sdn_queue_replace_old_packet(new_packet, new_len, pos, header_size, queue);
      }

      if (action == SDN_QUEUE_ACTION_MERGE) {
        SDN_DEBUG("RECEIVE QUEUE: Same type packet (type: %x) found, merging packets...\n", SDN_HEADER(new_packet)->type);
        if (is_src_routed) return sdn_queue_combine_src_packets(new_packet, new_len, pos, header_size, queue);
        return sdn_queue_combine_packets(new_packet, new_len, pos, header_size, queue);
      }
      pos = (pos+1) % sdn_recv_queue.max_size;
    }
  } 
  
  else {
    uint8_t pos = sdn_send_queue.head;
    uint8_t header_size;

#ifdef SDN_ENABLED_NODE
    if (sdn_get_send_status() != SDN_SEND_IDLE) {
      if (sdn_send_queue.size == 1) return 0;
      pos = (pos+1) % sdn_send_queue.max_size;
    }
#endif

    while (pos != sdn_send_queue.tail) {
      uint8_t *old_packet = sdn_send_queue_data[pos].data;
      uint16_t old_len = sdn_send_queue_data[pos].len;

      uint8_t action = sdn_queue_determine_pckt_action(new_packet, old_packet, new_len, old_len, &header_size, pos, queue, is_src_routed);

      if (action == SDN_QUEUE_ACTION_REPLACE) {
        SDN_DEBUG("SEND QUEUE: Duplicate packet (type: %x) found, replacing the old one...\n", SDN_HEADER(new_packet)->type);
        return sdn_queue_replace_old_packet(new_packet, new_len, pos, header_size, queue);
        
      }

      if (action == SDN_QUEUE_ACTION_MERGE) {
        SDN_DEBUG("SEND QUEUE: Same type packet (type: %x) found, merging packets...\n", SDN_HEADER(new_packet)->type);
        if (is_src_routed) return sdn_queue_combine_src_packets(new_packet, new_len, pos, header_size, queue);
        return sdn_queue_combine_packets(new_packet, new_len, pos, header_size, queue);
      }
      pos = (pos+1) % sdn_send_queue.max_size;
    }
  }
  return 0;
}

uint8_t sdn_is_final_destination(uint8_t* packet) {

#ifdef SDN_CONTROLLER_NODE
  return 0;

#else
  if (SDN_ROUTED_BY_FLOWID(packet)) {
    struct data_flow_entry *dfe = sdn_dataflow_get(SDN_GET_PACKET_FLOW(packet));
    if(dfe == NULL) return 0;
    else if(dfe->action == SDN_ACTION_RECEIVE){
      // SDN_DEBUG("packet is in the destination, not proceding the verification\n");
      return 1;
    }
  } 
  
  else if (SDN_ROUTED_BY_ADDR(packet)) {
    struct control_flow_entry *cfe = sdn_controlflow_get(SDN_GET_PACKET_ADDR(packet));
    if(cfe == NULL) return 0;
    if(cfe->action == SDN_ACTION_RECEIVE){
      // SDN_DEBUG("packet is in the destination, not proceding the verification\n");
      return 1;
    }
  }

  else if (SDN_ROUTED_BY_SRC(packet)) {
    sdnaddr_t * next_hop = &SDN_GET_PACKET_ADDR(packet);
    if (sdnaddr_cmp(next_hop, &sdn_node_addr) == SDN_EQUAL){
      // SDN_DEBUG("packet is in the destination, not proceding the verification\n");
      return 1;
    }
  }

  else if (SDN_ROUTED_NOT(packet)) {
    if (sdnaddr_cmp(&((sdn_header_t*)packet)->source, &sdn_node_addr) != SDN_EQUAL){
      // SDN_DEBUG("packet is in the destination, not proceding the verification\n");
      return 1;
    }
  }
  
  return 0;
#endif
}

#endif