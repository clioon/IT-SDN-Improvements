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
#include "lib/random.h"

#ifdef DEMO
#include "leds.h"
#endif

#include "sys/etimer.h"

#ifndef SDN_SIMULATION_N_SINKS
#define SDN_SIMULATION_N_SINKS 1
#endif

#define SENSING_AT_SECONDS 60

/*---------------------------------------------------------------------------*/
PROCESS(sdn_test_process, "Contiki SDN example process");
AUTOSTART_PROCESSES(&sdn_test_process);

/*---------------------------------------------------------------------------*/
static void
receiver(uint8_t *data, uint16_t len, sdnaddr_t *source_addr, uint16_t flowId) {

  SDN_DEBUG("Receiving message from ");
  sdnaddr_print(source_addr);
  SDN_DEBUG(" of len %d: %s\n", len, (char*) data);

  /* Put your code here to get data received from application. */
}

/* Use to defines either node is a sensor node and sends packets to sink or only router node and forward packets. */
static uint8_t isSensing() {

  uint8_t sensing = 0;

  sensing = 1;

  return sensing;
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sdn_test_process, ev, data)
{
  PROCESS_BEGIN();

  printf("SENSING AT SECONDS = %i\n", SENSING_AT_SECONDS);

  sdn_init(receiver);

  static flowid_t sink;

  sink = 2017 + (sdn_node_addr.u8[0] % SDN_SIMULATION_N_SINKS);

  printf("SDN_SIMULATION_N_SINKS %d\n", SDN_SIMULATION_N_SINKS);

  static struct etimer periodic_timer;

#ifdef SDN_HARDCODED_ROUTES
  int x, y, x_next_hop, y_next_hop;
  sdnaddr_t next_hop;
  int grid_side = 5;

  x = (sdn_node_addr.u8[0] - 1)/ grid_side + 1;
  y = (sdn_node_addr.u8[0] - 1) % grid_side + 1;

  printf("x:%d y:%d\n", x, y);

  if (y == 1) {
    y_next_hop = 1;
    if (x < (grid_side + 1) / 2) {
      x_next_hop = x + 1;
    } else {
      x_next_hop = x - 1;
    }
  } else {
    y_next_hop = y - 1;
    x_next_hop = x;
  }
  // x_sink = (grid_side + 1) / 2
  // y_sink = 1
  memset(&next_hop, 0 , sizeof(sdnaddr_t));
  sdnaddr_setbyte(&next_hop, 0, (x_next_hop-1) * grid_side + y_next_hop);
  sdn_dataflow_insert(sink, next_hop, SDN_ACTION_FORWARD);
#endif

  if(isSensing() == 1) {
    printf("Sensing data node.\n");
    printf("#A color=GREEN\n");
  } else {
    printf("#A color=BLUE\n");
  }

  etimer_set(&periodic_timer, (120 + random_rand() % SENSING_AT_SECONDS) * CLOCK_SECOND) ;

  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

  if(isSensing() == 1) {
    etimer_set(&periodic_timer, SENSING_AT_SECONDS * CLOCK_SECOND);
  }

  static char data[10];
  static uint8_t i = 0;

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    for(int j = 0; j < 3; j++) {
      sprintf(data, "teste %d", i++);
      SDN_DEBUG("Sending data to flow %d\n", sink);
      sdn_send((uint8_t*) data, 10, sink);
    }

    etimer_restart(&periodic_timer);

  // #ifdef DEMO
  //   leds_toogle(LEDS_BLUE);
  // #endif


    // if (sdn_node_addr.u8[0] == 1) {
    //   if (i%12 == 1)
    //     test_send_src_rt_csf();
//         test_send_src_rt_csf_incomplete();
//       if (i%12 == 3)
//         test_send_src_rt_dsf();
//         test_send_src_rt_dsf_incomplete();
//       if (i%12 == 7)
//         test_send_mult_dsf();
//       if (i%12 == 11)
//         test_send_mult_csf();
    // }
  }

  PROCESS_END();
}
