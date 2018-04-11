/**
 * \file servo.h
 * \brief Simple control of a servo using hardware PWM.
 *
 * Created: 19/08/2013 9:16:26 AM
 *  \Author: Daniel McIlvaney
 */


#ifndef SERVO_H_
#define SERVO_H_
// MG995 middle:1350 us, +/- 870 ms


void servo_init(){
	DDRE |= (1<<PE5);  //PWM Pins as Out
	DDRE |= (1<<PE4);  //PWM Pins as Out

	//Set to Fast PWM mode 15
	TCCR3A |= (1<<WGM30) | (1<<WGM31);
	TCCR3B |= (1<<WGM32) | (1<<WGM33);

	TCCR3A |= (1<<COM3C1);
	TCCR3A |= (1<<COM3B1);

	TCCR3B |= (1<<CS31)|(1<<CS30);

	OCR3A = 5000;  //20 ms period
	OCR3B = 375;
	OCR3C = 375;
}

void servo_set_pin3(uint16_t pos) {
	if(pos < 135) {
		pos = 135;
	}
	if(pos > 550) {
		pos = 550;
	}
    OCR3B = pos;
}

void servo_set_pin2(uint16_t pos) {
	if(pos < 135) {
		pos = 135;
	}
	if(pos > 550) {
		pos = 550;
	}
	OCR3C = pos;
}


void servo_pen_down(){
	servo_set_pin3(550L);
}

void servo_pen_up(){
	servo_set_pin3(340L);
}

#endif /* SERVO_H_ */
