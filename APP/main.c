/*
 * main.c
 *
 *  Created on: Apr 19, 2019
 *      Author: omar ehab
 */

#include "Timer0.h"
#include "LCD.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include "UART.h"
#include "BLUETOOTH.h"
#include "Keypad.h"
#include "RTC.h"
#include "EXTINT.h"
#include "TIMER1.h"
#include "SERVO_TIMER1.h"

int sec;                //integer variable to store seconds
int min;                //integer variable to store minutes
int total;              //integer variable to store total seconds
char timing[4];         //array of characters to store the timing entered by user
int  timing_dec[4];     //array of integers to store the converted array of characters entered by user
int Cooking_mode;       //integer variable to store cooking mode entered by user
int flag=0;             //flag that is used to enable enter to the main function
int door_flag=0;        //flag that indicate if door(SERVO) opened or closed
char temp[10];
char btn_pressed='#';
struct RTC clock;       // clock type defined structure to store the coming clock data

struct var {
    char hours[4];
	char min[4];
	char sec[4];
} v;                  //structure to store the converted integer values stored in the clock structure so it could be send on LCD


ISR(INT0_vect){
	 /*************************************************************************************
	     *This ISR is requested when The IR sensor radiation is obstacle by the user hand which means
	     *This her want to open or close the door the IR sensor generates a pulse which is connected to
	     *This INT0 and it will generate the external interrupt this ISR will check the door flag
	     *if door flag equals 0 it means door is closed so open the door while if it is 1 it means the
	     *door is opened so close the door the door is opened or closed through a SERVO motor
	     *going to angles 90 degrees to open and back to 0 degrees to close to generate these angle
	     *we have to configure the SERVO motor
	     *####################################################################################
	     *SERVO Motor configurations
	     *we have initiated the SERVO motor before to output a pulse of 50HZ the aim now is to output
	     *the proper duty cycle that will move the SERVO to certain angles we want the 0 and 90 degrees
	     *so to calculate the Duty cycle we will use this equation
	     *Duty cycle=top-OCR1A/(top+1)
	     *0 degrees from  SERVO data sheet requires 1.5ms pulse (8% duty cycle)
	     *90 degrees from SERVo data sheet requires 2ms pulse   (10%duty cycle)
	     *ICR1 initiated with 780 as explained before why
	     * so to calculate OCR1A to get proper PWM we use  OCR1A=-duty cycle*(ICR1+1)+ICR1
	     *####################################################################################
	     **************************************************************************************/


	if(door_flag==0)   //open the door so when the door is opened the value of the flag will e 1
{
		EXT_INT_DISABLE('0');        //disable external interrupt
		SERVO_MOTOR_DUTYCYCLE(15);   //set duty cycle by setting OCR1A
        SERVO_MOTOR_START();         //start The SERVO by starting timer1
        _delay_ms(500);
        door_flag=1;                //flag that door has been opened
        EXT_INT_ENABLE('0');        //enable the external interrupt
        BLUETOOTH_SENDSTRING("door is opened");//send to user that door is opened
        BLUETOOTH_SENDSTRING("\r\n");
}
	else if(door_flag==1)           // close the door so hen the door is closed the flag will be 0
	{
		EXT_INT_DISABLE('0');       //disable external interrupt
		SERVO_MOTOR_DUTYCYCLE(50);  //set duty cycle by setting OCR1A
		SERVO_MOTOR_START();        //start SERVO by output PWM from timer1
		_delay_ms(500);
		door_flag=0;               //set flag to 0 means door is closed
		EXT_INT_ENABLE('0');       //enable external interrupt
		BLUETOOTH_SENDSTRING("door is closed"); //send to user by bluetooth that door is closed
		BLUETOOTH_SENDSTRING("\r\n");
	}

	}


 ISR(TIMER0_OVF_vect)
{

	 /*************************************************************************************
	     * This Interrupt service routine is handled after the function START_get(); is executed
	     * the aim of this timer ISR is to count till 1 seconds has passed and after that decrements
	     * the set timing for the timer entered by the user before and display the count down through the
	     * LCD the ISR occurs after the timer overflows which means when it counts upto 255 count
	     * so we will calculate how much time does it take to overflows and from it we will calculate how
	     * much overflows needed for 1 second to pass and so every 1 second we will show the count down
	     * and check temperature sensor reading if seconds reached 0 we check if the total overflows
	     * counter reached 0 also which means the timing is finished then we stop all the running codes
	     * and return to main function
	     * #########################################################################################
	     * configurations and calculations
	     * timer counts up-to 255 count
	     * CPU clock is 8MHz PRE-scaled with 256 so clock 3125 HZ
	     * so it will take 1tick-----32 MICRO seconds
	     *                 255 tick---8.16 MILLI seconds
	     * so to get 1 second    (255/8.16 MILLI)
	     *#####################################################################################
	     *the ISR will keep going till the seconds is zero and overflow counter reached total overflows
	     *
	     **************************************************************************************/


	 flag=1;                      //set flag to 1 so Program doesn't enter main when ISR return
	 EXT_INT_DISABLE('0');        //disable all external interrupts
	 DDRD&=~(1<<PD5);
     door_flag=0;                 //clear the door flag which means it is closed
     static int counter=0;        //static counter to count of 1 seconds passed of static type so it is not defined again
     static int ovf_counter=0;    //static counter to counter number of overflows occurs
     int num_overflow=total;      //contains total number of overflows required and is compared with overflow counter
     char mins[10];               //array of character so we can use it to send from LCD
     char secs[10];               //array of character so we can use it to send from LCD
     counter++;                   //Increment the counter when ISR every time occurs

if((counter>=123)&&(ovf_counter<=num_overflow))
{   ovf_counter++;                                                    //increments overflow counter every second
    sec--;                                                            //decrement the number of seconds when every second pass

  if(TEMP_CHECK(PA5)>=45)                                            //temperature check if higher than 35 stop the program
  {
	buzzerr_on_off('3');                                             //buzzer for long period to notify a problem
	LCD_RESET();                                                    //reset the LCD
	LCD_NAVIGATE_SEND_STRING(0,0,"Temp Overload");                   //Print there is a temperature overload
	_delay_ms(1000);
	BLUETOOTH_SENDSTRING("Temp overload stoping the program ");     //send through uart to use phone
	BLUETOOTH_SENDSTRING("\r\n");                                  //this line is sent to make bluetooth go to next line
	_delay_ms(200);
	PORTC&=~(1<<PC3);
	PORTD&=~(1<<PD4);
	EXT_INT_DISABLE('0');                                          //disable external interrupts
	Timer0_stop();                                                //stop the timer
	TIMER1_FASTPWM_STOP();                                       //stop the DC motor
	flag=0;                                                      //clear flag so we can enter main function
	ovf_counter=0;                                              //clear the overflow counter
	counter=0;                                                 //clear the counter
	main();                                                   //call main to start the program again
  }

if(sec!=0&&(sec>0))           //checks if seconds not equal zero
{
	itoa(sec,secs,10);         //convert integer to string so we could send it through LCD
    itoa(min,mins,10);        //convert integer to string so we could send it through LCD
     if(sec>9){
	LCD_NAVIGATE_SEND_STRING(1,0,mins);
	LCD_NAVIGATE_SEND_STRING(1,2,":");
	LCD_NAVIGATE_SEND_STRING(1,3,secs);}
     if(sec<=9)
     {
    	    LCD_NAVIGATE_SEND_STRING(1,0,mins);
    	 	LCD_NAVIGATE_SEND_STRING(1,2,":");
    	 	LCD_NAVIGATE_SEND_STRING(1,3,"0");
    	 	LCD_NAVIGATE_SEND_STRING(1,4,secs);
     }
	}

if(sec==0) //checks if seconds equal 0
{ if(ovf_counter>=num_overflow)  //checks if overflow counter is equal to total overflows
{
	PORTC&=~(1<<PC3);
	PORTD&=~(1<<PD4);
	EXT_INT_DISABLE('0');         //disable external interrupts
	LCD_NAVIGATE_SEND_STRING(1,0,"00");
	LCD_NAVIGATE_SEND_STRING(1,2,":");
	LCD_NAVIGATE_SEND_STRING(1,3,"00");
	flag=0;                      //clear lag to 0 to enter main
	ovf_counter=0;               //clear overflow counter
	counter=0;                   //clear seconds counter
	buzzerr_on_off('3');         //let buzzer sund for long period
	Timer0_stop();               //stop the timer
	TIMER1_FASTPWM_STOP();       //stop the DC motor
	LCD_RESET();
	LCD_NAVIGATE_SEND_STRING(0,0,"END");
    BLUETOOTH_SENDSTRING("your food has been cooked");  //send that food has finished to user by uart and bluetooth
    BLUETOOTH_SENDSTRING("\r\n");
    _delay_ms(1000);
    //DDRD&=~(1<<PD5);
main();                           //call main fucntion to start the program again
}
	min--;    //decrement the minutes
    sec=60;   //set seconds to 60 to start decrement
	}
counter=0;    //clear the counter so to start from 0 again when ISR is handeled
	}

}



 int TEMP_CHECK(int ADC_CHANNEL){
	 /*************************************************************************************
	     * This function is intended to be used while the program is running to check if a temperature
	     * overload has happened the function parameter is the ADC channel the temperature sensor
	     * connected to and it return the temperature in celsius.
	     * ####################################################################################
	     * sensor reading
	     * we use here lm35 temperature sensor which output an analog voltage signal with 10milli volt
	     * for every 1 degree celsius change so to convert the reading of sensor into readable temperature
	     * we convert the reading using the ADC which is 10 bits so it has a maximum conversion number of
	     * 1024.
	     * since reference voltage of ADC is 5volt so 5v----1024
	     * if 10miili volt----1 celsius
	     * so 5volt(max)------x
	     * x=500 celsius maximum temperature could read
	     * if 500Celsisu---1024 ADC read
	     * so   Temp---ADC-read
	     * temp=ADC_read*500/1024
	     * by doing this we could get the tempratre in celsius from the sensor
	     * ########################################################################################
	     **************************************************************************************/

	    ADC_init();
	   _delay_ms(250);
	   unsigned int volatile temprature_value;
	 	temprature_value=ADC_start(ADC_CHANNEL);
	 	temprature_value=((500*temprature_value)/1024);
	 	return temprature_value;

 }

