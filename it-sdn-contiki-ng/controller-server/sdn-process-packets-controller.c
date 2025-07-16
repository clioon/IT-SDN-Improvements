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

#include <stdlib.h>
#include "sdn-debug.h"
#include <inttypes.h>
#include "digraph.h"

#include "sdn-flow-table-cache.h"
#include "sdn-serial-send.h"

#include "control-flow-table.h"
#include "data-flow-table.h"
#include "sdn-queue.h"

#include "sdn-send-packet.h"
#include "sdn-send-packet-controller.h"
#include "sdn-util-controller.h"
#include "sdn-serial-packet.h"
#include "sdn-process-packets-controller.h"
#include "sdn-packetbuf.h"
#include "mainwindow_wrapper.h"

#if !(defined(SDN_NEIGHBORINFO_SRC_TO_NEIGHBORS) || defined(SDN_NEIGHBORINFO_NEIGHBORS_TO_SRC))
#error SDN_NEIGHBORINFO_SRC_TO_NEIGHBORS nor SDN_NEIGHBORINFO_NEIGHBORS_TO_SRC defined
#endif

void configure_flow_id_route(uint16_t flowid, route_ptr route);

void checks_flow_id_table_changes();

void process_flow_id_table_changes();

void configure_flow_address_route(unsigned char* target, route_ptr route);

void checks_flow_address_table_changes();

void process_flow_address_table_changes();

void process_data_flow_setup(flow_id_table_ptr routing_field);

void process_control_flow_setup(flow_address_table_ptr routing_field);

void enqueue_sdn_data_flow_request_to_controller(uint8_t *source);

void update_flow_table_cache();

//####################################################################################

void process_energy_metric(sdnaddr_t *source, uint32_t battery) {

  int digraph_changed = 0;

  printf("Source ");
  sdnaddr_print(source);
  printf(" battery level (%u / 255)\n", battery);

  vertice_ptr vertice_source = digraph_find_vertice(source->u8);
  if(vertice_source == NULL) {
    vertice_source = digraph_add_vertice(source->u8);
    enqueue_sdn_data_flow_request_to_controller(source->u8);
    digraph_changed = 1;
  }

  if(digraph_update_energy_vertice(vertice_source, (unsigned char) battery) == 1) {
    digraph_changed = 1;
  }

  if(digraph_changed == 1) {
    update_flow_table_cache();
  }

// #if SDN_TX_RELIABILITY

//   #ifdef SDN_SOURCE_ROUTED
//   send_src_rtd_ack(&SDN_HEADER(sdn_energy_report)->source, SDN_HEADER(sdn_energy_report)->type, SDN_HEADER(sdn_energy_report)->seq_no);
//   #else
//   send_ack_by_flow_address(&SDN_HEADER(sdn_energy_report)->source, SDN_HEADER(sdn_energy_report)->type, SDN_HEADER(sdn_energy_report)->seq_no);
//   #endif

// #endif // SDN_TX_RELIABILITY
}

