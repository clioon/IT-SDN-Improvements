#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>

#include "sdn-queue.h"
#include "sdn-protocol.h"

void treat_merged_src_rtd_packet(uint8_t * packet, uint16_t len){
  uint8_t header_size = sdn_get_header_size(packet);
  uint8_t num_subpackets = SDN_GET_NUM_SUBPACKETS(packet, header_size);
  uint16_t offset = header_size + 1; //header and number of subpackets

  // copy of the packet header that will be used in the subpackets that will turn into idividual packets
  uint8_t individual_packet[SDN_MAX_PACKET_SIZE];
  memset(individual_packet, 0, SDN_MAX_PACKET_SIZE);
  memcpy(individual_packet, packet, header_size);
  SDN_PACKET_SET_NOT_MERGED(individual_packet);
  
  // copy the tail of the src routed packet that will also be added to the end of the individual packet
  uint8_t tail[SDN_MAX_PACKET_SIZE];
  memset(tail, 0, SDN_MAX_PACKET_SIZE);
  size_t body_len = sdn_get_src_rtd_merged_subpackets_len(packet);
  uint8_t tail_len = len - (body_len + header_size);
  memcpy(tail, (packet + body_len), tail_len);

  // for every subpacket in packet
  for (int i = 0; i < num_subpackets; i++) { 
    
    if (offset + 2 + LINKADDR_SIZE > len) break;

    uint8_t seq_no = packet[offset++];
    sdnaddr_t source;
    memcpy(&source, &packet[offset], LINKADDR_SIZE);
    offset += LINKADDR_SIZE;
    uint8_t subpacket_len = packet[offset++];

    if ((offset+subpacket_len) > len) break;

    memset(individual_packet + header_size, 0, SDN_MAX_PACKET_SIZE - header_size);

    //copy the subpacket payload to individual_packet after the header
    memcpy(individual_packet + header_size, packet + offset, subpacket_len);
    offset += subpacket_len;

    // restore the original seq no
    SDN_HEADER(individual_packet)->seq_no = seq_no;
    printf("after unmerged seq no: %x", SDN_HEADER(individual_packet)->seq_no);

    // restore the original source
    SDN_HEADER(individual_packet)->source = source;
    printf("after unmerged source: %02x:%02x", SDN_HEADER(individual_packet)->source.u8[0], SDN_HEADER(individual_packet)->source.u8[1]);

    // copy the tail to the end of the individual packet
    memcpy(individual_packet + header_size + subpacket_len, tail, tail_len);

    //treat this subpacket
    //SDN_DEBUG("treating packet %d\n", i);
    uint8_t header_size = sdn_get_header_size(individual_packet);
    imprimir(individual_packet, (subpacket_len + header_size + tail_len), header_size);
  }
}

void treat_merged_packet(uint8_t * packet, uint16_t len) {
  uint8_t header_size = sdn_get_header_size(packet);
  uint8_t num_subpackets = SDN_GET_NUM_SUBPACKETS(packet, header_size);
  uint16_t offset = header_size + 1; //header and number of subpackets

  // copy of the packet header that will be used separate the subpackets into idividual packets
  uint8_t individual_packet[SDN_MAX_PACKET_SIZE];
  memset(individual_packet, 0, SDN_MAX_PACKET_SIZE);
  memcpy(individual_packet, packet, header_size);
  SDN_PACKET_SET_NOT_MERGED(individual_packet);
  
  // for every subpacket in packet
  for (int i = 0; i < num_subpackets; i++) { 
    
    if (offset + 2 + LINKADDR_SIZE > len) break;

    uint8_t seq_no = packet[offset++];
    sdnaddr_t source;
    memcpy(&source, &packet[offset], LINKADDR_SIZE);
    offset += LINKADDR_SIZE;
    uint8_t subpacket_len = packet[offset++];

    if ((offset+subpacket_len) > len) break;

    memset(individual_packet + header_size, 0, SDN_MAX_PACKET_SIZE - header_size);

    // copy the subpacket payload to individual_packet after the header
    memcpy(individual_packet + header_size, packet + offset, subpacket_len);
    offset += subpacket_len;

    // restore the original seq no
    SDN_HEADER(individual_packet)->seq_no = seq_no;
    printf("after unmerged seq no: %x", SDN_HEADER(individual_packet)->seq_no);

    // restore the original source
    SDN_HEADER(individual_packet)->source = source;
    printf("after unmerged source: %02x:%02x", SDN_HEADER(individual_packet)->source.u8[0], SDN_HEADER(individual_packet)->source.u8[1]);

    // treat this subpacket
    //SDN_DEBUG("treating packet %d\n", i);
    uint8_t header_size = sdn_get_header_size(individual_packet);
    imprimir(individual_packet, (subpacket_len + header_size), header_size);
  }
}

