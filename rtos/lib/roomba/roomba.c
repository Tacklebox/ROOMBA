/*
 * roomba.c
 *
 *  Created on: 4-Feb-2009
 *      Author: nrqm
 */

#include <util/delay.h>
#include "../RoombaUART/RoombaUART.h"
#include "roomba.h"
#include "roomba_sci.h"
#include "sensor_struct.h"
#include "../os/os.h"

#define LOW_BYTE(v)   ((unsigned char) (v))
#define HIGH_BYTE(v)  ((unsigned char) (((unsigned int) (v)) >> 8))

#define DD_DDR DDRC
#define DD_PORT PORTC
#define DD_PIN PC5
#define ROOMBA_UART 1
#define BT_UART 2
#define TIMEOUT_MS 50

ROOMBA_STATE state = SAFE_MODE;

void Roomba_Init()
{
	ROOMBA_DD_DDR |= 1<<ROOMBA_DD_PIN;
	ROOMBA_DD_PORT &= ~(1<<ROOMBA_DD_PIN);

	// At 8 MHz, the AT90 generates a 57600 bps signal with a framing error rate of over 2%, which means that more than
	// 1 out of every 50 bits is wrong.  The fastest bitrate with a low error rate that the Roomba supports is
	// 38400 bps (0.2% error rate, or 1 bit out of every 500).

	// Try 57.6 kbps to start (this is the Roomba's default baud rate after the battery is installed).
	Roomba_UART_Init(UART_115200);

	// Try to start the SCI
	Roomba_Send_Byte(START);
	_delay_ms(20);

	// change the baud rate to 38400 bps.  Have to wait for 100 ms after changing the baud rate.
	Roomba_Send_Byte(BAUD);
	Roomba_Send_Byte(ROOMBA_19200BPS);
	_delay_ms(100);		// this delay will not work on old versions of WinAVR (new versions will see reduced but
						// still acceptable resolution; see _delay_ms definition)

	// change the AT90's UART clock
	Roomba_UART_Init(UART_19200);

	// start the SCI again in case the first start didn't go through.
	Roomba_Send_Byte(START);
	_delay_ms(20);

	// finally put the Roomba into safe mode.
	Roomba_Send_Byte(SAFE);
	_delay_ms(20);
}


void Roomba_Finish() {
	Roomba_Send_Byte(STOP);
}

void Roomba_UpdateSensorPacket(ROOMBA_SENSOR_GROUP group, roomba_sensor_data_t* sensor_packet)
{

	Roomba_Send_Byte(SENSORS);
	Roomba_Send_Byte(group);
	switch(group)
	{
	case EXTERNAL:
		while (uart_bytes_received() != 10);
		sensor_packet->bumps_wheeldrops = uart_get_byte(0);
		sensor_packet->wall = uart_get_byte(1);
		sensor_packet->cliff_left = uart_get_byte(2);
		sensor_packet->cliff_front_left = uart_get_byte(3);
		sensor_packet->cliff_front_right = uart_get_byte(4);
		sensor_packet->cliff_right = uart_get_byte(5);
		sensor_packet->virtual_wall = uart_get_byte(6);
		sensor_packet->motor_overcurrents = uart_get_byte(7);
		sensor_packet->dirt_left = uart_get_byte(8);
		sensor_packet->dirt_right = uart_get_byte(9);
		break;
	case CHASSIS:
		// chassis sensors
		while (uart_bytes_received() != 6);
		sensor_packet->remote_opcode = uart_get_byte(0);
		sensor_packet->buttons = uart_get_byte(1);
		sensor_packet->distance.bytes.high_byte = uart_get_byte(2);
		sensor_packet->distance.bytes.low_byte = uart_get_byte(3);
		sensor_packet->angle.bytes.high_byte = uart_get_byte(4);
		sensor_packet->angle.bytes.low_byte = uart_get_byte(5);
		break;
	case INTERNAL:
		// internal sensors
		while (uart_bytes_received() != 10);
		sensor_packet->charging_state = uart_get_byte(0);
		sensor_packet->voltage.bytes.high_byte = uart_get_byte(1);
		sensor_packet->voltage.bytes.low_byte = uart_get_byte(2);
		sensor_packet->current.bytes.high_byte = uart_get_byte(3);
		sensor_packet->current.bytes.low_byte = uart_get_byte(4);
		sensor_packet->temperature = uart_get_byte(5);
		sensor_packet->charge.bytes.high_byte = uart_get_byte(6);
		sensor_packet->charge.bytes.low_byte = uart_get_byte(7);
		sensor_packet->capacity.bytes.high_byte = uart_get_byte(8);
		sensor_packet->capacity.bytes.low_byte = uart_get_byte(9);
		break;
	case LIGHT_BUMPERS:
		while (uart_bytes_received() != 12);
		sensor_packet->light_left.bytes.high_byte = uart_get_byte(0);
		sensor_packet->light_left.bytes.low_byte = uart_get_byte(1);
		sensor_packet->light_front_left.bytes.high_byte = uart_get_byte(2);
		sensor_packet->light_front_left.bytes.low_byte = uart_get_byte(3);
		sensor_packet->light_center_left.bytes.high_byte = uart_get_byte(4);
		sensor_packet->light_center_left.bytes.low_byte = uart_get_byte(5);
		sensor_packet->light_center_right.bytes.high_byte = uart_get_byte(6);
		sensor_packet->light_center_right.bytes.low_byte = uart_get_byte(7);
		sensor_packet->light_front_right.bytes.high_byte = uart_get_byte(8);
		sensor_packet->light_front_right.bytes.low_byte = uart_get_byte(9);
		sensor_packet->light_right.bytes.high_byte = uart_get_byte(10);
		sensor_packet->light_right.bytes.low_byte = uart_get_byte(11);
		break;
	}
	uart_reset_receive();
}