void process_neighbor_report(sdn_neighbor_report_t* sdn_neighbor_report, void* neighbors) {

  vertice_ptr vertice_source = digraph_find_vertice(sdn_neighbor_report->header.source.u8);

  vertice_ptr vertice_dest;
  edge_ptr edge;

  int digraph_changed = 0;
  int index_neighbor;

  if(vertice_source == NULL) {
    vertice_source = digraph_add_vertice(sdn_neighbor_report->header.source.u8);
    if (digraph_count_vertice() == sdn_get_cli_nnodes()) {
        SDN_DEBUG("Graph complete!\n");
        uint8_t *v = (uint8_t *) malloc(sizeof(uint8_t));
        *v = sdn_get_cli_nnodes();
        if (sdn_send_queue_enqueue_custom(v, sizeof(uint8_t), 0, SDN_SERIAL_MSG_TYPE_FULL_GRAPH) == SDN_SUCCESS)
            sdn_set_cli_nnodes(0);
    }
    enqueue_sdn_data_flow_request_to_controller(sdn_neighbor_report->header.source.u8);
    digraph_changed = 1;
  }

  printf("Source ");
  sdnaddr_print(&(sdn_neighbor_report->header.source));
  printf(" neighbors (%d)\n", sdn_neighbor_report->num_of_neighbors);

  //Marks edges to remove before updates.
#if defined(SDN_NEIGHBORINFO_SRC_TO_NEIGHBORS) && defined(SDN_NEIGHBORINFO_NEIGHBORS_TO_SRC)
  digraph_mark_edges_to_del_from(vertice_source);

#else
  #if defined(SDN_NEIGHBORINFO_SRC_TO_NEIGHBORS)
  digraph_mark_edges_to_del_from(vertice_source);
  #endif

  #if defined(SDN_NEIGHBORINFO_NEIGHBORS_TO_SRC)
  digraph_mark_edges_to_del_towards(vertice_source);
  #endif
#endif

  for(index_neighbor = 0; index_neighbor < sdn_neighbor_report->num_of_neighbors; index_neighbor++) {

    sdn_neighbor_report_list_t* neighbor_list = (sdn_neighbor_report_list_t*) (neighbors + (sizeof(sdn_neighbor_report_list_t) * index_neighbor));

    vertice_dest = digraph_find_vertice(neighbor_list->neighbor.u8);

    if(vertice_dest == NULL) {
      vertice_dest = digraph_add_vertice(neighbor_list->neighbor.u8);
      if (digraph_count_vertice() == sdn_get_cli_nnodes()) {
          SDN_DEBUG("Graph complete!\n");
          uint8_t *v = (uint8_t *) malloc(sizeof(uint8_t));
          *v = sdn_get_cli_nnodes();
          sdn_send_queue_enqueue_custom(v, sizeof(uint8_t), 0, SDN_SERIAL_MSG_TYPE_FULL_GRAPH);
          sdn_set_cli_nnodes(0);
      }
      enqueue_sdn_data_flow_request_to_controller(neighbor_list->neighbor.u8);
    }

#if defined(SDN_NEIGHBORINFO_SRC_TO_NEIGHBORS) && defined(SDN_NEIGHBORINFO_NEIGHBORS_TO_SRC)
    edge = digraph_add_edge(vertice_source, vertice_dest, neighbor_list->etx);

    if(edge->status != EDGE_STATIC) {
      digraph_changed = 1;
    }

    edge = digraph_find_edge(vertice_dest, vertice_source);

    if(edge == NULL) { // Add new edge
      edge = digraph_add_edge(vertice_dest, vertice_source, neighbor_list->etx);
      if(edge->status != EDGE_STATIC) {
        digraph_changed = 1;
      }
    }
#else
  #ifdef SDN_NEIGHBORINFO_SRC_TO_NEIGHBORS
    edge = digraph_add_edge(vertice_source, vertice_dest, neighbor_list->etx);

    if(edge->status != EDGE_STATIC) {
      digraph_changed = 1;
    }
  #endif

    // A neighbor report indicates the existence of edges
    // from the nodes in the NR list to the NR sender
  #ifdef SDN_NEIGHBORINFO_NEIGHBORS_TO_SRC
    edge = digraph_add_edge(vertice_dest, vertice_source, neighbor_list->etx);

    if(edge->status != EDGE_STATIC) {
      digraph_changed = 1;
    }
  #endif
#endif

#ifdef SDN_INFORM_NEW_EDGE_SERIAL
     if(edge->status == EDGE_NEW) {
       // allocates memory for 2 addresses and one byte
       uint8_t* temp_packet = malloc(sizeof(sdnaddr_t)*2 + sizeof(uint8_t));
       // copies the "from vertex" of the edge into the allocated space
       sdnaddr_copy((sdnaddr_t*)temp_packet, &neighbor_list->neighbor);
       // copies the "to vertex" of the edge into the allocated space
       sdnaddr_copy((sdnaddr_t*)(temp_packet + sizeof(sdnaddr_t)), &sdn_neighbor_report->header.source);
       // copies the distance to the controller into the remaining byte
       *(temp_packet + 2*sizeof(sdnaddr_t)) = SDN_DEFAULT_TTL - SDN_HEADER(sdn_neighbor_report)->thl;

       SDN_DEBUG("Informing new edge to Contiki: ");
       sdnaddr_print(&neighbor_list->neighbor);
       SDN_DEBUG(" can reach ");
       sdnaddr_print(&sdn_neighbor_report->header.source);
       SDN_DEBUG(". (%d, %d)\n", SDN_HEADER(sdn_neighbor_report)->thl, SDN_DEFAULT_TTL - SDN_HEADER(sdn_neighbor_report)->thl);

       if (sdn_send_queue_enqueue_custom(temp_packet, sizeof(sdnaddr_t)*2 + sizeof(uint8_t), 0, SDN_SERIAL_MSG_TYPE_REACH_INFO) != SDN_SUCCESS) {
         free(temp_packet);
         SDN_DEBUG_ERROR ("Error on packet enqueue.\n");
       } else {
         sdn_send_down_once();
       }
     }
#endif
  }

  //Removes edges not informed.
#if defined(SDN_NEIGHBORINFO_SRC_TO_NEIGHBORS) && defined(SDN_NEIGHBORINFO_NEIGHBORS_TO_SRC)
  if(digraph_del_marked_edges_to_del_from(vertice_source) == 1) {
    digraph_changed = 1;
  }
#else
  #ifdef SDN_NEIGHBORINFO_SRC_TO_NEIGHBORS
  if(digraph_del_marked_edges_to_del_from(vertice_source) == 1) {
    digraph_changed = 1;
  }
  #endif

  #ifdef SDN_NEIGHBORINFO_NEIGHBORS_TO_SRC
  if(digraph_del_marked_edges_to_del_towards(vertice_source) == 1) {
    digraph_changed = 1;
  }
  #endif
#endif

  digraph_print();

  if(digraph_changed == 1) {
    update_flow_table_cache();
  } else {
    sdn_send_down_once();
  }

#if SDN_TX_RELIABILITY

  #ifdef SDN_SOURCE_ROUTED
  send_src_rtd_ack(&SDN_HEADER(sdn_neighbor_report)->source, SDN_HEADER(sdn_neighbor_report)->type, SDN_HEADER(sdn_neighbor_report)->seq_no);
  #else
  send_ack_by_flow_address(&SDN_HEADER(sdn_neighbor_report)->source, SDN_HEADER(sdn_neighbor_report)->type, SDN_HEADER(sdn_neighbor_report)->seq_no);
  #endif

#endif // SDN_TX_RELIABILITY
}