void merge(void)
{
	 /*************************************************************************************
	     *This function is intended to be called after void CHAR2DEC(char *ascii,int* dec);
	     *the purpose of this function is to merge the integer values of the seconds and minutes splitted
	     *in the array of integers we created and filled in the CHAR2DEC function.the first two
	     * index of integer array is the seconds so we want to merge them as a whole number and store
	     * them in a variable named sec and the third and fourth index of the array are the minutes
	     * so we want to merge them into a whole number and store them in a variable named minutes
	     * ######################################################################################
	     * conversion explained
	     * to merge the numbers we simply multiply the first number by 10 and then add the result to the
	     * number in the following index of the array by doing so we get a whole number from the two
	     * different numbers.
	     *#######################################################################################
	     *example two number 10,2 we want to merge them so 10*10+2=102 so the merge is done
	     * ######################################################################################
	     **************************************************************************************/

sec=timing_dec[0]*10+timing_dec[1];   //merge seconds into a whole number and store in sec
min=timing_dec[2]*10+timing_dec[3];   //merge minutes into a whole number and store in minutes
total=min*60+sec;                     //total seconds and store it in global variable named total
}

void START_get(void){
	 /*************************************************************************************
	     * This function is intended to be called after void merge(void);
	     * the aim of this function is to start the microwave program after the user confirm
	     * the setting by pressing number '1'only from the keypad if user confirmed by pressing '1'
	     *this function will first check if the door is closed using flag set through ISR(INT0_vect);
	     *external interrupt if door is closed the program will start by setting the DC motor on
	     *by configuring the TIMER1 mode and the values needed to output a proper duty cycle for the motor
	     * to run if door not closed the program will not start and will ask the user to close the door
	     * then it will start after that
	     *####################################################################################
	     *configurations made
	     *TIMER1_FASTPWMMODE_INIT('B'); this function will set Timer1 in pwm mode number 14
	     * ICR1=780; depending on frequency we want to generate (10hz)
	     * OCR1B=390; depending on duty cycle we want (50%)
		 * OCR1A=390;
		 * to generate a pwm signal with 10HZ then according to the equation
		 * FGEN=Foscillaotr/ICR1*N  -----Foscillator 8Mega HZ
		 *                          -----N 256 from pre-scaler
		 *                          -------  FGEN 10HZ required
		 *  therefore ICR1=780
		 *  to generate duty cycle of 50% we will use this equation to set the values
		 *  Duty cycle=top-OCR1B/(top+1)
		 *             -----top=ICR1-1=779
		 *             ------top+1=ICR1=780
		 *             ------Duty cycle=50/100
		 * therefore OCR1B is set to 390
	     *######################################################################################
	     **************************************************************************************/
	int x;
	 do{
			LCD_NAVIGATE_SEND_STRING(0,0,"START TO CONFIRM");
			x=Keypad_get_pressed_key();
		}while(x!='1');
        LCD_RESET();

		if(x=='1')
	{LOOP: if(door_flag==0){
			TIMER1_FASTPWMMODE_INIT('B');
		     ICR1=780;       //depending on frequency we want to generate (10hz)
			 OCR1B=390;     //depending on duty cylce we want (50%)
			 OCR1A=390;
		    Timer0_init();  //initiate timer 0
			Timer0_start(); //start timer 0
	TIMER1_FASTPWM_START();
	LCD_RESET();
	LCD_NAVIGATE_SEND_STRING(0,0,"READY IN");
	x=0;}

	else if(door_flag==1){
		LCD_NAVIGATE_SEND_STRING(0,0,"DOOR IS OPEN");
		LCD_NAVIGATE_SEND_STRING(1,0,"SHUT THE DOOR");
		goto LOOP;
	}
EXT_INT_DISABLE('0');
	}		}


