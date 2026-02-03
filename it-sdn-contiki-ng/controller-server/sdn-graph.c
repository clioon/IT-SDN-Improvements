#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdn-debug.h"
#include "sdn-graph.h"
#include "sdn-protocol.h"
// #include "sdn-packetbuf.h"
#include "digraph.h"
#include "sdn-flow-table-cache.h"
#include "sdn-send-packet-controller.h"
#include "sdn-send-packet.h"
#include "sdn-serial-send.h"

// ==========================================

#define HASH_TABLE_SIZE 101
#define SEGMENT_HASH_TABLE_SIZE 101
#define MAX_CHANGED_LIFES 3
static sdn_graph_node_ptr node_hash_table[HASH_TABLE_SIZE];
static sdn_graph_path_list_ptr final_paths_head = NULL;
static sdn_graph_segment_ptr segment_hash_table[SEGMENT_HASH_TABLE_SIZE];

// ==========================================
//                  DEBUG
// ==========================================
// static void print_graph_state(void) {
//   printf("\n--- graph build ---\n");
//   int total_nodes = 0;

//   for (int i = 0; i < HASH_TABLE_SIZE; i++) {
//     sdn_graph_node_ptr current_node = node_hash_table[i];

//     while (current_node != NULL) {
//       total_nodes++;
//       printf("\nNode: ");
//       sdnaddr_print(&current_node->id);
//       int changed = 0;
//       if (current_node->table_entry != NULL) changed = current_node->table_entry->changed;
//       printf(" (in_degree: %d, changed: %d)", current_node->in_degree, changed);
      
//       if (current_node->table_entry != NULL) {
//           printf(" [Table OK]");
//       } else {
//           printf(" [Table NULL]");
//       }

//       sdn_graph_neighbor_ptr neighbor = current_node->neighbors_head;
//       if (neighbor == NULL) {
//         printf("\n  -> (no neighbor)\n");
//       } else {
//         printf("\n  -> Neighbors:\n");
//         while (neighbor != NULL) {
//           printf("    -> ");
//           if (neighbor->node_ptr != NULL) {
//             sdnaddr_print(&neighbor->node_ptr->id);
//           } else {
//             printf("[ERROR: NULL neighbor pointer]");
//           }
//           printf("\n");
//           neighbor = neighbor->next;
//         }
//       }
//       current_node = current_node->next_in_bucket;
//     }
//   }
//   printf("\n--- Total nodes: %d ---\n", total_nodes);
// }


