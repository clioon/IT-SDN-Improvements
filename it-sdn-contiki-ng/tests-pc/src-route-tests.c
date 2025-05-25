#include "sdn-send-packet.h"
#include "sdn-packetbuf.h"
#include "sdn-debug.h"
#include "sdn-constants.h"
#include "sdn-queue.h"
#include "string.h"
#include "sdn-process-packets.h"

#ifndef SDN_CONTROLLER_PC
#include "sdn-send.h"
#else // SDN_CONTROLLER_PC
#include "sdn-serial-send.h"
#endif // SDN_CONTROLLER_PC

void test_send_src_rt_csf() {
  SDN_DEBUG("%s\n", __func__);
  sdn_send_queue_init();
  sdn_recv_queue_init();
  sdn_controlflow_init();

  sdn_control_flow_setup_t *sdn_packet = (sdn_control_flow_setup_t *) sdn_packetbuf_pool_get();
  sdnaddr_t temp_addr = {{2,0}};
  sdnaddr_t *addr_ptr;
  uint16_t packet_len = sizeof(sdn_src_rtd_control_flow_setup_t);
  action_t action;
  struct control_flow_entry *cfe;

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return;
  }

  // packet: next hop = 2; remaining route = 3, 4
  MAKE_SDN_HEADER(SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP, SDN_DEFAULT_TTL);
  sdnaddr_copy(&sdn_packet->destination, &temp_addr);
  temp_addr.u8[0] = 99;
  sdnaddr_copy(&sdn_packet->route_destination, &temp_addr);
  sdnaddr_copy(&sdn_packet->action_parameter, &temp_addr);
  sdn_packet->action_id = SDN_ACTION_FORWARD;
  ((sdn_src_rtd_control_flow_setup_t*)sdn_packet)->path_len = 2;
  temp_addr.u8[0] = 4;
  sdnaddr_copy( (sdnaddr_t*) (((uint8_t*)sdn_packet) + sizeof(sdn_src_rtd_control_flow_setup_t)), &temp_addr);
  sdnaddr_copy( &((sdn_src_rtd_control_flow_setup_t*)sdn_packet)->real_destination, &temp_addr);
  temp_addr.u8[0] = 3;
  sdnaddr_copy( (sdnaddr_t*) (((uint8_t*)sdn_packet) + sizeof(sdn_src_rtd_control_flow_setup_t) + sizeof(sdnaddr_t)), &temp_addr);
  packet_len += sizeof(sdnaddr_t)*2;

  // Lets pretend we are node 2
  temp_addr.u8[0] = 2;
  sdnaddr_copy(&sdn_node_addr, &temp_addr);

  // first call to sdn_treat_packet should adjust the next hop
  addr_ptr = sdn_treat_packet((uint8_t *) sdn_packet, packet_len, 0, &action);
  if (action != SDN_ACTION_RECEIVE || addr_ptr != NULL) {
    printf("\t\t >>>> ERROR (line %d) <<<<\n", __LINE__);
  }
  // second call to sdn_treat_packet should forward to the next hop
  addr_ptr = sdn_treat_packet((uint8_t *) sdn_packet, packet_len, 0, &action);
  temp_addr.u8[0] = 3;
  if (action != SDN_ACTION_FORWARD || sdnaddr_cmp(addr_ptr, &temp_addr) != SDN_EQUAL) {
    printf("\t\t >>>> ERROR (line %d) <<<<\n", __LINE__);
  }

  // Lets pretend we are node 3
  temp_addr.u8[0] = 3;
  sdnaddr_copy(&sdn_node_addr, &temp_addr);

  // first call to sdn_treat_packet should adjust the next hop
  addr_ptr = sdn_treat_packet((uint8_t *) sdn_packet, packet_len, 0, &action);
  if (action != SDN_ACTION_RECEIVE || addr_ptr != NULL) {
    printf("\t\t >>>> ERROR (line %d) <<<<\n", __LINE__);
  }

  // second call to sdn_treat_packet should forward to the next hop
  addr_ptr = sdn_treat_packet((uint8_t *) sdn_packet, packet_len, 0, &action);
  temp_addr.u8[0] = 4;
  if (action != SDN_ACTION_FORWARD || sdnaddr_cmp(addr_ptr, &temp_addr) != SDN_EQUAL) {
    printf("\t\t >>>> ERROR (line %d) <<<<\n", __LINE__);
  }

  // Lets pretend we are node 4
  temp_addr.u8[0] = 4;
  sdnaddr_copy(&sdn_node_addr, &temp_addr);

  // our address equals the final destination, packet should be received and processed
  addr_ptr = sdn_treat_packet((uint8_t *) sdn_packet, packet_len, 0, &action);
  if (action != SDN_ACTION_RECEIVE || addr_ptr != NULL) {
    printf("\t\t >>>> ERROR (line %d) <<<<\n", __LINE__);
  }

  // New rule should be set properly
  temp_addr.u8[0] = 99;
  cfe = sdn_controlflow_get(temp_addr);
  if (cfe != NULL && cfe->action == SDN_ACTION_FORWARD && sdnaddr_cmp(&cfe->next_hop, &temp_addr) == SDN_EQUAL) {
    printf("\t...get OK\n");
  } else {
    printf("    get __NOT__ OK\n");
  }
}