void test_compare() {
  printf("\n=== TESTANDO COMPARAÇÃO ===\n");

  // inicializar fila
  sdn_recv_queue_init();

  uint8_t p1[SDN_MAX_PACKET_SIZE];
  uint8_t p1_len;
  uint8_t p2[SDN_MAX_PACKET_SIZE];
  uint8_t p2_len;
  uint8_t p3[SDN_MAX_PACKET_SIZE];
  uint8_t p3_len;

  linkaddr_t addr1;
  addr1.u8[0] = 1;
  addr1.u8[1] = 0;

  linkaddr_t addr2;
  addr2.u8[0] = 1;
  addr2.u8[1] = 2;

  linkaddr_t addr3;
  addr3.u8[0] = 1;
  addr3.u8[1] = 3;
  
  // Pacote 1 (já na fila)
  ((sdn_data_t*)p1)->header.type = SDN_PACKET_DATA;
  ((sdn_data_t*)p1)->header.reserved = 0x00;
  ((sdn_data_t*)p1)->header.thl = 60;
  ((sdn_data_t*)p1)->header.seq_no = 1;
  ((sdn_data_t*)p1)->header.source = addr1;
  ((sdn_data_t*)p1)->flowid = 2025;
  memcpy(p1 + sizeof(sdn_data_t), "hello world", strlen("hello world"));
  p1_len = sizeof(sdn_data_t) + strlen("hello world");
  
  // Pacote 2
  ((sdn_data_t*)p2)->header.type = SDN_PACKET_DATA;
  ((sdn_data_t*)p2)->header.reserved = 0x00;
  ((sdn_data_t*)p2)->header.thl = 60;
  ((sdn_data_t*)p2)->header.seq_no = 2;
  ((sdn_data_t*)p2)->header.source = addr2;
  ((sdn_data_t*)p2)->flowid = 2025;
  memcpy(p2 + sizeof(sdn_data_t), "goodbye", strlen("goodbye"));
  p2_len = sizeof(sdn_data_t) + strlen("goodbye");

  // Pacote 3
  ((sdn_data_t*)p3)->header.type = SDN_PACKET_DATA;
  ((sdn_data_t*)p3)->header.reserved = 0x00;
  ((sdn_data_t*)p3)->header.thl = 60;
  ((sdn_data_t*)p3)->header.seq_no = 3;
  ((sdn_data_t*)p3)->header.source = addr3;
  ((sdn_data_t*)p3)->flowid = 2025;
  memcpy(p3 + sizeof(sdn_data_t), "hello world", strlen("hello world"));
  p3_len = sizeof(sdn_data_t) + strlen("hello world");
  
  // Coloca p1 na fila
  printf("\nProcessando novo pacote:  ");
  imprimir(p1, p1_len, sizeof(sdn_data_t));
  printf("\n");
  sdn_recv_queue_enqueue(p1, p1_len);
  printf("\n");
  sdn_recv_queue_print();
  printf("===============\n");
  
  // Coloca p2 na fila
  printf("\nProcessando novo pacote:  ");
  imprimir(p2, p2_len, sizeof(sdn_data_t));
  printf("\n");
  sdn_recv_queue_enqueue(p2, p2_len);
  printf("\n");
  sdn_recv_queue_print();
  printf("===============\n");

  // Coloca p3 na fila
  printf("\nProcessando novo pacote:  ");
  imprimir(p3, p3_len, sizeof(sdn_data_t));
  printf("\n");
  sdn_recv_queue_enqueue(p3, p3_len);
  printf("\n");
  sdn_recv_queue_print();

  printf("===============\n");
  printf("\n DEMERGE\n");
  sdn_recv_queue_data_t* packet = sdn_recv_queue_head();
  imprimir(packet->data, packet->len, sizeof(sdn_data_t));
  treat_merged_packet(packet->data, packet->len);

}

