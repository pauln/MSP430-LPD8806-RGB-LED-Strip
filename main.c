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

#include "23K256.h"

// constant defines
#define DATA    BIT7
#define CLOCK   BIT6
#define ROWS    8
#define COLS    32

// wdt delay constants
#define MCLK_FREQUENCY      2000000
#define WDT_DIVIDER         512

const unsigned long WDT_FREQUENCY = MCLK_FREQUENCY / WDT_DIVIDER;
volatile unsigned long wdtCounter = 0;

// data arrays
const unsigned int NUMLEDS = ROWS * COLS;

//incrementers
int p;
long i;

// prototypes
void init(void);
void display(void);
unsigned int calcIndex(unsigned int col, unsigned int row);
unsigned long color(unsigned char r, unsigned char g, unsigned char b);
unsigned long colorHex(unsigned long hex);
void setPixelS(unsigned int col, unsigned int row, unsigned long c);
void setPixel(unsigned int col, unsigned int row, unsigned char r, unsigned char g, unsigned char b);
unsigned long wheel(unsigned char wheelpos);

// pattern functions
void demos(void);
void randomchase(void);
void copcar(void);
void goJoe(unsigned long time); // larger time value is slower chase
void randomdance(unsigned int delay);
void solidblink(unsigned long c);
void colorwipe(unsigned long c);
void rainbowcycle(void);
void showrainbow(unsigned int delay);

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

// Include font/text handling after we've declared functions and colors
#include "font.h"

