/*******************************************************************/
/* Based on WS2801 control code by:                                */
/* Greg Whitmore                                                   */
/* greg@gwdeveloper.net                                            */
/* www.gwdeveloper.net                                             */
/*******************************************************************/
/* Modified to control LPD8806-based strips by:                    */
/* Paul Nicholls                                                   */
/* https://github.com/MaxThrax/MSP430-LPD8806-RGB-LED-Strip        */
/*******************************************************************/
/* released under the "Use at your own risk" license               */
/* use it how you want, where you want and have fun                */
/* debugging the code.                                             */
/* MSP430 spi bit bang WS2801 RGB LED strip                        */
/*******************************************************************/
/* WS2801 code was translated from Adafruit WS2801 Arduino library */
/* https://github.com/adafruit/WS2801-Library                      */
/* LPD8806 conversion based on Adafruit LPD806 Arduino library:    */
/* https://github.com/adafruit/LPD8806                             */
/*******************************************************************/

#include <msp430.h>
#include <legacymsp430.h>

// constant defines
#define DATA    BIT7
#define CLOCK   BIT6
#define NUMLEDS 64

// wdt delay constants
#define MCLK_FREQUENCY      1000000
#define WDT_DIVIDER         512

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
unsigned long adcGenRand24(void);
void delayMillis(unsigned long milliseconds);

// main colors
unsigned long clear = 0x808080 | 0x000000;
unsigned long red = 0x808080 | 0x00FF00;
unsigned long green = 0x808080 | 0xFF0000;
unsigned long blue = 0x808080 | 0x0000FF;
unsigned long white = 0x808080 | 0xFFFFFF;

unsigned long randomcolor;

void main(void) {
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
  delayMillis(1000);
  while (1) { 
    demos();
  }

}

/* use functions */

// random chase
void randomchase(void) {
  int m;
  for ( m = 0; m <= NUMLEDS; m++ ) {
    pixels[m] = adcGenRand24();
    pixels[m-1] = clear;
    display();
    delayMillis(100);
  }
  for ( m = NUMLEDS; m >= 0; m-- ) {
    pixels[m] = adcGenRand24();
    pixels[m+1] = clear;
    display();
    delayMillis(100);
  }
}

// police light bar chaser
void copcar(void) {
  int m;
  int middle = NUMLEDS / 2 ;

  colorwipe(clear);

  for ( m = 0; m < NUMLEDS; m++ ) {
    if ( m <= middle ) {
      pixels[m] = red;
      pixels[m - 2] = clear;

      pixels[NUMLEDS - m] = blue;
      pixels[NUMLEDS - m + 2] = clear;
    }
    display();

    if  ( m >= middle ) {
      pixels[m] = blue;
      pixels[m - 2] = clear;
  
      pixels[NUMLEDS - m] = red;
      pixels[NUMLEDS - m + 2] = clear;
    }
    display();
  }
}

// red white and blue chasers
void goJoe(unsigned long time) {
  int m;

  colorwipe(clear); // clear display from existing patterns
    
  for ( m = 0; m < NUMLEDS; m++ ) {
    pixels[m] = blue;
    if(m>=2) pixels[m - 2] = red;
    if(m>=4) pixels[m - 4] = white;
    if(m>=6) pixels[m - 6] = clear;
    display();
    delayMillis(time);
  }
  for ( m = NUMLEDS; m >= 0; m-- ) {
    pixels[m] = clear;
    if(m>=2) pixels[m - 2] = white;
    if(m>=4) pixels[m - 4] = red;
    if(m>=6) pixels[m - 6] = blue;
    display();
    delayMillis(time);
  }
}

// send random colors down each pixel
void randomdance(void) {
  int m;
    
    for ( m = 0; m < NUMLEDS; m++ ) {
      pixels[m] = adcGenRand24();
      display();
    }
}

void solidblink(unsigned long c) {
  colorwipe(c);
  delayMillis(500);
  colorwipe(clear);
}

// animate fading rainbow cycle
void rainbowcycle(void) {
  int k, j;
  
  for ( j=0; j<256; j++ ) {
    for ( k=0; k < NUMLEDS; k++ ) {
      setPixelS(k, wheel( ( k+j) % 255 ) );
    }
    display();
    delayMillis(100);
  }
}

// display static rainbow
void showrainbow(void) {
  int k, j;
  
  for ( j=0; j < 256 * 5; j++ ) {
    for ( k=0; k < NUMLEDS; k++ ) {
      setPixelS(k, wheel( ((k * 256 / NUMLEDS ) + j) % 255) );
    }
  }
  display();
  delayMillis(100);
}
      
