/* Project: src/ROOMBA */

#include <Arduino.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <Servo.h>

hd44780_I2Cexp lcd;

const int LCD_COLS = 16;
const int LCD_ROWS = 2;
Servo servo_x;
Servo servo_y;
int pos_x = 0;
int pos_y = 0;
int axis_x = A0;
int axis_y = A1;
int clicker = 50;
int laser = 13;

void setup()
{
  servo_1.attach(9);
  servo_2.attach(10);
	pinMode(clicker, INPUT);
	pinMode(laser, OUTPUT);
  int status;
  status = lcd.begin(LCD_COLS, LCD_ROWS);
  if(status)
  {
    status = -status;
    hd44780::fatalError(status);
  }
  lcd.print("Hello, World!");
}

void loop() {
  for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}
