#include <os.h>
#include <messaging.h>
#include <BlockingUART.h>
#include <joystick.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

#define LED_BLINK_DURATION 500
#define JOYSTICK_1_X 1
#define JOYSTICK_1_Y 0
#define JOYSTICK_2_X 2
#define JOYSTICK_2_Y 3
#define JOYSTICK_1_B PORTB0;
#define JOYSTICK_2_B PORTB1;

message controller_state = { 52, 0, 0, 0, 0, 0, 0 };

int x_buffer[10];
int y_buffer[10];
int x_index = 0;
int y_index = 0;

void sample_joysticks() {
    x_buffer[x_index % 5] = read_analog(JOYSTICK_1_X);
    y_buffer[y_index % 5] = read_analog(JOYSTICK_1_Y);
    x_buffer[(x_index++ % 5) + 5] = read_analog(JOYSTICK_2_X);
    y_buffer[(y_index++ % 5) + 5] = read_analog(JOYSTICK_2_Y);
    controller_state.btn1 |= (PORTB & PORTB0) ;
    controller_state.btn2 |= (PORTB & PORTB1);
}

void send_state() {
    int x = 0, y = 0;
    for(int i=0; i < 5; i++) {
        controller_state.x_pos_1 += x_buffer[i];
        controller_state.x_pos_2 += x_buffer[i+5];
        controller_state.y_pos_1 += y_buffer[i];
        controller_state.y_pos_2 += y_buffer[i+5];
    }
    controller_state.x_pos_1 /= 5;
    controller_state.x_pos_2 /= 5;
    controller_state.y_pos_1 /= 5;
    controller_state.y_pos_2 /= 5;
    send_message(controller_state);
}

int main() {
    PORTB |= _BV(PORTB6);
    setup_controllers();
    UART_Init0(19200);
    UART_Init1(19200);
    OS_Init();
    Task_Create_Period(sample_joysticks, 0, 30, 1, 0);
    Task_Create_Period(send_state, 0, 150, 5, 160);
    OS_Start();
    return 1;
}
