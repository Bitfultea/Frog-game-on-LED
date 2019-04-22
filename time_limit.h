/*
 * time_limit.h
 *
 * Created: 30/05/2018 7:58:00 PM
 *  Author: hyh86
 */ 


#ifndef TIME_LIMIT_H_
#define TIME_LIMIT_H_

void init_timer_1(void);

void init_countdown(void);

void start_countdown(void);

void pause_countdown(void);

void resume_countdown(void);

void stop_countdown(void);

uint32_t get_countdown(void);

uint32_t get_cc(void);

void display_countdown(void);

#endif /* TIME_LIMIT_H_ */