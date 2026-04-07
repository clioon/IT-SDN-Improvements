#ifndef SDN_GRAPH_H
#define SDN_GRAPH_H

#include "sdn-addr.h"
#include "sdn-flow-table-cache.h"

#define MAX_PATH_LEN 30

struct sdn_graph_neighbor {
  struct sdn_graph_node *node_ptr;
  struct sdn_graph_neighbor *next;
};
typedef struct sdn_graph_neighbor *sdn_graph_neighbor_ptr;

struct sdn_graph_node {
    sdnaddr_t id;
    int in_degree;
    flow_id_table_ptr table_entry;
    //struct sdn_graph_node *next_in_tree;
    sdn_graph_neighbor_ptr neighbors_head;
    struct sdn_graph_node *next_in_bucket;
};
typedef struct sdn_graph_node *sdn_graph_node_ptr;

struct sdn_graph_path {
    sdnaddr_t nodes[MAX_PATH_LEN];
    int len;
};
typedef struct sdn_graph_path *sdn_graph_path_ptr;

    struct sdn_graph_path_list {
        struct sdn_graph_path path;
        struct sdn_graph_path_list *next;
    };
typedef struct sdn_graph_path_list *sdn_graph_path_list_ptr;

struct sdn_graph_segment {
    sdnaddr_t u;
    sdnaddr_t v;

    int best_cost;
    int winning_index;
    sdn_graph_path_list_ptr winning_path_entry;

    struct sdn_graph_segment *next_in_bucket;
};
typedef struct sdn_graph_segment *sdn_graph_segment_ptr;

void sdn_free_graph(void);

#ifdef __cplusplus
extern "C" {
#endif

sdn_graph_path_list_ptr sdn_process_table_graph(uint16_t flowid);

#ifdef __cplusplus
}
#endif


#endif /* SDN_GRAPH_H */