void process_data_flow_request_packet(sdn_data_flow_request_t *sdn_data_flow_request) {

  flow_id_table_ptr flow_id_field;

  route_ptr route = NULL;

  flow_id_field = sdn_flow_id_table_find(sdn_data_flow_request->header.source.u8, sdn_data_flow_request->flowid);

  if(flow_id_field != NULL) {
#if !SDN_TX_RELIABILITY
    process_data_flow_setup(flow_id_field);
#else
    if (flow_id_field->online == 1) {
      process_data_flow_setup(flow_id_field);
    }
#endif //!SDN_TX_RELIABILITY
  } else {

    route = get_flow_id_better_route(sdn_data_flow_request->header.source.u8, sdn_data_flow_request->flowid);

    // If there are no routes to flow id:
    if(route == NULL) {

      printf("Route to flowid %d was not found.\n", sdn_data_flow_request->flowid);

      if (sdn_data_flow_request->flowid != SDN_CONTROLLER_FLOW)
        sdn_flow_id_table_add(sdn_data_flow_request->header.source.u8, sdn_data_flow_request->flowid, sdn_data_flow_request->header.source.u8, SDN_ACTION_DROP, ROUTE_MAX_WEIGHT, ROUTE_MAX_HOPS);

    // If there is at least one route to flow id:
    } else {
      printf("Route was found to flowId %d.\n", sdn_data_flow_request->flowid);

      configure_flow_id_route(sdn_data_flow_request->flowid, route);
    }

    process_flow_id_table_changes();
  }
}

