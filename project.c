/*
 * FroggerProject.c
 *
 * Main file
 *
 * Author: Peter Sutton. Modified by <Yinhuang Huang>
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "ledmatrix.h"
#include "scrolling_char_display.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "score.h"
#include "timer0.h"
#include "game.h"
//#include "time_limit.h"
#define F_CPU 8000000L
#include <util/delay.h>
#include <math.h>

// Function prototypes - these are defined below (after main()) in the order
// given here
void initialise_hardware(void);
void splash_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);
void display_lives(void);
void level(void);
void display_levels(void);
void init_countdown(void);
void countdown(void);
//////////////////timer//////////
void init_timer_1(void);
void init_countdown(void);
void start_countdown(void);
void pause_countdown(void);
void resume_countdown(void);
void stop_countdown(void);
uint32_t get_countdown(void);
void time_out(void);

// ASCII code for Escape character
#define ESCAPE_CHAR 27

///////////////gobal variable////////////////////
uint8_t lives;
uint8_t levels;
////////////////////////For interrupt part///////////////////////
volatile uint8_t is_digit_displayed;
volatile uint8_t countdown_on;
volatile uint32_t counts;
volatile uint8_t seven_seg_cc;

uint8_t seven_seg_data[10] = {63,6,91,79,102,109,125,7,127,111};
/////////////////////////////// main //////////////////////////////////
int main(void) {
	// Setup hardware and call backs. This will turn on 
	// interrupts.
	initialise_hardware();
		
	// Show the splash screen message. Returns when display
	// is complete
	splash_screen();
	while(1) {
		new_game();
		play_game();
		handle_game_over();
	}
	
}

void initialise_hardware(void) {
	ledmatrix_setup();
	init_button_interrupts();
	// Setup serial port for 19200 baud communication with no echo
	// of incoming characters
	init_serial_stdio(19200,0);
	
	init_timer0();
	init_timer_1();
	// Turn on global interrupts
	sei();
}

void splash_screen(void) {
	// Clear terminal screen and output a message
	clear_terminal();
	move_cursor(10,10);
	printf_P(PSTR("Frogger"));
	move_cursor(10,12);
	printf_P(PSTR("CSSE2010/7201 project by Yinhuang Huang"));
	
	// Output the scrolling message to the LED matrix
	// and wait for a push button to be pushed.
	ledmatrix_clear();
	while(1) {
		set_scrolling_display_text("4376709", COLOUR_GREEN);
		// Scroll the message until it has scrolled off the 
		// display or a button is pushed
		while(scroll_display()) {
			//_delay_ms(150);
			if(button_pushed() != NO_BUTTON_PUSHED) {
				return;
			}
		}
	}
}

void new_game(void) {
	// Initialise the game and display
	initialise_game();
	
	//Initialise the lives
	lives = 3;
	DDRC = 0xFF;
	display_lives();
	levels = 1;

	init_countdown();
	// Clear the serial terminal
	clear_terminal();
	
	// Initialise the score
	init_score();
	

	
	// Clear a button push or serial input if any are waiting
	// (The cast to void means the return value is ignored.)
	(void)button_pushed();
	clear_serial_input_buffer();
}

void play_game(void) {
	lives = get_live();
	levels = get_level();
	uint32_t last_move_time_1,last_move_time_2,last_move_time_3,last_move_time_4,last_move_time_5;
	uint32_t time_gap_1,time_gap_2,time_gap_3,time_gap_4,time_gap_5;
	uint32_t current_time_1,current_time_2,current_time_3,current_time_4,current_time_5;
	int8_t button;
	char serial_input, escape_sequence_char;
	uint8_t characters_into_escape_sequence = 0;
	uint8_t game_pause = 0;
	
	// Get the current time and remember this as the last time the vehicles
	// and logs were moved.
	current_time_1 = get_current_time();
	current_time_2 = get_current_time();
	current_time_3 = get_current_time();
	current_time_4 = get_current_time();
	current_time_5 = get_current_time();
	last_move_time_1 = current_time_1;
	last_move_time_2 = current_time_2;
	last_move_time_3 = current_time_3;
	last_move_time_4 = current_time_4;
	last_move_time_5 = current_time_5;
	time_gap_1 = 0;
	time_gap_2 = 0;
	time_gap_3 = 0;
	time_gap_4 = 0;
	time_gap_5 = 0;
	
	// We play the game while the frog is alive and we haven't filled up the 
	// far riverbank
	display_levels();
	init_countdown();
	start_countdown();
	
	while(!is_frog_dead() && !is_riverbank_full()) {
		if(!is_frog_dead() && frog_has_reached_riverbank()) {
			// Frog reached the other side successfully but the
			// riverbank isn't full, put a new frog at the start
			put_frog_in_start_position();
			init_countdown();
		}
		if(get_countdown()==0){
			time_out();
		}
		// Check for input - which could be a button push or serial input.
		// Serial input may be part of an escape sequence, e.g. ESC [ D
		// is a left cursor key press. At most one of the following three
		// variables will be set to a value other than -1 if input is available.
		// (We don't initalise button to -1 since button_pushed() will return -1
		// if no button pushes are waiting to be returned.)
		// Button pushes take priority over serial input. If there are both then
		// we'll retrieve the serial input the next time through this loop
		serial_input = -1;
		escape_sequence_char = -1;
		button = button_pushed();
		//display_countdown();
		//perform_countdown();
		//start_countdown();
		
		if(button == NO_BUTTON_PUSHED) {
			// No push button was pushed, see if there is any serial input
			if(serial_input_available()) {
				// Serial data was available - read the data from standard input
				serial_input = fgetc(stdin);
				// Check if the character is part of an escape sequence
				if(characters_into_escape_sequence == 0 && serial_input == ESCAPE_CHAR) {
					// We've hit the first character in an escape sequence (escape)
					characters_into_escape_sequence++;
					serial_input = -1; // Don't further process this character
				} else if(characters_into_escape_sequence == 1 && serial_input == '[') {
					// We've hit the second character in an escape sequence
					characters_into_escape_sequence++;
					serial_input = -1; // Don't further process this character
				} else if(characters_into_escape_sequence == 2) {
					// Third (and last) character in the escape sequence
					escape_sequence_char = serial_input;
					serial_input = -1;  // Don't further process this character - we
										// deal with it as part of the escape sequence
					characters_into_escape_sequence = 0;
				} else {
					// Character was not part of an escape sequence (or we received
					// an invalid second character in the sequence). We'll process 
					// the data in the serial_input variable.
					characters_into_escape_sequence = 0;
				}
			}
		}
		
		if (game_pause==0){		
			// Process the input. 
			if(button==3 || escape_sequence_char=='D' || serial_input=='L' || serial_input=='l') {
				// Attempt to move left
				move_frog_to_left();
			} else if(button==2 || escape_sequence_char=='A' || serial_input=='U' || serial_input=='u') {
				// Attempt to move forward
				move_frog_forward();
			} else if(button==1 || escape_sequence_char=='B' || serial_input=='D' || serial_input=='d') {
				// Attempt to move down
				move_frog_backward();
			} else if(button==0 || escape_sequence_char=='C' || serial_input=='R' || serial_input=='r') {
				// Attempt to move right
				move_frog_to_right();
			}
			}
		
		if(serial_input == 'p' || serial_input == 'P') {
			// Unimplemented feature - pause/unpause the game until 'p' or 'P' is
			// pressed again
			if (game_pause==0){
				game_pause = 1;
				pause_countdown();
				//pause_time = get_current_time();
		        move_cursor(10,20);
		        printf_P(PSTR("Game Paused!"));
			}
			else if(game_pause==1){
				game_pause = 0;
				resume_countdown();
				time_gap_1 = get_current_time()-current_time_1;
				time_gap_2 = get_current_time()-current_time_1;
				time_gap_3 = get_current_time()-current_time_1;
				time_gap_4 = get_current_time()-current_time_1;
				time_gap_5 = get_current_time()-current_time_1;
				clear_terminal();
			}
			
		} 
		// else - invalid input or we're part way through an escape sequence -
		// do nothing
		
		if(game_pause==0){
			current_time_1 = get_current_time();
			current_time_2 = get_current_time();
			current_time_3 = get_current_time();
			current_time_4 = get_current_time();
			current_time_5 = get_current_time();
			
			if(!is_frog_dead()&&current_time_1>=last_move_time_1 + time_gap_1 + 750*(1-(levels-1)/10)){
				scroll_vehicle_lane(0,1);
				last_move_time_1 = current_time_1;
				time_gap_1 = 0;
			}
			if(!is_frog_dead()&&current_time_2>=last_move_time_2 + time_gap_2 + 1100*(1-(levels-1)/10)){
				scroll_vehicle_lane(1,-1);
				last_move_time_2 = current_time_2;
				time_gap_2 = 0;
			}
			if(!is_frog_dead()&&current_time_3>=last_move_time_3 + time_gap_3 + 900*(1-(get_level()-1)/10)){
				scroll_vehicle_lane(2,1);
				last_move_time_3 = current_time_3;
				time_gap_3 = 0;
			}
			if(!is_frog_dead()&&current_time_4>=last_move_time_4 + time_gap_4 + 1100*(1-(get_level()-1)/10)){
				scroll_river_channel(0,1);
				last_move_time_4 = current_time_4;
				time_gap_4 = 0;
			}
			if(!is_frog_dead()&&current_time_5>=last_move_time_5 + time_gap_5 + 800*(1-(get_level()-1)/10)){
				scroll_river_channel(1,-1);
				last_move_time_5 = current_time_5;
				time_gap_5 = 0;
			}
		}
		
	}
	// We get here if the frog is dead or the riverbank is full
	level_upgrade();
	// The game is over.
}

void handle_game_over() {
	lives--;
	display_lives();
	if(lives>0){
		put_frog_in_start_position();
		play_game();
		handle_game_over();
	}
	else if(lives==0){
		stop_countdown();
		move_cursor(10,14);
		printf_P(PSTR("GAME OVER"));
		move_cursor(10,15);
		printf_P(PSTR("Press a button to start again"));
		while(button_pushed() == NO_BUTTON_PUSHED) {
			; // wait
		}
	}
}

void display_lives(){
	if (lives==5){
		PORTC = 0x1F;
	}
	if (lives==4){
		PORTC = 0x0F;
	}
	if (lives==3){
		PORTC = 0x07;
	}
	if (lives==2){
		PORTC = 0x03;
	}
	if (lives == 1){
		PORTC = 0x01;
	}
	if (lives == 0){
		PORTC = 0x00;
	}
	//PORTC = 0x0F;	
}


void display_levels(){
	move_cursor(10,15);
	printf_P(PSTR("Level %i"), levels);
}


void init_timer_1(void){
	is_digit_displayed = 0;
	countdown_on = 0;
	DDRA = 0xFF;
	DDRD |= (1<<DDRD7);
	//DDRD = 0xFF;
	seven_seg_cc = 1;
	/* Set up timer/counter 1 so that we get an 
	** interrupt 100 times per second, i.e. every
	** 10 milliseconds.
	*/
	OCR1A =  9999; 
	TCCR1A = 0;
	TCCR1B = (1<<WGM12)|(1<<CS11);

	/* Enable interrupt on timer on output compare match 
	*/
	TIMSK1 = (1<<OCIE1A); 
	
	/* Ensure interrupt flag is cleared */
	TIFR1 = (1<<OCF1A);

	/* Turn on global interrupts */
	sei();
}

