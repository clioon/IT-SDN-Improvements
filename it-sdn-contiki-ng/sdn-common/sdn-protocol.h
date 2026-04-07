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
 *         Header file for the SDN protocols representation
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
 * \defgroup sdn-protocol SDN protocol
 * @{
 *
 * The sdn-protocol module is an abstract representation of protocol in
 * SDN.
 *
 */

#include <stdio.h>
#ifndef SDN_PROTOCOL_H
#define SDN_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SDN_CONTROLLER_PC
#define LINKADDR_SIZE SDNADDR_SIZE
#include <string.h>
typedef union {
  unsigned char u8[LINKADDR_SIZE];
} linkaddr_t;
#include <stdint.h>
#else //SDN_ENABLED_NODE || SDN_CONTROLLER_NODE
#include "linkaddr.h"
#define SDNADDR_SIZE LINKADDR_SIZE
#endif

#ifdef RDC_CONF_UNIDIR_SUPPORT
#define RDC_UNIDIR_SUPPORT RDC_CONF_UNIDIR_SUPPORT
#else
#define RDC_UNIDIR_SUPPORT 0
#endif

#define SDN_DEFAULT_TTL 60

/**
 * \brief	The type for SDN address of the nodes.
 */
typedef linkaddr_t sdnaddr_t;

/**
 * \brief  The type definition of flow id.
 */
typedef uint16_t flowid_t;

/**
 * \brief  The type definition of action.
 */
typedef uint8_t action_t;

#define SDN_ACTION_RECEIVE  1
#define SDN_ACTION_FORWARD  2
#define SDN_ACTION_DROP     4
#define SDN_ACTION_FORWARD_UNIDIR  8

/**
 * \brief  SDN related events.
 */
#define SDN_EVENT_ND_CHANGE   0x70
#define SDN_EVENT_CD_FOUND    0x71
#define SDN_EVENT_NEW_PACKET  0x72

/**
 * \brief  default flows.
 */
#define SDN_CONTROLLER_FLOW 0
#define SDN_NULL_FLOW 0xFFFF

extern flowid_t sdn_controller_flow;

/**
 * \brief  The enum type to identify types of SDN packets.
 */
enum sdn_packet_type {
  SDN_PACKET_CONTROL_FLOW_SETUP = 0,
  SDN_PACKET_DATA_FLOW_SETUP = 1,
  SDN_PACKET_CONTROL_FLOW_REQUEST = 2,
  SDN_PACKET_DATA_FLOW_REQUEST = 3,
  SDN_PACKET_NEIGHBOR_REPORT = 4,
  SDN_PACKET_DATA = 5,
  SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP = 6 + 0xE0,
  SDN_PACKET_SRC_ROUTED_DATA_FLOW_SETUP = 7 + 0xE0,
  SDN_PACKET_MULTIPLE_CONTROL_FLOW_SETUP = 8 + 0xD0,
  SDN_PACKET_MULTIPLE_DATA_FLOW_SETUP = 9 + 0xD0,
  SDN_PACKET_ND = 10,
  SDN_PACKET_CD = 11,
  SDN_PACKET_ACK_BY_FLOW_ID = 12,
  SDN_PACKET_SRC_ROUTED_ACK = 13,
  SDN_PACKET_REGISTER_FLOWID = 14,
  //SDN_PACKET_ENERGY_REPORT = 15,
  SDN_PACKET_ACK_BY_FLOW_ADDRESS = 16,
  SDN_PACKET_MNGT_CONT_SRC_RTD = 17,
  SDN_PACKET_MNGT_NODE_DATA = 18,
  SDN_PACKET_GENERIC_FLOW = 0x70,
  SDN_PACKET_GENERIC_SRC_ROUTED = 0x71,
  SDN_PACKET_GENERIC_ADDR = 0x72,
  SDN_PACKET_PHASE_INFO = 0x73,
};

typedef uint8_t sdn_packet_t;

/**
 * \brief  The enum type to identify management metrics of SDN.
 */
enum sdn_mngt_metric_id {
  SDN_MNGT_METRIC_BATTERY = 1,
  SDN_MNGT_METRIC_QTY_DATA = 2,
  SDN_MNGT_METRIC_QUEUE_DELAY = 4,
};

typedef uint16_t sdn_mngt_metric_id_t;

/**
 * \brief  The struct to define header types of SDN packets.
 */
struct sdn_header {
  uint8_t       type;
  uint8_t       reserved; //For compatibility with rime packets.
  uint8_t       thl;
  uint8_t       seq_no;
  sdnaddr_t     source;

} __attribute__((packed));

typedef struct sdn_header sdn_header_t;

struct sdn_control_flow_setup {
  sdn_header_t  header;
  sdnaddr_t     destination;  /* This packet destination */
  sdnaddr_t     route_destination; /* Route to change in the destination flow table */
  uint8_t       action_id;
  sdnaddr_t     action_parameter; /* Next hop parameter */

} __attribute__((packed));

typedef struct sdn_control_flow_setup sdn_control_flow_setup_t;

struct sdn_src_rtd_control_flow_setup {
  sdn_control_flow_setup_t  flow_setup;
  sdnaddr_t real_destination;
  uint8_t path_len;
#if RDC_UNIDIR_SUPPORT
  uint64_t unidir_list;
#endif
  // list of address (for src routing, reverse ordered)
} __attribute__((packed));

typedef struct sdn_src_rtd_control_flow_setup sdn_src_rtd_control_flow_setup_t;

struct sdn_data_flow_setup {
  sdn_header_t  header;
  sdnaddr_t     destination;
  flowid_t      flowid;
  uint8_t       action_id;
  sdnaddr_t     action_parameter;

} __attribute__((packed));

typedef struct sdn_data_flow_setup sdn_data_flow_setup_t;

struct sdn_src_rtd_data_flow_setup {
  sdn_data_flow_setup_t  flow_setup;
  sdnaddr_t real_destination;
  uint8_t path_len;
#if RDC_UNIDIR_SUPPORT
  uint64_t unidir_list;
#endif
  // list of address (for src routing, reverse ordered)
} __attribute__((packed));

typedef struct sdn_src_rtd_data_flow_setup sdn_src_rtd_data_flow_setup_t;

struct sdn_mult_data_flow_setup {
  sdn_header_t  header;
  sdnaddr_t     destination;
  flowid_t      flowid;
  uint8_t       path_len;
  uint8_t       set_len;
  // list of set flows [action_id, action_parameter] (reverse ordered)
  // list of address (for src routing, reverse ordered)
} __attribute__((packed));

typedef struct sdn_mult_data_flow_setup sdn_mult_data_flow_setup_t;

struct sdn_mult_control_flow_setup {
  sdn_header_t  header;
  sdnaddr_t     destination;
  sdnaddr_t     route_destination;
  uint8_t       path_len;
  uint8_t       set_len;
  // list of set flows [action_id, action_parameter] (reverse ordered)
  // list of address (for src routing, reverse ordered)
} __attribute__((packed));

typedef struct sdn_mult_control_flow_setup sdn_mult_control_flow_setup_t;

struct sdn_mult_flow_elem {
  uint8_t       action_id;
  sdnaddr_t     action_parameter;
} __attribute__((packed));

typedef struct sdn_mult_flow_elem sdn_mult_flow_elem_t;

struct sdn_control_flow_request {
  sdn_header_t  header;
  flowid_t      dest_flowid;
  sdnaddr_t     address;

} __attribute__((packed));

typedef struct sdn_control_flow_request sdn_control_flow_request_t;

struct sdn_data_flow_request {
  sdn_header_t  header;
  flowid_t      dest_flowid;
  flowid_t      flowid;

} __attribute__((packed));

typedef struct sdn_data_flow_request sdn_data_flow_request_t;

struct sdn_neighbor_report {
  sdn_header_t  header;
  flowid_t      flowid;
  uint8_t       num_of_neighbors;

} __attribute__((packed));

typedef struct sdn_neighbor_report sdn_neighbor_report_t;

struct sdn_energy_report {
  sdn_header_t  header;
  flowid_t      flowid;
  uint8_t       energy;

} __attribute__((packed));

typedef struct sdn_energy_report sdn_energy_report_t;

struct sdn_neighbor_report_list {
  sdnaddr_t     neighbor;
  uint16_t      etx;

} __attribute__((packed));

typedef struct sdn_neighbor_report_list sdn_neighbor_report_list_t;

struct sdn_data {
  sdn_header_t  header;
  flowid_t      flowid;

} __attribute__((packed));

typedef struct sdn_data sdn_data_t;

struct sdn_ack_by_flow_id {
  sdn_header_t  header;
  flowid_t      dest_flowid;

} __attribute__((packed));

typedef struct sdn_ack_by_flow_id sdn_ack_by_flow_id_t;

struct sdn_ack_by_flow_address {
  sdn_header_t  header;
  sdnaddr_t     destination;
  uint8_t       acked_packed_type;
  uint8_t       acked_packed_seqno;

} __attribute__((packed));

typedef struct sdn_ack_by_flow_address sdn_ack_by_flow_address_t;

typedef struct {
  sdn_header_t  header;
  sdnaddr_t     destination;
  uint8_t       acked_packed_type;
  uint8_t       acked_packed_seqno;
  sdnaddr_t     real_destination;
  uint8_t       path_len;
#if RDC_UNIDIR_SUPPORT
  uint64_t      unidir_list;
#endif
  // list of address (for src routing, reverse ordered)
} __attribute__((packed)) sdn_src_rtd_ack_t;

struct sdn_src_rtd_cont_mngt {
  sdn_header_t  header;
  sdnaddr_t     destination;
  sdnaddr_t     real_destination;
  uint8_t       path_len;
  uint16_t      mngt_request;
#if RDC_UNIDIR_SUPPORT
  uint64_t      unidir_list;
#endif
} __attribute__((packed));

typedef struct sdn_src_rtd_cont_mngt sdn_src_rtd_cont_mngt_t;

struct sdn_flow_id_mngt_node_data {
  sdn_header_t  header;
  flowid_t      flowid;
  uint16_t      mngt_metrics;
} __attribute__((packed));

typedef struct sdn_flow_id_mngt_node_data sdn_flow_id_mngt_node_data_t;

struct sdn_mngt_metric{
  uint32_t metric_value;
} __attribute__((packed));

typedef struct sdn_mngt_metric sdn_mngt_metric_t;

typedef struct {
  sdn_header_t  header;
  sdnaddr_t     destination;  /* This packet destination */
  uint8_t       port;
  /* data */
} __attribute__((packed)) sdn_generic_addr_t;

typedef struct {
  sdn_generic_addr_t  sdn_generic_addr_t;
  sdnaddr_t           real_destination;
  uint8_t             path_len;
#if RDC_UNIDIR_SUPPORT
  uint64_t            unidir_list;
#endif
  /* data */
  // list of address (for src routing, reverse ordered)
} __attribute__((packed)) sdn_generic_src_rtd_t;

typedef struct {
  sdn_header_t  header;
  flowid_t      flowid;
  uint8_t       port;
  /* data */
} __attribute__((packed)) sdn_generic_flow_t;

typedef struct {
  sdn_generic_flow_t  generic_flow;
  sdnaddr_t           phase_sender;
  uint16_t            phase;
  /* data */
} __attribute__((packed)) sdn_phase_info_flow_t;

typedef struct {
  sdn_generic_addr_t  generic_flow;
  sdnaddr_t           phase_receiver;
  uint16_t            phase;
  /* data */
} __attribute__((packed)) sdn_phase_info_addr_t;

typedef struct {
  sdn_generic_src_rtd_t generic_flow;
  sdnaddr_t             phase_receiver;
  uint16_t              phase;
  /* data */
} __attribute__((packed)) sdn_phase_info_src_rtd_t;

/**
 * \brief  Macro that returns true if a packet is routed by flowid.
 * \param packet: a pointer to the SDN packet
 */
#define SDN_ROUTED_BY_FLOWID(packet) ( (((sdn_header_t*) packet)->type) == SDN_PACKET_CONTROL_FLOW_REQUEST || \
                                       (((sdn_header_t*) packet)->type) == SDN_PACKET_DATA_FLOW_REQUEST || \
                                       (((sdn_header_t*) packet)->type) == SDN_PACKET_NEIGHBOR_REPORT || \
                                       (((sdn_header_t*) packet)->type) == SDN_PACKET_DATA || \
                                       (((sdn_header_t*) packet)->type) == SDN_PACKET_ACK_BY_FLOW_ID || \
                                       (((sdn_header_t*) packet)->type) == SDN_PACKET_REGISTER_FLOWID || \
                                       (((sdn_header_t*) packet)->type) == SDN_PACKET_MNGT_NODE_DATA || \
                                       (((sdn_header_t*) packet)->type) == SDN_PACKET_GENERIC_FLOW )

