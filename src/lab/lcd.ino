/* Project: src/ROOMBA */

#include <Arduino.h>
#include <LiquidCrystal.h>

#define LCD_SIZE 16
int ir_pin = A8;

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

void setup() {
    pinMode (ir_pin, INPUT);   
    lcd.begin(16, 2);
}

void loop() {
    int val = analogRead(ir_pin);
    char buf[LCD_SIZE];
    Serial.println(buf);
    sprintf(buf, "%d    ", val);
    lcd.setCursor(0,0);
    lcd.print(buf);
    delay(100);
}   
