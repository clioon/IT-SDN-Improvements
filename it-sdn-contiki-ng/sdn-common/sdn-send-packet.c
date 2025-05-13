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

#include "sdn-send-packet.h"
#include "sdn-packetbuf.h"
#include "sdn-debug.h"
#include "sdn-constants.h"
#include "sdn-queue.h"
#include "string.h"
#ifndef SDN_CONTROLLER_PC
#include "sdn-neighbor-table.h"
#include "sdn-send.h"
#include "control-flow-table.h"

#ifdef ENERGY_MANAGER
#include "energy-manager.h"
#endif // ENERGY_MANAGER

#else // SDN_CONTROLLER_PC
#include "stdlib.h"
#include "sdn-serial-send.h"
#include "sdn-serial-packet.h"
#endif // SDN_CONTROLLER_PC

#ifdef SDN_CONTROLLER_NODE
#include "sdn-serial-packet.h"
#include "sdn-serial.h"
#endif // SDN_CONTROLLER_NODE

#ifdef MANAGEMENT
#include "manage-info.h"
#endif


#ifdef DEMO
#include "leds.h"
#endif // DEMO

uint8_t sdn_seq_no = 0;

uint8_t sdn_send_control_flow_setup(sdnaddr_t* packet_destination, sdnaddr_t* route_destination, sdnaddr_t* route_nexthop, action_t action) {
  sdn_control_flow_setup_t *sdn_packet = (sdn_control_flow_setup_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len = sizeof(sdn_control_flow_setup_t);

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_CONTROL_FLOW_SETUP, SDN_DEFAULT_TTL);
  sdnaddr_copy(&sdn_packet->route_destination, route_destination);
  sdnaddr_copy(&sdn_packet->destination, packet_destination);
  sdnaddr_copy(&sdn_packet->action_parameter, route_nexthop);
  sdn_packet->action_id = action;

  //printf("Sending packet: ");
  //print_packet((uint8_t *)sdn_packet, packet_len);

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
}

uint8_t sdn_send_data_flow_setup(sdnaddr_t* packet_destination, flowid_t dest_flowid, sdnaddr_t* route_nexthop, action_t action) {
  sdn_data_flow_setup_t *sdn_packet = (sdn_data_flow_setup_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len = sizeof(sdn_data_flow_setup_t);

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_DATA_FLOW_SETUP, SDN_DEFAULT_TTL);
  flowid_copy(&sdn_packet->flowid, &dest_flowid);
  sdnaddr_copy(&sdn_packet->destination, packet_destination);
  sdnaddr_copy(&sdn_packet->action_parameter, route_nexthop);
  sdn_packet->action_id = action;

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
}

uint8_t sdn_send_control_flow_request(sdnaddr_t address) {

#if SDN_TX_RELIABILITY && !defined(SDN_CONTROLLER_PC)
  if (has_control_flow_request_retransmission(address)){

    return SDN_ERROR;
  }
#endif

  sdn_control_flow_request_t *sdn_packet = (sdn_control_flow_request_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len = sizeof(sdn_control_flow_request_t);

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_CONTROL_FLOW_REQUEST, SDN_DEFAULT_TTL);
  sdnaddr_copy(&sdn_packet->address, &address);
  flowid_copy(&sdn_packet->dest_flowid, &sdn_controller_flow);

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
}

uint8_t sdn_send_data_flow_request(flowid_t flowid) {

#if SDN_TX_RELIABILITY && !defined(SDN_CONTROLLER_PC)
  if (has_data_flow_request_retransmission(flowid)){

    return SDN_ERROR;
  }
#endif

  sdn_data_flow_request_t *sdn_packet = (sdn_data_flow_request_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len = sizeof(sdn_data_flow_request_t);

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_DATA_FLOW_REQUEST, SDN_DEFAULT_TTL);
  flowid_copy(&sdn_packet->flowid, &flowid);
  flowid_copy(&sdn_packet->dest_flowid, &sdn_controller_flow);

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
}

