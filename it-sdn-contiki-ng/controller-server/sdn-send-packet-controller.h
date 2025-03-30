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

#ifndef SDN_SEND_PACKET_CONTROLLER_H
#define SDN_SEND_PACKET_CONTROLLER_H

#include <inttypes.h>
#include "sdn-protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#if SDN_TX_RELIABILITY
int send_ack_by_flow_address(sdnaddr_t* packet_destination, uint8_t acked_packed_type, uint8_t acked_packed_seqno);

int send_src_rtd_ack(sdnaddr_t* packet_destination, uint8_t acked_packed_type, uint8_t acked_packed_seqno);
#endif //SDN_TX_RELIABILITY

int send_src_rtd_data_flow_setup(sdnaddr_t* packet_destination, flowid_t dest_flowid, sdnaddr_t* route_nexthop, action_t action);

int send_src_rtd_control_flow_setup(sdnaddr_t* packet_destination, sdnaddr_t* route_destination, sdnaddr_t* route_nexthop, action_t action);

uint8_t sdn_send_src_rtd_phase_addr(sdnaddr_t* packet_destination, sdnaddr_t* phase_receiver, uint16_t phase);

#ifdef MANAGEMENT
int send_src_rtd_cont_mngt(sdnaddr_t* packet_destination, uint16_t metrics);
#endif

#ifdef __cplusplus
}
#endif

#endif // SDN_SEND_PACKET_CONTROLLER_H