void process_control_flow_request(sdn_control_flow_request_t *sdn_control_flow_request) {

  flow_address_table_ptr routing_field;

  route_ptr route = NULL;

  printf("Control Flow Request: ");
  sdnaddr_print(&(sdn_control_flow_request->header.source));
  printf(" -> ");
  sdnaddr_print(&(sdn_control_flow_request->address));
  printf("\n");

  routing_field = sdn_flow_address_table_find(sdn_control_flow_request->header.source.u8, sdn_control_flow_request->address.u8);

  if(routing_field != NULL) {
#if !SDN_TX_RELIABILITY
    process_control_flow_setup(routing_field);
#else
    if (routing_field->online == 1) {
      process_control_flow_setup(routing_field);
    }
#endif //!SDN_TX_RELIABILITY
  } else {

    route = get_flow_address_better_route(sdn_control_flow_request->header.source.u8, sdn_control_flow_request->address.u8);

    if(route == NULL) {
      printf("Route not found.\n");

      sdn_flow_address_table_add(sdn_control_flow_request->header.source.u8, sdn_control_flow_request->address.u8, sdn_control_flow_request->header.source.u8, SDN_ACTION_DROP, ROUTE_MAX_WEIGHT, ROUTE_MAX_HOPS);

    } else {

      configure_flow_address_route(sdn_control_flow_request->address.u8, route);
    }

    process_flow_address_table_changes();
  }
}

void configure_flow_id_route(uint16_t flowid, route_ptr route) {
  // SDN_DEBUG("configure_flow_id_route: configuring the following route for flowid %d :", flowid);

  route_ptr route_src = route;
  route_ptr current_route;
  flow_id_table_ptr flow_id_table_field;

  //Gets the last ptr as a source node from route
  while (route_src->next != NULL) {
    // sdnaddr_print((sdnaddr_t *)route_src->vertice_id);
    // SDN_DEBUG(" ");
    route_src = route_src->next;
  }
  // SDN_DEBUG("\n");

  // Gets field from current route
  flow_id_table_field = sdn_flow_id_table_find(route_src->vertice_id, flowid);

  if (flow_id_table_field != NULL) {
    //Mount current route to compare if yet exist and updates weight
    current_route = get_current_route_from_flowid(flow_id_table_field);

    if (current_route != NULL) {
      //verifies if yet there is the route and updates weight
      current_route = dijkstra_checks_route(current_route);

      if (current_route != NULL) {
        //Update the current route weight
        flow_id_table_field->route_weight = current_route->route_weight;
      }
    }
  }

  //Verifies if route weight escapes from SDN_ROUTE_SENSIBILITY.
  if (flow_id_table_field == NULL || route_src->route_weight < (flow_id_table_field->route_weight / (1.0 + SDN_ROUTE_SENSIBILITY)) ) {
    // SDN_DEBUG("\tReplace/add route\n");

    while (route->next != NULL) {
#if RDC_UNIDIR_SUPPORT
      if (route->unidir)
        sdn_flow_id_table_add(route->next->vertice_id, flowid, route->vertice_id, SDN_ACTION_FORWARD_UNIDIR, route->next->route_weight, route->next->hops);
      else
#endif
        sdn_flow_id_table_add(route->next->vertice_id, flowid, route->vertice_id, SDN_ACTION_FORWARD, route->next->route_weight, route->next->hops);

      route = route->next;
    }

  } else {
    // SDN_DEBUG("\tDo not Replace route, only check for action change\n");
#if RDC_UNIDIR_SUPPORT
     while (route->next != NULL) {
      if (route->unidir)
        sdn_flow_id_table_update_action(route->next->vertice_id, flowid, route->vertice_id, SDN_ACTION_FORWARD_UNIDIR);
      else
        sdn_flow_id_table_update_action(route->next->vertice_id, flowid, route->vertice_id, SDN_ACTION_FORWARD);

      route = route->next;
    }
#endif

  }

  dijkstra_free_route(route);
}

