#include "manage-info.h"
#include "sdn-debug.h"
#include "sdn-protocol.h"
#include "contiki.h"

clock_time_t time_in, time_queue;

static uint8_t qty =0;
static unsigned long delay=0;

void manag_update(uint32_t time_in, uint8_t operation) {

  clock_time_t time_send =  clock_time();

  if (operation == 1) //if is sending
    qty = qty + 1;
  delay = delay + (time_send - time_in);

  if (qty > 50){
    qty = 1;
    delay = (time_send - time_in);
  }

  printf ("Manag Update - delay: %lu, timein: %lu, time send: %lu, qtd: %d \n", delay, time_in, time_send, qty);
}

uint8_t manag_get_info(uint8_t metric_id) {

  uint8_t ret = 0;

  if (metric_id == SDN_MNGT_METRIC_QTY_DATA) {

    ret = qty;

  } else if (metric_id == SDN_MNGT_METRIC_QUEUE_DELAY) {

    ret = delay;
  }

  return ret;
  //send info
}