void print_path_list(sdn_graph_path_list_ptr head) {
    int path_num = 1;
    sdn_graph_path_list_ptr current = head;
    
    if (current == NULL) {
        printf("No path found.\n");
        return;
    }

    while (current != NULL) {
        printf("Path %d (len=%d): ", path_num++, current->path.len);
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

void print_addr(sdnaddr_t* addr) {
  uint8_t *charaddr = (uint8_t*)addr;
  uint8_t i;

  printf("[");

  for (i = 0; i < sizeof(sdnaddr_t); i++) {
    printf(" %02X", charaddr[i]);
  }

  printf(" ]");
}

// ==========================================
//               HASH TABLES
// ==========================================

static unsigned int sdn_hash_address(unsigned char *addr_array) {
  unsigned int hash = 5381;
  for (int i = 0; i < SDNADDR_SIZE; i++) {
    hash = ((hash << 5) + hash) + addr_array[i];  // djb2 hash
  }
  return hash % HASH_TABLE_SIZE;
}

static unsigned int sdn_segment_hash_address(unsigned char *u, unsigned char *v) {
  unsigned int hash = 5381;
  for (int i = 0; i < SDNADDR_SIZE; i++) {
    hash = ((hash << 5) + hash) + u[i];
    hash = ((hash << 5) + hash) + v[i];
  }
  return hash % SEGMENT_HASH_TABLE_SIZE;
}

void sdn_free_graph(void) {
  sdn_graph_path_list_ptr current_path = final_paths_head;
  while (current_path != NULL) {
    sdn_graph_path_list_ptr path_to_free = current_path;
    current_path = current_path->next;
    free(path_to_free);
  }

  final_paths_head = NULL;

  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    sdn_graph_node_ptr current_node = node_hash_table[i];

    while (current_node != NULL) {
      sdn_graph_neighbor_ptr current_neighbor = current_node->neighbors_head;
      while (current_neighbor != NULL) {
        struct sdn_graph_neighbor *neighbor_to_free = current_neighbor;
        current_neighbor = current_neighbor->next;
        free(neighbor_to_free);
      }

      struct sdn_graph_node *node_to_free = current_node;
      current_node = current_node->next_in_bucket;
      free(node_to_free);
    }

    node_hash_table[i] = NULL;
  }

  for (int i = 0; i < SEGMENT_HASH_TABLE_SIZE; i++) {
    sdn_graph_segment_ptr current_seg = segment_hash_table[i];
    while (current_seg != NULL) {
      sdn_graph_segment_ptr seg_to_free = current_seg;
      current_seg = current_seg->next_in_bucket;
      free(seg_to_free);
    }
    segment_hash_table[i] = NULL;
  }
}

// ==========================================
//       BUILD GRAPH FROM FLOW TABLE
// ==========================================

static sdn_graph_node_ptr sdn_add_node(sdnaddr_t *addr) {
  unsigned int hash = sdn_hash_address((unsigned char *)addr);

  sdn_graph_node_ptr current = node_hash_table[hash];
  while (current != NULL) {
    if ((sdnaddr_cmp(addr, (sdnaddr_t *)&current->id)) == 0) return current;
    current = current->next_in_bucket;
  }

  sdn_graph_node_ptr new_node = (sdn_graph_node_ptr)malloc(sizeof(struct sdn_graph_node));

  if (new_node == NULL) {
    SDN_DEBUG_ERROR("sdn-graph: Graph node malloc error\n");
    return NULL;
  }

  memset(new_node, 0, sizeof(struct sdn_graph_node));
  sdnaddr_copy(&new_node->id, addr);
  new_node->in_degree = 0;
  new_node->table_entry = NULL;
  new_node->neighbors_head = NULL;
  new_node->next_in_bucket = NULL;

  new_node->next_in_bucket = node_hash_table[hash];
  node_hash_table[hash] = new_node;

  return new_node;
}

static void build_graph(uint16_t flowid) {
  memset(node_hash_table, 0, sizeof(node_hash_table));

  flow_id_table_ptr entry = sdn_flow_id_table_get();

  while (entry != NULL) {
    if (entry->flowid == flowid) {
      sdn_graph_node_ptr current_node = sdn_add_node((sdnaddr_t *)&entry->source);
      sdn_graph_node_ptr next_node = sdn_add_node((sdnaddr_t *)&entry->next_hop);

      if (current_node == NULL) continue;
      if (next_node == NULL) {
        //printf("NEXT NULL\n");
        current_node->table_entry = entry;
        continue;
      }

      current_node->table_entry = entry;
      next_node->in_degree++;

      sdn_graph_neighbor_ptr new_neighbor = (sdn_graph_neighbor_ptr)malloc(sizeof(struct sdn_graph_neighbor));

      if (new_neighbor == NULL) {
        SDN_DEBUG_ERROR("sdn-graph: Graph neighbornode malloc error\n");
        continue;
      }

      new_neighbor->node_ptr = next_node;
      new_neighbor->next = current_node->neighbors_head;
      current_node->neighbors_head = new_neighbor;
    }
    entry = entry->next;
  }
}

// ==========================================
//      BUILD PATHS FROM LEAF NODES 
// ==========================================

static void add_path_to_list(sdn_graph_path_ptr path_to_add) {
  sdn_graph_path_list_ptr new_entry = (sdn_graph_path_list_ptr)malloc(sizeof(struct sdn_graph_path_list));
  if (new_entry == NULL) {
    SDN_DEBUG_ERROR("sdn-graph: Path list entry malloc error\n");
    return;
  }

  memcpy(&new_entry->path, path_to_add, sizeof(struct sdn_graph_path));

  new_entry->next = final_paths_head;
  final_paths_head = new_entry;
}

static int is_node_in_path(sdn_graph_path_ptr path, sdnaddr_t *node) {
  for (int i = 0; i < path->len; i++) {
    if (sdnaddr_cmp(&path->nodes[i], node) == 0) return 1;
  }

  return 0;
}

static void get_node_path(sdn_graph_node_ptr current_node, sdn_graph_path_ptr path, int changed_lifes, int *is_active, int is_extended) {

  int was_changed = 0;
  if (current_node->table_entry != NULL) {
    was_changed = (current_node->table_entry->changed == 1);
  }

  if (was_changed) {
    *is_active = 1;
    changed_lifes = MAX_CHANGED_LIFES;
  }

  if (*is_active) {
    if (path->len < MAX_PATH_LEN) {
      sdnaddr_copy(&path->nodes[path->len], &current_node->id);
      path->len++;
    }

    changed_lifes--;

    // ============================================================
    // LÓGICA DE RENASCIMENTO (CHAINING)
    // ============================================================
    if (changed_lifes < 0) {
      add_path_to_list(path);

      path->len = 0; 
      //sdnaddr_copy(&path->nodes[0], &current_node->id);

      changed_lifes = MAX_CHANGED_LIFES; 

      is_extended = 1;
      *is_active = 0;
    }
    // ============================================================

    if (current_node->neighbors_head == NULL) {
      add_path_to_list(path);
      return;
    }
  }

  sdn_graph_neighbor_ptr neighbor = current_node->neighbors_head;
  while (neighbor != NULL) {
    if (*is_active && is_node_in_path(path, &neighbor->node_ptr->id)) {
      add_path_to_list(path);
    } else {
      struct sdn_graph_path new_path;
      memcpy(&new_path, path, sizeof(struct sdn_graph_path));
      get_node_path(neighbor->node_ptr, &new_path, changed_lifes, is_active, is_extended);
    }
    neighbor = neighbor->next;
  }
}

static void get_all_paths(void) {
  final_paths_head = NULL;

  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    sdn_graph_node_ptr current_node = node_hash_table[i];

    while (current_node != NULL) {
      if (current_node->in_degree == 0) {
        struct sdn_graph_path path;
        memset(&path, 0, sizeof(struct sdn_graph_path));
        int is_active = 0;

        get_node_path(current_node, &path, 0, &is_active, 0);
      }
      current_node = current_node->next_in_bucket;
    }
  }
}