/**
* \brief  Macro that returns true if a packet is routed by address.
* \param packet: a pointer to the SDN packet
*/
#define SDN_ROUTED_BY_ADDR(packet) ( (((sdn_header_t*) packet)->type) == SDN_PACKET_CONTROL_FLOW_SETUP || \
                                     (((sdn_header_t*) packet)->type) == SDN_PACKET_ACK_BY_FLOW_ADDRESS || \
                                     (((sdn_header_t*) packet)->type) == SDN_PACKET_DATA_FLOW_SETUP || \
                                     (((sdn_header_t*) packet)->type) == SDN_PACKET_GENERIC_ADDR )

/**
* \brief  Macro that returns true if a packet is source routed.
* \param packet: a pointer to the SDN packet
*/
#define SDN_ROUTED_BY_SRC(packet) ( (((sdn_header_t*) packet)->type) == SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP || \
                                    (((sdn_header_t*) packet)->type) == SDN_PACKET_SRC_ROUTED_DATA_FLOW_SETUP || \
                                    (((sdn_header_t*) packet)->type) == SDN_PACKET_MULTIPLE_CONTROL_FLOW_SETUP || \
                                    (((sdn_header_t*) packet)->type) == SDN_PACKET_MULTIPLE_DATA_FLOW_SETUP || \
                                    (((sdn_header_t*) packet)->type) == SDN_PACKET_SRC_ROUTED_ACK || \
                                    (((sdn_header_t*) packet)->type) == SDN_PACKET_MNGT_CONT_SRC_RTD || \
                                    (((sdn_header_t*) packet)->type) == SDN_PACKET_GENERIC_SRC_ROUTED )

