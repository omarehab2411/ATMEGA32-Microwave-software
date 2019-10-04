/*
 * Timer0.c
 *
 *  Created on: Sep 11, 2018
 *      Author: omar ehab
 */



#include <avr/io.h>
void Timer0_init(void)
{
TCCR0=(1<<FOC0);
TCNT0=0;
TIMSK=(1<<TOIE0);
SREG|=(1<<7);
}

void Timer0_start(void)
{
TCCR0 |= (1<<CS02);   //PRESCALED BY 256
}

void Timer0_stop(void)
{TCCR0 &=~ (1<<CS02);

}

void Timer0_COUNTERMODE(void)
{TCCR0 |= (1<<CS01)|(1<<CS02);
	}


void Timer0_clear(void)
{
TCNT0=0;
}

void Timer0_showvalue(unsigned char *Address)
{
*Address=TCNT0;
}

void Timer0_PWM_INIT(void)
{
	DDRB|=(1<<PB3);
	TCNT0=0;
	 OCR0=0;
	TCCR0=(1<<WGM00)|(1<<WGM01)|(1<<COM00)|(1<<COM01)|(1<<CS02)|(1<<CS00);
	TIMSK|=(1<<TOIE0);
	SREG|=(1<<7);
	}
void Timer0_PWM_DUTYCYCLE(int duty_cycle)
{
	OCR0=duty_cycle;

}