void num_get(void)
{
	 /*************************************************************************************
	     *  This function is intended to get the timing user want the microwave to cook the food
	     *  so this function basically go in a for loop for 4 times to get the maximum 60 seconds
	     *  and a two digits hours maximum number so in each loop iteration it keeps checking for
	     *  user to enter a number from the keypad and after the user enter the number it stores
	     *  the entered number in a global array named timing[4] which will store the seconds and
	     *  minutes entered by the user.
	     * counter defined as static so it is not defined again
	     **************************************************************************************/
static int j = 0;                           //static counter variable to use to display numbers on different columns of LCD
for(int i=0;i<4;i++)
{
  do{
	  timing[i]=Keypad_get_pressed_key();  //get the number entered by the user from the keypad
  }while(timing[i]=='#');
  _delay_ms(200);
  		LCD_NAVIGATE(1, j);               //go to row 1 and column j in the LCD to Display
  		LCD_SEND_CHARACHTER(timing[i]);   //send the entered number on the LCD
  		buzzerr_on_off('1');              //let buzzer on for short period
  		if (j == 1) {
  			j = j + 1;
  			LCD_NAVIGATE_SEND_STRING(1, j, ":");
  		}
  		j++;
  		if (i ==3) {
  			j = 0;
  		}

  	   }
 j=0;                                     //reset the j counter to zero so when function is called again the LCD displays from the start
   }

