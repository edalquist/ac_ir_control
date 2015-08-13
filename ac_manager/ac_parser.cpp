#include "application.h"
#include "ac_parser.h"

namespace AcManager {

void AcParser::updateStates(struct AcState* dest, int temp, double timer, enum FanSpeeds speed, enum AcModes mode, bool isSleep) {
  // Update the next index in the states array with the pushed data
  dest->timestamp = Time.now();
  dest->temp = temp;
  dest->timer = timer;
  dest->speed = speed;
  dest->mode = mode;
  dest->sleep = isSleep;
}

/**
 * Decode the number bits that control the 8 segment display
 */
int AcParser::decodeDigit(uint8_t digitBits, bool hasDecimal) {
  if (hasDecimal && (digitBits & 0b10000000) == 0b10000000) {
    // should have decimal but doesn't
    return -1;
  } else if (!hasDecimal && (digitBits & 0b10000000) == 0) {
    // Shouldn't have a decimal but does
    return -1;
  }

  // Ensure the top bit is true to make the switch work no matter the state of the decimal place
  uint8_t decimalMasked = digitBits | 0x80;
  switch (decimalMasked) {
    case 0x90:
      return 9;
    case 0x80:
      return 8;
    case 0xF8:
      return 7;
    case 0x82:
      return 6;
    case 0x92:
      return 5;
    case 0x99:
      return 4;
    case 0xB0:
      return 3;
    case 0xA4:
      return 2;
    case 0xF9:
      return 1;
    case 0xC0:
      return 0;
    default:
      return -1;
  }
}

double AcParser::decodeDisplayNumber(uint8_t tensBits, uint8_t onesBits, bool isTimer) {
  int tens = decodeDigit(tensBits, isTimer);
  int ones = decodeDigit(onesBits, false);

  // One of the digits must be invalid, return -1
  if (tens == -1 || ones == -1) {
    return -1;
  }

  if (isTimer) {
    // For the timer the number is a double between 0.0 and 9.9
    return tens + (ones / 10.0);
  } else {
    // Non timer the number is an int between 00 and 99
    return (tens * 10) + ones;
  }
}

AcModes AcParser::decodeAcMode(uint8_t modeFanBits) {
  // Mode bits are 4-6, mask out the rest
  uint8_t modeBitsMasked = modeFanBits & 0b01110000;

  // Logic is the same for V1_2 & V1_4
  switch (modeBitsMasked) {
    case 0x60: // 0110 0000
      return MODE_COOL;
    case 0x50: // 0101 0000
      return MODE_ECO;
    case 0x30: // 0011 0000
      return MODE_FAN;
    default:
      return MODE_INVALID;
  }
}

}
