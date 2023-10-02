
#include "ihex1802.h"
#include "1802.h"
#include <cstdio>

// Intel hex stuff

int ihex1802::getch(void) {
  int c;
  while ((c = getch()) == -1)
    ;
  return c;
}

void ihex1802::setmem(uint16_t a, uint8_t d) {
  ram[a & MAXMEM] = d;
}

int ihexo1802::putch(int c) {
  putchar((uint8_t)c);
  return 0;
}
