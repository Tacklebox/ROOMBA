#include "error.h"
#include "LED_Test.h"
#include <avr/io.h>
#include <util/delay.h>

#define Disable_Interrupt() asm volatile("cli" ::)

void throw_error(error_code err) {
    Disable_Interrupt();
    init_LED_D12();

    while (1) {
        int i;
        for (i = 0; i < err; i++) {
            PORTB |= _BV(PORTB6);
            _delay_ms(500);
            PORTB ^= _BV(PORTB6);
            _delay_ms(500);
        }
        _delay_ms(2000);
    }
}