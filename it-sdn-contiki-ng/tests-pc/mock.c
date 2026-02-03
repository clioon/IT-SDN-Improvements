#include <inttypes.h>
#include <string.h>
#include "sdn-protocol.h"
#include "sdn-debug.h"
#include "sdn-flow-table-cache.h"
#define MAX_MOCK_ENTRIES 100
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

static struct flow_id_table mock_list[MAX_MOCK_ENTRIES];
static int mock_list_initialized = 0;


// ========================================================

// void init_mock_data() {
//     if (mock_list_initialized) return;

//     memset(mock_list, 0, sizeof(mock_list));

//     // Caminho 1
//     mock_list[0] = (struct flow_id_table){
//         .source = {1,1},
//         .flowid = 1,
//         .next_hop = {1,2},
//         .changed = 1
//     };
//     mock_list[1] = (struct flow_id_table){
//         .source = {1,2},
//         .flowid = 1,
//         .next_hop = {1,3},
//         .changed = 1
//     };
//     mock_list[2] = (struct flow_id_table){
//         .source = {1,3},
//         .flowid = 1,
//         .next_hop = {1,4},
//         .changed = 1
//     };
//     mock_list[3] = (struct flow_id_table){
//         .source = {1,4},
//         .flowid = 1,
//         .next_hop = {1,5},
//         .changed = 1
//     };
//     mock_list[4] = (struct flow_id_table){
//         .source = {1,5},
//         .flowid = 1,
//         .next_hop = {1,6},
//         .changed = 1
//     };
//     mock_list[5] = (struct flow_id_table){
//         .source = {1,6},
//         .flowid = 1,
//         .next_hop = {0,0},
//         .changed = 1
//     };

//     // Caminho 2
//     mock_list[6] = (struct flow_id_table){
//         .source = {2,1},
//         .flowid = 1,
//         .next_hop = {2,2},
//         .changed = 1 // W->X
//     };
//     mock_list[7] = (struct flow_id_table){
//         .source = {2,2},
//         .flowid = 1,
//         .next_hop = {2,3},
//         .changed = 1 // X->Y
//     };
//     mock_list[8] = (struct flow_id_table){
//         .source = {2,3},
//         .flowid = 1,
//         .next_hop = {1,4},
//         .changed = 1
//     };

//     // Caminho 3
//     mock_list[9] = (struct flow_id_table){
//         .source = {3,1},
//         .flowid = 1,
//         .next_hop = {1,3},
//         .changed = 1
//     };

//     // sink
//     mock_list[10] = (struct flow_id_table){
//         .source = {0,0},
//         .flowid = 1,
//         .next_hop = {0},
//         .changed = 1
//     };

//     int num_links_definidos = 11;

//     for (int i = 0; i < num_links_definidos; i++) {
//         if (i < (num_links_definidos - 1)) {
//             mock_list[i].next = &mock_list[i+1];
//         } else {
//             mock_list[i].next = NULL;
//         }
//     }

//     mock_list_initialized = 1;
// }

// ========================================================

// void init_mock_data() {
//     if (mock_list_initialized) return;

//     memset(mock_list, 0, sizeof(mock_list));
//     int idx = 0;


//     // --- Caminho 1 (10 nós) -> Junta em {2,1} ---
//     mock_list[idx++] = (struct flow_id_table){ .source = {10,1}, .flowid = 1, .next_hop = {10,2}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {10,2}, .flowid = 1, .next_hop = {10,3}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {10,3}, .flowid = 1, .next_hop = {10,4}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {10,4}, .flowid = 1, .next_hop = {10,5}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {10,5}, .flowid = 1, .next_hop = {10,6}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {10,6}, .flowid = 1, .next_hop = {10,7}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {10,7}, .flowid = 1, .next_hop = {10,8}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {10,8}, .flowid = 1, .next_hop = {10,9}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {10,9}, .flowid = 1, .next_hop = {10,10}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {10,10}, .flowid = 1, .next_hop = {2,1}, .changed = 1 };

//     // --- Caminho 2 (10 nós) -> Junta em {2,1} ---
//     mock_list[idx++] = (struct flow_id_table){ .source = {20,1}, .flowid = 1, .next_hop = {20,2}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {20,2}, .flowid = 1, .next_hop = {20,3}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {20,3}, .flowid = 1, .next_hop = {20,4}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {20,4}, .flowid = 1, .next_hop = {20,5}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {20,5}, .flowid = 1, .next_hop = {20,6}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {20,6}, .flowid = 1, .next_hop = {20,7}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {20,7}, .flowid = 1, .next_hop = {20,8}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {20,8}, .flowid = 1, .next_hop = {20,9}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {20,9}, .flowid = 1, .next_hop = {20,10}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {20,10}, .flowid = 1, .next_hop = {2,1}, .changed = 1 };

//     // --- Caminho 3 (10 nós) -> Junta em {2,2} ---
//     mock_list[idx++] = (struct flow_id_table){ .source = {30,1}, .flowid = 1, .next_hop = {30,2}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {30,2}, .flowid = 1, .next_hop = {30,3}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {30,3}, .flowid = 1, .next_hop = {30,4}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {30,4}, .flowid = 1, .next_hop = {30,5}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {30,5}, .flowid = 1, .next_hop = {30,6}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {30,6}, .flowid = 1, .next_hop = {30,7}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {30,7}, .flowid = 1, .next_hop = {30,8}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {30,8}, .flowid = 1, .next_hop = {30,9}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {30,9}, .flowid = 1, .next_hop = {30,10}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {30,10}, .flowid = 1, .next_hop = {2,2}, .changed = 1 };

