#include <os.h>
#include <BlockingUART.h>
#include <RoombaUART.h>
#include <joystick.h>
#include <servo.h>
#include <roomba.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "../lib/roomba/roomba_sci.h"

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

typedef enum mode_type {
    CRUISE = 1,
    STAND_STILL,
    DEAD
} mode;

uint16_t x = 0;
uint16_t y = 0;
uint16_t avg_ir = 0;
uint16_t cur_ir = 0;
roomba_sensor_data_t roomba_data;
mode cur_mode = CRUISE;

void sample_joysticks() {
    UART_print0("%2d,%2d\n", x, y);
    x = read_analog(JOYSTICK_1_X);
    y = read_analog(JOYSTICK_1_Y);
}

void process_joysticks() {
    if(x > FORWARD_GATE){
        UART_print0("forward\n");
        Roomba_Drive(1 * MOVE_SPEED, 0);
    }else if(x < BACKWARD_GATE){
        UART_print0("backward\n");
        Roomba_Drive(-1 * MOVE_SPEED, 0);
    }else if(y > LEFT_GATE){
        UART_print0("left\n");
        Roomba_DriveDirect(1 * ROTATE_SPEED, -1 * ROTATE_SPEED);
    }else if(y < RIGHT_GATE){
        UART_print0("right\n");
        Roomba_DriveDirect(-1 * ROTATE_SPEED, 1 * ROTATE_SPEED);
    }else {
        UART_print0("stop\n");
        Roomba_Drive(0, 0);
    }
}

void joystick_task() {
    for (;;) {
        UART_print0( "%2d,%2d\n", x, y);
        x = read_analog(JOYSTICK_1_X);
        y = read_analog(JOYSTICK_1_Y);
        Task_Next();
    }
}

void set_servo_positions() {
    for (;;) {
        UART_print0("%2d,%2d\n", x, y);
        servo_set_pin3(x / 2);
        servo_set_pin2(y / 2);
        Task_Next();
    }
}

void sample_roomba_sensor_data() {
    Roomba_UpdateSensorPacket(EXTERNAL, &roomba_data);
    UART_print0("t%2d\n", roomba_data.bumps_wheeldrops);
}

void backup(){
    Roomba_Drive(1 * MOVE_SPEED, 0);
    _delay_ms(500);
    Roomba_Drive(0, 0);
}

void process_roomba_sensor_data() {
    if(roomba_data.virtual_wall || roomba_data.wall){
        roomba_data.virtual_wall = 0;
        roomba_data.wall = 0;
        //Task_Create_System(backup, 0);
    }
}

void roomba_sensor_task(){  
    for (;;) {
        sample_roomba_sensor_data();
        process_roomba_sensor_data();
        Roomba_UpdateSensorPacket(EXTERNAL, &roomba_data);
        // UART_print0("%2d\n", roomba_data.bumps_wheeldrops);
        Task_Next();
    }
}

void die(){
    // Roomba_ConfigPowerLED(0, 255);
    UART_print0("die\n");
    for(;;){};
}

void process_ir_sensor(){
    if((cur_ir - avg_ir) > IR_WINDOW_SIZE){
        UART_print0("Going in to die\n");
        Task_Create_System(die, 0);
    }
}

void ir_sensor_task() {
    uint16_t i = 0;
    int size = 0;
    uint16_t val_buffer[IR_WINDOW_SIZE];
    for (;;){
        cur_ir = read_analog(IR_SENSOR);
        val_buffer[i] = cur_ir;
        i = (i + 1) % IR_WINDOW_SIZE;
        if(size < IR_WINDOW_SIZE){
            size++;
        }
        uint16_t sum = 0;
        int index;
        for(index = 0; index < size; index++){
            sum += val_buffer[index];
        }
        avg_ir = sum / size;
        UART_print0("IR %d, %d\n", avg_ir, cur_ir);
        Task_Next();        
    }
}

void swap_modes() {
    for (;;) {
        if(cur_mode == CRUISE){
            cur_mode = STAND_STILL;
        }else{
            cur_mode = CRUISE;
        }
        Task_Next();
    }
}

void test() {
    for(;;){
        UART_print0("Test\n");
        Task_Next();
    }
}

int main() {
    init_LED_D12();
    PORTB |= _BV(PORTB6);
    setup_controllers();
    UART_Init0(19200);
    UART_Init1(19200);
    servo_init();
    Roomba_Init();
    Roomba_ChangeState(SAFE_MODE);
    _delay_ms(20);
    //Roomba_ConfigPowerLED(128, 128);
    OS_Init();
    // Task_Create_Period(test, 0, 20, 1, 0);
    // Task_Create_Period(joystick_task, 0, 20, 1, 2);
    Task_Create_Period(roomba_sensor_task, 0, 100, 80, 0);
    // Task_Create_Period(ir_sensor_task, 0, 50, 20, 5);
    // Task_Create_Period(swap_modes, 0, 6000, 1, 0);
    OS_Start();
    return 1;
}
