#include <stdio.h>
#include <stdlib.h>
#include "sdn-addr.h"
#include "sdn-flow-table-cache.h"
#include "sdn-graph.h"

void print_addr(sdnaddr_t* addr) {
  uint8_t *charaddr = (uint8_t*)addr;
  uint8_t i;

  printf("[");

  for (i = 0; i < sizeof(sdnaddr_t); i++) {
    printf(" %02X", charaddr[i]);
  }

  printf(" ]");
}

void print_path_list(sdn_graph_path_list_ptr head) {
    int path_num = 1;
    sdn_graph_path_list_ptr current = head;
    
    if (current == NULL) {
        printf("Nenhum caminho encontrado.\n");
        return;
    }

    while (current != NULL) {
        printf("Caminho %d (len=%d): ", path_num++, current->path.len);
        for (int i = 0; i < current->path.len; i++) {
            print_addr(&current->path.nodes[i]);
            if (i < current->path.len - 1) {
                printf(" -> ");
            }
        }
        printf("\n");
        current = current->next;
    }
}


int main() {
    printf("============ sdn_process_table_graph ============\n");

    sdn_graph_path_list_ptr result_head = sdn_process_table_graph(1);

    printf("\n--- Resultados ---\n");
    print_path_list(result_head);

    sdn_free_graph();
    printf("\nsdn_free_graph()\n");

    return 0;
}
