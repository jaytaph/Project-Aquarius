#include <avr/interrupt.h>
#include <avr/io.h>

#define INIT_TIMER_COUNT (256-125)  // 8 bit counter 
#define RESET_TIMER2 TCNT2 = INIT_TIMER_COUNT

// These are the digits from 0 to 9, with 7 segments
byte ssd[10][7] = {
   { 1,1,1,1,1,1,0 },
   { 0,1,1,0,0,0,0 },
   { 1,1,0,1,1,0,1 },
   { 1,1,1,1,0,0,1 },
   { 0,1,1,0,0,1,1 },
   { 1,0,1,1,0,1,1 },
   { 1,0,1,1,1,1,1 },
   { 1,1,1,0,0,0,0 },
   { 1,1,1,1,1,1,1 },
   { 1,1,1,1,0,1,1 }
};

// LCD pins, LSB (least significant digit) is 2, MSB is 5
// So the number "1234"  gets stored to:
// 4 => pin 2
// 3 => pin 3
// 2 => pin 4
// 1 => pin 5
byte lcd_offset[] = { 
  5,4,3,2,
  25,24,23,22,
  26,27,28,29,
  0
};

#define SEG_OFFSET  6
#define SEG_DOT    13

// Counters. 1 counter per 'block' of 4 digits
int rc[4];

// Called lots of times per second (can't remember the freq)
ISR(TIMER2_OVF_vect) {
  // Reset timer again
  RESET_TIMER2;

  // Increase our mini counter
  
  // Increase counter
  rc[0]++;
  
  // Overflow? increase next block
  if (rc[0] > 9999) {
    rc[0] = 0;
    rc[1]++;
  }

  // Overflow? increase next block
  if (rc[1] > 9999) {
    rc[1]= 0;
    rc[2]++;
  }
  
  // Overflow? increase next block
  if (rc[2] > 9999) {
    rc[2]= 0;
    rc[3]++;
  }
    
  // Don't care about rc3 overflow
};


/**
 * Will be run on startup/reset
 */
void setup() {
  // Set all pins to output (lazy, we should only set pins we actually use)
  for (int i=2; i!=32; i++) pinMode(i, OUTPUT);
  pinMode(37, OUTPUT);

  // Reset counter  
  rc[0] = rc[1] = rc[2] = rc[3] = 0;

  // Set scaler prescale 110 = /128 == 125Khz
//  TCCR2B |= (1 | 2 | 4);
  TCCR2B |= (1 | 4);
  
  // Normal operation
  TCCR2A &= ~((0<<WGM21) | (1<<WGM20));
  
  // Overflow enable, triggers the interrupt on overflow of the 
  // Timer/Counter2 occurs
  TIMSK2 |= (1<<TOIE2);
  
  // Disable output compare, triggers the interrupt when a compare
  // match occurs (OCF0 bit is 1 in TIFR)?
  TIMSK2 &= ~( (1<<OCIE2A) | (1<<OCIE2B) );
  
  // Reset timer
  RESET_TIMER2;
  
  // Enable interrupts
  sei();  
}


/**
 * Selects the current LCD we want to set, all others will be set to HIGH.
 * Sounds strange, but we actually set something by making it LOW instead of 
 * high. Maybe I just switched something around. That is still a possibility
 */
void lcd_select(int lcd) {
  int i = 0;
  while (lcd_offset[i]) {
     digitalWrite(lcd_offset[i], (i == lcd) ? LOW : HIGH);     
     i++;
  }
}

/**
 * Write a number to an LCD block. A number is a max 4 digit number (between 0 and 
 * 9999) so it writes to 4 sequential LCD's. BLockNr which block (in our case, 
 * block 0, 1 or 2)
 */
void lcd_write(int block_nr, int r) {
  int i;
  
  // Write to 4 digits (LSB first)
  for (int l=3; l>=0; l--) {  
    // First, all segments should be turned off. Otherwise we get "ghosts" on the 
    // block we want to write
    for (int j=0; j!=7; j++) digitalWrite(SEG_OFFSET+j, HIGH);
    
    // All segments are off, we can safely select our new block.
    lcd_select((block_nr*4) + l);
    
    // Get LSB digit xxx4
    i = r % 10;
    r /= 10; // shift so next LSB will be the next number
    
    // Set all correct segments (according to our lookup table).
    for (int j=0; j!=7; j++) {
      digitalWrite(SEG_OFFSET+j, ssd[i][j] ? LOW : HIGH);
    }  
    
    // Don't write a dot.. Still need to figure out how to deal with this 
    // (maybe do fixedpoint stuff?)
    digitalWrite(SEG_DOT, HIGH);
    
    // Delay a bit. 5 is like watching a CRT on a home-video, 1 gives the best result.
    delay(1);
  }  
}


void loop() {
  // Just write the data. Timer will take care of increasing the counters
  lcd_write(0, rc[0]);
  lcd_write(1, rc[1]);
  lcd_write(2, rc[2]);

  // Blink LED
  digitalWrite(37, (rc[1] % 2) ? HIGH : LOW);
}
