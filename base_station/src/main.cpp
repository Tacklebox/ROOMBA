/*
 * main.cpp
 *
 *  Created on: 18-Feb-2011
 *      Author: nrqm
 *
 * Example program for time-triggered scheduling on Arduino.
 *
 * This program pulses one pin every 500 ms and another pin every 300 ms
 *
 * There are two important parts in this file:
 * 		- at the end of the setup function I call Scheduler_Init and Scheduler_StartTask.  The latter is called once for
 * 		  each task to be started (note that technically they're not really tasks because they share the stack and don't
 * 		  have separate contexts, but I call them tasks anyway because whatever).
 * 		- in the loop function, I call the scheduler dispatch function, which checks to see if a task needs to run.  If
 * 		  there is a task to run, then it its callback function defined in the StartTask function is called.  Otherwise,
 * 		  the dispatch function returns the amount of time (in ms) before the next task will need to run.  The idle task
 * 		  can then do something useful during that idle period (in this case, it just hangs).
 *
 * To use the scheduler, you just need to define some task functions and call Scheduler_StartTask in the setup routine
 * for each of them.  Keep the loop function below the same.  The scheduler will call the tasks you've created, in
 * accordance with the creation parameters.
 *
 * See scheduler.h for more documentation on the scheduler functions.  It's worth your time to look over scheduler.cpp
 * too, it's not difficult to understand what's going on here.
 */
#define DEADZONE 20

#include <Arduino.h>
#include "scheduler.h"

#define sign(x) ((x > 0) - (x < 0))

#define FORWARD 'f'
#define REVERSE 'b'
#define STOP 's'
#define LEFT 'l'
#define RIGHT 'r'
#define DOCK 'd'
#define POWER 'p'

uint8_t pulse1_pin = 3;
uint8_t pulse2_pin = 4;
uint8_t idle_pin = 7;

// Forwards is towards the wires which is x = 0, back is x = 1023, left is y=1023, right is y=0.
uint8_t  joy_b_pin = 5;
uint8_t  joy_x_pin = A1;
uint8_t  joy_y_pin = A0;
int16_t joy_x     = 0;
int16_t joy_y     = 0;
uint8_t  joy_b     = 0;


// task function for PulsePin task
void sample_joy_task()
{
  joy_b = !digitalRead(joy_b_pin);
  joy_x = map(analogRead(joy_x_pin),0,1023,90,-90);
  joy_y = map(analogRead(joy_y_pin),0,1023,90,-90);
  if (abs(joy_x) >= abs(joy_y)) {
    joy_y = 0;
  } else {
    joy_x = 0;
  }
  if (abs(joy_x) < DEADZONE) {
    joy_x = 0;
  }
  if (abs(joy_y) < DEADZONE) {
    joy_y = 0;
  }
  joy_x = sign(joy_x);
  joy_y = sign(joy_y);

}

// task function for PulsePin task
void send_command_task()
{
  char command;
  if( joy_b ){
    command = DOCK;
  }else if ( joy_x == 1 ){
    command = FORWARD;
  }else if ( joy_x == -1 ){
    command = REVERSE;
  }else if ( joy_y == 1 ){
    command = RIGHT;
  }else if ( joy_y == -1 ) {
    command = LEFT;
  }else {
    command = STOP;
  }
  Serial.write(command);
  //Serial1.write(command); //testing the nano
}

// idle task
void idle(uint32_t idle_period)
{
  // this function can perform some low-priority task while the scheduler has nothing to run.
  // It should return before the idle period (measured in ms) has expired.  For example, it
  // could sleep or respond to I/O.

  // example idle function that just pulses a pin.
  digitalWrite(idle_pin, HIGH);
  delay(idle_period);
  digitalWrite(idle_pin, LOW);
}

void setup()
{
  pinMode(joy_b_pin, INPUT_PULLUP);
  pinMode(idle_pin, OUTPUT);
  Serial.begin(9600);
  //Serial1.begin(9600); //testing the nano

  Scheduler_Init();

  // Start task arguments are:
  //		start offset in ms, period in ms, function callback

  Scheduler_StartTask(50, 100, sample_joy_task);
  Scheduler_StartTask(0, 300, send_command_task);
}

void loop()
{
  uint32_t idle_period = Scheduler_Dispatch();
  if (idle_period)
  {
    idle(idle_period);
  }
}

int main()
{
  init();
  setup();

  for (;;)
  {
    loop();
  }
  for (;;);
  return 0;
}
