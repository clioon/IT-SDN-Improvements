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

#include "sdn-util-controller.h"
#include "sdn-packetbuf.h"
#include "sdn-debug.h"
#include "sdn-addr.h"
#include "sdn-constants.h"
#include "sdn-common-send.h"
#include <stdlib.h>

route_ptr get_better_route(uint8_t *source, uint8_t *target, route_ptr route_better, unsigned char demand_bidir_path, uint8_t* blacklisted);

/**********************************************************************************************************************************/
route_ptr get_better_route(uint8_t *source, uint8_t *target, route_ptr route_better, unsigned char demand_bidir_path, uint8_t* blacklisted) {

  route_ptr route_search = dijkstra_shortest_path(source, target, demand_bidir_path, blacklisted);

  // if (route_search == NULL and blacklisted != NULL) {
  //   route_search = dijkstra_shortest_path(source, target, demand_bidir_path, NULL);
  // }

  if(route_search != NULL) {

    if(route_better == NULL) {

      route_better = route_search;

    } else {

      if(route_search->route_weight > route_better->route_weight) {

        dijkstra_free_route(route_search);

      } else {

        dijkstra_free_route(route_better);

        route_better = route_search;
      }
    }
  }

  return route_better;
}

/**********************************************************************************************************************************/
route_ptr get_flow_address_better_route(uint8_t *source, uint8_t *target) {

  route_ptr route_better = NULL;

  route_better = get_better_route(source, target, route_better, CONTROLLER_BIDIR_ROUTES_ONLY, NULL);

  return route_better;
}

/**********************************************************************************************************************************/
route_ptr get_flow_id_better_route(uint8_t *source, uint16_t flowid) {//, flow_id_table_ptr flow_id_field) {

  route_ptr route_better = NULL;

  flow_id_list_table_ptr flow_id_list_table_field = sdn_flow_id_list_table_get();

  uint8_t* blacklisted = NULL;

  if (flowid != SDN_CONTROLLER_FLOW){
    blacklisted = sdn_node_addr.u8;
  }

  // Getting all possible targets from specific flow id to calculate better route.
  while(flow_id_list_table_field != NULL) {

    if(flow_id_list_table_field->flowid == flowid) {

      route_better = get_better_route(source, flow_id_list_table_field->target, route_better, CONTROLLER_BIDIR_ROUTES_ONLY, blacklisted);
    }

    flow_id_list_table_field = flow_id_list_table_field->next;
  }

  return route_better;
}

/**********************************************************************************************************************************/
route_ptr get_current_route_from_flowid(flow_id_table_ptr flow_id_table) {

  route_ptr route;
  route_ptr route_head = NULL;
  route_ptr route_tail = NULL;
  unsigned char target[SDNADDR_SIZE];
  int loop_hops;

  if (flow_id_table->action == SDN_ACTION_FORWARD || flow_id_table->action == SDN_ACTION_FORWARD_UNIDIR) {

    while(flow_id_table != NULL && (flow_id_table->action == SDN_ACTION_FORWARD || flow_id_table->action == SDN_ACTION_FORWARD_UNIDIR)) {

      loop_hops = flow_id_table->hops;

      route = (route_ptr) malloc(sizeof *route);
      memcpy(route->vertice_id, flow_id_table->source, SDNADDR_SIZE);
      route->next = NULL;
      route->route_weight = flow_id_table->route_weight;
      route->hops = flow_id_table->hops;
      route->keep_route_cache = 0;

      if (route_head == NULL) {
        route_head = route;
      } else {
        route_tail->next = route;
      }

      route_tail = route;
      memcpy(target, flow_id_table->next_hop, SDNADDR_SIZE);
      flow_id_table = sdn_flow_id_table_find(flow_id_table->next_hop, flow_id_table->flowid);

      if (flow_id_table != NULL && loop_hops <= flow_id_table->hops) {
        //route loop detected, discarding route;
        flow_id_table = NULL;

        dijkstra_free_route(route_head);

        route_head = NULL;
      }
    }

    if (route_head != NULL) {
      route = (route_ptr) malloc(sizeof *route);
      memcpy(route->vertice_id, target, SDNADDR_SIZE);
      route->next = NULL;
      route->route_weight = 0;
      route->hops = 0;
      route->keep_route_cache = 0;

      route_tail->next = route;
    }
  }

  return route_head;
}
/**********************************************************************************************************************************/
route_ptr get_current_route_from_address(flow_address_table_ptr flow_address_table) {

  route_ptr route;
  route_ptr route_head = NULL;
  route_ptr route_tail = NULL;
  unsigned char target[SDNADDR_SIZE];
  int loop_hops;

  if (flow_address_table->action == SDN_ACTION_FORWARD) {

    while(flow_address_table != NULL && flow_address_table->action == SDN_ACTION_FORWARD) {

      loop_hops = flow_address_table->hops;

      route = (route_ptr) malloc(sizeof *route);
      memcpy(route->vertice_id, flow_address_table->source, SDNADDR_SIZE);
      route->next = NULL;
      route->route_weight = flow_address_table->route_weight;
      route->hops = flow_address_table->hops;
      route->keep_route_cache = 0;

      if (route_head == NULL) {
        route_head = route;
      } else {
        route_tail->next = route;
      }

      route_tail = route;
      memcpy(target, flow_address_table->next_hop, SDNADDR_SIZE);
      flow_address_table = sdn_flow_address_table_find(flow_address_table->next_hop, flow_address_table->target);

      if (flow_address_table != NULL && loop_hops <= flow_address_table->hops) {
        //route loop detected, discarding route;
        flow_address_table = NULL;

        dijkstra_free_route(route_head);

        route_head = NULL;
      }
    }

    if (route_head != NULL) {
      route = (route_ptr) malloc(sizeof *route);
      memcpy(route->vertice_id, target, SDNADDR_SIZE);
      route->next = NULL;
      route->route_weight = 0;
      route->hops = 0;
      route->keep_route_cache = 0;

      route_tail->next = route;
    }
  }

  return route_head;
}
/**********************************************************************************************************************************/