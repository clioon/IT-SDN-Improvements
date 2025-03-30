#ifndef MANAGE_INFO_H
#define MANAGE_INFO_H

#include <stdint.h>

void manag_update(uint32_t time_in, uint8_t operation);

uint8_t manag_get_info(uint8_t metric_id);

#endif //MANAGE_INFO_H
