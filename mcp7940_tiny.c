#include <stdint.h>
#include "mcp7940_tiny.h"
#include "twimaster/i2cmaster.h"
#include <stdbool.h>

// Initialize, and return if we were able to confirm the RTC exists
uint8_t mcp7940_init(void) {
  uint8_t failCode = i2c_start(MCP7940_ADDR + I2C_WRITE);
  if(failCode) {
    return failCode;
  }
  // Grab the current seconds register, which also has the oscillator enabled bit
  i2c_write(MCP7940_RTCSEC);
  i2c_rep_start(MCP7940_ADDR + I2C_READ);
  uint8_t secsVal = i2c_readNak();
  i2c_stop();
  secsVal = secsVal | (1<<MCP7940_ST);
  i2c_start_wait(MCP7940_ADDR + I2C_WRITE);
  i2c_write(MCP7940_RTCSEC);
  i2c_write(secsVal);
  i2c_stop();
  return false;
}
// Get the current seconds from the RTC
uint8_t mcp7940_getSeconds(void) {
  uint8_t secondsVal;
  i2c_start_wait(MCP7940_ADDR + I2C_WRITE);
  i2c_write(MCP7940_RTCSEC);
  i2c_rep_start(MCP7940_ADDR + I2C_READ);
  secondsVal = i2c_readNak();
  i2c_stop();
  // The MCP7940 stores its values as binary-encoded decimals for some dumb reason
  // For seconds and minutes, bits 0-3 are the ones digit, bits 4-6 are the tens digit
  return ((0b1110000 & secondsVal)>>4)*10 + (0b1111 & secondsVal);
}
// Get the current minutes from the RTC
uint8_t mcp7940_getMinutes(void) {
  uint8_t minutesVal;
  i2c_start_wait(MCP7940_ADDR + I2C_WRITE);
  i2c_write(MCP7940_RTCMIN);
  i2c_rep_start(MCP7940_ADDR + I2C_READ);
  minutesVal = i2c_readNak();
  i2c_stop();
  // The MCP7940 stores its values as binary-encoded decimals for some dumb reason
  // For seconds and minutes, bits 0-3 are the ones digit, bits 4-6 are the tens digit
  return (((0b111<<4) & minutesVal)>>4)*10 + (0b1111 & minutesVal);
}
// Get the current hours from the RTC
// If in 24 hour mode, will return the hour value 0-23
// If in 12 hour mode, will return the hour value 0-11 in bits 0-3 and bit 4 will be am(0)/pm(1), bit 5 will be 1
uint8_t mcp7940_getHours(void) {
  uint8_t hoursVal;
  i2c_start_wait(MCP7940_ADDR + I2C_WRITE);
  i2c_write(MCP7940_RTCHOUR);
  i2c_rep_start(MCP7940_ADDR + I2C_READ);
  hoursVal = i2c_readNak();
  i2c_stop();
  // The MCP7940 stores its values as binary-encoded decimals for some dumb reason
  // For hours, there are two options here:
  //  If in 12 hour mode, bit 6 will be 1 and bit 5 will indicate am(0) or pm(1)
  //   and bit 4 will indicate if the hour tens digit is 0 or 1
  //  If in 24 hour mode, bit 6 will be 0 and bits 4-5 will be the tens digit
  // Either way bits 0-3 will be the ones digit
  if(hoursVal & (1<<MCP7940_12_24)) {
    // 12 hour mode
    return (1<<5) | ((1<<MCP7940_AM_PM) & hoursVal ? (1<<4) : 0) | (((0b10000 & hoursVal)>>4)*10 + (0b1111 & hoursVal));
  } else {
    // 24 hour mode
    return ((0b110000 & hoursVal)>>4)*10 + (0b1111 & hoursVal);
  }
}

// Retrieve various control register settings
uint8_t mcp7940_getControlRegister(void) {
  uint8_t controlRegister;
  i2c_start_wait(MCP7940_ADDR + I2C_WRITE);
  i2c_write(MCP7940_CONTROL);
  i2c_rep_start(MCP7940_ADDR + I2C_READ);
  controlRegister = i2c_readNak();
  i2c_stop();
  return controlRegister;
}

// Enable various control register settings
void mcp7940_setControlRegister(uint8_t newSetting) {
  i2c_start_wait(MCP7940_ADDR + I2C_WRITE);
  i2c_write(MCP7940_CONTROL);
  i2c_write(newSetting);
  i2c_stop();
}

// Set the seconds to this new value; also can enable or disable the oscillator
void mcp7940_setSeconds(uint8_t newSeconds, bool enableOscillator) {
  newSeconds = newSeconds&0b111111;
  newSeconds = ((newSeconds / 10) << 4) | (newSeconds%10);
  newSeconds = newSeconds | (enableOscillator? 1<<MCP7940_ST : 0);
  i2c_start_wait(MCP7940_ADDR + I2C_WRITE);
  i2c_write(MCP7940_RTCSEC);
  i2c_write(newSeconds);
  i2c_stop();
}
// Set the minutes to this new value
void mcp7940_setMinutes(uint8_t newMinutes) {
  newMinutes = newMinutes&0b111111;
  newMinutes = ((newMinutes / 10) << 4) | (newMinutes%10);
  i2c_start_wait(MCP7940_ADDR + I2C_WRITE);
  i2c_write(MCP7940_RTCMIN);
  i2c_write(newMinutes);
  i2c_stop();
}
// Set the hours to this new value; also can set 12 hour mode (true) or 24 hour mode (false)
void mcp7940_setHours(uint8_t newHours, bool set12Hour) {
  newHours = 0b11111&newHours;
  newHours = (set12Hour? (newHours>12?1<<5:0) :0) | (((newHours/10)&(set12Hour?1:0b11)) <<4) | (newHours%10);
  i2c_start_wait(MCP7940_ADDR + I2C_WRITE);
  i2c_write(MCP7940_RTCHOUR);
  i2c_write(newHours);
  i2c_stop();
}

// Enable or disable using the battery backup
// If the battery backup is enabled, when main power is lost, the internal timekeeping will continue working
//  The device will not be externally operational, however, so i2c and the MFP will be disabled
void mcp7940_setBatteryBackup(bool enabled) {
  i2c_start_wait(MCP7940_ADDR + I2C_WRITE);
  i2c_write(MCP7940_RTCWKDAY);
  i2c_rep_start(MCP7940_ADDR + I2C_READ);
  uint8_t curSetting = i2c_readNak();
  i2c_stop();
  curSetting = ((~(1<<MCP7940_VBATEN))&curSetting) | (enabled? (1<<MCP7940_VBATEN):0 );
  i2c_start_wait(MCP7940_ADDR + I2C_WRITE);
  i2c_write(MCP7940_RTCWKDAY);
  i2c_write(curSetting);
  i2c_stop();
}
