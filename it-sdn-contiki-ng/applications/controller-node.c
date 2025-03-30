/*
* Header Here!
*/

#include <stdio.h>
#include "contiki.h"
#include "string.h"
#include "sys/etimer.h"

#include "sdn-protocol.h"
#include "net/packetbuf.h"
#include "sdn-debug.h"
#include "sdn-network.h"
#include "sdn-addr.h"
#include "net/netstack.h"
#include "sdn-constants.h"
#include "sys/ctimer.h"
#include "sdn-serial.h"
#include "sdn-send.h"
#include "sdn-queue.h"
#include "sdn-packetbuf.h"
#include "sdn-send-packet.h"
#include "sdn-receive.h"
#include "sdn-neighbor-table.h"

#if RDC_UNIDIR_SUPPORT
#include "contikimac.h"
#include "phase.h"
#endif

void sdn_receive();
void nd_event();
void cd_event();

extern uint8_t sdn_seq_no;

/*---------------------------------------------------------------------------*/
PROCESS(sdn_core_process, "SDN Controller process");
AUTOSTART_PROCESSES(&sdn_core_process);

void sdn_receive() {

  sdn_serial_packet_t serial_pkt;
  uint8_t * packet_ptr = packetbuf_dataptr();
  uint8_t * pkt_to_enqueue_ptr;
  uint8_t packet_len = packetbuf_datalen();

#ifdef DEBUG_SDN
  int index;
  SDN_DEBUG ("Packet Received from [%02X", packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[0]);
  for (index = 1; index < SDNADDR_SIZE; index++) {
    SDN_DEBUG (":%02X", packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[index]);
  }
#endif

  SDN_DEBUG ("] \n");


// int index;
// printf ("Packet Received from [%02X", packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[0]);
// for (index = 1; index < SDNADDR_SIZE; index++) {
// printf (":%02X", packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[index]);
// }
// printf ("] \n");
// printf ("[ \n");
// for (index=0; index < packet_len; index++) {
// printf (":%02X", ((uint8_t *)packetbuf_dataptr())[index]);
// }
// printf ("] \n");

  print_packet((uint8_t*)packetbuf_dataptr(), packetbuf_datalen());

  if (sdn_seqno_is_duplicate(SDN_HEADER(packetbuf_dataptr())) == SDN_YES) {
    SDN_DEBUG("Packet is duplicated, dropping.\n");
    return;
  }
  sdn_seqno_register(SDN_HEADER(packetbuf_dataptr()));

  if (SDN_HEADER(packet_ptr)->thl == 0) {
    SDN_DEBUG ("THL expired.\n");
    return;
  }
  SDN_HEADER(packet_ptr)->thl --;

#ifdef SDN_METRIC
  // assuming the controller receives packets only to SDN_CONTROLLER_FLOW or its own address
  // source routed packets are not expected
  if ( (SDN_ROUTED_BY_ADDR(packet_ptr) && sdnaddr_cmp(&sdn_node_addr, &SDN_GET_PACKET_ADDR(packet_ptr)) == SDN_EQUAL) ||
       (SDN_ROUTED_BY_FLOWID(packet_ptr) && flowid_cmp(&sdn_controller_flow, &SDN_GET_PACKET_FLOW(packet_ptr))== SDN_EQUAL) ||
        SDN_ROUTED_NOT(packet_ptr) ) {
    SDN_METRIC_RX(packet_ptr);
  }
#endif // SDN_METRIC

  switch (SDN_HEADER(packet_ptr)->type) {
    case SDN_PACKET_ND:
    //   SDN_ND.input(packet_ptr, packet_len);
    // break;
    case SDN_PACKET_CD:
      // SDN_CD.input(packet_ptr, packet_len);
      pkt_to_enqueue_ptr = (uint8_t*) sdn_packetbuf_pool_get();
      if (pkt_to_enqueue_ptr) {
        memcpy(pkt_to_enqueue_ptr, packet_ptr, packet_len);
        if (sdn_recv_queue_enqueue(pkt_to_enqueue_ptr, packet_len) != SDN_SUCCESS) {
          sdn_packetbuf_pool_put((struct sdn_packetbuf *)pkt_to_enqueue_ptr);
          SDN_DEBUG_ERROR ("Error on packet enqueue.\n");
        } else {
          if(sdn_recv_queue_size() == 1) {
            process_post(&sdn_core_process, SDN_EVENT_NEW_PACKET, 0);
          }
        }
      }
    break;

    default:
      memcpy(serial_pkt.payload, packet_ptr, packet_len);
      // Setting serial packet address as the MAC sender.
      sdnaddr_copy(&serial_pkt.header.node_addr, (sdnaddr_t*)packetbuf_addr(PACKETBUF_ADDR_SENDER));
      serial_pkt.header.msg_type = SDN_SERIAL_MSG_TYPE_RADIO;
      serial_pkt.header.payload_len = packet_len;
      // printf("Sending to Serial\n");
      sdn_serial_send(&serial_pkt);
    break;

  }
}

