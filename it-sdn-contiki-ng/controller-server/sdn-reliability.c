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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdn-debug.h"
#include "sdn-protocol.h"
#include "sdn-reliability.h"
//#include "sdn-packetbuf.h"
#include "sdn-send-packet.h"
#include "sdn-serial-send.h"
#include "sdn-flow-table-cache.h"
#include "digraph.h"
#include "sdn-send-packet-controller.h"

void sdn_reliability_timer_event() {

  int ret;
  flow_id_table_ptr flow_id_table_field = sdn_flow_id_table_get();

  while(flow_id_table_field != NULL) {
    //For Hysteresis
    if(flow_id_table_field->countdown >= 0) {

      flow_id_table_field->countdown--;
    }

    if(flow_id_table_field->online == 0 && flow_id_table_field->countdown == 0) {

      SDN_DEBUG("Retransmitting data flow setup with ");

#ifdef SDN_SOURCE_ROUTED
      SDN_DEBUG("SDN_SOURCE_ROUTED\n");
      ret = send_src_rtd_data_flow_setup((sdnaddr_t*)flow_id_table_field->source, flow_id_table_field->flowid, (sdnaddr_t*)flow_id_table_field->next_hop, flow_id_table_field->action);
#else
      SDN_DEBUG("NOT SDN_SOURCE_ROUTED\n");
      ret = sdn_send_data_flow_setup((sdnaddr_t*)flow_id_table_field->source, flow_id_table_field->flowid, (sdnaddr_t*)flow_id_table_field->next_hop, flow_id_table_field->action);
#endif

      //TODO: update time for retransmit when packet was sent by serial/radio
      // flow_id_table_field->countdown = SDN_RETRANSMIT_TIME_S;
      if (ret == SDN_ERROR) {
        flow_id_table_field->countdown = SDN_RETRANSMIT_TIME_S;
      }
    }

    flow_id_table_field = flow_id_table_field->next;
  }

  flow_address_table_ptr flow_address_search = sdn_flow_address_table_get();

  while(flow_address_search != NULL) {

    if(flow_address_search->countdown >= 0) {

      flow_address_search->countdown--;
    }

    if (flow_address_search->countdown == 0) {
      if (flow_address_search->online == 0) {

        SDN_DEBUG("Retransmitting control flow setup with");


#ifdef SDN_SOURCE_ROUTED
        SDN_DEBUG("SDN_SOURCE_ROUTED\n");
        ret = send_src_rtd_control_flow_setup((sdnaddr_t*)flow_address_search->source, (sdnaddr_t*)flow_address_search->target, (sdnaddr_t*)flow_address_search->next_hop, flow_address_search->action);
#else
        SDN_DEBUG("NOT SDN_SOURCE_ROUTED\n");
        ret = sdn_send_control_flow_setup((sdnaddr_t*)flow_address_search->source, (sdnaddr_t*)flow_address_search->target, (sdnaddr_t*)flow_address_search->next_hop, flow_address_search->action);
#endif
        if (ret == SDN_ERROR) {
          flow_address_search->countdown = SDN_RETRANSMIT_TIME_S;
        }

      //TODO: update time for retransmit when packet was sent by serial/radio
      // flow_address_search->countdown = SDN_RETRANSMIT_TIME_S;
      } else {
        SDN_DEBUG("Address flow expired!\n");
        flow_address_search = sdn_flow_address_table_remove(flow_address_search);
      }
    }

    if (flow_address_search != NULL)
      flow_address_search = flow_address_search->next;

  }
}