void Roomba_ChangeState(ROOMBA_STATE newState)
{
	if (newState == SAFE_MODE)
	{
		if (state == PASSIVE_MODE)
			UART_Transmit1(CONTROL);
		else if (state == FULL_MODE)
			UART_Transmit1(SAFE);
	}
	else if (newState == FULL_MODE)
	{
		Roomba_ChangeState(SAFE_MODE);
		UART_Transmit1(FULL);
	}
	else if (newState == PASSIVE_MODE)
	{
		UART_Transmit1(POWER);
	}
	else
	{
		// already in the requested state
		return;
	}

	state = newState;
	_delay_ms(20);
}

//Used to stop the Roomba when it gets shot.
void Roomba_Drive( int16_t velocity, int16_t radius )
{
	Roomba_Send_Byte(DRIVE);
	Roomba_Send_Byte(HIGH_BYTE(velocity));
	Roomba_Send_Byte(LOW_BYTE(velocity));
	Roomba_Send_Byte(HIGH_BYTE(radius));
	Roomba_Send_Byte(LOW_BYTE(radius));
}

// Set the Roomba's main LED color,intensity and stuff
void Roomba_LED(int8_t led_bits,int8_t color, int8_t intensity)
{
	Roomba_Send_Byte(LEDS);
	Roomba_Send_Byte(led_bits);
	Roomba_Send_Byte(color);
	Roomba_Send_Byte(intensity);
}

uint8_t Roomba_OI_Mode(){
    Roomba_Send_Byte(SENSORS);
    Roomba_Send_Byte(35); // 35 is OI_MODE packet id
    while( uart_bytes_received() != 1);
    uint8_t rs = uart_get_byte(0);
    uart_reset_receive();
    return rs;
}

void Roomba_Query_List(int8_t* requested_packets, int8_t request_len,
                        int8_t* output_data,int8_t data_len)
{
    if( requested_packets == 0 || output_data == 0){
        return;
    }

    // send down the command
    Roomba_Send_Byte(149); // Query List Command OPCODE
    Roomba_Send_Byte(request_len);
    int i = 0;
    for(i = 0;i < request_len;++i)
    {
        Roomba_Send_Byte(requested_packets[i]);
    }

    // retrieve the data bytes out of the uart.
    while(uart_bytes_received() != data_len);
    for(i = 0; i < data_len; ++i)
    {
        output_data[i] = uart_get_byte(i);
    }
    uart_reset_receive();
}

uint8_t Roomba_LightBumperDetection(){
    Roomba_Send_Byte(SENSORS); // 45 is the Light Bumper Sensor pakcet Id
    Roomba_Send_Byte(45); // 45 is the Light Bumper Sensor pakcet Id
    while( uart_bytes_received() != 1);
    uint8_t rs = uart_get_byte(0);
    uart_reset_receive();
    return rs;
}
