#include "dim_curve.h"
#include <stdint.h>

void getRGB(uint16_t hue, uint8_t val, uint8_t colors[3]) {
  /* convert hue, saturation and brightness ( HSB/HSV ) to RGB
     The dim_curve is used only on brightness/value and on saturation (inverted).
     This looks the most natural.
  */

  val = dim_curve[val];
  hue = hue % 360;

  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t base;

  switch(hue/60) {
    case 0:
        r = val;
        g = ((val*hue)/60)+base;
        b = base;
    break;

    case 1:
        r = ((val*(60-(hue%60)))/60)+base;
        g = val;
        b = base;
    break;

    case 2:
        r = base;
        g = val;
        b = ((val*(hue%60))/60)+base;
    break;

    case 3:
        r = base;
        g = ((val*(60-(hue%60)))/60)+base;
        b = val;
    break;

    case 4:
        r = ((val*(hue%60))/60)+base;
        g = base;
        b = val;
    break;

    case 5:
        r = val;
        g = base;
        b = ((val*(60-(hue%60)))/60)+base;
    break;
  }

  colors[0]=r;
  colors[1]=g;
  colors[2]=b;
}