void configure_flow_address_route(unsigned char* target, route_ptr route) {
  // SDN_DEBUG("configure_flow_address_route\n");

  route_ptr route_src = route;
  route_ptr current_route;
  flow_address_table_ptr flow_address_table_field;

  //Gets the last ptr as a source node from route
  while (route_src->next != NULL) {

    route_src = route_src->next;
  }

  // Gets field from current route
  flow_address_table_field = sdn_flow_address_table_find(route_src->vertice_id, target);

  if (flow_address_table_field != NULL) {
    //Mount current route to compare if yet exist and updates weight
    current_route = get_current_route_from_address(flow_address_table_field);

    if (current_route != NULL) {
      //verifys if yet there is the route and updates weight
      current_route = dijkstra_checks_route(current_route);

      if (current_route != NULL) {
        //Update the current route weight
        flow_address_table_field->route_weight = current_route->route_weight;
      }
    }

    if (current_route == NULL) {

      flow_address_table_field = NULL;
    }
  }

  //Verifies if route weight escapes from SDN_ROUTE_SENSIBILITY.
  if(flow_address_table_field == NULL || route_src->route_weight < (flow_address_table_field->route_weight / (1.0 + SDN_ROUTE_SENSIBILITY)) ) {
    // SDN_DEBUG("\tReplace/add route\n");

    while (route->next != NULL) {
#if RDC_UNIDIR_SUPPORT
      if (route->unidir)
        sdn_flow_address_table_add(route->next->vertice_id, target, route->vertice_id, SDN_ACTION_FORWARD_UNIDIR, route->next->route_weight, route->next->hops);
      else
#endif
        sdn_flow_address_table_add(route->next->vertice_id, target, route->vertice_id, SDN_ACTION_FORWARD, route->next->route_weight, route->next->hops);

      route = route->next;
    }
  } else {
    // SDN_DEBUG("\tDo not Replace route, only check for action change\n");
#if RDC_UNIDIR_SUPPORT
    while (route->next != NULL) {
      if (route->unidir)
        sdn_flow_address_table_update_action(route->next->vertice_id, target, route->vertice_id, SDN_ACTION_FORWARD_UNIDIR);
      else
        sdn_flow_address_table_update_action(route->next->vertice_id, target, route->vertice_id, SDN_ACTION_FORWARD);

      route = route->next;
    }
#endif

  }

  dijkstra_free_route(route);
}


void update_flow_table_cache() {

  SDN_DEBUG("\nUpdating flow table cache with digraph changes.\n");

  checks_flow_id_table_changes();

  checks_flow_address_table_changes();

  sdn_send_down_once();
}

void checks_flow_address_table_changes() {

  route_ptr route = NULL;

  flow_address_table_ptr flow_address_table_field = sdn_flow_address_table_get();
  SDN_DEBUG("checks_flow_address_table_changes()\n");
//TODO: comment this
  while(flow_address_table_field != NULL) {

    if(flow_address_table_field->changed == 1) {
      printf("ERROR: (inconsistency) SOURCE ");
      sdnaddr_print((sdnaddr_t *)flow_address_table_field->source);
      printf(" TARGET ");
      sdnaddr_print((sdnaddr_t *)flow_address_table_field->target);
      printf(" CHANGED = %d\n", flow_address_table_field->changed);
    }

    flow_address_table_field = flow_address_table_field->next;
  }

  flow_address_table_field = sdn_flow_address_table_get();
//end TODO

  while(flow_address_table_field != NULL) {

    //Control for it does not read and change the registry more than once.
    if(flow_address_table_field->changed == 0) {

      route = get_flow_address_better_route(flow_address_table_field->source, flow_address_table_field->target);

      // If there are no routes to address:
      if(route == NULL) {

        sdn_flow_address_table_add(flow_address_table_field->source, flow_address_table_field->target, flow_address_table_field->source, SDN_ACTION_DROP, ROUTE_MAX_WEIGHT, ROUTE_MAX_HOPS);

      // If there is at least one route to flow id:
      } else {

        configure_flow_address_route(flow_address_table_field->target, route);
      }
    }

    flow_address_table_field = flow_address_table_field->next;
  }

  process_flow_address_table_changes();
}

