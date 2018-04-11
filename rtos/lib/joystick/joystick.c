#include "joystick.h"


void setup_controllers()
{
    /* We use a single ADC onboard the package, using an
       onbaord multiplexer to select one of the 16 available
       analog inputs. Joystck channels are connected as follows
         0 -  3 Controller 1.  0/ 1 Left X/Y,  2/ 3 Right X/Y.  
         4 -  7 Controller 2.  4/ 5 Left X/Y,  6/ 7 Right X/Y.  
         8 - 11 Controller 3.  8/ 9 Left X/Y, 10/11 Right X/Y.  
        12 - 15 Controller 4. 12/13 Left X/Y, 14/15 Right X/Y.

       Each Controller also features a single push button connected
       to digital inputs as follows, all pins are on PORTC bits 0 to 3
        37 - PORTC0 - Controller 1 : Button A
        36 - PORTC1 - Controller 2 : Button A
        35 - PORTC2 - Controller 3 : Button A
        34 - PORTC3 - Controller 4 : Button A
    */
	
	/* Configure PORTC to received digital inputs for bits 0 to 3 */
	DDRC = (DDRC & 0xF0);
	
	/* Configure Analog Inputs using ADC */
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Set ADC prescalar to 128 - 125KHz sample rate @ 16MHz

    ADMUX |= (1 << REFS0); // Set ADC reference to AVCC
    ADMUX |= (1 << ADLAR); // Left adjust ADC result to allow easy 8 bit reading

    ADCSRA |= (1 << ADEN);  // Enable ADC
	ADCSRA |= (1 << ADSC); //Start a conversion to warmup the ADC.
	
	//We're using the ADC in single Conversion mode, so this is all the setup we need. 
}

/**
 * Read an analog value from a given channel. 
 * On the AT mega2560, there are 16 available channels, thus
 * channel can be any value 0 to 15, which will correspond
 * to the analog input on the arduino board. 
 */
uint16_t read_analog(uint16_t channel)
{
	/* We're using Single Ended input for our ADC readings, this requires some
	 * work to correctly set the mux values between the ADMUX and ADCSRB registers. 
	 * ADMUX contains the four LSB of the multiplexer, while the fifth bit is kept
	 * within the ADCSRB register. Given the specifications, we want to keep the 
	 * three least significant bits as is, and check to see if the fourth bit is set, if it
	 * is, we need to set the mux5 pin. 
	 */
	
	/* Set the three LSB of the Mux value. */
	/* Caution modifying this line, we want MUX4 to be set to zero, always */
    ADMUX = (ADMUX & 0xF0 ) | (0x07 & channel); 
	/* We set the MUX5 value based on the fourth bit of the channel, see page 292 of the 
	 * ATmega2560 data sheet for detailed information */
	ADCSRB = (ADCSRB & 0xF7) | (channel & (1 << MUX5));
	
    /* We now set the Start Conversion bit to trigger a fresh sample. */
    ADCSRA |= (1 << ADSC);
    /* We wait on the ADC to complete the operation, when it completes, the hardware
       will set the ADSC bit to 0. */
    while ((ADCSRA & (1 << ADSC)));
    /* We setup the ADC to shift input to left, so we simply return the High register. */

	int lowADC = ADCL;
	int highADC = ADCH;
	return (lowADC>>6) | (highADC<<2);
}