void test_compare_src_rtd_flow_setup() {
  printf("\n=== TESTANDO COMPARAÇÃO SRC_ROUTED_CONTROL_FLOW_SETUP ===\n");

  // inicializar fila
  sdn_recv_queue_init();

  uint8_t p1[SDN_MAX_PACKET_SIZE];
  uint8_t p1_len;
  uint8_t p2[SDN_MAX_PACKET_SIZE];
  uint8_t p2_len;
  uint8_t p3[SDN_MAX_PACKET_SIZE];
  uint8_t p3_len;

  linkaddr_t addr1 = { .u8 = {1, 0} };
  linkaddr_t addr2 = { .u8 = {1, 2} };
  linkaddr_t addr3 = { .u8 = {1, 3} };
  
  // Pacote 1
  sdn_src_rtd_control_flow_setup_t *pckt1 = (sdn_src_rtd_control_flow_setup_t *)p1;
  pckt1->flow_setup.header.type = SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP;
  pckt1->flow_setup.header.reserved = 0x00;
  pckt1->flow_setup.header.thl = 60;
  pckt1->flow_setup.header.seq_no = 1;
  pckt1->flow_setup.header.source = addr1;
  pckt1->flow_setup.destination = addr2;
  pckt1->flow_setup.route_destination = addr3;
  pckt1->flow_setup.action_id = 5;
  pckt1->flow_setup.action_parameter = addr1;
  pckt1->real_destination = addr2;
  pckt1->path_len = 2;
  
  // colocar caminho (2 hops)
  memcpy(p1 + sizeof(sdn_src_rtd_control_flow_setup_t), &addr1, sizeof(linkaddr_t));
  memcpy(p1 + sizeof(sdn_src_rtd_control_flow_setup_t) + sizeof(linkaddr_t), &addr2, sizeof(linkaddr_t));
  
  p1_len = sizeof(sdn_src_rtd_control_flow_setup_t) + 2 * sizeof(linkaddr_t);

  // Pacote 2
  sdn_src_rtd_control_flow_setup_t *pckt2 = (sdn_src_rtd_control_flow_setup_t *)p2;
  pckt2->flow_setup.header.type = SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP;
  pckt2->flow_setup.header.reserved = 0x00;
  pckt2->flow_setup.header.thl = 60;
  pckt2->flow_setup.header.seq_no = 2;
  pckt2->flow_setup.header.source = addr2;
  pckt2->flow_setup.destination = addr3;
  pckt2->flow_setup.route_destination = addr2;
  pckt2->flow_setup.action_id = 5;
  pckt2->flow_setup.action_parameter = addr2;
  pckt2->real_destination = addr2;
  pckt2->path_len = 2;
  
  memcpy(p2 + sizeof(sdn_src_rtd_control_flow_setup_t), &addr2, sizeof(linkaddr_t));
  memcpy(p2 + sizeof(sdn_src_rtd_control_flow_setup_t) + sizeof(linkaddr_t), &addr3, sizeof(linkaddr_t));
  
  p2_len = sizeof(sdn_src_rtd_control_flow_setup_t) + 2 * sizeof(linkaddr_t);

  // Pacote 3
  sdn_src_rtd_control_flow_setup_t *pckt3 = (sdn_src_rtd_control_flow_setup_t *)p3;
  pckt3->flow_setup.header.type = SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP;
  pckt3->flow_setup.header.reserved = 0x00;
  pckt3->flow_setup.header.thl = 60;
  pckt3->flow_setup.header.seq_no = 3;
  pckt3->flow_setup.header.source = addr3;
  pckt3->flow_setup.destination = addr1;
  pckt3->flow_setup.route_destination = addr1;
  pckt3->flow_setup.action_id = 5;
  pckt3->flow_setup.action_parameter = addr3;
  pckt3->real_destination = addr2;
  pckt3->path_len = 2;
  
  memcpy(p3 + sizeof(sdn_src_rtd_control_flow_setup_t), &addr3, sizeof(linkaddr_t));
  memcpy(p3 + sizeof(sdn_src_rtd_control_flow_setup_t) + sizeof(linkaddr_t), &addr1, sizeof(linkaddr_t));
  
  p3_len = sizeof(sdn_src_rtd_control_flow_setup_t) + 2 * sizeof(linkaddr_t);

  //  inserir pacote 1 na fila:
  printf("\nProcessando novo pacote:  ");
  imprimir(p1, p1_len, sizeof(sdn_src_rtd_control_flow_setup_t));
  printf("\n");
  sdn_recv_queue_enqueue(p1, p1_len);
  printf("\n");
  sdn_recv_queue_print();
  printf("===============\n");

  // inserir pacote 2 na fila:
  printf("\nProcessando novo pacote:  ");
  imprimir(p2, p2_len, sizeof(sdn_src_rtd_control_flow_setup_t));
  printf("\n");
  //sdn_recv_queue_combine_src_packets(p2, p2_len, 0, (sizeof(sdn_header_t) + sizeof(sdnaddr_t)));
  sdn_recv_queue_enqueue(p2, p2_len);
  printf("\n");
  sdn_recv_queue_print();
  printf("===============\n");

  printf("\nProcessando novo pacote:  ");
  imprimir(p3, p3_len, sizeof(sdn_src_rtd_control_flow_setup_t));
  printf("\n");
  //sdn_recv_queue_combine_src_packets(p3, p3_len, 0, (sizeof(sdn_header_t) + sizeof(sdnaddr_t)));
  sdn_recv_queue_enqueue(p3, p3_len);
  printf("\n");
  sdn_recv_queue_print();

  printf("===============\n");
  printf("\nDEMERGE\n");
  sdn_recv_queue_data_t* packet = sdn_recv_queue_head();
  imprimir(packet->data, packet->len, sizeof(sdn_src_rtd_control_flow_setup_t));
  treat_merged_src_rtd_packet(packet->data, packet->len);
}