// wipe strip to selected color
void colorwipe(unsigned long c) {
  int v;
  
  for ( v=0; v < NUMLEDS; v++)
    setPixelS(v, c);
  display();
    //delayMillis(100);
}

// run all functions as demo
void demos(void) {
  int x;
    
// run demos for display
    
  for (x = 0; x < 3; x++) {
    copcar();
  }
    
  for (x = 0; x < 3; x++) {
    goJoe(50);
  }

  for (x = 0; x < 5; x++) {
    randomdance();
  }

  colorwipe(clear);
    
  for (x = 0; x < 3; x++) {
    solidblink(green);
  }

  for (x = 0; x < 2; x++) {
    rainbowcycle();
  }
}

/* library functions */

// write a bunch of zeroes, used for latch
void writezeros(unsigned int n) {
  unsigned int i;
  P1OUT &= ~DATA; // Data low
  for(i = 8 * n; i>0; i--) {
    P1OUT |= CLOCK;
    P1OUT &= ~CLOCK;
  }
}

//initialization
void init(void) {
  int i;
  P1DIR |= DATA + CLOCK; // set data and clock pins to output
  P1OUT &= ~DATA; // Data low
  P1OUT &= ~CLOCK;
  for(i=0; i<NUMLEDS; i++) {
    pixels[i] = 0x808080;
  }
  writezeros(3 * ((NUMLEDS + 63) / 64)); // latch to wake it up
}

// send data to led strip; create patten with a 'use' function then send with display
void display(void) {
  unsigned long data;
    
    // send all the pixels
    for ( p=0; p < NUMLEDS ; p++ ) {
      data = pixels[p];
      // 24 bits of data per pixel
      for ( i=0x800000; i>0 ; i>>=1 ) {
        if (data & i) {
            P1OUT |= DATA;
        } else {
            P1OUT &= ~DATA;
        }
        P1OUT |= CLOCK;    // latch on clock rise
        P1OUT &= ~CLOCK;
      }
    }
    writezeros(3 * ((NUMLEDS + 63) / 64)); // latch
    delayMillis(3);
}

// create 24bit color value
unsigned long color(unsigned char r, unsigned char g, unsigned char b) {
  unsigned long c;
  
  c = 0x808080 | ((unsigned long)g << 16) | ((unsigned long)r << 8) | (unsigned long)b;
  return c;
}
// create color value from an unsigned long, so you can use colorHex(0xABCDEF);
unsigned long colorHex(unsigned long hex) {
  return 0x808080 | hex;
}

// set pixel to specified color
void setPixel(unsigned int n, unsigned char r, unsigned char g, unsigned char b) {
  if (n > NUMLEDS) return;
  
  pixels[n] = color(r, g, b);
}

//set pixel to color by function
void setPixelS(unsigned int n, unsigned long c) {
  if ( n > NUMLEDS ) return;
  
  pixels[n] = c;
}

// rotate colorwheel for rainbows
unsigned long wheel(unsigned char wheelpos) {
  if (wheelpos <=85) {
    return color( wheelpos * 3, 255 - wheelpos * 3, 0 );
  }
  else if ( wheelpos < 170 ) {
    return color( 255 - wheelpos * 3, 0, wheelpos * 3 );
  }  else {
    wheelpos -= 170;
    return color( 0, wheelpos * 3, 255 - wheelpos * 3 );
  }
}

// generate random 24bit number using ADC10 channel 5; leave P1.4 & P1.5 floating
unsigned long adcGenRand24(void) {
  char bit;
  unsigned long random;
  
  for(bit = 0; bit < 24; bit++) {
    ADC10CTL1 |= INCH_5;
    ADC10CTL0 |= SREF_1 + ADC10SHT_1 + REFON + ADC10ON;
    ADC10CTL0 |= ENC + ADC10SC;
    while(ADC10CTL1 & ADC10BUSY);
    random <<= 1;
    random |= (ADC10MEM & 0x01);
  }
  return random | 0x808080;
}

// millisecond delay counter using WDT
void delayMillis(unsigned long milliseconds) {
  unsigned long wakeTime = wdtCounter + (milliseconds * WDT_FREQUENCY / 1000);
  while(wdtCounter < wakeTime);
}

// wdt isr
interrupt(WDT_VECTOR) watchdog_timer(void) {
  wdtCounter++;
}
