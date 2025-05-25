#include <inttypes.h>
#include "sdn-protocol.h"
#include "sdn-debug.h"

// funcoes que acabam sendo chamadas mas nao sao importantes para os testes (pelo menos por enquanto)

void sdn_send_done(int status) {
	SDN_DEBUG("\t mock %s\n");
}

void sdn_send_down_once() {
	SDN_DEBUG("\t mock %s\n");
}

void process_energy_metric(sdnaddr_t *source, uint32_t battery) {
	SDN_DEBUG("\t mock %s\n");
}

void process_neighbor_report(sdn_neighbor_report_t* sdn_neighbor_report, void* neighbors) {
	SDN_DEBUG("\t mock %s\n");
}

void process_data_flow_request_packet(sdn_data_flow_request_t *sdn_data_flow_request) {
	SDN_DEBUG("\t mock %s\n");
}

void process_control_flow_request(sdn_control_flow_request_t *sdn_control_flow_request) {
	SDN_DEBUG("\t mock %s\n");
}

void process_register_flowid(uint16_t flowid, unsigned char* target) {
	SDN_DEBUG("\t mock %s\n");
}

void process_phase_info_controller(sdn_phase_info_flow_t *) {
	SDN_DEBUG("\t mock %s\n");
}

void sdn_flow_id_table_set_online(unsigned char* source, uint16_t flowid, unsigned char* next_hop, action_t action) {
	SDN_DEBUG("\t mock %s\n");
}

void sdn_flow_address_table_set_online(unsigned char* source, unsigned char* target, unsigned char* next_hop, action_t action) {
	SDN_DEBUG("\t mock %s\n");
}