void test_send_src_rt_csf_incomplete() {
  SDN_DEBUG("%s\n", __func__);
  sdn_send_queue_init();
  sdn_recv_queue_init();
  sdn_controlflow_init();

  sdn_control_flow_setup_t *sdn_packet = (sdn_control_flow_setup_t *) sdn_packetbuf_pool_get();
  sdnaddr_t temp_addr = {{2,0}};
  uint16_t packet_len = sizeof(sdn_src_rtd_control_flow_setup_t);
  sdnaddr_t *addr_ptr;
  action_t action;
  struct control_flow_entry *cfe;

  if (sdn_packet == NULL) {
    SDN_DEBUG_ERROR ("SDN packetbuf pool is empty.\n");
    return;
  }

  // packet: next hop = 2; remaining route = 3; destination is 4 (source route path too short)
  MAKE_SDN_HEADER(SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP, SDN_DEFAULT_TTL);
  sdnaddr_copy(&sdn_packet->destination, &temp_addr);
  temp_addr.u8[0] = 99;
  sdnaddr_copy(&sdn_packet->route_destination, &temp_addr);
  sdnaddr_copy(&sdn_packet->action_parameter, &temp_addr);
  sdn_packet->action_id = SDN_ACTION_FORWARD;
  ((sdn_src_rtd_control_flow_setup_t*)sdn_packet)->path_len = 1;
  temp_addr.u8[0] = 4;
  sdnaddr_copy( &((sdn_src_rtd_control_flow_setup_t*)sdn_packet)->real_destination, &temp_addr);
  temp_addr.u8[0] = 3;
  sdnaddr_copy( (sdnaddr_t*) (((uint8_t*)sdn_packet) + sizeof(sdn_src_rtd_control_flow_setup_t)), &temp_addr);
  packet_len += sizeof(sdnaddr_t)*2;

  // Lets pretend we are node 2
  temp_addr.u8[0] = 2;
  sdnaddr_copy(&sdn_node_addr, &temp_addr);

  // first call to sdn_treat_packet should adjust the next hop
  addr_ptr = sdn_treat_packet((uint8_t *) sdn_packet, packet_len, 0, &action);
  if (action != SDN_ACTION_RECEIVE || addr_ptr != NULL) {
    printf("\t\t >>>> ERROR (line %d) <<<<\n", __LINE__);
  }
  // second call to sdn_treat_packet should forward to the next hop
  addr_ptr = sdn_treat_packet((uint8_t *) sdn_packet, packet_len, 0, &action);
  temp_addr.u8[0] = 3;
  if (action != SDN_ACTION_FORWARD || sdnaddr_cmp(addr_ptr, &temp_addr) != SDN_EQUAL) {
    printf("\t\t >>>> ERROR (line %d) <<<<\n", __LINE__);
  }

  // Lets pretend we are node 3, the source routed header will end here
  temp_addr.u8[0] = 3;
  sdnaddr_copy(&sdn_node_addr, &temp_addr);

  // Lets add a cfe towards node 4
  temp_addr.u8[0] = 4;
  sdn_controlflow_insert(temp_addr, temp_addr, SDN_ACTION_FORWARD);

  // first call to sdn_treat_packet should transform it into non source routed and re-added it to the queue
  addr_ptr = sdn_treat_packet((uint8_t *) sdn_packet, packet_len, 0, &action);
  if (action != SDN_ACTION_RECEIVE || addr_ptr != NULL) {
    printf("\t\t >>>> ERROR (line %d) <<<<\n", __LINE__);
  }

  // second call to sdn_treat_packet should forward to the next hop according to flow table rule
  addr_ptr = sdn_treat_packet((uint8_t *) sdn_packet, packet_len, 0, &action);
  print_packet_header(sdn_packet, sizeof(sdn_header_t));
  if (action != SDN_ACTION_FORWARD || sdnaddr_cmp(addr_ptr, &temp_addr) != SDN_EQUAL) {
    printf("\t\t >>>> ERROR (line %d) <<<<\n", __LINE__);
  }

  // Lets pretend we are node 4
  temp_addr.u8[0] = 4;
  sdnaddr_copy(&sdn_node_addr, &temp_addr);
  sdn_controlflow_insert(temp_addr, temp_addr, SDN_ACTION_RECEIVE);

  // the SDN_ACTION_RECEIVE above should be working
  addr_ptr = sdn_treat_packet((uint8_t *) sdn_packet, packet_len, 0, &action);
  if (action != SDN_ACTION_RECEIVE || addr_ptr != NULL) {
    printf("\t\t >>>> ERRO <<<< %d %p\n", action, addr_ptr);
  }

  // New rule should be set
  temp_addr.u8[0] = 99;
  cfe = sdn_controlflow_get(temp_addr);
  if (cfe != NULL && cfe->action == SDN_ACTION_FORWARD && sdnaddr_cmp(&cfe->next_hop, &temp_addr) == SDN_EQUAL) {
    printf("\t...get OK\n");
  } else {
    printf("    get __NOT__ OK\n");
  }
}


int main() {
  test_send_src_rt_csf();
  test_send_src_rt_csf_incomplete();

  // test_send_src_rt_dsf();
  // test_send_src_rt_dsf_incomplete();

  // test_send_mult_csf();
  // test_send_mult_dsf();

  return 0;
}