/**
* \brief  Macro that returns true if a packet should not be route (i.e. ND or CD).
* \param packet: a pointer to the SDN packet
*/
#define SDN_ROUTED_NOT(packet) ( (((sdn_header_t*) packet)->type) == SDN_PACKET_ND || \
                                 (((sdn_header_t*) packet)->type) == SDN_PACKET_CD )

/**
* \brief  Returns a pointer to the packet flowid.
*         It should be checked with SDN_ROUTED_BY_FLOWID before
* \param packet: a pointer to the SDN packet
*/
#define SDN_GET_PACKET_FLOW(packet)  ( *((flowid_t*) &((uint8_t *)packet)[sizeof(sdn_header_t)]) )

/**
* \brief  Returns a pointer to the packet address.
*         It should be checked with SDN_ROUTED_BY_ADDR before
* \param packet: a pointer to the SDN packet
*/
#define SDN_GET_PACKET_ADDR(packet)  ( *((sdnaddr_t*) &((uint8_t *)packet)[sizeof(sdn_header_t)]) )

/**
* \brief Returns a pointer to real destination of merged src routed packets.
*         Packet type should be checked beforehand
* \param packet: a pointer to the SDN packet
*/
sdnaddr_t *sdn_get_real_dest_from_merged_packet(uint8_t *packet);