void CHAR2DEC(char *ascii,int* dec)
{
	 /*************************************************************************************
	     *   This function is intended to be called after function void num_get(void); is
	     *   executed the aim of this function is to change the entered numbers from user from characters into integer numbers
	     *   the parameter of this function is a pointer to character and a pointer to
	     *   an integer.
	     *   char *ascii-----pointer to char so we should send to it and address of array of characters
	     *   int * dec  ------pointer to integer so we should send to it address of array of integer
	     *
	     *   function call example
	     *   ####################################################################
	     *   CHAR2DEC(timing,timing_dec);
	     *   timing-----array of characters which contains the timing as characters entered from user
	     *   timing_dec----array of integers global created to store the converted integers
	     *   #####################################################################
	     *   function conversion explained
	     *   #####################################################################
	     *   dec[i]=ascii[i]-'0';
	     *   the function simply change characters to integers by minus the ascii value of zero
	     *   from any character which will give us the integer number and then store the converted
	     *   number in the created integer array.
	     *   ######################################################################
	     *   ASCII values of number in c
	     *   0-----48
	     *   1------49
	     *   2------50
	     *   3------51
	     *   4------52
	     *   5------53
	     *   6------54
	     *   7------55
	     *   8------56
	     *   9------57
	     * so if we make any character-ASCII of zero it will give use the integer
	     *  example
	     *  '1'-'0'=49-48=1
	     *  ##########################################################################
	     **************************************************************************************/


   for(int i=0;i<4;i++)
   {
	   dec[i]=ascii[i]-'0';
   }
}

