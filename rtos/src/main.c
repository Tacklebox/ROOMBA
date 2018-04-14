#include <os.h>
#include <messaging.h>
#include <BlockingUART.h>
#include <joystick.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

#define LED_BLINK_DURATION 500
#define MOVE_SPEED 300
#define ROTATE_SPEED 300
#define JOYSTICK_1_X 1
#define JOYSTICK_1_Y 0
#define IR_SENSOR 7
#define FORWARD_GATE 800
#define BACKWARD_GATE 300
#define LEFT_GATE 800
#define RIGHT_GATE 300
#define IR_WINDOW_SIZE 5

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
    controller_state.btn1 |= 
}

void send_state() {
}

int main() {
    PORTB |= _BV(PORTB6);
    setup_controllers();
    UART_Init0(19200);
    UART_Init1(19200);
    _delay_ms(20);
    OS_Init();
    Task_Create_Period(sample_joysticks, 0, 30, 1, 2);
    //Task_Create_Period(roomba_sensor_task, 0, 100, 80, 0);
    // Task_Create_Period(ir_sensor_task, 0, 50, 20, 5);
    // Task_Create_Period(swap_modes, 0, 6000, 1, 0);
    OS_Start();
    return 1;
}