/**
* \brief Returns a pointer to real destination of not merged src routed packets.
*         Packet type should be checked beforehand
* \param packet: a pointer to the SDN packet
*/
#define SDN_GET_PACKET_REAL_DEST(packet)  \
( \
((sdnaddr_t*) \
  ( ( (((sdn_header_t*) packet)->type) == SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP ) ? \
    &SDN_PACKET_GET_FIELD(packet, sdn_src_rtd_control_flow_setup_t, real_destination) : \
    ( \
      ( (((sdn_header_t*) packet)->type) == SDN_PACKET_SRC_ROUTED_DATA_FLOW_SETUP ) ? \
      &SDN_PACKET_GET_FIELD(packet, sdn_src_rtd_data_flow_setup_t, real_destination) : \
        ( \
          ( (((sdn_header_t*) packet)->type) == SDN_PACKET_SRC_ROUTED_ACK ) ? \
          &SDN_PACKET_GET_FIELD(packet, sdn_src_rtd_ack_t, real_destination) : \
            ( \
              ( (((sdn_header_t*) packet)->type) == SDN_PACKET_MNGT_CONT_SRC_RTD ) ? \
              &SDN_PACKET_GET_FIELD(packet, sdn_src_rtd_cont_mngt_t, real_destination) : \
                ( \
                  ( (((sdn_header_t*) packet)->type) == SDN_PACKET_GENERIC_SRC_ROUTED ) ? \
                  &SDN_PACKET_GET_FIELD(packet, sdn_generic_src_rtd_t, real_destination) : \
                    NULL \
                ) \
            ) \
        ) \
    ) \
  ) \
) )\