void buzzerr_on_off(char state)
{
	 /*************************************************************************************
	     * This function is intended to make the buzzer output sound according to selected mode
	     * mode1----buzzer sound for a small period of time
	     * mode0-----buzzer is off
	     * mode3-----buzzer sound for a large period of time
	     *
	     **************************************************************************************/


if(state=='1')
{
	PORTD|=(1<<PD7);
_delay_ms(100);
PORTD&=~(1<<PD7);
PORTD|=(1<<PD7);
_delay_ms(100);
PORTD&=~(1<<PD7);
}
else if(state=='0')
{
	PORTD&=~(1<<PD7);}
if (state=='3')
{PORTD|=(1<<PD7);
_delay_ms(1000);
PORTD&=~(1<<PD7);
PORTD|=(1<<PD7);
_delay_ms(1000);
PORTD&=~(1<<PD7);}
}
void cooking(void)
{
	 /*************************************************************************************
	     * This function is intended to show the user a list of cooking modes available in
	     * the microwave and let the user choose one mode of them through pressing a number
	     * from the keypad and store this mode in a variable
	     * cooking modes are--------cooking
	     *                  ---------DEFROST
	     *                  ----------POPCORN
	     *                  ----------BAKE
	     * So this function will keep checking for user to choose if he didn't choose
	     * the program will stay in this stage until he chooses a mode after that
	     * the program flow will continue
	     *
	     **************************************************************************************/

	LCD_RESET();
LCD_NAVIGATE_SEND_STRING(0, 0, "cooking");
LCD_NAVIGATE_SEND_STRING(0, 9, "Defrost");
LCD_NAVIGATE_SEND_STRING(1, 0, "Popcorn");
LCD_NAVIGATE_SEND_STRING(1, 10, "Bake");
Cooking_mode = '#';
do {
	Cooking_mode = Keypad_get_pressed_key();
} while (Cooking_mode == '#');
buzzerr_on_off('1');
}

void TIME_MODE(void)
{
	 /*************************************************************************************
	     *   This function simply takes the clock time from the RTC module and store the timing
	     *   minutes,seconds,hours in a structure named clock then use this values in the structre
	     *   and send them on the LCD display to the user,then it keeps checking if user pressed
	     *   any key if so the it will take the user to choose the cooking mode
	     *   #######################################################################
	     *   RTC_GET(&clock);
	     *   This function is from the RTC driver it's parameter is a pointer to structure
	     *   so we send to it the address of structure we created to store the RTC time
	     *   and then use this timing to print on LCD
	     *    &clock---------is the address of the structure we created global to store
	     *    the RTC time
	     *   #########################################################################
	     *   	btn_pressed = Keypad_get_pressed_key();
	     *      This function gets a keypress from the keypad if user pressed a key
	     *      then use if (btn_pressed !='#') {buzzerr_on_off('1'); to check if user
	     *      pressed a key from the keypad or not why this syntax because the keypad
	     *      driver returns '#' when there is no presses key so if it always detect # this
	     *      means user didn't press any key so keep checking
	     *   ###########################################################################
	     *   if (btn_pressed !='#') {buzzerr_on_off('1');
	     *   case True---------------this will get the user to cooking mode
	     *   case False---------------the function will go to the start of this function and keeps checking
	     *   ############################################################################
	     *
	     **************************************************************************************/


 loop:	RTC_GET(&clock);
			LCD_NAVIGATE_SEND_STRING(0, 0, LCD_INT2STRING(clock.hour, v.hours));// send the seconds after converting to string
			LCD_NAVIGATE_SEND_STRING(0, 2, ":");
			LCD_NAVIGATE_SEND_STRING(0, 3, LCD_INT2STRING(clock.min, v.min));    // send the minutes after converting to string
			LCD_NAVIGATE_SEND_STRING(0, 5, ":");
			LCD_NAVIGATE_SEND_STRING(0, 6, LCD_INT2STRING(clock.sec, v.sec));    // send the Hours after converting to string
			LCD_NAVIGATE_SEND_STRING(1, 0, "PRESS ANY KEY");
			btn_pressed = Keypad_get_pressed_key();                             //get number from the  user and save it in btn pressed variable
							if (btn_pressed !='#') {buzzerr_on_off('1');        // check if user pressed a key
							LCD_INIT();
							LCD_RESET();
							cooking();                                         //if user pressed a key then call cooking mode function
							}
							else{goto loop;}                                  // if user didn't press a key keep checking
}