int main() {
 
  //test_compare_src_rtd_flow_setup();
  test_compare();

  return 0;
}

// 05 | 00 | 3C | 01 | 01 00 | E9 07
// 05 | 00 | 3C | 02 | 01 00 | E9 07
// 05 | 00 | 3C | 03 | 01 00 | E9 07
// T  | r  | thl| seq|  src  | flow id

// 05 00 3C 01 01 00 | E9 07 | 68 65 6C 6C 6F 20 77 6F 72 6C 64
// 05 01 3C 01 01 00 | E9 07 | 02 | 01 | 0B | 68 65 6C 6C 6F 20 77 6F 72 6C 64 | 02 | 07 | 67 6F 6F 64 62 79 65
// 05 01 3C 01 01 00 | E9 07 | 03 | 01 | 0B | 68 65 6C 6C 6F 20 77 6F 72 6C 64 | 02 | 07 | 67 6F 6F 64 62 79 65 | 03 | 0B | 68 65 6C 6C 6F 20 77 6F 72 6C 64


// E6 | 00 | 3C | 01 | 01 00 | 01 02 | 01 03 | 05 | 01 00 | 01 02 | 02 | 01 00 | 01 02
// E6 | 00 | 3C | 02 | 01 00 | 01 03 | 01 02 | 05 | 01 02 | 01 02 | 02 | 01 02 | 01 03
// E6 | 00 | 3C | 03 | 01 00 | 01 00 | 01 00 | 05 | 01 03 | 01 02 | 02 | 01 00 | 01 03
//  T   res  thl  seq   src  |   dst   r_dst   act  param | real_d  len   hop1   hop2
 
// E6 | 01 | 3C | 01 | 01 00 | 01 02 | 03 | 01 | 05 | 01 03 05 01 00 | 02 | 05 | 01 02 05 01 02 | 03 | 05 | 01 00 05 01 03 | 01 02 02 01 00 01 02
//                           |       |    |         |                |    |    |                |         |                | 

// with source
// [ 05 00 3C 01 01 00 ] [ E9 07 68 65 6C 6C 6F 20 77 6F 72 6C 64 ]
// [ 05 01 3C 01 01 00 ] [ E9 07 02 01 01 00 0B 68 65 6C 6C 6F 20 77 6F 72 6C 64 02 01 02 07 67 6F 6F 64 62 79 65 ]
// [ 05 01 3C 01 01 00 ] [ E9 07 03 01 01 00 0B 68 65 6C 6C 6F 20 77 6F 72 6C 64 02 01 02 07 67 6F 6F 64 62 79 65 03 01 03 0B 68 65 6C 6C 6F 20 77 6F 72 6C 64 ]

// [ E6 00 3C 01 01 00 ] [ 01 02 01 03 05 01 00 01 02 02 01 00 01 02 ]
// [ E6 01 3C 01 01 00 ] [ 01 02 02 01 01 00 05 01 03 05 01 00 02 01 02 05 01 02 05 01 02 01 02 02 01 00 01 02 ]
// [ E6 01 3C 01 01 00 ] [ 01 02 03 01 01 00 05 01 03 05 01 00 02 01 02 05 01 02 05 01 02 03 01 03 05 01 00 05 01 03 01 02 02 01 00 01 02 ]
