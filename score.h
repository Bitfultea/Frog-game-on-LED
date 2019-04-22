/*
 * score.h
 * 
 * Author: Peter Sutton
 */

#ifndef SCORE_H_
#define SCORE_H_

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "scrolling_char_display.h"
#include "serialio.h"

void init_score(void);
void add_to_score(uint16_t value);
uint32_t get_score(void);

#endif /* SCORE_H_ */