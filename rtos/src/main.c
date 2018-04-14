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

message controller_state = { 85, 0, 0, 0, 0, 0 };


/* void sample_joysticks() { */
/*     char buffer[7]; */
/*     sprintf(buffer, "%2d,%2d\n", x, y); */
/*     int i; */
/*         for(i = 0; i < strlen(buffer); i++) { */
/*             UART_Transmit0(buffer[i]); */
/*     } */
/*     x = read_analog(JOYSTICK_1_X); */
/*     y = read_analog(JOYSTICK_1_Y); */
/* } */

struct size_test {
    char     magic:    6;
    char     btn1:     1;
    char     btn2:     1;
    uint16_t x_pos_1: 10;
    uint16_t y_pos_1: 10;
    uint16_t x_pos_2: 10;
    uint16_t y_pos_2: 10;
} __attribute__((__packed__)) ;

int main() {
    UART_Init0(19200);
    for(;;){
    UART_print0("%d\n", sizeof(struct size_test));
    _delay_ms(3000);
    }
    /*
    init_LED_D12();
    PORTB |= _BV(PORTB6);
    setup_controllers();
    UART_Init0(19200);
    UART_Init1(19200);
    _delay_ms(20);
    //Roomba_ConfigPowerLED(128, 128);
    OS_Init();
    // Task_Create_Period(joystick_task, 0, 20, 1, 2);
    Task_Create_Period(roomba_sensor_task, 0, 100, 80, 0);
    // Task_Create_Period(ir_sensor_task, 0, 50, 20, 5);
    // Task_Create_Period(swap_modes, 0, 6000, 1, 0);
    OS_Start();
    */
    return 1;
}
