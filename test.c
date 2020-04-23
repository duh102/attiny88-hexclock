#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include "ws2812.h"
#include "hsv_rgb.h"

#define MAX_LED 18
uint16_t state = 0;
uint8_t curLed;
uint8_t colors[MAX_LED][3];
uint8_t seconds = 0;
uint8_t minutes = 0;
uint8_t hours = 0;
#define STATUS_LED 17
#define HOUR_START 12
#define MINUTE_START 6
#define SECOND_START 0

int main() {
  CLKPR = 0x80;   // allow writes to CLKPSR
  CLKPR = 0;   // disable system clock prescaler

  ws2812_init();
  while(1) {
    state+=60;
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
    getRGB(state+180, seconds%2? 50 : 0, colors[STATUS_LED]);
    cli();
    for(curLed = 0; curLed < MAX_LED; curLed++) {
      ws2812_set_single(colors[curLed][0],colors[curLed][1],colors[curLed][2]);
    }
    sei();
    _delay_ms(1000);
    seconds++;
    if(seconds == 60) {
        seconds = 0;
        minutes++;
        if(minutes == 60) {
            minutes = 0;
            hours = (hours+1) % 24;
        }
    }
  }
}
