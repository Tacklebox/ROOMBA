/* Project: src/ROOMBA */

#include <Arduino.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

hd44780_I2Cexp lcd;

const int LCD_COLS = 16;
const int LCD_ROWS = 2;

void setup()
{
  int status;
  status = lcd.begin(LCD_COLS, LCD_ROWS);
  if(status)
  {
    status = -status;
    hd44780::fatalError(status);
  }
  lcd.print("Hello, World!");
}

void loop() {}
