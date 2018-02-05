/* Project: src/ROOMBA */

#include <Arduino.h>
#include <LiquidCrystal.h>
#include "../command.h"

#define LCD_SIZE 16

const char ETX = 0x03;

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int JoyStick_X = A8;
int JoyStick_Y = A9;
int JoyStick_Z = 22;


void setup() {
    pinMode (JoyStick_X, INPUT);   
    pinMode (JoyStick_Y, INPUT);
    pinMode (JoyStick_Z, INPUT_PULLUP);
    lcd.begin(16, 2);
    Serial.begin(9600);
    Serial1.begin(9600);
}

void send_packet(packet_t pack){
    Serial1.write((char *) &pack, sizeof(pack));
}

void loop() {
    int x, y, z;
    x = analogRead(JoyStick_X);
    y = analogRead(JoyStick_Y);
    z = digitalRead(JoyStick_Z);

    packet_t pack{ .cmd = TEST, .param = 5 };
    send_packet(pack);

    char buf[LCD_SIZE];
    Serial.println(buf);
    sprintf(buf, "%d, %d, %d", sizeof(pack), pack.cmd, pack.param);
    lcd.setCursor(0,0);
    lcd.print(buf);
}   