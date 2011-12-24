/****************************************************************/
/* Greg Whitmore												*/
/* greg@gwdeveloper.net											*/
/* www.gwdeveloper.net											*/
/****************************************************************/
/* released under the "Use at your own risk" license			*/
/* use it how you want, where you want and have fun				*/
/* debugging the code.											*/
/* MSP430 spi bit bang WS2801 RGB LED strip						*/
/****************************************************************/
/* code was translated from adafruit ws2801 arduino library		*/
/* https://github.com/adafruit/WS2801-Library					*/

#include <msp430.h>

// constant defines
#define DATA BIT7
#define CLOCK BIT6
#define NUMLEDS 32

// wdt delay constants
#define MCLK_FREQUENCY      1000000
#define WDT_DIVIDER        512

const unsigned long WDT_FREQUENCY = MCLK_FREQUENCY / WDT_DIVIDER;
volatile unsigned long wdtCounter = 0;

// data arrays
unsigned long pixels[NUMLEDS];

//incrementers
int p;
long i;

// prototypes
void init(void);
void display(void);
unsigned long color(unsigned char r, unsigned char g, unsigned char b);
void setPixelS(unsigned int n, unsigned long c);
void setPixel(unsigned int n, unsigned char r, unsigned char g, unsigned char b);
unsigned long wheel(unsigned char wheelpos);

// pattern functions
void demos(void);
void randomchase(void);
void copcar(void);
void goJoe(unsigned long time); // larger time value is slower chase
void randomdance(void);
void solidblink(unsigned long c);
void colorwipe(unsigned long c);
void rainbowcycle(void);
void showrainbow(void);

// random function and delay millis borrowed from NatureTM.com
// random generator slightly modified to create 32bit value
unsigned long adcGenRand32(void);
void delayMillis(unsigned long milliseconds);

// main colors
unsigned long clear = 0x000000;
unsigned long red = 0xFF0000;
unsigned long green = 0x00FF00;
unsigned long blue = 0x0000FF;
unsigned long white = 0x80FFFF;

unsigned long randomcolor;

void main(void)
{
	//WDTCTL = WDTPW + WDTHOLD;
	
	// use 1MHz calibrated values
	DCOCTL = CALDCO_1MHZ;
	BCSCTL1 = CALBC1_1MHZ;
	// wdt set as interval
	WDTCTL = WDTPW + WDTTMSEL + WDTIS1;
	// wdt interrupt
	IE1 |= WDTIE;
	// enable global interrupts using intrinsic
	__enable_interrupt();
	
	// initialize pins for SPI
	init();
	
	colorwipe(clear); // clear led strip
	
	while (1)
	{	
		demos();
	}

}

/* use functions */

// random chase
void randomchase(void)
{
	int m;
	for ( m = 0; m <= NUMLEDS; m++ )
	{
		pixels[m] = adcGenRand32();
		pixels[m-1] = clear;
		display();
		delayMillis(100);
	}
	for ( m = NUMLEDS; m >= 0; m-- )
	{
		pixels[m] = adcGenRand32();
		pixels[m+1] = clear;
		display();
		delayMillis(100);
	}			
}

// police light bar chaser
void copcar(void)
{
	int m;
	int middle = NUMLEDS / 2 ;

	colorwipe(clear);

	for ( m = 0; m < NUMLEDS; m++ )
	{
		if ( m <= middle )
		{
			pixels[m] = red;
			pixels[m - 2] = clear;

			pixels[NUMLEDS - m] = blue;
			pixels[NUMLEDS - m + 2] = clear;
		}
		display();

		if  ( m >= middle )
		{
			pixels[m] = blue;
			pixels[m - 2] = clear;
	
			pixels[NUMLEDS - m] = red;
			pixels[NUMLEDS - m + 2] = clear;
		}
		display();
	}
}

// red white and blue chasers
void goJoe(unsigned long time)
{
	int m;

	colorwipe(clear); // clear display from existing patterns
		
	for ( m = 0; m < NUMLEDS; m++ )
	{
		pixels[m] = blue;
		pixels[m - 2] = red;
		pixels[m - 4] = white;
		pixels[m - 6] = clear;
		display();
		delayMillis(time);
	}
	for ( m = NUMLEDS; m >= 0; m-- )
	{
		pixels[m] = clear;
		pixels[m - 2] = white;
		pixels[m - 4] = red;
		pixels[m - 6] = blue;
		display();
		delayMillis(time);
	}
}

// send random colors down each pixel
void randomdance(void)
{
	int m;
		
		for ( m = 0; m < NUMLEDS; m++ )
		{
			pixels[m] = adcGenRand32();
			display();
		}
}

