#include "types.h"
#include "defs.h"
#include "e1000.h"

int sys_transmit(void) {
  int length;
  char* buf;

  if (argint(1, &length) < 0) return -1;
  if (argptr(0, &buf, length) < 0) return -1;

  e1000_transmit(buf, length);

  return 0;
}

int sys_receive(void) {
  int length;
  char* buf;

  if (argint(1, &length) < 0) return -1;
  if (argptr(0, &buf, length) < 0) return -1;

  return e1000_receive(buf, length);
}

int sys_get_mac(void) {
  char* buf;

  if (argptr(0, &buf, 6) < 0) return -1;

  e1000_get_mac((uint8_t*)buf);

  return 0;
}

int sys_get_ip(void) {
  char* buf;

  if (argptr(0, &buf, 4) < 0) return -1;

  e1000_get_ip((uint8_t*)buf);

  return 0;
}