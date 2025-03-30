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

#include "sdn-send-packet-controller.h"
#include "sdn-send-packet.h"
#include "sdn-packetbuf.h"
#include "sdn-debug.h"
#include "sdn-addr.h"
#include "sdn-constants.h"
#include "sdn-queue.h"
#include "sdn-serial-send.h"
#include "sdn-serial-packet.h"
#include "sdn-common-send.h"
#include "sdn-util-controller.h"

uint16_t add_src_rtd_header(sdn_packetbuf* sdn_packet, sdnaddr_t* packet_destination);

/**********************************************************************************************************************************/
int send_src_rtd_data_flow_setup(sdnaddr_t* packet_destination, flowid_t dest_flowid, sdnaddr_t* route_nexthop, action_t action) {

  //If this packet is for me, sends the control flow setup
  if(memcmp(&sdn_node_addr, packet_destination, SDNADDR_SIZE) == 0) {
    return sdn_send_data_flow_setup(packet_destination, dest_flowid, route_nexthop, action);
  }

  sdn_data_flow_setup_t *sdn_packet = (sdn_data_flow_setup_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len = sizeof(sdn_src_rtd_data_flow_setup_t);

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_SRC_ROUTED_DATA_FLOW_SETUP, SDN_DEFAULT_TTL);
  flowid_copy(&sdn_packet->flowid, &dest_flowid);
  sdnaddr_copy(&sdn_packet->action_parameter, route_nexthop);
  sdn_packet->action_id = action;

  packet_len = add_src_rtd_header((sdn_packetbuf*)sdn_packet, packet_destination);

  if (!packet_len) {
    SDN_DEBUG_ERROR("Failed to append source route header\n");
    return SDN_ERROR;
  }

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
}
/**********************************************************************************************************************************/
int send_src_rtd_control_flow_setup(sdnaddr_t* packet_destination, sdnaddr_t* route_destination, sdnaddr_t* route_nexthop, action_t action) {

  //If this packet is for me, sends the control flow setup
  if(memcmp(&sdn_node_addr, packet_destination, SDNADDR_SIZE) == 0) {
    return sdn_send_control_flow_setup(packet_destination, route_destination, route_nexthop, action);
  }

   sdn_control_flow_setup_t *sdn_packet = (sdn_control_flow_setup_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len;

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP, SDN_DEFAULT_TTL);
  sdnaddr_copy(&sdn_packet->route_destination, route_destination);
  sdnaddr_copy(&sdn_packet->action_parameter, route_nexthop);
  sdn_packet->action_id = action;

  packet_len = add_src_rtd_header((sdn_packetbuf*)sdn_packet, packet_destination);

  if (!packet_len) {
    SDN_DEBUG_ERROR("Failed to append source route header\n");
    return SDN_ERROR;
  }

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
}
/**********************************************************************************************************************************/
uint8_t sdn_send_src_rtd_phase_addr(sdnaddr_t* packet_destination, sdnaddr_t* phase_receiver, uint16_t phase) {

  if (sdnaddr_cmp(packet_destination, &sdn_node_addr) == SDN_EQUAL) {
    sdn_send_phase_addr(packet_destination, phase_receiver, phase);
  }

  sdn_generic_addr_t *sdn_packet = (sdn_generic_addr_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len;

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_GENERIC_SRC_ROUTED, SDN_DEFAULT_TTL);
  sdn_packet->port = SDN_PACKET_PHASE_INFO;
  sdnaddr_copy(&((sdn_phase_info_src_rtd_t*) sdn_packet)->phase_receiver, phase_receiver);
  SDN_PACKET_GET_FIELD(sdn_packet, sdn_phase_info_src_rtd_t, phase) = phase;

  packet_len = add_src_rtd_header((sdn_packetbuf*)sdn_packet, packet_destination);

  if (!packet_len) {
    SDN_DEBUG_ERROR("Failed to append source route header\n");
    return SDN_ERROR;
  }

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);

  return SDN_SUCCESS;
}
/**********************************************************************************************************************************/
#if SDN_TX_RELIABILITY
int send_ack_by_flow_address(sdnaddr_t* packet_destination, uint8_t acked_packed_type, uint8_t acked_packed_seqno) {

  if(sdnaddr_cmp(&sdn_node_addr, packet_destination) == SDN_EQUAL) {
//    SDN_DEBUG("Do not send ack for myself\n");
    return SDN_SUCCESS;
  }
  sdn_ack_by_flow_address_t *sdn_packet = (sdn_ack_by_flow_address_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len = sizeof(sdn_ack_by_flow_address_t);

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_ACK_BY_FLOW_ADDRESS, SDN_DEFAULT_TTL);

  sdn_packet->acked_packed_type = acked_packed_type;
  sdn_packet->acked_packed_seqno = acked_packed_seqno;

  sdnaddr_copy( &((sdn_ack_by_flow_address_t*)sdn_packet)->destination, packet_destination);

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
}
/**********************************************************************************************************************************/
int send_src_rtd_ack(sdnaddr_t* packet_destination, uint8_t acked_packed_type, uint8_t acked_packed_seqno) {
  sdn_src_rtd_ack_t *sdn_packet = (sdn_src_rtd_ack_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len;

  if(sdnaddr_cmp(&sdn_node_addr, packet_destination) == SDN_EQUAL) {
//    SDN_DEBUG("Do not send ack for myself\n");
    return SDN_SUCCESS;
  }

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return SDN_ERROR;
  }

  MAKE_SDN_HEADER(SDN_PACKET_SRC_ROUTED_ACK, SDN_DEFAULT_TTL);
  sdn_packet->acked_packed_type = acked_packed_type;
  sdn_packet->acked_packed_seqno = acked_packed_seqno;

  packet_len = add_src_rtd_header((sdn_packetbuf*)sdn_packet, packet_destination);

  if (!packet_len) {
    SDN_DEBUG_ERROR("Failed to append source route header\n");
    return SDN_ERROR;
  }

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);

  return SDN_SUCCESS;
}
#endif //SDN_TX_RELIABILITY
/**********************************************************************************************************************************/
#ifdef MANAGEMENT
int send_src_rtd_cont_mngt(sdnaddr_t* packet_destination, uint16_t metrics) {

  if(memcmp(&sdn_node_addr, packet_destination, SDNADDR_SIZE) == 0) {
    return SDN_ERROR;
  }

  sdn_src_rtd_cont_mngt_t *sdn_packet = (sdn_src_rtd_cont_mngt_t *) sdn_packetbuf_pool_get();
  uint16_t packet_len = sizeof(sdn_src_rtd_cont_mngt_t);


  MAKE_SDN_HEADER(SDN_PACKET_MNGT_CONT_SRC_RTD, SDN_DEFAULT_TTL);
  sdn_packet->mngt_request = metrics;

  packet_len = add_src_rtd_header((sdn_packetbuf*)sdn_packet, packet_destination);

  if (!packet_len) {
    SDN_DEBUG_ERROR("Failed to append source route header\n");
    return SDN_ERROR;
  }

  ENQUEUE_AND_SEND(sdn_packet, packet_len, 0);
  return SDN_SUCCESS;
}
#endif
/**********************************************************************************************************************************/
uint16_t add_src_rtd_header(sdn_packetbuf* sdn_packet, sdnaddr_t* packet_destination) {
  //Source Routed data flow setup packet
  route_ptr route = NULL;
  uint16_t packet_len = 0;
  uint8_t* path_len = NULL;
  sdnaddr_t* real_dest = NULL;
#if RDC_UNIDIR_SUPPORT
  uint64_t* unidir_list = NULL;
#endif

  printf("appending source route header\n");

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("NULL packet pointer.\n");
    return 0;
  }

  switch (SDN_HEADER(sdn_packet)->type) {

    case SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP:
      packet_len  =                            sizeof(sdn_src_rtd_control_flow_setup_t);
      path_len    = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_src_rtd_control_flow_setup_t, path_len);
      real_dest   = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_src_rtd_control_flow_setup_t, real_destination);
      #if RDC_UNIDIR_SUPPORT
      unidir_list = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_src_rtd_control_flow_setup_t, unidir_list);
      #endif
      break;

    case SDN_PACKET_SRC_ROUTED_DATA_FLOW_SETUP:
      packet_len  =                            sizeof(sdn_src_rtd_data_flow_setup_t);
      path_len    = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_src_rtd_data_flow_setup_t, path_len);
      real_dest   = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_src_rtd_data_flow_setup_t, real_destination);
      #if RDC_UNIDIR_SUPPORT
      unidir_list = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_src_rtd_data_flow_setup_t, unidir_list);
      #endif
      break;

    case SDN_PACKET_SRC_ROUTED_ACK:
      packet_len  =                            sizeof(sdn_src_rtd_ack_t);
      path_len    = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_src_rtd_ack_t, path_len);
      real_dest   = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_src_rtd_ack_t, real_destination);
      #if RDC_UNIDIR_SUPPORT
      unidir_list = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_src_rtd_ack_t, unidir_list);
      #endif
      break;

    case SDN_PACKET_GENERIC_SRC_ROUTED:
      if (SDN_PACKET_GET_FIELD(sdn_packet, sdn_generic_addr_t, port) == SDN_PACKET_PHASE_INFO) {
        packet_len  =                            sizeof(sdn_phase_info_src_rtd_t);
      } else {
        SDN_DEBUG_ERROR("Unknown generic port\n");
        return 0;
      }
      path_len    = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_generic_src_rtd_t, path_len);
      real_dest   = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_generic_src_rtd_t, real_destination);
      #if RDC_UNIDIR_SUPPORT
      unidir_list = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_generic_src_rtd_t, unidir_list);
      #endif
      break;

    case SDN_PACKET_MNGT_CONT_SRC_RTD:
      packet_len  =                            sizeof(sdn_src_rtd_cont_mngt_t);
      path_len    = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_src_rtd_cont_mngt_t, path_len);
      real_dest   = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_src_rtd_cont_mngt_t, real_destination);
      #if RDC_UNIDIR_SUPPORT
      unidir_list = &SDN_PACKET_GET_FIELD(sdn_packet, sdn_src_rtd_cont_mngt_t, unidir_list);
      #endif
      break;

    default:
      if (SDN_ROUTED_BY_SRC(sdn_packet)) {
        SDN_DEBUG_ERROR("Source routed packet not in switch\n");
      } else {
        SDN_DEBUG_ERROR("Packet type not source routed\n");
      }
      return 0;
  }

  if (! (path_len ||
#if RDC_UNIDIR_SUPPORT
          unidir_list ||
#endif
          real_dest) ) {
    SDN_DEBUG_ERROR("NULL pointers\n");
    return 0;
  }

  route = get_flow_address_better_route(sdn_node_addr.u8, packet_destination->u8);

  if(route == NULL) {
    printf("ERROR: route not found to send packet type %02X to address: ", SDN_HEADER(sdn_packet)->type);
    sdnaddr_print(packet_destination);
    printf("\n");

    //sdn_packetbuf_pool_put((struct sdn_packetbuf *)sdn_packet);
    return 0;
  }


  route_ptr route_head = route;
  int count_hops = 0;

  while (route != NULL) {
    digraph_print_vertice_id(route->vertice_id);
    printf("(w %d h %d u %d) <- ", route->route_weight, route->hops, route->unidir);
    count_hops++;
    route = route->next;
  }
  printf("\n");
  route = route_head;

  sdnaddr_copy(real_dest, packet_destination);

  int index_hops = count_hops - 2;
  int max_hops = (SDN_MAX_PACKET_SIZE - packet_len) / sizeof(sdnaddr_t);

  for(;index_hops > max_hops; index_hops--) {
    route = route->next;
  }

  *path_len = index_hops;
#if RDC_UNIDIR_SUPPORT
  *unidir_list = 0;
#endif

  for(;index_hops > 0; index_hops--) {
    sdnaddr_copy( (sdnaddr_t*) (((uint8_t*)sdn_packet) + packet_len), (sdnaddr_t *)route->vertice_id);
    packet_len += sizeof(sdnaddr_t);
#if RDC_UNIDIR_SUPPORT
    *unidir_list = ((*unidir_list<< 1) | (route->unidir & 1));
#endif

    route = route->next;
  }
#if RDC_UNIDIR_SUPPORT
  *unidir_list = ((*unidir_list<< 1) | (route->unidir & 1));
#endif

  // assuming every packet is composed of HEADER followed by DESTINATION
  sdnaddr_copy(&SDN_PACKET_GET_FIELD(sdn_packet, sdn_generic_addr_t, destination), (sdnaddr_t *)route->vertice_id);

  route_head = NULL;
  dijkstra_free_route(route);
  return packet_len;
}