void process_flow_address_table_changes() {

  flow_address_table_ptr flow_address_table_field = sdn_flow_address_table_get();
  printf("process_flow_address_table_changes()\n");

  while(flow_address_table_field != NULL) {

    if(flow_address_table_field->changed == 1) {

      process_control_flow_setup(flow_address_table_field);
      flow_address_table_field->changed = 0;
    }

    flow_address_table_field = flow_address_table_field->next;
  }
}

void checks_flow_id_table_changes() {

  route_ptr route;

  flow_id_table_ptr flow_id_table_field = sdn_flow_id_table_get();

  printf("checks_flow_id_table_changes()\n");

//TODO: comment this
  while(flow_id_table_field != NULL) {

    if(flow_id_table_field->changed == 1) {
      printf("ERROR: (inconsistency) SOURCE ");
      sdnaddr_print((sdnaddr_t *)flow_id_table_field->source);
      printf(" FLOW ID [%d] CHANGED = %d.\n", flow_id_table_field->flowid, flow_id_table_field->changed);
    }

    flow_id_table_field = flow_id_table_field->next;
  }

  flow_id_table_field = sdn_flow_id_table_get();
//end TODO

  while(flow_id_table_field != NULL) {

    if(flow_id_table_field->changed == 0) {

      route = get_flow_id_better_route(flow_id_table_field->source, flow_id_table_field->flowid);

      // If there are no routes to flow id:
      if(route == NULL) {

        if (flow_id_table_field->flowid != SDN_CONTROLLER_FLOW)
          sdn_flow_id_table_add(flow_id_table_field->source, flow_id_table_field->flowid, flow_id_table_field->source, SDN_ACTION_DROP, ROUTE_MAX_WEIGHT, ROUTE_MAX_HOPS);

      // If there is at least one route to flow id:
      } else {

        configure_flow_id_route(flow_id_table_field->flowid, route);
      }
    }

    flow_id_table_field = flow_id_table_field->next;
  }

  process_flow_id_table_changes();
}

void process_flow_id_table_changes() {

  flow_id_table_ptr flow_id_table_field = sdn_flow_id_table_get();
  printf("process_flow_id_table_changes()\n");

  while(flow_id_table_field != NULL) {

    if(flow_id_table_field->changed == 1) {

      process_data_flow_setup(flow_id_table_field);
      flow_id_table_field->changed = 0;
    }

    flow_id_table_field = flow_id_table_field->next;
  }
}

void process_data_flow_setup(flow_id_table_ptr routing_field) {

#ifdef SDN_SOURCE_ROUTED
  send_src_rtd_data_flow_setup((sdnaddr_t*)routing_field->source, routing_field->flowid, (sdnaddr_t*)routing_field->next_hop, routing_field->action);
#else
  sdn_send_data_flow_setup((sdnaddr_t*)routing_field->source, routing_field->flowid, (sdnaddr_t*)routing_field->next_hop, routing_field->action);
#endif

}

void process_control_flow_setup(flow_address_table_ptr routing_field) {

#ifdef SDN_SOURCE_ROUTED
    send_src_rtd_control_flow_setup((sdnaddr_t*)routing_field->source, (sdnaddr_t*)routing_field->target, (sdnaddr_t*)routing_field->next_hop, routing_field->action);
#else
    sdn_send_control_flow_setup((sdnaddr_t*)routing_field->source, (sdnaddr_t*)routing_field->target, (sdnaddr_t*)routing_field->next_hop, routing_field->action);
#endif
}

void process_register_flowid(uint16_t flowid, unsigned char* target) {

  sdn_flow_id_list_table_add(flowid, target);
  checks_flow_id_table_changes();
}

