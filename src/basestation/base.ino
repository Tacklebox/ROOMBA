/* Project: src/ROOMBA */

#include <Arduino.h>
#include <LiquidCrystal.h>

#define LCD_SIZE 16

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int JoyStick_X = A8;
int JoyStick_Y = A9;
int JoyStick_Z = 22;

void setup() {
    pinMode (JoyStick_X, INPUT);   
    pinMode (JoyStick_Y, INPUT);
    pinMode (JoyStick_Z, INPUT_PULLUP);
    lcd.begin(16, 2);
    Serial1.begin(9600);
}

void loop() {
    int x, y, z;
    x = analogRead(JoyStick_X);
    y = analogRead(JoyStick_Y);
    z = digitalRead(JoyStick_Z);
    char buf[LCD_SIZE];
    sprintf(buf, "%d, %d, %d \n", x, y, z);
    lcd.setCursor(0,0);
    lcd.print(buf);
    Serial1.print(buf);
}   