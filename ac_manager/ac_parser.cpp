#include "application.h"
#include "ac_parser.h"

namespace AcManager {

  // "PARSE_ERROR" -> config.parseErrorEventName

bool AcParser::parseState(struct AcState* dest, uint8_t parseBuffer[], int pbLen) {
  if (pbLen != getDataLength()) {
    Spark.publish("PARSE_ERROR", "BAD LENGTH");
    // Something is wrong, skip parsing
    return false;
  }

  // Verify the header bytes in the parser buffer match the header array
  for (int i = 0; i < headerLength; i++) {
    if (parseBuffer[i] != headerAndMask[i]) {
      return false;
    }
  }

  // Check the rest of the buffer for the unit being OFF (all bytes 0xFF) or verify the rest of the
  // bytes match the expected mask. This ensures that bits that should alwasy be 1 are without
  // having to add that logic into the various parsing bits
  bool isOff = true;
  bool maskMatches = true;
  for (int i = headerLength; i < pbLen; i++) {
    isOff = isOff && parseBuffer[i] == 0xFF;
    maskMatches = maskMatches && ((parseBuffer[i] & headerAndMask[i]) == headerAndMask[i]);
  }
  if (isOff) {
    updateStates(dest, 0, 0, FAN_OFF, MODE_OFF, false);
    return true;
  }
  if (!maskMatches) {
    char msg[20];
    sprintf(msg, "INVALID MASKED BITS"); // TODO print parseBuffer
    Spark.publish("PARSE_ERROR", msg);
    return false;
  }

  bool timer = isTimer(parseBuffer, pbLen);

  uint8_t tensBits = parseBuffer[headerLength];
  uint8_t onesBits = parseBuffer[headerLength + 1];
  double display = decodeDisplayNumber(tensBits, onesBits, timer);
  if (display == -1) {
    // Display digits were invalid, ignore buffer
    char msg[20];
    sprintf(msg, "INVALID DISPLAY: hl:%d %02x %02x", headerLength, tensBits, onesBits);
    Spark.publish("PARSE_ERROR", msg);
    return false;
  }

  uint8_t acModeBits = parseBuffer[acModeByteIndex];
  enum AcModes acMode = decodeAcMode(acModeBits);
  if (acMode == MODE_INVALID) {
    // AC Mode was invalid, ignore buffer
    char msg[20];
    sprintf(msg, "INVALID MODE: %02x", acModeBits);
    Spark.publish("PARSE_ERROR", msg);
    return false;
  }

  uint8_t fanSpeedBits = parseBuffer[fanSpeedByteIndex];
  enum FanSpeeds fanSpeed = decodeFanSpeed(fanSpeedBits);
  if (fanSpeed == FAN_INVALID) {
    // Fan Speed was invalid, ignore buffer
    char msg[20];
    sprintf(msg, "INVALID FAN: %02x", fanSpeedBits);
    Spark.publish("PARSE_ERROR", msg);
    return false;
  }

  if (timer) {
    updateStates(dest, 0, display, fanSpeed, acMode, false);
  } else {
    updateStates(dest, (int) display, 0, fanSpeed, acMode, false);
  }

  return true;
}

void AcParser::updateStates(struct AcState* dest, int temp, double timer, enum FanSpeeds speed, enum AcModes mode, bool isSleep) {
  // Update the next index in the states array with the pushed data
  dest->timestamp = Time.now();
  dest->temp = temp;
  dest->timer = timer;
  dest->speed = speed;
  dest->mode = mode;
  dest->sleep = isSleep;
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

/**
 * Decode the number bits that control the 8 segment display
 */
int AcParser::decodeDigit(uint8_t digitBits, bool shouldHaveDecimal) {
  bool hasDecimal = (digitBits & decimalBitMask) != decimalBitMask;
  if (shouldHaveDecimal != hasDecimal) {
    // decimal in the wrong state
    return -1;
  }

  // Ensure the decimal bit is true to make the switch work no matter the state of the decimal place
  uint8_t decimalMasked = digitBits | decimalBitMask;
  for (int i = 0; i < 10; i++) {
    if (numberBytes[i] == decimalMasked) {
      return i;
    }
  }

  return -1;
}

AcModes AcParser::decodeAcMode(uint8_t acModeBits) {
  uint8_t acModeBitsMasked = acModeBits | acModeBitMask;

  for (int i = 0; i < 3; i++) {
    if (acModeBytes[i] == acModeBitsMasked) {
      switch (i) {
        case 0:
          return MODE_COOL;
        case 1:
          return MODE_ECO;
        case 2:
          return MODE_FAN;
      }
    }
  }

  return MODE_INVALID;
}

FanSpeeds AcParser::decodeFanSpeed(uint8_t fanSpeedBits) {
  uint8_t fanSpeedBitsMasked = fanSpeedBits | fanSpeedBitMask;

  for (int i = 0; i < 4; i++) {
    if (fanSpeedBytes[i] == fanSpeedBitsMasked) {
      switch (i) {
        case 0:
          return FAN_LOW;
        case 1:
          return FAN_MEDIUM;
        case 2:
          return FAN_HIGH;
        case 3:
          return FAN_AUTO;
      }
    }
  }

  return FAN_INVALID;
}

}
