#include <os.h>
#include <uart.h>
#include <joystick.h>
#include <servo.h>
#include <roomba.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

#define LED_BLINK_DURATION 500
uint16_t x = 0;
uint16_t y = 0;
roomba_sensor_data_t roomba_data;

void SampleJoystick() {
    for (;;) {
        x = read_analog(0);
        y = read_analog(1);
        Task_Next();
    }
}

void SetServoPositions() {
    char buffer[7];
    for (;;) {
        sprintf(buffer, "%2d,%2d\n", x, y);
        uart_send_string(buffer, 0);
        servo_set_pin3(x / 2);
        servo_set_pin2(y / 2);
        Task_Next();
    }
}

void SampleRoombaData() {
    char buffer[10];
    for (;;) {
        Roomba_UpdateSensorPacket(EXTERNAL, &roomba_data);
        sprintf(buffer, "%2d\n", roomba_data.bumps_wheeldrops);
        uart_send_string(buffer, 0);
        Task_Next();
    }
}

void ControlRoomba() {
    for (;;) {
        if(x > 512){
            Roomba_Drive(1, 0);
        }else{
            Roomba_Drive(-1, 0);
        }
    }
}

int main() {
    init_LED_D12();
    PORTB |= _BV(PORTB6);
    setup_controllers();
    uart_init(UART_19200);
    servo_init();
    Roomba_Init();
    // roomba_data.bumps_wheeldrops = 0;
    Roomba_ChangeState(SAFE_MODE);
    // Roomba_ConfigPowerLED(0, 125);
    // char buffer[10];
    // Roomba_UpdateSensorPacket(EXTERNAL, &roomba_data);
    Roomba_Drive(10, 5);
    // sprintf(buffer, "%2d\n", roomba_data.bumps_wheeldrops);
    // uart_send_string(buffer, 0);


    OS_Init();
    Task_Create_Period(SampleJoystick, 0, 50, 5, 0);
    //Task_Create_Period(ControlRoomba, 0, 50, 5, 10);
    //Task_Create_Period(SetServoPositions, 0, 50, 5, 10);
    //Task_Create_Period(SampleRoombaData, 0, 50, 25, 20);
    OS_Start();
    return 1;
}