void SYS_INIT(void)
{   /*************************************************************************************
     *  This function is intended to initiate all the application used modules
     *  ############################################################################################
     *  SERVO_MOTOR_INIT(624);
     *  This function call is intended to set up the timer 1 module in pwm mode
     *  the function parameter is the ICR1 value which is used to set the period of the signal
     *  which in this case a 20 millisecond according to the servo motor datasheet
     *  so to generate a 20miilisecond signal according to the equation
     *  FGEN=Foscillaotr/(TOP+1)*N ------Foscillator is 8mega
     *                             ------ TOP+1 is the ICR1
     *                             ------ N is the prescaler which is in this case 256
     *   so according to the calculations to generate a 50 HZ pwm signal the ICR1 shoudl have 624 value
     *
     *  ##############################################################################################
     *   BLUETOOTH_INIT(9600);
     *   THis function is used to set the uart module baudrate with 9600 symbol per second why we choose this value
     *   as it is a standard according to the bluetooth mdule datasheet it communicate with 9600 baudrate so that's
     *   why 9600 is chosen as the baudrate value
     *   ############################################################################################
     *   EXT_INT_ENABLE('0');
     *   This function is used to enable the external interrupt on pin zero by sending 0 to the function
     *   it will enable the external interrupt on pin INT0
     *   ############################################################################################
     *
     **************************************************************************************/

SERVO_MOTOR_INIT(624);  //initiate the ICR1 of timer1 with 624 to generate a 20millisecond period PWM signal
EXT_INT_ENABLE('0');    //enable the external interrupt
DDRD |= (1 << PD7);     //buzzer enabled
PORTD &= ~(1 << PD7);   //buzzer off
DDRC|=(1<<PC3);         //PC3 is output
PORTC&=~(1<<PC3);       //pc3 gives 0 volt as an output to the motor channel module
LCD_INIT();             //initiate the LCD module
Keypad_init();          //initiate the keypad
BLUETOOTH_INIT(9600);   //initiate the UART module with BAUDRATE 9600
flag=0;                 //initiate the flag by 0
SERVO_MOTOR_INIT(624);  //initiate the ICR1 of timer1 with 624 to generate a 20millisecond period PWM signal
}

int main(void)
{
	SYS_INIT();                                          //function to initiate all the system peripherals
	while(1)
		if(flag==0)
	{   {EXT_INT_ENABLE('0');                           //enable external interrupt on pin INT0
		TIME_MODE();                                    //shows the clock using real time clock module
	    LCD_RESET();                                    //reset the LCD display
	    LCD_NAVIGATE_SEND_STRING(0,0,"SET THE TIMER");
	    num_get();                                     //get the timer value from the user
		LCD_RESET();
	    CHAR2DEC(timing,timing_dec);                   //change the taken numbers into decimal values
	    merge();                                       //merge these numbers into 1 variable
	}
START_get();                                           //function to start the program
	}
return 0;}

