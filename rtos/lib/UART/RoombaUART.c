#include "RoombaUART.h"

#define UART_BUFFER_SIZE    32

// ATMega2560 has 4 USART interfaces
// UART0 is the line that is used by the USB plug??
// UART1 is the interface that we are using for this one...


static volatile uint8_t uart_buffer[UART_BUFFER_SIZE];
static volatile uint8_t uart_buffer_index;

void Roomba_Send_Byte(uint8_t data_out){
	// UDREn -UART Data Register Empty flag
	// UCSRnA  - UART Control and Status register

	// poll the uart status register until it says that
	// the data register is empty and ready to use
	// UDREn it is set/1 when the transmit buffer is empty,
	// else it is cleared/0.
	while(! (UCSR1A & (1<<UDRE1)));

	// Put the data into the register in order to be sent out.
	// writing to this register clears the UDREn
	UDR1 = data_out;
}

void Roomba_UART_Init(UART_BPS baud){
	uint8_t sreg = SREG;
	cli();

	// Make sure I/O clock to USART1 is enabled
	// PRR<n> - Power Reduction Register 1
	// PRUSART<n> - Power Reduction USART 1
	// write a 1 to shut this down
	// 0 to turn on??
	PRR1 &= ~(1 << PRUSART1);

	// Set baud rate to 19.2k at fOSC = 16 MHz
	// UBBR<n> USART Baud Rate Register
	// UBBR<n>[H|L] - USART Baud Rate Register [High | Low]
	// From the manual
	//	 BAUD = fOSC / (16 * UBBR<n> + 1)
	//	 UBBR<n> = (fOSC / (16* BAUD) )-1
	// 	16 - async normal mode  ( U2Xn = 0)
	// 	8 - async double speed  ( U2Xn = 1)
	// 	2 - sync master mode
	// For arbitrary baud rates...
	// 16 assumes normal async mode
	// uint16_t baud_rate = (F_CPU/16/target_baud - 1);
	// UBBR0H = (uint8_t) baud_rate >> 8;
	// UBBR0H = (uint8_t) baud_rate;

	switch(baud) {
		case UART_19200:
			UBRR1 = 51;
			break;
		case UART_38400:
			UBRR1 = 25;
			break;
		case UART_57600:
			UBRR1 = 16;
			break;
		case UART_115200:
			UBRR1 = 8;
			break;
		default:
			// 58 823??
			UBRR1 = 16;
	}

	// Clear USART Transmit complete flag, normal USART transmission speed
	UCSR1A = (1 << TXC1) | (0 << U2X1);

	// Enable receiver, transmitter, and rx complete interrupt.
	// Enable transmission
	UCSR1B = (1<<TXEN1) | ( 1<< RXEN1) | (1 << RXCIE1);

	// 8-bit data
	UCSR1C = ((1<<UCSZ11)|(1<<UCSZ10));

	// disable 2x speed
	UCSR1A &= ~(1<<U2X1);

	SREG = sreg;
}

uint8_t uart_bytes_received(void)
{
	return uart_buffer_index;
}

void uart_reset_receive(void)
{
	uart_buffer_index = 0;
}

// uint8_t UART_recv() {
// 	// check the status register until the RC1 flag is cleared
// 	while( !(UCSR1A & ( 1 << RXC1)));
// 	// retrieve the value from the data register
// 	return UDR1;
// }

/**
 * UART receive byte ISR
 */
ISR(USART1_RX_vect)
{
	// FEn - frame error
	// DORn - data overrun
	// UPEn - uart pairty error

    uart_buffer[uart_buffer_index] = UDR1;
    uart_buffer_index = (uart_buffer_index + 1) % UART_BUFFER_SIZE;
}

uint8_t uart_get_byte(int index)
{
	if (index < UART_BUFFER_SIZE)
	{
		return uart_buffer[index];
	}
	return 0;
}


// UDRn - uart io data register n
// 	shared by both the tranmission and receiving

// UCSRnA
// 	[RXCn,TXCn,UDREn,FEn,DORn,UPEn, U2Xn, MPCMn]

// 	RXCn - receive complete flag
// 	TXCn - transmission complete flag. check that all trans are complete
// 	UDREn - uart data register empty flag
// 	FEn - frame erro flag
// 	DORn - data overrun flag
// 	UPEn - uart parity error
// 	U2Xn - flag to set the transmission mode
// 			0 - normal async
// 			1 - double time async

// UCSRnB
// 	RXCIEn - receive complete interrupt enable flag
// 	TXCIEn - TX complete interrupt enable flag
// 	UDRIEn - uart data register empty interrupt enable flag
// 	RXEn - enable ( set high)  the Uart to do receiving
// 	TXEn - enables (set high) the uart to do transmit
// 	UCSZn2 - character size n
// 		uses this with UCSZn1:0 to set the number of bits in a frame
// 	RXB8n - Receive data bit 8
// 	TXB8n - transmit data bit 8

// UCSRnC
// 	UMSELn1,UMSELn0  - Select eh uart mode of operation
// 	00 - async uart
// 	01 - sync uart
// 	10 - (reserved)
// 	11 - master spi

// 	UPMn1,UPMn0 - uart parity mode
// 	00 - disabled
// 	01 - reserved
// 	10 - enabled, even parity
// 	11 - enabled, odd parity

// 	USBSn - uart stop bit select
// 	0 - 1 stop bit
// 	1 - 2 stop bits

// 	UCSZn1 - character size n
// 		use this with the UCZn2 in UCSRnB register to determine the number
// 			of bits in the frame

//     UCPOLn - uart clock parity

// UCSZn2,UCSZn1,UCSZn0
// 000 - 5 bits
// 001 - 6 bits
// 010 - 7 bits
// 011 - 8 bits
// 100
// 101
// 110
// 111 - 9 bits

// TXEN1 -- enable the transmission flag
// RXEN1 -- enable the receive flag
// RXCIEn -- receive control interrupt flag
// TXCIEn -- allow tranmit interrupt
//UCSR1B = (1<<TXEN1) | ( 1<< RXEN1) | (1 << RXCIE1)

// UDRIEn -- UART Data Register Empty Interrupt Enable into UCSRnB
//  This will allow the data register empty interrupt to run...

// need to set the USART tranmission mode ( i.e async normal mode)
// baud rate
// frame format
// enabling the tranmsission and receiver

// UCSR<n><ABC> -- USART Control and Status Register
// UCSR<n>A
// UCSR<n>B
// UCSR<n>C

// RXEN<n> = enable receiver
// TXEN<n> = enable transmitter
// TXCn - transmit complete ( set one when)
// RXCn - receiver register C

// set the UCSR1A - trans mode, clear the tran register
// set the UCSR1B - enable transmite and receive
// set the UCSR1C - data rate
