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
#include <sdn-addr.h>


#define RECEIVER_DELAY (2 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
PROCESS(sdn_test_process, "Contiki SDN example process");
AUTOSTART_PROCESSES(&sdn_test_process);

/*---------------------------------------------------------------------------*/
static void
receiver(uint8_t *data, uint16_t len, sdnaddr_t *source_addr, uint16_t flowId) {
  //printf("receiver");
  SDN_DEBUG("Receiving message from ");
  sdnaddr_print(source_addr);
  SDN_DEBUG(" of len %d: %s\n", len, (char*) data);

  /* Put your code here to get data received from application. */
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sdn_test_process, ev, data) {

  PROCESS_BEGIN();
  
  sdn_init(receiver);
  
  static struct etimer periodic_timer;
  static struct etimer start_timer;
  
  etimer_set(&periodic_timer, 1 * CLOCK_SECOND);
  etimer_set(&start_timer, 40 * CLOCK_SECOND);
  
  if (sdn_node_addr.u8[0] == 2) {
  
    sdn_register_flowid(2018);
    
  }

  
  
  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&start_timer));
    etimer_restart(&start_timer);
    etimer_set(&start_timer, 0 * CLOCK_SECOND);
  
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    
    etimer_restart(&periodic_timer);
    
    if (sdn_node_addr.u8[0] == 3 || sdn_node_addr.u8[0] == 4 || sdn_node_addr.u8[0] == 5) {
    
      //printf("sender");
    
      //char *data = "Hello World!\n";
      char *data4 = "Goodbye World!";
      //char *data3 = "Test 3";
      //char *data2 = "Test 4";

      //sdn_send((uint8_t *)data, strlen(data), 2018);
      //sdn_send((uint8_t *)data, strlen(data), 2018);
      
      //sdn_send((uint8_t *)data3, strlen(data3), 2018);
      //sdn_send((uint8_t *)data4, strlen(data4), 2018);
      sdn_send((uint8_t *)data4, strlen(data4), 2018);
  
    }
    
  }
  
  PROCESS_END();
  
}