//     // --- Caminho 4 (10 nós) -> Junta em {2,3} ---
//     mock_list[idx++] = (struct flow_id_table){ .source = {40,1}, .flowid = 1, .next_hop = {40,2}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {40,2}, .flowid = 1, .next_hop = {40,3}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {40,3}, .flowid = 1, .next_hop = {40,4}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {40,4}, .flowid = 1, .next_hop = {40,5}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {40,5}, .flowid = 1, .next_hop = {40,6}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {40,6}, .flowid = 1, .next_hop = {40,7}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {40,7}, .flowid = 1, .next_hop = {40,8}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {40,8}, .flowid = 1, .next_hop = {40,9}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {40,9}, .flowid = 1, .next_hop = {40,10}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {40,10}, .flowid = 1, .next_hop = {2,3}, .changed = 1 };

//     // --- Caminho 5 (10 nós) -> Junta em {2,4} ---
//     mock_list[idx++] = (struct flow_id_table){ .source = {50,1}, .flowid = 1, .next_hop = {50,2}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {50,2}, .flowid = 1, .next_hop = {50,3}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {50,3}, .flowid = 1, .next_hop = {50,4}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {50,4}, .flowid = 1, .next_hop = {50,5}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {50,5}, .flowid = 1, .next_hop = {50,6}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {50,6}, .flowid = 1, .next_hop = {50,7}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {50,7}, .flowid = 1, .next_hop = {50,8}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {50,8}, .flowid = 1, .next_hop = {50,9}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {50,9}, .flowid = 1, .next_hop = {50,10}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {50,10}, .flowid = 1, .next_hop = {2,4}, .changed = 1 };

//     // --- Nível 2: Junções {2,x} -> Junções {1,x} ---
//     mock_list[idx++] = (struct flow_id_table){ .source = {2,1}, .flowid = 1, .next_hop = {1,1}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {2,2}, .flowid = 1, .next_hop = {1,1}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {2,3}, .flowid = 1, .next_hop = {1,2}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {2,4}, .flowid = 1, .next_hop = {1,2}, .changed = 1 };

//     // --- Nível 1: Junções {1,x} -> Sink {0,0} ---
//     mock_list[idx++] = (struct flow_id_table){ .source = {1,1}, .flowid = 1, .next_hop = {0,0}, .changed = 1 };
//     mock_list[idx++] = (struct flow_id_table){ .source = {1,2}, .flowid = 1, .next_hop = {0,0}, .changed = 1 };

//     // --- Sink (Entrada "Dummy" para garantir que table_entry não seja NULL) ---
//     mock_list[idx++] = (struct flow_id_table){ .source = {0,0}, .flowid = 1, .next_hop = {0,0}, .changed = 1 };


//     int num_links_definidos = idx;

//     for (int i = 0; i < num_links_definidos; i++) {
//         if (i < (num_links_definidos - 1)) {
//             mock_list[i].next = &mock_list[i+1];
//         } else {
//             mock_list[i].next = NULL;
//         }
//     }

//     mock_list_initialized = 1;
// }

// ==============================================

void init_mock_data() {
    if (mock_list_initialized) return;

    memset(mock_list, 0, sizeof(mock_list));

    // Caminho 1
    mock_list[0] = (struct flow_id_table){
        .source = {1,1},
        .flowid = 1,
        .next_hop = {1,2},
        .changed = 1
    };
    mock_list[1] = (struct flow_id_table){
        .source = {1,2},
        .flowid = 1,
        .next_hop = {1,3},
        .changed = 1
    };
    mock_list[2] = (struct flow_id_table){
        .source = {1,3},
        .flowid = 1,
        .next_hop = {1,4},
        .changed = 1
    };
    mock_list[3] = (struct flow_id_table){
        .source = {1,4},
        .flowid = 1,
        .next_hop = {1,5},
        .changed = 1
    };
    mock_list[4] = (struct flow_id_table){
        .source = {1,5},
        .flowid = 1,
        .next_hop = {1,6},
        .changed = 1
    };
    mock_list[5] = (struct flow_id_table){
        .source = {1,6},
        .flowid = 1,
        .next_hop = {1,7},
        .changed = 1
    };

    mock_list[6] = (struct flow_id_table){
        .source = {1,7},
        .flowid = 1,
        .next_hop = {1,8},
        .changed = 1 // W->X
    };
    mock_list[7] = (struct flow_id_table){
        .source = {1,8},
        .flowid = 1,
        .next_hop = {1,9},
        .changed = 1 // X->Y
    };
    mock_list[8] = (struct flow_id_table){
        .source = {1,9},
        .flowid = 1,
        .next_hop = {1,10},
        .changed = 1
    };

    mock_list[9] = (struct flow_id_table){
        .source = {1,10},
        .flowid = 1,
        .next_hop = {0,0},
        .changed = 1
    };

    // sink
    mock_list[10] = (struct flow_id_table){
        .source = {0,0},
        .flowid = 1,
        .next_hop = {0},
        .changed = 1
    };

    int num_links_definidos = 11;

    for (int i = 0; i < num_links_definidos; i++) {
        if (i < (num_links_definidos - 1)) {
            mock_list[i].next = &mock_list[i+1];
        } else {
            mock_list[i].next = NULL;
        }
    }

    mock_list_initialized = 1;
}

flow_id_table_ptr sdn_flow_id_table_get(void) {
    init_mock_data();
    return &mock_list[0];
}