#ifndef SDN_CONTROLLER_PC
uint8_t sdn_send_neighbor_report() {
  sdn_neighbor_report_t* sdn_packet;
  uint16_t packet_len = sizeof(sdn_neighbor_report_t);
  struct sdn_neighbor_entry* sne;
  sdn_neighbor_report_list_t nrl;
  sdn_send_queue_data_t* queue_entry;
  __attribute__((unused)) uint8_t temp_seqno = 0;

  queue_entry = sdn_send_queue_find_by_type(SDN_PACKET_NEIGHBOR_REPORT);

  // if (sdn_neighbor_table_count() == 0) {
  //   SDN_DEBUG("No neighbors to send.\n");
  //   return SDN_ERROR;
  // }

  if (queue_entry != NULL) {
    sdn_packet = (sdn_neighbor_report_t *) queue_entry->data;
    temp_seqno = SDN_HEADER(sdn_packet)->seq_no;
    SDN_DEBUG("Replacing neighbor report in sending queue\n");
  } else {
    sdn_packet = (sdn_neighbor_report_t *) sdn_packetbuf_pool_get();
  }

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

#if SDN_TX_RELIABILITY
  sld_purge(SDN_PACKET_NEIGHBOR_REPORT);
#endif

  MAKE_SDN_HEADER(SDN_PACKET_NEIGHBOR_REPORT, SDN_DEFAULT_TTL);
  sdn_packet->flowid = SDN_CONTROLLER_FLOW;
  sdn_packet->num_of_neighbors = 0;

  for (sne = sdn_neighbor_table_head(); sne != NULL && packet_len < SDN_MAX_PACKET_SIZE; \
       sne = sdn_neighbor_table_next(sne->neighbor_addr)) {
    // copy_info
    sdnaddr_copy(&nrl.neighbor, &sne->neighbor_addr);
    nrl.etx = sne->metric;
    memcpy(((uint8_t *)sdn_packet) + packet_len, (uint8_t *) &nrl, sizeof(nrl));
    sdn_packet->num_of_neighbors ++;
    packet_len += sizeof(sdn_neighbor_report_list_t);
  }

  if (sne != NULL) {
    SDN_DEBUG_ERROR("Too many neighbors to fit in a packet.\n");
  }

#ifdef SDN_ENABLED_NODE
  #ifdef DEMO
  printf("LED BLUE NEIGHBOR\n");
  leds_toggle(LEDS_BLUE);
  #endif

  if (queue_entry != NULL) {
    SDN_HEADER(sdn_packet)->seq_no = temp_seqno;
    queue_entry->len = packet_len;
  } else {
    ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
  }
#elif defined(SDN_CONTROLLER_NODE)
  sdn_serial_packet_t serial_pkt;

  memcpy(serial_pkt.payload, (uint8_t *)sdn_packet, packet_len);

  sdn_packetbuf_pool_put((sdn_packetbuf*)sdn_packet);

  serial_pkt.header.msg_type = SDN_SERIAL_MSG_TYPE_RADIO;
  serial_pkt.header.payload_len = packet_len;

  sdn_serial_send(&serial_pkt);
#endif

  return SDN_SUCCESS;
}

uint8_t sdn_send_register_flowid(flowid_t flowid) {
  sdn_data_flow_request_t* sdn_packet = (sdn_data_flow_request_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len = sizeof(sdn_data_flow_request_t);

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_REGISTER_FLOWID, SDN_DEFAULT_TTL)

  flowid_copy(&sdn_packet->flowid, &flowid);
  flowid_copy(&sdn_packet->dest_flowid, &sdn_controller_flow);

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);

  return SDN_SUCCESS;
}

#endif // SDN_CONTROLLER_PC

uint8_t sdn_send_data_flow_setup_ack(__attribute__((unused)) sdnaddr_t* packet_destination, __attribute__((unused)) flowid_t dest_flowid, __attribute__((unused)) sdnaddr_t* route_nexthop, __attribute__((unused)) action_t action) {

//}
//uint8_t sdn_send_ack(uint8_t seq_no, uint8_t packet_type) {

#ifdef SDN_ENABLED_NODE
  sdn_data_flow_setup_t *sdn_packet = (sdn_data_flow_setup_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len = sizeof(sdn_data_flow_setup_t) + sizeof(sdn_ack_by_flow_id_t);

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_DATA_FLOW_SETUP, SDN_DEFAULT_TTL)
  flowid_copy(&sdn_packet->flowid, &dest_flowid);
  sdnaddr_copy(&sdn_packet->destination, packet_destination);
  sdnaddr_copy(&sdn_packet->action_parameter, route_nexthop);
  sdn_packet->action_id = action;

  // SDN_DEBUG("### ACK FLOW SETUP: flowid %u action %u destination ", sdn_packet->flowid, sdn_packet->action_id);
  // sdnaddr_print(&sdn_packet->destination);
  // SDN_DEBUG("\n");

  memmove((uint8_t *)sdn_packet + sizeof(sdn_ack_by_flow_id_t), (uint8_t *)sdn_packet, sizeof(sdn_data_flow_setup_t));

  MAKE_SDN_HEADER(SDN_PACKET_ACK_BY_FLOW_ID, SDN_DEFAULT_TTL)

  sdn_ack_by_flow_id_t *sdn_packet_ack = (sdn_ack_by_flow_id_t *) sdn_packet;

  flowid_copy(&(sdn_packet_ack->dest_flowid), &sdn_controller_flow);

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
#endif

  return SDN_SUCCESS;
}

