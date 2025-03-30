/*
* Header Here!
*/

#include <stdio.h>
#include "contiki.h"
#include "sdn-core.h"
// #include "flow-table-tests.h"
// #include "src-route-tests.h"
#include "sdn-debug.h"
#include "string.h"
#include "sdn-constants.h"

#include "leds.h"

#include "sys/etimer.h"

#ifndef SDN_SIMULATION_N_SINKS
#define SDN_SIMULATION_N_SINKS 1
#endif

/*---------------------------------------------------------------------------*/
PROCESS(sdn_sink_process, "Contiki SDN example process");
AUTOSTART_PROCESSES(&sdn_sink_process);

sdnaddr_t red_node;
sdnaddr_t green_node;
sdnaddr_t blue_node;

/*---------------------------------------------------------------------------*/
static void
receiver(uint8_t *data, uint16_t len, sdnaddr_t *source_addr, uint16_t flowId) {

  SDN_DEBUG("Receiving message from ");
  sdnaddr_print(source_addr);
  SDN_DEBUG(" of len %d: %s\n", len, (char*) data);

  if(sdnaddr_cmp(&red_node, &sdnaddr_null) == SDN_EQUAL) {
    sdnaddr_copy(&red_node, source_addr);
    leds_toggle(LEDS_RED);
    return;
  } else {
    if(sdnaddr_cmp(&red_node, source_addr) == SDN_EQUAL) {
      leds_toggle(LEDS_RED);
      return;
    }
  }

  if(sdnaddr_cmp(&green_node, &sdnaddr_null) == SDN_EQUAL) {
    sdnaddr_copy(&green_node, source_addr);
    leds_toggle(LEDS_GREEN);
    return;
  } else {
    if(sdnaddr_cmp(&green_node, source_addr) == SDN_EQUAL) {
      leds_toggle(LEDS_GREEN);
      return;
    }
  }

  if(sdnaddr_cmp(&blue_node, &sdnaddr_null) == SDN_EQUAL) {
    sdnaddr_copy(&blue_node, source_addr);
    leds_toggle(LEDS_BLUE);
    return;
  } else {
    if(sdnaddr_cmp(&blue_node, source_addr) == SDN_EQUAL) {
      leds_toggle(LEDS_BLUE);
      return;
    }
  }
  /* Put your code here to get data received from/to application. */
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sdn_sink_process, ev, data)
{
  PROCESS_BEGIN();

  printf("#A color=GREEN\n");

  sdn_init(receiver);

  static flowid_t flowid;

  flowid = 2017 + (sdn_node_addr.u8[0] % SDN_SIMULATION_N_SINKS);

  sdn_register_flowid(flowid);

  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
