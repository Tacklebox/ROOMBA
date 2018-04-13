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
    char buffer[7];
    sprintf(buffer, "%2d,%2d\n", x, y);
    int i;
        for(i = 0; i < strlen(buffer); i++) {
            UART_Transmit0(buffer[i]);
    }
    x = read_analog(JOYSTICK_1_X);
    y = read_analog(JOYSTICK_1_Y);
}

void process_joysticks() {
    char buffer[7];
    if(x > FORWARD_GATE){
        sprintf(buffer, "forward\n");
        int i;
        for(i = 0; i < strlen(buffer); i++) {
            UART_Transmit0(buffer[i]);
        }
        Roomba_Drive(1 * MOVE_SPEED, 0);
    }else if(x < BACKWARD_GATE){
        sprintf(buffer, "backward\n");
        int i;
      for(i = 0; i < strlen(buffer); i++) {
            UART_Transmit0(buffer[i]);
        }
        Roomba_Drive(-1 * MOVE_SPEED, 0);
    }else if(y > LEFT_GATE){
        sprintf(buffer, "left\n");
        int i;
      for(i = 0; i < strlen(buffer); i++) {
            UART_Transmit0(buffer[i]);
        }
        Roomba_DriveDirect(1 * ROTATE_SPEED, -1 * ROTATE_SPEED);
    }else if(y < RIGHT_GATE){
        sprintf(buffer, "right\n");
        int i;
      for(i = 0; i < strlen(buffer); i++) {
            UART_Transmit0(buffer[i]);
        }
        Roomba_DriveDirect(-1 * ROTATE_SPEED, 1 * ROTATE_SPEED);
    }else {
        sprintf(buffer, "stop\n");
        int i;
      for(i = 0; i < strlen(buffer); i++) {
            UART_Transmit0(buffer[i]);
        }
        Roomba_Drive(0, 0);
    }
}

void joystick_task() {
    char buffer[7];
    for (;;) {
        sprintf(buffer, "%2d,%2d\n", x, y);
        int i;
        for(i = 0; i < strlen(buffer); i++) {
            UART_Transmit0(buffer[i]);
        }
        x = read_analog(JOYSTICK_1_X);
        y = read_analog(JOYSTICK_1_Y);
        Task_Next();
    }
}

void set_servo_positions() {
    char buffer[7];
    for (;;) {
        sprintf(buffer, "%2d,%2d\n", x, y);
        int i;
      for(i = 0; i < strlen(buffer); i++) {
            UART_Transmit0(buffer[i]);
        }
        servo_set_pin3(x / 2);
        servo_set_pin2(y / 2);
        Task_Next();
    }
}

void sample_roomba_sensor_data() {
    char buffer[10];
    Roomba_UpdateSensorPacket(EXTERNAL, &roomba_data);
    sprintf(buffer, "t%2d\n", roomba_data.bumps_wheeldrops);
    int i;
    for(i = 0; i < strlen(buffer); i++) {
        UART_Transmit0(buffer[i]);
    }
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
        sprintf(buffer, "%2d\n", roomba_data.bumps_wheeldrops);
        int i;
      for(i = 0; i < strlen(buffer); i++) {
            UART_Transmit0(buffer[i]);
        }
        Task_Next();
    }
}

void die(){
    char buffer[10];
    Roomba_ConfigPowerLED(0, 255);
    sprintf(buffer, "die\n");
    uart_send_string(buffer, 0);
    for(;;){};
}

void process_ir_sensor(){
    char buffer[10];
    sprintf(buffer, "%d\n", cur_ir - avg_ir);
    uart_send_string(buffer, 0);
    if((cur_ir - avg_ir) > IR_WINDOW_SIZE){
        sprintf(buffer, "addie\n");
        uart_send_string(buffer, 0);
        Task_Create_System(die, 0);
    }
}

void ir_sensor_task() {
    char buffer[10];
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
        sprintf(buffer, "%d, %d\n", avg_ir, cur_ir);
        int i;
      for(i = 0; i < strlen(buffer); i++) {
            UART_Transmit0(buffer[i]);
        }
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
    // Task_Create_Period(joystick_task, 0, 20, 1, 2);
    Task_Create_Period(roomba_sensor_task, 0, 100, 80, 0);
    // Task_Create_Period(ir_sensor_task, 0, 50, 20, 5);
    // Task_Create_Period(swap_modes, 0, 6000, 1, 0);
    OS_Start();
    return 1;
}