uint8_t sdn_send_control_flow_setup_ack(__attribute__((unused)) sdnaddr_t* packet_destination, __attribute__((unused)) sdnaddr_t* route_destination, __attribute__((unused)) sdnaddr_t* route_nexthop, __attribute__((unused)) action_t action) {

#ifdef SDN_ENABLED_NODE
  sdn_control_flow_setup_t *sdn_packet = (sdn_control_flow_setup_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len = sizeof(sdn_control_flow_setup_t) + sizeof(sdn_ack_by_flow_id_t);

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_CONTROL_FLOW_SETUP, SDN_DEFAULT_TTL);
  sdnaddr_copy(&sdn_packet->route_destination, route_destination);
  sdnaddr_copy(&sdn_packet->destination, packet_destination);
  sdnaddr_copy(&sdn_packet->action_parameter, route_nexthop);
  sdn_packet->action_id = action;

  memmove((uint8_t *)sdn_packet + sizeof(sdn_ack_by_flow_id_t), (uint8_t *)sdn_packet, sizeof(sdn_data_flow_setup_t));

  MAKE_SDN_HEADER(SDN_PACKET_ACK_BY_FLOW_ID, SDN_DEFAULT_TTL)

  sdn_ack_by_flow_id_t *sdn_packet_ack = (sdn_ack_by_flow_id_t *) sdn_packet;

  flowid_copy(&(sdn_packet_ack->dest_flowid), &sdn_controller_flow);

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
#endif

  return SDN_SUCCESS;
}

uint8_t sdn_send_data(uint8_t *data, uint16_t len, flowid_t flowid) {
  sdn_data_t* sdn_packet = (sdn_data_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len = sizeof(sdn_data_t) + len;

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  memset(((uint8_t *)sdn_packet), 0, SDN_MAX_PACKET_SIZE);

  MAKE_SDN_HEADER(SDN_PACKET_DATA, SDN_DEFAULT_TTL)

  flowid_copy(&sdn_packet->flowid, &flowid);

  memcpy(((uint8_t *)sdn_packet) + sizeof(sdn_data_t), data, len);

#if defined (SDN_ENABLED_NODE) && defined (MANAGEMENT)
  ENQUEUE_AND_SEND(sdn_packet, packet_len, clock_time());
#else
  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
#endif

  return SDN_SUCCESS;
}

uint8_t sdn_send_nd(struct sdn_packetbuf* sdn_nd_packet, uint16_t packet_len) {
  sdn_data_t* sdn_packet = (sdn_data_t*)sdn_nd_packet;
  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("Null packet.\n");
    return SDN_ERROR;
  }
  MAKE_SDN_HEADER(SDN_PACKET_ND, 1)
  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
  return SDN_SUCCESS;
}

uint8_t sdn_send_cd(struct sdn_packetbuf* sdn_cd_packet, uint16_t packet_len) {
  sdn_data_t* sdn_packet = (sdn_data_t*)sdn_cd_packet;
  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("Null packet.\n");
    return SDN_ERROR;
  }
  MAKE_SDN_HEADER(SDN_PACKET_CD, 1)
  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
  return SDN_SUCCESS;
}

#if defined (SDN_ENABLED_NODE) && defined (MANAGEMENT)
uint8_t sdn_send_data_management(uint16_t metrics) {

  uint8_t ret = SDN_ERROR;

  uint16_t metrics_position = 0;
  sdn_mngt_metric_t mngt_metric;
  sdn_flow_id_mngt_node_data_t* sdn_packet;

  uint16_t packet_len = sizeof(sdn_flow_id_mngt_node_data_t);

  sdn_packet = (sdn_flow_id_mngt_node_data_t *) sdn_packetbuf_pool_get();

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_MNGT_NODE_DATA, SDN_DEFAULT_TTL);
  sdn_packet->flowid = SDN_CONTROLLER_FLOW;
  sdn_packet->mngt_metrics = metrics;

#ifdef ENERGY_MANAGER
  if (metrics & SDN_MNGT_METRIC_BATTERY) {
    mngt_metric.metric_value = (uint32_t) em_get_battery();
    memcpy(((uint8_t *)sdn_packet) + packet_len + metrics_position, &mngt_metric, sizeof(sdn_mngt_metric_t));
    metrics_position += sizeof(sdn_mngt_metric_t);
  }
