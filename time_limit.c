#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "countdown.h"
#include "terminal.h"
#include "timer0.h"

static volatile uint8_t seven_seg_status;
static volatile uint8_t countdown_on;
static volatile uint16_t countdown;

static volatile uint16_t EEMEM ee_countdown;

static const uint8_t seven_seg[10] PROGMEM = {63,6,91,79,102,109,125,7,127,111};

void init_countdown(void){
	seven_seg_status = 0;
	countdown_on = 0;

	DDRC = 0xFF;
	DDRD |= (1<<DDRD2);
}

void save_countdown(void){
	eeprom_update_word(&ee_countdown,countdown);
}

void load_countdown(void){
	uint8_t interrupt_on = bit_is_set(SREG, SREG_I);
	cli();
	countdown = eeprom_read_word(&ee_countdown);
	if(interrupt_on) {
		sei();
	}
}

void start_countdown(uint8_t start){
	countdown = ( (uint16_t) start ) * 100;
	seven_seg_status = 1;
	countdown_on = 1;
}

void pause_countdown(void){
	countdown_on = 0;
}

void resume_countdown(void){
	countdown_on = 1;
}

void stop_countdown(void){
	seven_seg_status = 0;
	countdown = 0;
	countdown_on = 0;
}

uint16_t get_countdown(void){
	uint16_t return_value;

	uint8_t interrupt_on = bit_is_set(SREG, SREG_I);
	cli();
	return_value = countdown;
	if(interrupt_on) {
		sei();
	}
	return ceil(return_value/100);
}

void inc_countdown(void){
	if (countdown_on && countdown > 0 && !get_paused()){
		countdown--;
	}
	if (seven_seg_status){
		uint16_t limitedCountdown = countdown/100;
		if (limitedCountdown > 15){
			limitedCountdown = 15;
		}
		if (seven_seg_status == 1){
			PORTD |= (1<<PORTD2);
			uint8_t temp = (limitedCountdown/10)%10;
			if (temp > 0 && temp < 10){
				PORTC = (const uint8_t)pgm_read_word(&seven_seg[temp]);
			} else {
				PORTC = 0;
			}
		} else {
			PORTD &= ~(1<<PORTD2);
			PORTC =(const uint8_t)pgm_read_word(&seven_seg[limitedCountdown%10]);
		}
		seven_seg_status ^= 3;
	} else {
		PORTC = 0;
	}
}
