#include <os.h>
#include <uart.h>
#include <joystick.h>
#include <servo.h>
#include <roomba.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

#define LED_BLINK_DURATION 500
#define MOVE_SPEED 300
#define ROTATE_SPEED 300
#define JOYSTICK_1_X 1
#define JOYSTICK_1_Y 0
#define FORWARD_GATE 800
#define BACKWARD_GATE 300
#define LEFT_GATE 800
#define RIGHT_GATE 300
uint16_t x = 0;
uint16_t y = 0;
roomba_sensor_data_t roomba_data;


void sample_joysticks() {
    char buffer[7];
    for (;;) {
        sprintf(buffer, "%2d,%2d\n", x, y);
        uart_send_string(buffer, 0);
        x = read_analog(JOYSTICK_1_X);
        y = read_analog(JOYSTICK_1_Y);
        Task_Next();
    }
}

void set_servo_positions() {
    char buffer[7];
    for (;;) {
        sprintf(buffer, "%2d,%2d\n", x, y);
        uart_send_string(buffer, 0);
        servo_set_pin3(x / 2);
        servo_set_pin2(y / 2);
        Task_Next();
    }
}

void sample_roomba_sensor_data() {
    char buffer[10];
    for (;;) {
        Roomba_UpdateSensorPacket(EXTERNAL, &roomba_data);
        sprintf(buffer, "%2d\n", roomba_data.bumps_wheeldrops);
        uart_send_string(buffer, 0);
        Task_Next();
    }
}

void control_roomba() {
    char buffer[7];
    for (;;) {
        if(x > FORWARD_GATE){
            sprintf(buffer, "forward\n");
            uart_send_string(buffer, 0);
            Roomba_Drive(1 * MOVE_SPEED, 0);
        }else if(x < BACKWARD_GATE){
            sprintf(buffer, "backward\n");
            uart_send_string(buffer, 0);
            Roomba_Drive(-1 * MOVE_SPEED, 0);
        }else if(y > LEFT_GATE){
            sprintf(buffer, "left\n");
            uart_send_string(buffer, 0);
            Roomba_DriveDirect(1 * ROTATE_SPEED, -1 * ROTATE_SPEED);
        }else if(y < RIGHT_GATE){
            sprintf(buffer, "right\n");
            uart_send_string(buffer, 0);
            Roomba_DriveDirect(-1 * ROTATE_SPEED, 1 * ROTATE_SPEED);
        }else {
            sprintf(buffer, "stop\n");
            uart_send_string(buffer, 0);
            Roomba_Drive(0, 0);
        }
        Task_Next();
    }
}

int main() {
    init_LED_D12();
    PORTB |= _BV(PORTB6);
    setup_controllers();
    uart_init(UART_19200);
    servo_init();
    Roomba_Init();
    Roomba_ChangeState(SAFE_MODE);
    _delay_ms(20);
    Roomba_ConfigPowerLED(128, 128);
    OS_Init();
    Task_Create_Period(sample_joysticks, 0, 10, 2, 0);
    Task_Create_Period(sample_roomba_sensor_data, 0, 10, 2, 3);
    Task_Create_Period(control_roomba, 0, 10, 2, 6);
    OS_Start();
    return 1;
}