/**MANAGEMENT*/
#ifdef MANAGEMENT
void process_mngt_data(sdn_flow_id_mngt_node_data_t* sdn_mngt_node_report, void* ptr_metrics_report, sdnaddr_t* packet_source){

  // printf("Management data received\n");

  uint16_t node_metrics;
  uint8_t metric_position = 0;
  sdn_mngt_metric_t temp_metric_struct;
  void* ptr_temp_metrics = ptr_metrics_report;

  node_metrics = sdn_mngt_node_report->mngt_metrics;

  // printf("Node metrics sent %u\n", node_metrics);

  // printf("Metric ID: %u, Value: %"PRIu32"\n", temp_metric_struct.metric_id, temp_metric_struct.metric_value);
  // ptr_temp_metrics += sizeof(sdn_mngt_metric_t);

  if (node_metrics & SDN_MNGT_METRIC_BATTERY) {
      memcpy(&temp_metric_struct, ptr_temp_metrics + metric_position, sizeof(sdn_mngt_metric_t));
      process_energy_metric(packet_source, temp_metric_struct.metric_value);
      metric_position += sizeof(sdn_mngt_metric_t);
  }

  if (node_metrics & SDN_MNGT_METRIC_QTY_DATA) {
      memcpy(&temp_metric_struct, ptr_temp_metrics + metric_position, sizeof(sdn_mngt_metric_t));
      //tratar o que fazer
      printf("QTY_DATA: %d\n", temp_metric_struct.metric_value );
      metric_position += sizeof(sdn_mngt_metric_t);
  }

  if (node_metrics & SDN_MNGT_METRIC_QUEUE_DELAY) {
      memcpy(&temp_metric_struct, ptr_temp_metrics + metric_position, sizeof(sdn_mngt_metric_t));
      //tratar o que fazer
      printf("QUEUE_DELAY: %d\n", temp_metric_struct.metric_value );
      metric_position += sizeof(sdn_mngt_metric_t);
  }

  // send_src_rtd_cont_mngt(packet_source, node_metrics);
}
#endif

void enqueue_sdn_data_flow_request_to_controller(uint8_t *source) {

  sdn_data_flow_request_t *sdn_packet = (sdn_data_flow_request_t *) malloc(sizeof(sdn_data_flow_request_t));

  if (!sdn_packet) {
    return;
  }

  MAKE_SDN_HEADER(SDN_PACKET_DATA_FLOW_REQUEST, SDN_DEFAULT_TTL);
  sdn_packet->flowid = SDN_CONTROLLER_FLOW;
  sdn_packet->dest_flowid = SDN_CONTROLLER_FLOW;

  memcpy(sdn_packet->header.source.u8, source, SDNADDR_SIZE);

  if (sdn_send_queue_enqueue((uint8_t *) sdn_packet, sizeof(sdn_data_flow_request_t), 0) != SDN_SUCCESS) {
    printf("ERROR: It was not possible enqueue the packet sdn_data_flow_request_t.\n");
    free(sdn_packet);
  }

}

void process_phase_info_controller(sdn_phase_info_flow_t *phase_info_pkt) {
  sdn_control_flow_request_t req_back_route;

#ifdef SDN_SOURCE_ROUTED
  sdn_send_src_rtd_phase_addr(&phase_info_pkt->phase_sender, &SDN_HEADER(phase_info_pkt)->source, phase_info_pkt->phase);
#else
  sdn_send_phase_addr(&phase_info_pkt->phase_sender, &SDN_HEADER(phase_info_pkt)->source, phase_info_pkt->phase);
#endif
  if (sdnaddr_cmp(&phase_info_pkt->phase_sender, &sdn_node_addr) != SDN_EQUAL) {
    sdnaddr_copy(&req_back_route.header.source, &SDN_HEADER(phase_info_pkt)->source);
    sdnaddr_copy(&req_back_route.address, &phase_info_pkt->phase_sender);
    process_control_flow_request(&req_back_route);
  }
}