void nd_event() {

  SDN_DEBUG ("Neighbor Information!\n");
  sdn_send_neighbor_report();
}

void cd_event() {

}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sdn_core_process, ev, data)
{
  PROCESS_BEGIN();

  SDN_CD.init(cd_event, SDN_YES);
  SDN_ND.init(nd_event, SDN_YES);
#if RDC_UNIDIR_SUPPORT
  set_unidir_phase_callback(&sdn_send_phase);
#endif

  memcpy(&sdn_node_addr, &linkaddr_node_addr, sizeof(sdnaddr_t));
  sdn_serial_init(&putchar);
  sdn_send_init();
  sdn_recv_queue_init();

  // printf("Sending to Serial\n");
  // sdn_serial_packet_t my_serial;
  // for (int i = 0; i < 26; i++)
  //   my_serial.payload[i] = 'a' + i;
  // my_serial.header.node_addr.u8[0] = 0xEE;
  // my_serial.header.node_addr.u8[1] = 0xDD;
  // my_serial.header.msg_type = SDN_SERIAL_MSG_TYPE_RADIO;
  // my_serial.header.payload_len = 26;

  // for (int i = 0; i < 30; i++) {
  //   printf(" %02x", ((uint8_t*)&my_serial)[i]);
  // }
  // printf(" ...done \n");
  // sdn_serial_send(&my_serial);

#if SDN_TX_RELIABILITY
  static sdn_serial_packet_t serial_pkt_time;
  serial_pkt_time.header.msg_type = SDN_SERIAL_MSG_TYPE_TIMER;
  serial_pkt_time.header.payload_len = 0;

  static struct etimer periodic_timer;
  //sets Clock pulse to sicronize controller_pc timer
  etimer_set(&periodic_timer, CLOCK_SECOND) ;
#endif // SDN_TX_RELIABILITY

  while(1) {
    PROCESS_WAIT_EVENT();

    // if (ev == SDN_EVENT_ND_CHANGE) {
    //    send neighbor report through serial connection
    //   SDN_DEBUG ("Neighbor Information!\n");
    //   sdn_send_neighbor_report();
    // }
#if SDN_TX_RELIABILITY
    if (etimer_expired(&periodic_timer)) {
      sdn_serial_send(&serial_pkt_time);
      etimer_restart(&periodic_timer);
    }
#endif // SDN_TX_RELIABILITY

    if (ev == SDN_EVENT_NEW_PACKET) {
      sdn_recv_queue_data_t* queue_data = sdn_recv_queue_dequeue();
      if (queue_data) {
        switch (SDN_HEADER(queue_data->data)->type) {
          case SDN_PACKET_ND:
            SDN_ND.input(queue_data->data, queue_data->len);
          break;
          case SDN_PACKET_CD:
            SDN_CD.input(queue_data->data, queue_data->len);
          break;
        }
        sdn_packetbuf_pool_put((struct sdn_packetbuf *)queue_data->data);
      }

      if(sdn_recv_queue_size() > 0) {
        process_post(&sdn_core_process, SDN_EVENT_NEW_PACKET, 0);
      }
    }

    if (ev == sdn_raw_binary_packet_ev) { /* Packet received from serial */

      /* Send packet->payload to Radio Stack */
      sdn_serial_packet_t *packet = (sdn_serial_packet_t*)data;
      struct sdn_packetbuf * pkt_to_enqueue_ptr;
#ifdef SDN_METRIC
      if (sdnaddr_cmp(&sdn_node_addr, &SDN_HEADER(packet->payload)->source) == SDN_EQUAL) {
        SDN_METRIC_TX(packet->payload);
      }
#endif //SDN_METRIC

      pkt_to_enqueue_ptr = sdn_packetbuf_pool_get();
      if (pkt_to_enqueue_ptr == NULL) {
        SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
        sdn_serial_send_nack();
      } else {
        memcpy(pkt_to_enqueue_ptr, packet->payload, packet->header.payload_len);
        if (sdn_send_queue_enqueue((uint8_t *) pkt_to_enqueue_ptr, packet->header.payload_len, 0) != SDN_SUCCESS) {
          sdn_packetbuf_pool_put((struct sdn_packetbuf *)pkt_to_enqueue_ptr);
          SDN_DEBUG_ERROR ("Error on packet enqueue.\n");
          sdn_serial_send_nack();
        } else {
          // trick to avoid bogus seq_num duplicate detection in ND and CD packets
          sdn_seq_no = SDN_HEADER(pkt_to_enqueue_ptr)->seq_no + SDN_MAX_SEQNOS;
          sdnaddr_copy(&sdn_send_queue_tail()->last_next_hop, &packet->header.node_addr);
          sdn_send_queue_tail()->action = packet->header.reserved[0];
          // printf("res %u", packet->header.reserved[0]);

          sdn_send_down_once();
        }
      }
    }

    if (ev == sdn_custom_packet_ev) {
      sdn_serial_packet_t *serial_packet = (sdn_serial_packet_t*)data;

      SDN_DEBUG("PACKET ID %d\n", serial_packet->header.msg_type);
      if (serial_packet->header.msg_type == SDN_SERIAL_MSG_TYPE_REACH_INFO) {
        uint8_t * cd_packet_ptr = (uint8_t *) sdn_packetbuf_pool_get();
        if (cd_packet_ptr) {
          SDN_HEADER(cd_packet_ptr)->type = SDN_PACKET_CD;
          sdnaddr_copy(&SDN_HEADER(cd_packet_ptr)->source, &sdn_node_addr);
          cd_packet_ptr[sizeof(sdn_header_t)] = 1;
          memcpy(cd_packet_ptr + sizeof(sdn_header_t) + 1, serial_packet->payload, serial_packet->header.payload_len);

          SDN_CD.input(cd_packet_ptr, serial_packet->header.payload_len);
          sdn_packetbuf_pool_put((struct sdn_packetbuf *)cd_packet_ptr);
        }
      }
#ifdef SDN_METRIC
      if (serial_packet->header.msg_type == SDN_SERIAL_MSG_TYPE_FULL_GRAPH) {
        printf("=FG=%d\n",(uint8_t)*serial_packet->payload);
      }
#endif // SDN_METRIC
      if (serial_packet->header.msg_type == SDN_SERIAL_MSG_TYPE_PC_TO_NODE) {
        if (SDN_HEADER(serial_packet->payload)->type == SDN_PACKET_GENERIC_ADDR &&
            SDN_PACKET_GET_FIELD(serial_packet->payload, sdn_generic_addr_t, port) == SDN_PACKET_PHASE_INFO ) {

          if (sdn_neighbor_table_get(SDN_PACKET_GET_FIELD(serial_packet->payload, sdn_phase_info_addr_t, phase_receiver)) == NULL) {
#if RDC_UNIDIR_SUPPORT
            SDN_DEBUG("==NULL, phase_update_unidir(...)\n");
            phase_update_unidir(
              (linkaddr_t *) &SDN_PACKET_GET_FIELD(serial_packet->payload, sdn_phase_info_addr_t, phase_receiver),
              SDN_PACKET_GET_FIELD(serial_packet->payload, sdn_phase_info_addr_t, phase)
              );
#endif
          } else {
            SDN_DEBUG("Link is bidir, do not phase_update_unidir(...)\n");
          }

        } else {
          SDN_DEBUG("Unexpected SDN_SERIAL_MSG_TYPE_PC_TO_NODE message\n");
        }
      }
      sdn_serial_send_ack();
    }

  }

  PROCESS_END();
}