// =====================================================
//                 OPTIMIZE PATHS
// =====================================================

static sdn_graph_segment_ptr find_or_add_segment(sdnaddr_t *u, sdnaddr_t *v) {
  unsigned int hash = sdn_segment_hash_address((unsigned char *)u, (unsigned char *)v);

  sdn_graph_segment_ptr current_seg = segment_hash_table[hash];
  while (current_seg != NULL) {
    if ((sdnaddr_cmp(&current_seg->u, u) == 0) && (sdnaddr_cmp(&current_seg->v, v) == 0)) {
      return current_seg;
    }
    current_seg = current_seg->next_in_bucket;
  }

  sdn_graph_segment_ptr new_seg = (sdn_graph_segment_ptr)malloc(sizeof(struct sdn_graph_segment));
  if (new_seg == NULL) {
    SDN_DEBUG_ERROR("sdn-graph: Segment malloc error\n");
    return NULL;
  }

  sdnaddr_copy(&new_seg->u, u);
  sdnaddr_copy(&new_seg->v, v);
  new_seg->best_cost = 99999;
  new_seg->winning_index = 0;
  new_seg->winning_path_entry = NULL;

  new_seg->next_in_bucket = segment_hash_table[hash];
  segment_hash_table[hash] = new_seg;

  return new_seg;
}

static void resolve_path_conflicts(void) {
  memset(segment_hash_table, 0, sizeof(segment_hash_table));

  sdn_graph_path_list_ptr current_path_entry = final_paths_head;
  while (current_path_entry != NULL) {
    for (int i = 0; i < current_path_entry->path.len - 1; i++) {
      sdnaddr_t *u = &current_path_entry->path.nodes[i];
      sdnaddr_t *v = &current_path_entry->path.nodes[i + 1];

      int custo = i + 1;

      sdn_graph_segment_ptr segment = find_or_add_segment(u, v);
      if (segment == NULL) continue;

      if (custo < segment->best_cost) {
        segment->best_cost = custo;
        segment->winning_index = i + 1;
        segment->winning_path_entry = current_path_entry;
      }
    }
    current_path_entry = current_path_entry->next;
  }

  current_path_entry = final_paths_head;
  sdn_graph_path_list_ptr prev_path_entry = NULL;
  while (current_path_entry != NULL) {
    for (int i = 0; i < current_path_entry->path.len - 1; i++) {
      sdnaddr_t *u = &current_path_entry->path.nodes[i];
      sdnaddr_t *v = &current_path_entry->path.nodes[i + 1];

      sdn_graph_segment_ptr segment = find_or_add_segment(u, v);

      if (segment->winning_path_entry != current_path_entry) {

        // printf("\n--- Conflict ---\n");
        // printf("  Segment: ");
        // sdnaddr_print(u); 
        // printf(" -> "); 
        // sdnaddr_print(v);
        // printf("\n");
        // printf("  WINNER (Cost %d): Path from  ", segment->best_cost);
        // sdnaddr_print(&segment->winning_path_entry->path.nodes[0]);
        // printf("\n");
        // printf("  Loser:           Path from ");
        // sdnaddr_print(&current_path_entry->path.nodes[0]);
        // printf("\n");

        int cut_index = i + 1;
        if (cut_index < current_path_entry->path.len) {
          current_path_entry->path.len = cut_index;
        }
        break;
      }
    }

    if (current_path_entry->path.len <= 1) {
      sdn_graph_path_list_ptr node_to_free = current_path_entry;

      if (prev_path_entry == NULL) {
        final_paths_head = current_path_entry->next;
        current_path_entry = final_paths_head;
      } 

      else {
        prev_path_entry->next = current_path_entry->next;
        current_path_entry = current_path_entry->next;
      }


      free(node_to_free);

    } else {
      prev_path_entry = current_path_entry;
      current_path_entry = current_path_entry->next;
    }
  }
}

// =====================================================
//          UPDATE FLOW TABLE: CHANGED = 0
// =====================================================

void sdn_graph_update_changes(void) {
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    sdn_graph_node_ptr current_node = node_hash_table[i];

    while (current_node != NULL) {
      if (current_node->table_entry != NULL) {
        current_node->table_entry->changed = 0;
      }
      current_node = current_node->next_in_bucket;
    }
  }
}

// =====================================================
//
// =====================================================


sdn_graph_path_list_ptr sdn_process_table_graph(uint16_t flowid) {
  sdn_free_graph();

  build_graph(flowid);
  //print_graph_state();

  get_all_paths();

  resolve_path_conflicts();

  if (final_paths_head != NULL) {
    printf("\n====== paths =========\n");
    print_path_list(final_paths_head);
    printf("\n======================\n\n");

    sdn_graph_update_changes();
  }
  
  return final_paths_head;
}
