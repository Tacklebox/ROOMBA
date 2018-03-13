#include <avr/io.h>
#include "error.h"

#define BUSY_TIME 32000

void init_error() {
  DDRB |= _BV(DDB7);
}
void sos(int code) {
  int i;
  volatile int j;
  for (i = 0;i<code;i++) {
    PORTB |= _BV(PORTB7);
    for (j = 0;j<BUSY_TIME;j++) {/*busywait*/}
    PORTB &= ~_BV(PORTB7);
    for (j = 0;j<BUSY_TIME;j++) {/*busywait*/}
  }
}