/**
* \brief  Gets the subpackets length of a source routed merged packet (does not include the length of the header and the destination)
*         Packet type should be checked beforehand
* \param ptr: a pointer to the SDN packet
*/
size_t sdn_get_src_rtd_merged_subpackets_len(uint8_t *packet);

/**
* \brief  Gets a certain packet field.
*         It should be checked beforehand that the pointer is to the correct \
*           packet type.
* \param ptr: a pointer to the SDN packet
* \param pkt_type: the struct that represents the packet (definded in this file)
* \param field: the wanted field
*/
#define SDN_PACKET_GET_FIELD(ptr, pkt_type, field) ( ((pkt_type *)ptr)->field )

/**
* \brief  Gets the current (i.e. last) sdn_mult_flow_elem_t from a mult packet.
* \param ptr: a pointer to the SDN packet
* \param pkt_type: sdn_mult_control_flow_setup_t or sdn_mult_data_flow_setup_t
*/
#define SDN_PACKET_GET_FLOW_ELEM(ptr, pkt_type) \
  ( (sdn_mult_flow_elem_t*) ( packet_ptr \
                              + sizeof(pkt_type) \
                              + sizeof(sdn_mult_flow_elem_t) \
                              * SDN_PACKET_GET_FIELD( packet_ptr, \
                                                      pkt_type, \
                                                      set_len) \
                            ) \
  )

/**
* \brief  Gets the [index] element of an sdnaddr_t array.
*         Used by other macros.
* \param addr_ptr: a pointer to the array. Will be casted to (sdnaddr_t *)
* \param index: array position
*/
#define SDN_GET_ADDR_IN_ARRAY(addr_ptr, index) \
  ( (sdnaddr_t*) (addr_ptr) ) [index]

