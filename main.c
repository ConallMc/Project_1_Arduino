/* Project 1
* main.c
*
* The aim of this project is to gain familiarity with programming a
* micro controller, its timers and analog to digital converter using interrupts.
*
* Last Modified: 17/11/2020
* Author: Conall McAteer
* Student ID: 18173586
*/


#include <avr/io.h>
#include <avr/interrupt.h>

// values of the adc's
#define LOWER_THRESHOLD_VOLTAGE 511
#define ONE_EIGHT_VOLTAGE 128
#define ONE_QUARTER_VOLTAGE 256
#define THREE_EIGHT_VOLTAGE 384
#define HALF_VOLTAGE 512
#define FIVE_EIGHT_VOLTAGE 639
#define THREE_QUARTER_VOLTAGE 767
#define SEVEN_EIGHT_VOLTAGE 895
#define MAX_THRESHOLD_VOLTAGE 1023

// declaring my global variables as volatile to avoid compiler optimization

volatile unsigned int time_overflow;						// Amount of overflows needed
volatile unsigned int timecount0;						// Amount of overflows reached
volatile int tcnt0_start;							// Start the counter
volatile int display_flag;							// check if portb bit 4 is pressed
volatile int adc_flag;								// new adc result flag
volatile uint16_t adc_reading;							// variable to hold adc reading
volatile int Position;								// Position cylon eyes travel
volatile int led;								// current active led/pin in cylon pattern




void adc_initialize(void)			//adc initialization function
{
	adc_flag = 0;				// set if new adc result available
	display_flag = 1;			// initialize to 8-bit thermometer display
	Position = 0;
	led = 0;
	
	// ADC initialization happens here
	ADMUX = ((1<<REFS0) | (0 << ADLAR) | (0<<MUX0));				// AVCC selected for VREF, ADC0 as ADC input
	ADCSRA = ((1<<ADEN)|(1<<ADSC)|(1<<ADATE)|(1<<ADIE)|(6<<ADPS0));			/* Enable ADC, Start Conversion, Auto Trigger enabled, Interrupt enabled, Prescale = 64  */
	ADCSRB = (0<<ADTS0);								// Select AutoTrigger Source to Free Running Mode
}


void timer_initialize(void)			//timer initialization function
{
	timecount0 = 0;				// initialize to 0
	tcnt0_start = 125;			// begin timer count at 125
	time_overflow = 0;			// initialize to 0
	
	TCCR0B = (5<<CS00);			// Set T0 Source = Clock (16MHz)/1024 and put Timer in Normal mode
	
	TIMSK0 = (1<<TOIE0);			// Enable Timer 0 interrupt
	TCNT0 = tcnt0_start;			// assign timer count start
	
}


void initialize(void)	//initializing the function
{
	DDRD = 0xff;				// 0xff = 0b11111111; all ones
	PORTD = 0;
	DDRB = 0b00000000;			// set PORTB to inputs
	PORTB = 0b00110000;			// enable pull up resistors on pins 4 & 5

	sei();					// Global interrupt enable
}


void cylon_loop()
{
	int end;
	
	if (Position == 0)				// check if position\direction is down (Bit 0)
	{
		led--;					// decrement the active led
		if (led <= end)				// check if led has reached the end
		{
			Position = 1;			// set position\direction to up (Bit 7)
		}
		PORTD = 0b00000001 << led;		// set pin portd to current led
		
		
	} else if (Position == 1)				// check if Position is up
	{
		led++;						// increment the active led
		if (led >= 7)					// check if led has reached bit 7
		{
			Position = 0;				// set Position to down
		}
		PORTD = 0b00000001 << led;			// set pin of portd to current led
	}
}

void adc_display(int display_flag)
{
	if (display_flag) 					//8-bit display, controls PortD
	{
		if (adc_reading <= ONE_EIGHT_VOLTAGE)
		{
			PORTD = 0b00000000;
		} else if (adc_reading <= ONE_QUARTER_VOLTAGE)
		{
			PORTD = 0b00000001;
		} else if (adc_reading <= THREE_EIGHT_VOLTAGE)
		{
			PORTD = 0b00000011;
		} else if (adc_reading <= HALF_VOLTAGE)
		{
			PORTD = 0b00000111;
		} else if (adc_reading <= FIVE_EIGHT_VOLTAGE)
		{
			PORTD = 0b00001111;
		} else if (adc_reading <= THREE_QUARTER_VOLTAGE)
		{
			PORTD = 0b00011111;
		} else if (adc_reading <= SEVEN_EIGHT_VOLTAGE)
		{
			PORTD = 0b00111111;
		} else if (adc_reading < MAX_THRESHOLD_VOLTAGE)
		{
			PORTD = 0b01111111;
		} else if (adc_reading == MAX_THRESHOLD_VOLTAGE)
		{
			PORTD = 0b11111111;
		}
	} else if (!display_flag) 				//4-bit display, this only changes bits 0,1,2,3
	{
		PORTD &= ~0b00001111;
		if (adc_reading <= ONE_QUARTER_VOLTAGE)
		{
			PORTD |= 0b00000000;
		} else if (adc_reading <= HALF_VOLTAGE)
		{
			PORTD |= 0b00000001;
		} else if (adc_reading <= THREE_QUARTER_VOLTAGE)
		{
			PORTD |= 0b00000011;
		} else if (adc_reading < MAX_THRESHOLD_VOLTAGE)
		{
			PORTD |= 0b00000111;
		} else if (adc_reading == MAX_THRESHOLD_VOLTAGE)
		{
			PORTD |= 0b00001111;
		}
	}
}

int main(void)			// main function
{
	timer_initialize();
	adc_initialize();
	initialize();
	while(1)
	{
		if (adc_flag)				// checks new adc result available
		{
			if ((PINB & 0b00100000) == 0b00100000)
			{
				if ((PINB & 0b00010000) == 0)
				{
					adc_display(display_flag = 1);			// set to full 8-bit mode
					adc_flag = 0;					// reset
				}
			}
			else if ((PINB & 0b00100000) == 0)
			{
				adc_display(display_flag = 0);			// set to half 4-bit mode
				adc_flag = 0;
			}
		}
	}
}

ISR(TIMER0_OVF_vect)				// timer0 ISR
{
	TCNT0 = tcnt0_start;			// set to start value based on 0.125s or 0.5s
	++timecount0;			        // count the number of times the overflow has been reached
	
	if (timecount0 >= time_overflow)					 	// check if amount of overflows equals adc setting
	{
		if ((PINB & 0b00100000) == 0b00100000)
		{
			if ((PINB & 0b00010000) == 0b00010000)
			{
				cylon_loop(0);						// start 8-bit cylon pattern
				timecount0 = 0;						// Restart the overflow counter
			}
		}
		else {
			cylon_loop(4);							// start 4-bit cylon pattern
			timecount0 = 0;
		}
	}
}

ISR (ADC_vect)										// ADC ISR
{
	
	adc_reading = ADC;								// stores ADC value in a variable called "ADC"
	adc_flag = 1;									// sets the flag
	
	if (adc_reading < LOWER_THRESHOLD_VOLTAGE)		  // check if the adc voltage is 0V-2.5V
	{
		// 0.125s delay
		tcnt0_start = 39;				  // for 0.125s on/off time
		time_overflow = 9;
		
	}
	else if (adc_reading < MAX_THRESHOLD_VOLTAGE)		  // if its not 0V -2.5 its 2.5V-5V
	{
		// 0.5s delay
		tcnt0_start = 142;				  // for 0.5s on/off time
		time_overflow = 55;
	}
}