void init_countdown(){
	is_digit_displayed = 0;
	countdown_on = 0;
	counts = 1500;
}

void start_countdown(){
	is_digit_displayed = 1;
	countdown_on = 1;
}

void pause_countdown(void){
	countdown_on = 0;
}

void resume_countdown(void){
	countdown_on = 1;
}

void stop_countdown(void){
	is_digit_displayed = 0;
	counts = 0;
	countdown_on = 0;
}


uint32_t get_countdown(void) {
	uint32_t returnValue;

	/* Disable interrupts so we can be sure that the interrupt
	 * doesn't fire when we've copied just a couple of bytes
	 * of the value. Interrupts are re-enabled if they were
	 * enabled at the start.
	 */
	uint8_t interruptsOn = bit_is_set(SREG, SREG_I);
	cli();
	returnValue = counts;
	if(interruptsOn) {
		sei();
	}
	return returnValue;
}

ISR(TIMER1_COMPA_vect) {
	if(countdown_on && counts > 0){
		counts--;
	}
	
	seven_seg_cc = !seven_seg_cc;
	
	if(is_digit_displayed && counts>0 && countdown_on) {
		/* Display a digit */
		if(counts>1000 || counts==1000){
			if(seven_seg_cc == 1) {
				/* Display leftmost digit - tens of seconds */
				PORTA = seven_seg_data[((counts)/100)/10];
				PORTD = 0xFF;
				//PORTD |= (PORTD4<<1);
			} else if(seven_seg_cc == 0) {
				PORTA = seven_seg_data[((counts/100)%10)];
				PORTD = 0x00;
				//PORTD &= ~(1<<PORTD7);
			}
		}
		else if(counts<1000|| counts==100){
			if(seven_seg_cc == 1) {
				/* Display leftmost digit - tenths of seconds */
				PORTA = 0x00;
				PORTD = 0xFF;
				} else {
				/* Display rightmost digit - seconds + decimal point */
				PORTA = seven_seg_data[((counts/10)/10)];
				PORTD = 0x00;
			}
		}
		else if(counts<100){
			if(seven_seg_cc == 1){
				PORTA = seven_seg_data[0]+128;
				PORTD = 0xFF;
				}else{
				PORTA = seven_seg_data[counts/10];
				PORTD = 0x00;
			}
		}
		/* Output the digit selection (CC) bit */
	}
	else if(counts==0){
		PORTA = seven_seg_data[0];
	}
	else if(is_digit_displayed==0){
		/* No digits displayed -  display is blank */
		PORTA = 0x00;
	}
}