void main(void) {
  //WDTCTL = WDTPW + WDTHOLD;
  
  // use 16MHz calibrated values
  BCSCTL1 = CALBC1_16MHZ;
  DCOCTL  = CALDCO_16MHZ;
  BCSCTL2 |= DIVS_3;
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
/*
// random chase
void randomchase(void) {
  int m;
  for ( m = 0; m <= NUMLEDS; m++ ) {
    setPixelS(m, adcGenRand24());
    setPixelS(m-1, clear);
    display();
    delayMillis(100);
  }
  for ( m = NUMLEDS; m >= 0; m-- ) {
    setPixelS(m, adcGenRand24());
    setPixelS(m+1, clear);
    display();
    delayMillis(100);
  }
}*/

// police light bar chaser
void copcar(void) {
  unsigned int row, col;
  unsigned int middle = COLS / 2 ;

  colorwipe(clear);

  for ( col = 0; col < COLS; col++ ) {
    if ( col <= middle ) {
      for ( row = 0; row < ROWS; row++ ) {
        setPixelS(col, row, red);
        setPixelS(col - 2, row, clear);

        setPixelS(COLS - col, row, blue);
        setPixelS(COLS - col + 2, row, clear);
      }
    }
    display();

    if  ( col >= middle ) {
      for ( row = 0; row < ROWS; row++ ) {
        setPixelS(col, row, blue);
        setPixelS(col - 2, row, clear);
    
        setPixelS(COLS - col, row, red);
        setPixelS(COLS - col + 2, row, clear);
      }
    }
    display();
    delayMillis(50);
  }
}

// red white and blue flag
void goJoe(unsigned long time) {
  int bluecols, bluerows, col, row;
  bluecols = (COLS+1)/3;
  bluerows = (ROWS+1)/2;

  colorwipe(clear); // clear display from existing patterns
    
  for ( col = 0; col < COLS; col++ ) {
    for ( row = 0; row < ROWS; row++ ) {
      if ( col < bluecols && row < bluerows ) {
        if ( (col%2==0 && row%2==1) || (col%2==1 && row%2==0) ) {
          setPixelS(col, row, white);
        } else {
          setPixelS(col, row, blue);
        }
      } else {
        if ( row%2==0 ) {
          setPixelS(col, row, red);
        } else if (row < (ROWS-1)) {
          setPixelS(col, row, white);
        }
      }
    }
  }
  display();
  delayMillis(time);
}

// send random colors down each pixel
void randomdance(unsigned int delay) {
  unsigned int max, m, n;
  max = COLS>ROWS?COLS:ROWS;
    
  setPixelS(0, 0, adcGenRand24());
  display();
  delayMillis(delay);
  for ( m = 0; m < max; m++ ) {
    for (n = 0; n < m; n++ ) {
      setPixelS(n, m, adcGenRand24());
      setPixelS(m, n, adcGenRand24());
    }
    display();
    delayMillis(delay);
  }
}

void solidblink(unsigned long c) {
  colorwipe(c);
  delayMillis(500);
  colorwipe(clear);
}

// animate fading rainbow cycle
void rainbowcycle(void) {
  unsigned int row, col;
  int j;
  
  for ( j=0; j<256; j++ ) {
    for ( col=0; col < COLS; col++ ) {
      for ( row=0; row < ROWS; row++ ) {  
        setPixelS(col, row, wheel( ( col+j) % 255 ) );
      }
    }
    display();
    delayMillis(10);
  }
}

// display static rainbow
void showrainbow(unsigned int delay) {
  unsigned int row, col;
  
  for ( col=0; col < COLS; col++ ) {
    for ( row=0; row < ROWS; row++ ) {
      setPixelS(col, row, wheel( ((col * 256 / COLS )) % 255) );
    }
  }
  display();
  delayMillis(delay);
}
      
// wipe strip to selected color
void colorwipe(unsigned long c) {
  unsigned int col, row;
  
  for ( col=0; col < COLS; col++)
    for ( row=0; row < ROWS; row++)
      setPixelS(col, row, c);
  display();
    //delayMillis(100);
}

// run all functions as demo
void demos(void) {
  int x;

  // font demo
  colorwipe(clear);
  initText();
  displayChar('H');
  displayChar('E');
  displayChar('L');
  displayChar('L');
  displayChar('O');
  display();
  delayMillis(1000);
  initText();
  displayChar('4');
  displayChar('3');
  displayChar('O');
  displayChar('H');
  displayChar('!');
  display();
  delayMillis(1000);
    
// run demos for display
    
  for (x = 0; x < 3; x++) {
    copcar();
  }
    
  goJoe(3000);

  colorwipe(clear);
  
  for (x = 0; x < 15; x++) {
    randomdance(0);
  }
  
  showrainbow(5000);

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
  P1DIR |= DATA + CLOCK; // set data and clock pins to output
  P1OUT &= ~DATA; // Data low
  P1OUT &= ~CLOCK;
  writezeros(3 * ((NUMLEDS + 63) / 64)); // latch to wake it up
  sram_init();
}

// send data to led strip; create patten with a 'use' function then send with display
void display(void) {

  // Stream data from SRAM to LED strip
  sram_select_chip();
  sram_send_command(SRAM_READ);
  sram_send_address(0);
  for ( sram_byte=0; sram_byte < NUMLEDS*3; sram_byte++ ) {
    for ( sram_bit=0x80; sram_bit>0; sram_bit>>=1 ) {
      P2OUT |= SRAM_CLK;
      P1OUT &= ~CLOCK;
      if (P2IN & SRAM_IN) {
        P1OUT |= DATA;
      } else {
        P1OUT &= ~DATA;
      }
      P2OUT &= ~SRAM_CLK;
      P1OUT |= CLOCK;
    }
  }
  sram_deselect_chip();

  writezeros(3 * ((NUMLEDS + 63) / 64)); // LED strip latch
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

unsigned int calcIndex(unsigned int col, unsigned int row) {
  if(row%2==1) {
    col = COLS-col-1;
  }
  return COLS*row + col; // 0-based means that the first row will already be row=0, no need to subtract one
}

// set pixel to specified color
void setPixel(unsigned int col, unsigned int row, unsigned char r, unsigned char g, unsigned char b) {
  int n;
  unsigned char bytes[3];
  if ( row >= ROWS || row < 0 || col >= COLS || col < 0) return;
  n = calcIndex(col, row);
  bytes[0] = 0x80 | g;
  bytes[1] = 0x80 | r;
  bytes[2] = 0x80 | b;
  sram_write_bytes(n*3, bytes, 3);
}

//set pixel to color by function
void setPixelS(unsigned int col, unsigned int row, unsigned long c) {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  if ( row >= ROWS || row < 0 || col >= COLS || col < 0) return;
  g = c >> 16;
  r = c >> 8;
  b = c;
  setPixel(col, row, r, g, b);
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
