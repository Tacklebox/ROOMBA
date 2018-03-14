#include <avr/io.h>
#include "LED_Test.h"

void init_LED_D10(void)
{
  DDRB |= _BV(DDB4);
  PORTB = 0x00;
}

void init_LED_D11(void)
{
  DDRB |= _BV(DDB5);
  PORTB = 0x00;
}

void init_LED_D12(void)
{
  DDRB |= _BV(DDB6);
  PORTB = 0x00;
}

void init_LED_D13(void)
{
  DDRB |= _BV(DDB7);
  PORTB = 0x00;
}

void enable_LED(unsigned int mask)
{
	PORTB = mask;
}

void disable_LEDs(void)
{
		PORTB = 0x00;
}