/**
* \brief  Gets the current (i.e. last) sdnaddr_t from
*           a multiple packet src route header.
* \param ptr: a pointer to the SDN packet
* \param pkt_type: sdn_mult_control_flow_setup_t or sdn_mult_data_flow_setup_t
*/
#define SDN_PACKET_GET_NEXT_SRC_ADDR_MULT(ptr, pkt_type) \
  SDN_GET_ADDR_IN_ARRAY( \
    packet_ptr \
      + sizeof(pkt_type) \
      + sizeof(sdn_mult_flow_elem_t) \
      * SDN_PACKET_GET_FIELD(packet_ptr, pkt_type, set_len), \
    SDN_PACKET_GET_FIELD(packet_ptr, pkt_type, path_len) \
  )

/**
* \brief  Gets the current (i.e. last) sdnaddr_t from
*           a packet src route header.
* \param ptr: a pointer to the SDN packet
* \param pkt_type: sdn_src_rtd_control_flow_setup_t, sdn_src_rtd_data_flow_setup_t
*/
#define SDN_PACKET_GET_NEXT_SRC_ADDR(ptr, pkt_type) \
  sdn_packet_get_next_src_addr((uint8_t *)(ptr), sizeof(pkt_type))
 
sdnaddr_t *sdn_packet_get_next_src_addr(uint8_t *p, size_t pkt_type_size);

/**
* \brief  Macro that returns true is packet is merged (reserved == 1)
* \param packet_ptr: a pointer to the SDN packet
*/
#define SDN_PACKET_IS_MERGED(packet_ptr) ( ((sdn_header_t *) packet_ptr)->reserved & 0x1 )

/**
* \brief  Macro that sets the packet as merged (reserved == 1)
* \param packet_ptr: a pointer to the SDN packet
*/
#define SDN_PACKET_SET_MERGED(packet_ptr) ( ((sdn_header_t *) packet_ptr)->reserved |= 0x1 )

/**
* \brief  Macro that sets the packet as not merged (reserved == 0)
* \param packet_ptr: a pointer to the SDN packet
*/
#define SDN_PACKET_SET_NOT_MERGED(packet_ptr) ( ((sdn_header_t *) packet_ptr)->reserved &= ~0x1 )

/**
* \brief  Macro that sets the packet as merged (reserved == 1)
* \param packet_ptr: a pointer to the SDN packet
* \param header_size: length of all the fields before the number of subpacket byte (ex: sdn_header_t + sdnaddr_t)
*/
#define SDN_GET_NUM_SUBPACKETS(packet_ptr, header_size) (SDN_PACKET_IS_MERGED(packet_ptr) ? (packet_ptr)[header_size] : 1)

/**
* \brief  Returns a pointer to the packet header.
* \param packet_ptr: a pointer to the SDN packet
*/
#define SDN_HEADER(packet_ptr) ((sdn_header_t *) packet_ptr)

#ifdef ENABLE_SDN_TREATMENT
static inline uint8_t sdn_get_header_size(uint8_t *packet) {
  uint8_t type = SDN_HEADER(packet)->type;

  switch(type) {
    case SDN_PACKET_CONTROL_FLOW_SETUP:
    case SDN_PACKET_DATA_FLOW_SETUP:
    case SDN_PACKET_ACK_BY_FLOW_ADDRESS:
    case SDN_PACKET_SRC_ROUTED_ACK:
    case SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP:
    case SDN_PACKET_SRC_ROUTED_DATA_FLOW_SETUP:
      return sizeof(sdn_header_t) + sizeof(sdnaddr_t);

    case SDN_PACKET_CONTROL_FLOW_REQUEST:
    case SDN_PACKET_DATA_FLOW_REQUEST:
    case SDN_PACKET_REGISTER_FLOWID:
    case SDN_PACKET_ACK_BY_FLOW_ID:
      return sizeof(sdn_header_t) + sizeof(flowid_t);

    case SDN_PACKET_DATA:
      return sizeof(sdn_data_t);

    case SDN_PACKET_NEIGHBOR_REPORT:
      return sizeof(sdn_header_t);

    default:
      return sizeof(sdn_header_t);
  }
}
#endif

#ifdef __cplusplus
}
#endif

#endif //SDN_PROTOCOL_H

/** @} */
/** @} */
/** @} */
