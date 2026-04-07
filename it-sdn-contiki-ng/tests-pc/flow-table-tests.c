#include "sdn-protocol.h"
#include "control-flow-table.h"
#include "data-flow-table.h"
#include "stdio.h"

#define ERROR_MSG(format, ...) printf("\t>>>>\tERROR\t<<<<\t (%s:%d): " format, __FILE__, __LINE__, ##__VA_ARGS__);

uint8_t flow_tables_tests() {
  sdnaddr_t addr1, addr2;
  flowid_t f1, f2;
  action_t ac;
  uint8_t ret;
  unsigned int max = 10;
  unsigned int i;

  struct control_flow_entry * cfe;
  struct data_flow_entry * dfe;

  printf("\n<start> %s\n", __func__);

  printf("Address functions tests...\n");
  // addr operations

  for (i = 0; i < sizeof(sdnaddr_t); i++) {
    sdnaddr_setbyte(&addr1, i , 0);
    sdnaddr_setbyte(&addr2, i , 0);
  }

  if (sdnaddr_cmp(&addr1, &addr2) != SDN_EQUAL) {
    ERROR_MSG("Adresses should be equal\n");
  }

  for (i = 0; i < sizeof(sdnaddr_t); i++) {
    sdnaddr_setbyte(&addr1, i , 1);
  }

  if (sdnaddr_cmp(&addr1, &addr2) == SDN_EQUAL) {
    ERROR_MSG("Adresses should not be equal\n");
  }

  sdnaddr_copy(&addr2, &addr1);

  if (sdnaddr_cmp(&addr1, &addr2) != SDN_EQUAL) {
    ERROR_MSG("  Adresses should be equal (2)\n");
  }

  // flowid operations

  f1 = f2 = 0;

  if (flowid_cmp(&f1, &f2) != SDN_EQUAL) {
    ERROR_MSG("  flowids should be equal\n");
  }

  f1 = 1;

  if (flowid_cmp(&f1, &f2) == SDN_EQUAL) {
    ERROR_MSG("  flowids should not be equal\n");
  }

  flowid_copy(&f2, &f1);

  if (flowid_cmp(&f1, &f2) != SDN_EQUAL) {
    ERROR_MSG("  flowids should be equal (2)\n");
  }

  printf("Control flow table tests...\n");

  // control flow only

  for (i = 0; i < sizeof(sdnaddr_t); i++) {
    sdnaddr_setbyte(&addr1, i , 0);
    sdnaddr_setbyte(&addr2, i , 0);
  }
  sdnaddr_setbyte(&addr1, 0, 1);
  sdnaddr_setbyte(&addr2, 0, 2);
  ac = 3;
  ret = sdn_controlflow_insert(addr1, addr2, ac);

  if (ret == SDN_SUCCESS) {
    cfe = sdn_controlflow_get(addr1);
    if (cfe != NULL && cfe->action == ac && sdnaddr_cmp(&cfe->next_hop, &addr2) == SDN_EQUAL) {
      printf("\t...get OK\n");
    } else {
      ERROR_MSG("    get NOK\n");
    }
  }

  ret = sdn_controlflow_remove(addr1);
  if (ret == SDN_SUCCESS) {
    printf("\t...remove OK\n");
  } else {
    ERROR_MSG("  remove NOK\n");
  }

  cfe = sdn_controlflow_get(addr1);
  if (cfe == NULL) {
    printf("\t...get after remove OK\n");
  } else {
    ERROR_MSG("  get after remove NOK\n");
  }

  for (i = 0; i < max; i++) {
    sdnaddr_setbyte(&addr1, 0, i);
    ret = sdn_controlflow_insert(addr1, addr2, i);
    if (ret != SDN_SUCCESS) {
      ERROR_MSG("  error in multiple insert\n");
    }
  }

  for (i = 0; i < max; i++) {
    sdnaddr_setbyte(&addr1, 0, i);
    cfe = sdn_controlflow_get(addr1);
    if (cfe == NULL || cfe->action != i || sdnaddr_cmp(&cfe->next_hop, &addr2) != SDN_EQUAL) {
      ERROR_MSG("  error in multiple get\n");
    }
  }

  for (i = 0; i < max; i++) {
    sdnaddr_setbyte(&addr1, 0, i);
    ret = sdn_controlflow_remove(addr1);
    if (ret != SDN_SUCCESS) {
      ERROR_MSG("  error in multiple remove\n");
    }
  }

  // data flow only
  printf("Data flow table tests...\n");

  for (i = 0; i < sizeof(sdnaddr_t); i++) {
    sdnaddr_setbyte(&addr1, i , 0);
    sdnaddr_setbyte(&addr2, i , 0);
  }

  sdnaddr_setbyte(&addr1, 0, 1);
  f1 = 1;
  ac = 3;
  ret = sdn_dataflow_insert(f1, addr2, ac);

  if (ret == SDN_SUCCESS) {
    dfe = sdn_dataflow_get(f1);
    if (dfe != NULL && dfe->action == ac && sdnaddr_cmp(&dfe->next_hop, &addr2) == SDN_EQUAL) {
      printf("\t...get OK\n");
    } else {
      ERROR_MSG("    get NOK\n");
    }
  }

  ret = sdn_dataflow_remove(f1);
  if (ret == SDN_SUCCESS) {
    printf("\t...remove OK\n");
  } else {
    ERROR_MSG("  remove NOK\n");
  }

  dfe = sdn_dataflow_get(f1);
  if (dfe == NULL) {
    printf("\t...get after remove OK\n");
  } else {
    ERROR_MSG("  get after remove NOK\n");
  }

  for (i = 0; i < max; i++) {
    sdnaddr_setbyte(&addr1, 1, i-1);
    f1 = i;
    ret = sdn_dataflow_insert(f1, addr1, i+1);
    if (ret != SDN_SUCCESS) {
      ERROR_MSG("  error in multiple insert\n");
    }
  }

  for (i = 0; i < max; i++) {
    sdnaddr_setbyte(&addr1, 1, i-1);
    f1 = i;
    dfe = sdn_dataflow_get(f1);
    if (dfe == NULL || (dfe->action != i+1) || sdnaddr_cmp(&dfe->next_hop, &addr1) != SDN_EQUAL) {
      ERROR_MSG("  error in multiple get\n");
    }
  }

  for (i = 0; i < max; i++) {
    f1 = i;
    ret = sdn_dataflow_remove(f1);
    if (ret != SDN_SUCCESS) {
      ERROR_MSG("  error in multiple remove\n");
    }
  }

  sdn_controlflow_clear();
  sdn_dataflow_clear();

  printf("<end> %s\n\n", __func__);

  return 0;
}

int main() {
  flow_tables_tests();

  return 0;
}