/* Project: src/ROOMBA */

#include <Arduino.h>
#include <Servo.h>

#define LCD_SIZE 16

int JoyStick_X = A8;
int JoyStick_Y = A9;
int JoyStick_Z = 22;
int pin_laser = 24;
int pin_servo_a = 7;
int pin_servo_b = 6;
int max_val = 1023;

Servo servo_a;
Servo servo_b;


void setup() {
    pinMode (JoyStick_X, INPUT);   
    pinMode (JoyStick_Y, INPUT);
    pinMode (pin_laser, OUTPUT);
    pinMode (JoyStick_Z, INPUT_PULLUP);
    
    servo_a.attach(pin_servo_a);
    servo_b.attach(pin_servo_b);
}

void loop() {
    int x, y, z;
    x = analogRead(JoyStick_X);
    y = analogRead(JoyStick_Y);
    z = digitalRead(JoyStick_Z);

    if(!z){
      digitalWrite(pin_laser, HIGH);
    }else{
      digitalWrite(pin_laser, LOW);
    }

    int pos_a = map(x, 0, max_val, 20, 120);
    int pos_b = map(y, 0, max_val, 45, 100);

    servo_a.write(pos_a);
    servo_b.write(pos_b);
    delay(15);
}   
