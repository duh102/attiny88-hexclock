#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include "ws2812.h"
#include "hsv_rgb.h"
#include <stdbool.h>
#include <avr/pgmspace.h>
#include "twimaster/i2cmaster.h"
#include "mcp7940_tiny.h"

#define DOUT PC0
#define SQW PD2
#define HH PC1
#define MM PC2
#define UPMIN (1<<MM)
#define UPHOUR (1<<HH)
#define BUTTONDOWN_RESET 20
volatile uint8_t buttonDown = 0;
volatile bool checkButton = false;
volatile bool updateDigits = false;
volatile bool led = false;

uint8_t digit_value = 0;
uint8_t temp0 = 0;

#define MAX_LED 18
// The state of the rainbow
uint16_t state = 0;
// reserving a byte for loop variant
uint8_t curLed;
// reserving 3*(leds) bytes for keeping the data easily accessible
uint8_t colors[MAX_LED][3];

#define STATUS_D 17
// The start of the 10s place in hour
#define HH_D 12
// The start of the 10s place in minute
#define MM_D 6
// The start of the 10s place in second
#define SS_D 0

volatile uint8_t seconds = 99;
volatile uint8_t minutes = 99;
volatile uint8_t hours = 99;

ISR(PCINT1_vect) {
  checkButton=true;
}
ISR(INT0_vect) {
  seconds++;
  led = !led;
  updateDigits = true;
}

void updateDisplay();
void loop();

int main() {
  CLKPR = 1<<CLKPCE;   // allow writes to CLKPR
  CLKPR = 0;   // disable system clock prescaler (run at full 8MHz)

  //setup PCI1 for PCINT9 and 10, for PC1 and 2
  PCMSK1 |= (1<<PCINT9) | (1<<PCINT10);
  //Setup PCINT1 to be enabled
  PCICR |= 1<<PCIE1;

  //Setup HH and MM as inputs, all other pins on port C as outputs
  DDRC = (uint8_t)( ~(UPMIN | UPHOUR));
  PORTC |= (UPMIN | UPHOUR);

  // Setup INT0 to trigger on falling edge
  EICRA = 1<<ISC01;
  // Setup INT0 to be enabled
  EIMSK = 1<<INT0;

  // Enable the display
  ws2812_init();

  // Enable I2C communication
  i2c_init();
  // Enable the RTC
  uint8_t failCode = 1;
  while(failCode) {
    failCode = mcp7940_init();
    if(failCode) {
      _delay_ms(100);
    }
  }

  mcp7940_setControlRegister( (1<<MCP7940_SQWEN) | SQWV_1HZ );

  mcp7940_setBatteryBackup(true);

  seconds = mcp7940_getSeconds();

  minutes = mcp7940_getMinutes();

  // bit 5 indicates whether we're in 12 or 24 hour mode
  hours = mcp7940_getHours();

  if(hours & (1<<5)) {
    //we want to be in 24 hour mode
    mcp7940_setHours( (hours&(1<<4)?12:0) + (hours&0b111), false);
    hours = mcp7940_getHours();
  }
  hours = hours & 0b11111;

  updateDigits=true;
  while(1) {
    loop();
  }
}

void loop() {
  if(seconds>59) {
    seconds = seconds % 60;
    minutes++;
    if(minutes == 60) {
      minutes = mcp7940_getMinutes();
      hours = mcp7940_getHours();
      hours = hours & 0b11111;
    }
  }
  if(!checkButton && !updateDigits) {
    _delay_ms(100);
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
        if(buttonState&UPMIN) {
          minutes = (minutes + 1) % 60;
          seconds = 0;
          mcp7940_setSeconds(seconds, true);
          mcp7940_setMinutes(minutes);
        }
        if(buttonState&UPHOUR) {
          hours = (hours + 1) % 24;
          mcp7940_setHours(hours, false);
        }
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
  // seconds
  digit_value = seconds & 0b111111;
  for(curLed = SS_D; curLed < MM_D; curLed++) {
    temp0 = curLed-SS_D;
    if( !(digit_value & (1<<temp0)) ) {
      colors[curLed][0] = 0;
      colors[curLed][1] = 0;
      colors[curLed][2] = 0;
      continue;
    }
    getRGB(state+(14*curLed), 50, colors[curLed]);
  }
  // minutes
  digit_value = minutes & 0b111111;
  for(curLed = MM_D; curLed < HH_D; curLed++) {
    temp0 = curLed-MM_D;
    if( !(digit_value & (1<<temp0)) ) {
      colors[curLed][0] = 0;
      colors[curLed][1] = 0;
      colors[curLed][2] = 0;
      continue;
    }
    getRGB(state+(14*curLed), 50, colors[curLed]);
  }
  // hours
  digit_value = hours & 0b11111;
  for(curLed = HH_D; curLed < STATUS_D; curLed++) {
    temp0 = curLed-HH_D;
    if( !(digit_value & (1<<temp0)) ) {
      colors[curLed][0] = 0;
      colors[curLed][1] = 0;
      colors[curLed][2] = 0;
      continue;
    }
    getRGB(state+(14*curLed), 50, colors[curLed]);
  }
  if(led) {
    getRGB(state+(14*STATUS_D), 50, colors[STATUS_D]);
  } else {
    colors[STATUS_D][0] = 0;
    colors[STATUS_D][1] = 0;
    colors[STATUS_D][2] = 0;
  }
  cli();
  for(curLed = 0; curLed < MAX_LED; curLed++) {
    ws2812_set_single(colors[curLed][0],colors[curLed][1],colors[curLed][2]);
  }
  sei();
}
