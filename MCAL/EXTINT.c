/*
 * EXTINT.c
 *
 *  Created on: Apr 15, 2019
 *      Author: omar ehab
 */


#include "EXTINT.h"

void EXT_INT_ENABLE(char mode)
{
SREG|=(1<<7);
if(mode=='1'){

GICR|=(1<<INT1);
}
 if(mode=='0')
	{
       GICR|=(1<<INT0);
}
 if(mode=='3')
{GICR|=(1<<INT1);
GICR|=(1<<INT0);}
	}
void EXT_INT_DISABLE(char mode){

	if(mode=='1'){
	GICR=0;
	}
	 if(mode=='0')
		{MCUCR=0;
	GICR=0;
	}
	 if(mode=='3')
	{
	 GICR=0;		}
}

int EXT_INT_FLAG(char mode){

	if(mode=='0')
	{
		return (GIFR&(1<<INTF0));
	}
	else if(mode=='1')
	{
		return (GIFR&(1<<INTF1));
	}
}
