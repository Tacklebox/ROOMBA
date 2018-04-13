/*
 * uart.h
 *
 * Created: 25/01/2015 12:53:55 PM
 *  Author: Daniel
 */


#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#define UART_BUFFER_SIZE    32

typedef enum _uart_bps
{
	UART_19200,
	UART_38400,
	UART_57600,
	UART_115200,
	UART_DEFAULT, // what baud rate is this?
} UART_BPS;

// seems like we should refactor into a Roomba UART
void Roomba_UART_Init(UART_BPS baud);
// send out a single byte over the uart
void Roomba_Send_Byte(uint8_t data_out);

// how many bytes are in the received buffer
uint8_t uart_bytes_received(void);

// reset the buffer index to 0
void uart_reset_receive(void);

// retrieve the byte stored at 'index' from
// the received buffer array
uint8_t uart_get_byte(int index);

#endif /* UART_H_ */