#endif

  if (metrics & SDN_MNGT_METRIC_QTY_DATA) {
    mngt_metric.metric_value = (uint32_t) manag_get_info(SDN_MNGT_METRIC_QTY_DATA);
    memcpy(((uint8_t *)sdn_packet) + packet_len + metrics_position, &mngt_metric, sizeof(sdn_mngt_metric_t));
    metrics_position += sizeof(sdn_mngt_metric_t);
  }

  if (metrics & SDN_MNGT_METRIC_QUEUE_DELAY) {
    mngt_metric.metric_value = (uint32_t) manag_get_info(SDN_MNGT_METRIC_QUEUE_DELAY);
    memcpy(((uint8_t *)sdn_packet) + packet_len + metrics_position, &mngt_metric, sizeof(sdn_mngt_metric_t));
    metrics_position += sizeof(sdn_mngt_metric_t);
  }

  if (metrics_position > 0) {
    ret = SDN_SUCCESS;

  #ifdef SDN_ENABLED_NODE
    ENQUEUE_AND_SEND(sdn_packet, packet_len + metrics_position, 0);
  #elif defined(SDN_CONTROLLER_NODE)
    sdn_serial_packet_t serial_pkt;

    memcpy(serial_pkt.payload, (uint8_t *)sdn_packet, packet_len + metrics_position);

    sdn_packetbuf_pool_put((sdn_packetbuf*)sdn_packet);

    serial_pkt.header.msg_type = SDN_SERIAL_MSG_TYPE_RADIO;
    serial_pkt.header.payload_len = packet_len + metrics_position;

    sdn_serial_send(&serial_pkt);
#endif
  }

  return ret;
}
#endif

#if defined(SDN_ENABLED_NODE) | defined(SDN_CONTROLLER_NODE)
uint8_t sdn_send_phase(uint16_t phase, sdnaddr_t sender) {
#ifndef SDN_CONTROLLER_NODE
  if (sdn_controlflow_get(sender)) {
    if (sdn_send_queue_find_by_type(SDN_PACKET_GENERIC_ADDR) == NULL) {
      return sdn_send_phase_addr(&sender, &sdn_node_addr, phase);
    }
  }
#endif
  if (sdn_send_queue_find_by_type(SDN_PACKET_GENERIC_FLOW) == NULL) {
    return sdn_send_phase_flow(phase, sender);
  }
  return SDN_SUCCESS;
}

uint8_t sdn_send_phase_flow(uint16_t phase, sdnaddr_t sender) {
  uint8_t ret = SDN_ERROR;

  sdn_generic_flow_t* sdn_packet;

  uint16_t packet_len = sizeof(sdn_phase_info_flow_t);

  sdn_packet = (sdn_generic_flow_t *) sdn_packetbuf_pool_get();

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_GENERIC_FLOW, SDN_DEFAULT_TTL);
  sdn_packet->flowid = SDN_CONTROLLER_FLOW;
  sdn_packet->port   = SDN_PACKET_PHASE_INFO;
  sdnaddr_copy(&((sdn_phase_info_flow_t*)sdn_packet)->phase_sender, &sender);
  ((sdn_phase_info_flow_t*)sdn_packet)->phase = phase;

#ifdef SDN_ENABLED_NODE
  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
#elif defined(SDN_CONTROLLER_NODE)
  sdn_serial_packet_t serial_pkt;

  memcpy(serial_pkt.payload, (uint8_t *)sdn_packet, packet_len);

  sdn_packetbuf_pool_put((sdn_packetbuf*)sdn_packet);

  serial_pkt.header.msg_type = SDN_SERIAL_MSG_TYPE_RADIO;
  serial_pkt.header.payload_len = packet_len;

  sdn_serial_send(&serial_pkt);
#endif

  return ret;
}
#endif //defined(SDN_ENABLED_NODE) | defined(SDN_CONTROLLER_NODE)

uint8_t sdn_send_phase_addr(sdnaddr_t* packet_destination, sdnaddr_t* phase_receiver, uint16_t phase) {
  sdn_generic_addr_t *sdn_packet = (sdn_generic_addr_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len = sizeof(sdn_phase_info_addr_t);

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_GENERIC_ADDR, SDN_DEFAULT_TTL);
  sdn_packet->port = SDN_PACKET_PHASE_INFO;
  sdnaddr_copy(&sdn_packet->destination, packet_destination);
  sdnaddr_copy(&((sdn_phase_info_addr_t*) sdn_packet)->phase_receiver, phase_receiver);
  ((sdn_phase_info_addr_t*) sdn_packet)->phase = phase;

#ifdef SDN_CONTROLLER_PC
  if (sdnaddr_cmp(packet_destination, &sdn_node_addr) == SDN_EQUAL) {
    ENQUEUE_AND_SEND_CUSTOM(sdn_packet, packet_len, 0, SDN_SERIAL_MSG_TYPE_PC_TO_NODE);
  } else
#endif
  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
}