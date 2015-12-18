#include <string.h>

#include "common.h"
#include "led.h"

void InitLeds() {
  int i;
  
  // Hardcoded out of laziness.
  PORTC_PCR5 = PORT_PCR_MUX(0x1);
  PORTC_PCR0 = PORT_PCR_MUX(0x1);
  PORTD_PCR1 = PORT_PCR_MUX(0x1);
  PORTC_PCR3 = PORT_PCR_MUX(0x1);
  PORTC_PCR4 = PORT_PCR_MUX(0x1);
  PORTC_PCR7 = PORT_PCR_MUX(0x1);
  GPIOC_PDDR = 1 | (1<<3) | (1<<4) | (1<<5) | (1<<7);
  GPIOD_PDDR = (1<<1);
  
  for (i = 0; i < 6; i ++) {
    LedOn(i);
  }
  for (i = 0; i < 100000; i ++);
  for (i = 0; i < 6; i ++) {
    LedOff(i);
  }
}

inline void LedOn(int led) {
  if (led == 0) {
    LED0_PORTSET = LED0_PORTMASK;
  } else if (led == 1) {
    LED1_PORTSET = LED1_PORTMASK;
  } else if (led == 2) {
    LED2_PORTSET = LED2_PORTMASK;
  } else if (led == 3) {
    LED3_PORTSET = LED3_PORTMASK;
  } else if (led == 4) {
    LED4_PORTSET = LED4_PORTMASK;
  } else if (led == 5) {
    LED5_PORTSET = LED5_PORTMASK;
  }
}

inline void LedOff(int led) {
  if (led == 0) {
    LED0_PORTCLEAR = LED0_PORTMASK;
  } else if (led == 1) {
    LED1_PORTCLEAR = LED1_PORTMASK;
  } else if (led == 2) {
    LED2_PORTCLEAR = LED2_PORTMASK;
  } else if (led == 3) {
    LED3_PORTCLEAR = LED3_PORTMASK;
  } else if (led == 4) {
    LED4_PORTCLEAR = LED4_PORTMASK;
  } else if (led == 5) {
    LED5_PORTCLEAR = LED5_PORTMASK;
  }
}

inline void LedToggle(int led) {
  if (led == 0) {
    LED0_PORTTOGGLE = LED0_PORTMASK;
  } else if (led == 1) {
    LED1_PORTTOGGLE = LED1_PORTMASK;
  } else if (led == 2) {
    LED2_PORTTOGGLE = LED2_PORTMASK;
  } else if (led == 3) {
    LED3_PORTTOGGLE = LED3_PORTMASK;
  } else if (led == 4) {
    LED4_PORTTOGGLE = LED4_PORTMASK;
  } else if (led == 5) {
    LED5_PORTTOGGLE = LED5_PORTMASK;
  }
}
