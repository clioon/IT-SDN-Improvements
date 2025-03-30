/*
* Header Here!
*/

#include <stdio.h>
#include "contiki.h"
#include "sdn-core.h"
#include "flow-table-tests.h"
#include "src-route-tests.h"
#include "string.h"
#include "dev/sht11/sht11-sensor.h"

#ifdef DEMO
#include "leds.h"
#endif

#include "sys/etimer.h"

/*---------------------------------------------------------------------------*/
PROCESS(sdn_test_process, "Contiki SDN example process");
AUTOSTART_PROCESSES(&sdn_test_process);

/*---------------------------------------------------------------------------*/
static void
receiver(uint8_t *data, uint16_t len, sdnaddr_t *source_addr, uint16_t flowId) {

  printf("Receiving message from ");
  sdnaddr_print(source_addr);
  printf(" of len %d: %s\n", len, (char*) data);

  /* Put your code here to get data received from/to application. */
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sdn_test_process, ev, data)
{
  PROCESS_BEGIN();

  sdn_init(receiver);

  static flowid_t sink = 2017;

  static struct etimer periodic_timer;
  unsigned int temperature;

  etimer_set(&periodic_timer, 20 * CLOCK_SECOND);

  static char data[10];
  static uint8_t i = 0;

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
#if PLATFORM_HAS_SHT11
    SENSORS_ACTIVATE(sht11_sensor);
#endif

    etimer_restart(&periodic_timer);

    printf("Timer reset\n");
#ifdef PLATFORM_HAS_SHT11
    temperature = sht11_sensor.value(SHT11_SENSOR_TEMP);
    sprintf(data, "%d.%d ºC", temperature / 100 - 40, (temperature / 10 - 400) % 10);
#else
    sprintf(data, "data");
#endif
    sdn_send((uint8_t*) data, 10, sink);

#if PLATFORM_HAS_SHT11
    SENSORS_DEACTIVATE(sht11_sensor);
#endif
  }

  PROCESS_END();
}