// blink solid color
void solidblink(unsigned long c)
{
	colorwipe(c);
	delayMillis(500);
	colorwipe(clear);
}

// animate fading rainbow cycle
void rainbowcycle(void)
{
	int k, j;
	
	for ( j=0; j<256; j++ )
	{
		for ( k=0; k < NUMLEDS; k++ )
		{
			setPixelS(k, wheel( ( k+j) % 255 ) );
		}
		display();
		delayMillis(100);
	}
}

// display static rainbow
void showrainbow(void)
{
	int k, j;
	
	for ( j=0; j < 256 * 5; j++ )
	{
		for ( k=0; k < NUMLEDS; k++ )
		{
			setPixelS(k, wheel( ((k * 256 / NUMLEDS ) + j) % 255) );
		}
	}
	display();
	delayMillis(100);
}
			
// wipe strip to selected color
void colorwipe(unsigned long c)
{
	int v;
	
	for ( v=0; v < NUMLEDS; v++)
		setPixelS(v, c);
		display();
		//delayMillis(100);
}

// run all functions as demo
void demos(void)
{
	int x;
		
// run demos for display
		
	for (x = 0; x < 3; x++)
	{
		copcar();
	}
		
	for (x = 0; x < 3; x++)
	{
		goJoe(50);
	}

	for (x = 0; x < 5; x++)
	{
		randomdance();
	}

	colorwipe(clear);
		
	for (x = 0; x < 3; x++)
	{
		solidblink(green);
	}

	for (x = 0; x < 2; x++)
	{
		rainbowcycle();
	}
}

/* library functions */

//initialization
void init(void)
{
	P1DIR |= DATA + CLOCK; // set data and clock pins to output
}

// send data to led strip; create patten with a 'use' function then send with display
void display(void)
{
	unsigned long data;
	
    P1OUT &= ~CLOCK;
    delayMillis(1);
    
    // send all the pixels
    for ( p=0; p < NUMLEDS ; p++ )
    {
        data = pixels[p];
	// 24 bits of data per pixel
        for ( i=0x800000; i>0 ; i>>=1 )
        {
            P1OUT &= ~CLOCK;
            if (data & i)
            {
                P1OUT |= DATA;
            }
            else
            {
                P1OUT &= ~DATA;
            }
            P1OUT |= CLOCK;    // latch on clock rise
        }
    }
    // toggle clock low to display data
    P1OUT &= ~CLOCK;
    delayMillis(1);
}

// create 24bit color value
unsigned long color(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned long c;
	c = r;
	c <<= 8;
	c |= g;
	c <<= 8;
	c |= b;
	return c;
}

// set pixel to specified color
void setPixel(unsigned int n, unsigned char r, unsigned char g, unsigned char b)
{
  unsigned long data;

  if (n > NUMLEDS) return;

  data = g;
  data <<= 8;
  data |= b;
  data <<= 8;
  data |= r;
  
  pixels[n] = data;
}

//set pixel to color by function
void setPixelS(unsigned int n, unsigned long c)
{
	if ( n > NUMLEDS ) return;
	
	pixels[n] = c & 0xFFFFFF;
}

// rotate colorwheel for rainbows
unsigned long wheel(unsigned char wheelpos)
{
	if (wheelpos <=85)
	{
		return color( wheelpos * 3, 255 - wheelpos * 3, 0 );
	}
	else if ( wheelpos < 170 )
	{
		return color( 255 - wheelpos * 3, 0, wheelpos * 3 );
	}
	else
	{
		wheelpos -= 170;
		return color( 0, wheelpos * 3, 255 - wheelpos * 3 );
	}
}

// generate random 32bit number using ADC10 channel 5; leave P1.4 & P1.5 floating
unsigned long adcGenRand32(void)
{
  char bit;
  unsigned long random;
  
  for(bit = 0; bit < 32; bit++){
    ADC10CTL1 |= INCH_5;
    ADC10CTL0 |= SREF_1 + ADC10SHT_1 + REFON + ADC10ON;
    ADC10CTL0 |= ENC + ADC10SC;
    while(ADC10CTL1 & ADC10BUSY);
    random <<= 1;
    random |= (ADC10MEM & 0x01);
  }
  return random;
}

// millisecond delay counter using WDT
void delayMillis(unsigned long milliseconds){
  unsigned long wakeTime = wdtCounter + (milliseconds * WDT_FREQUENCY / 1000);
  while(wdtCounter < wakeTime);
}

// wdt isr
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void){
  wdtCounter++;
}
