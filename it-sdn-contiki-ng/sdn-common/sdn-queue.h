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

/**
 * \file
 *         ...
 * \author
 *         Doriedson A. G. O. <doliveira@larc.usp.br>
 *         Renan C. A. Alves <ralves@larc.usp.br>
 */

/**
 * \addtogroup SDN
 * @{
 */

/**
* \addtogroup SDN-common
* @{
*/

/**
 * \defgroup sdn-q SDN Queue
 * @{
 *
 *
 */

#ifndef SDN_QUEUE_H
#define SDN_QUEUE_H

#include "sdn-protocol.h"
#include "sdn-addr.h"

#ifdef SDN_ENABLED_NODE
#ifdef MANAGEMENT
#include "contiki.h"
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t *data;
  uint16_t len;
  sdnaddr_t last_next_hop;
#ifdef SDN_CONTROLLER_PC
  uint8_t msg_type;
#endif
#ifdef SDN_CONTROLLER_NODE
  action_t action;
#endif
#if defined (MANAGEMENT) && defined (SDN_ENABLED_NODE)
  clock_time_t time;
#endif
} sdn_send_queue_data_t;

typedef struct {
  uint8_t *data;
  uint16_t len;
#if defined (MANAGEMENT) && defined (SDN_ENABLED_NODE)
  clock_time_t time;
#endif

} sdn_recv_queue_data_t;

void sdn_send_queue_init();

uint8_t sdn_send_queue_empty();

uint8_t sdn_send_queue_size();

uint8_t sdn_send_queue_maxSize();

sdn_send_queue_data_t* sdn_send_queue_head();

sdn_send_queue_data_t* sdn_send_queue_tail();

sdn_send_queue_data_t* sdn_send_queue_dequeue();

uint8_t sdn_send_queue_enqueue(uint8_t *data, uint16_t len, uint32_t time);

sdn_send_queue_data_t* sdn_send_queue_find_by_type(uint8_t type);

#ifdef SDN_CONTROLLER_PC
uint8_t sdn_send_queue_enqueue_custom(uint8_t *data, uint16_t len, uint32_t time, uint8_t msg_type);
#endif

void sdn_send_queue_print();

void sdn_recv_queue_init();

uint8_t sdn_recv_queue_empty();

uint8_t sdn_recv_queue_size();

uint8_t sdn_recv_queue_maxSize();

sdn_recv_queue_data_t* sdn_recv_queue_head();

sdn_recv_queue_data_t* sdn_recv_queue_dequeue();

uint8_t sdn_recv_queue_enqueue(uint8_t *data, uint16_t len);

void sdn_recv_queue_print();

//

#ifdef ENABLE_SDN_TREATMENT
extern volatile uint8_t sdn_processing_send_queue;
#endif

void imprimir(uint8_t *p, uint16_t len, uint8_t header_size);

uint16_t sdn_merged_new_length(uint8_t *p, uint8_t header_size);

uint8_t sdn_queue_combine_packets(uint8_t *p1, uint16_t p1_len, uint8_t pos, uint8_t header_size, uint8_t queue);

void sdn_get_src_routing_info(uint8_t *p, uint16_t p_len, uint8_t header_size, sdnaddr_t *real_dest, uint8_t *path_len, sdnaddr_t **path);

uint8_t sdn_queue_combine_src_packets(uint8_t *p1, uint16_t p1_len, uint8_t pos, uint8_t header_size, uint8_t queue);

uint8_t sdn_queue_replace_old_packet(uint8_t *data, uint16_t len, uint8_t pos, uint8_t header_size, uint8_t queue);

uint8_t sdn_get_routing_dest(uint8_t *p, uint8_t **dest, size_t *dest_len);

uint8_t  sdn_queue_delete_subpacket(uint8_t *p, uint16_t *p_len, uint8_t subpacket_num, uint8_t header_size, uint8_t p_pos, uint8_t queue);

uint8_t sdn_queue_compare_packets(uint8_t *p1, uint16_t p1_len, uint8_t *p2, uint16_t p2_len, uint8_t header_size, uint8_t compare_size, uint8_t p2_pos, uint8_t queue, uint8_t same_src);

uint8_t sdn_queue_determine_pckt_action(uint8_t *p1, uint8_t *p2, uint16_t p1_len, uint16_t p2_len, uint8_t *header_size, uint8_t p2_pos, uint8_t queue, uint8_t is_src_routed);

uint8_t sdn_queue_process_new_packet(uint8_t *new_packet, uint16_t new_len, uint8_t queue);

uint8_t sdn_is_final_destination(uint8_t* packet);

//

#ifdef __cplusplus
}
#endif

#endif /* SDN_QUEUE_H */

/** @} */
/** @} */
/** @} */
