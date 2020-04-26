#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include "ws2812.h"
#include "hsv_rgb.h"
#include <stdbool.h>

#define DOUT PC0
#define HH PC1
#define MM PC2
#define UPMIN (1<<MM)
#define UPHOUR (1<<HH)
#define BUTTONDOWN_RESET 20
volatile uint8_t buttonDown = 0;
volatile bool checkButton = false;
volatile bool updateDigits = false;
bool led = false;
#define _USE_DELAY


#define MAX_LED 18
uint16_t state = 0;
uint8_t curLed;
uint8_t colors[MAX_LED][3];
#define STATUS_LED 17
#define HOUR_START 12
#define MINUTE_START 6
#define SECOND_START 0


uint8_t seconds = 0;
uint8_t minutes = 0;
uint8_t hours = 0;
volatile uint8_t pSec = 0, qSec = 0;
#define QSEC_MAX 128

ISR(PCINT1_vect) {
  checkButton=true;
}
#ifndef _USE_DELAY
ISR(INT0_vect) {
  pSec++;
  if(!pSec) {
    qSec++;
  }
}
#endif

void updateDisplay();
void loop();

int main() {
  CLKPR = 0x80;   // allow writes to CLKPSR
  CLKPR = 0;   // disable system clock prescaler (run at full 8MHz)

  //setup PCI1 for PCINT9 and 10, for PC1 and 2
  PCMSK1 |= (1<<PCINT9) | (1<<PCINT10);
  //Setup PCINT0 to be enabled
  PCICR |= 1<<PCIE1;

  //Setup HH and MM as inputs, all other pins on port C as outputs (including DOUT)
  DDRC = ~((1<<HH) | (1<<MM));
  PORTC |= ((1<<HH) | (1<<MM));

#ifndef _USE_DELAY
  // Setup INT0 to trigger on falling edge
  EICRA |= (1<<ISC01) | (1<<ISC00);
  // Setup INT0 to be enabled
  EIMSK |= 1<<INT0;
#endif

  ws2812_init();
  updateDigits=true;

  sei();

  while(1) {
    loop();
  }
}

void loop() {
#ifndef _USE_DELAY
  if(qSec >= QSEC_MAX) {
    qSec-= QSEC_MAX;
#endif
#ifdef _USE_DELAY
  if(qSec >= 10) {
    qSec = 0;
#endif
    led = !led;
    seconds++;
    if(seconds == 60) {
      seconds = 0;
      minutes++;
      if(minutes == 60) {
        minutes = 0;
        hours++;
        if(hours == 24) {
          hours = 0;
        }
      }
    }
    updateDigits = true;
  }
  if(!checkButton && !updateDigits) {
    _delay_ms(100);
#ifdef _USE_DELAY
    qSec++;
#endif
    return;
  }
  if(checkButton) {
    uint8_t buttonState = (~PINC) & (UPMIN | UPHOUR);
    if(!buttonState) {
      checkButton=false;
      updateDigits = true;
      buttonDown = 0;
    } else {
      if(buttonDown == 0) {
        buttonDown = BUTTONDOWN_RESET;
        minutes = (minutes + (buttonState&UPMIN? 1 : 0)) % 60;
        if(buttonState&UPMIN) {
          seconds = 0;
        }
        hours = (hours + (buttonState&UPHOUR? 1 : 0)) % 24;
        updateDigits = true;
      }
      buttonDown--;
      _delay_ms(10);
    }
  }
  if(updateDigits) {
    updateDisplay();
    updateDigits=false;
  }
}

void updateDisplay() {
  state+=5;
  for(curLed = 0; curLed < MINUTE_START; curLed++) {
    if(!(1<<curLed & seconds)) {
      colors[curLed][0] = 0;
      colors[curLed][1] = 0;
      colors[curLed][2] = 0;
      continue;
    }
    getRGB(state, 50, colors[curLed]);
  }
  for(curLed = MINUTE_START; curLed < HOUR_START; curLed++) {
    if(!(1<<(curLed-6) & minutes)) {
      colors[curLed][0] = 0;
      colors[curLed][1] = 0;
      colors[curLed][2] = 0;
      continue;
    }
    getRGB(state+60, 50, colors[curLed]);
  }
  for(curLed = HOUR_START; curLed < STATUS_LED; curLed++) {
    if(!(1<<(curLed-12) & hours)) {
      colors[curLed][0] = 0;
      colors[curLed][1] = 0;
      colors[curLed][2] = 0;
      continue;
    }
    getRGB(state+120, 50, colors[curLed]);
  }
  getRGB(state+180, led? 50 : 0, colors[STATUS_LED]);
  cli();
  for(curLed = 0; curLed < MAX_LED; curLed++) {
    ws2812_set_single(colors[curLed][0],colors[curLed][1],colors[curLed][2]);
  }
  sei();
}
