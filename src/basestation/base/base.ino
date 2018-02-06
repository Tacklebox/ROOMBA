/* Project: src/ROOMBA */

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Servo.h>

#define LCD_SIZE 16

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int JoyStick_X = A8;
int JoyStick_Y = A9;
int JoyStick_Z = 22;
int pin_servo_a = 7;
int pin_servo_b = 6;
int max_val = 1023;

Servo servo_a;
Servo servo_b;


void setup() {
    pinMode (JoyStick_X, INPUT);   
    pinMode (JoyStick_Y, INPUT);
    pinMode (JoyStick_Z, INPUT_PULLUP);
    pinMode (pin_servo_a, OUTPUT);   
    pinMode (pin_servo_b, OUTPUT);
    lcd.begin(16, 2);
    Serial.begin(9600);
    Serial1.begin(9600);
    servo_a.attach(pin_servo_a);
    servo_b.attach(pin_servo_b);
}

void loop() {
    int x, y, z;
    x = analogRead(JoyStick_X);
    y = analogRead(JoyStick_Y);
    z = digitalRead(JoyStick_Z);

    char buf[LCD_SIZE];
    Serial.println(buf);
    sprintf(buf, "%d, %d, %d", x, y, z);
    lcd.setCursor(0,0);
    lcd.print(buf);
    int pos_a = map(val, 0, max_val, 30, 120);
    int pos_b = map(val, 0, max_val, 0, 180);
    servo_a.write(40);
    servo_b.write(60);
}   
