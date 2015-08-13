#include "application.h"
#include "ac_parser.h"
#include "ac_parser_v12.h"

namespace AcManager {

// "PARSE_ERROR" -> config.parseErrorEventName

bool AcParserV12::parseState(struct AcState* dest, uint8_t parseBuffer[], int pbLen) {
  if (pbLen != getDataLength()) {
    Spark.publish("PARSE_ERROR", "BAD LENGTH");
    // Something is wrong, skip parsing
    return false;
  }

  // Ensure header of "FF"
  if (parseBuffer[0] != 0xFF) {
    return false;
  }

  // If the next 4 bytes are 0xFF the unit is off
  bool isOff = true;
  for (int i = 1; i < pbLen && isOff; i++) {
    isOff = parseBuffer[i] == 0xFF;
  }
  if (isOff) {
    updateStates(dest, 0, 0, FAN_OFF, MODE_OFF, false);
    return true;
  }

  // If bit 5 of byte 3 is false timer is on
  bool isTimer;
  uint8_t timerMasked = parseBuffer[3] & 0b11110000;
  switch (timerMasked) {
    case 0xD0:
      isTimer = true;
      break;
    case 0x70:
      isTimer = false;
      break;
    default:
      // Timer bits are invalid
      char msg[20];
      sprintf(msg, "INVALID TIMER: %02x", parseBuffer[3]);
      Spark.publish("PARSE_ERROR", msg);
      return false;
  }

  // If bit 5 of byte 3 is false sleep is on
  bool isSleep;
  uint8_t sleepMasked = parseBuffer[3] & 0b00001111;
  switch (sleepMasked) {
    case 0x0E:
      isSleep = true;
      break;
    case 0x0F:
      isSleep = false;
      break;
    default:
      // Timer bits are invalid
      char msg[20];
      sprintf(msg, "INVALID SLEEP: %02x", parseBuffer[3]);
      Spark.publish("PARSE_ERROR", msg);
      return false;
  }

  double display = decodeDisplayNumber(parseBuffer[1], parseBuffer[2], isTimer);
  if (display == -1) {
    // Display digits were invalid, ignore buffer
    char msg[20];
    sprintf(msg, "INVALID DISPLAY: %02x %02x", parseBuffer[1], parseBuffer[2]);
    Spark.publish("PARSE_ERROR", msg);
    return false;
  }

  enum AcModes acMode = decodeAcMode(parseBuffer[4]);
  if (acMode == MODE_INVALID) {
    // AC Mode was invalid, ignore buffer
    char msg[20];
    sprintf(msg, "INVALID MODE: %02x", parseBuffer[4]);
    Spark.publish("PARSE_ERROR", msg);
    return false;
  }

  enum FanSpeeds fanSpeed = decodeFanSpeed(parseBuffer[4]);
  if (fanSpeed == FAN_INVALID) {
    // Fan Speed was invalid, ignore buffer
    char msg[20];
    sprintf(msg, "INVALID FAN: %02x", parseBuffer[4]);
    Spark.publish("PARSE_ERROR", msg);
    return false;
  }

  if (isTimer) {
    updateStates(dest, 0, display, fanSpeed, acMode, false);
  } else {
    updateStates(dest, (int) display, 0, fanSpeed, acMode, false);
  }

  return true;
}

FanSpeeds AcParserV12::decodeFanSpeed(uint8_t modeFanBits) {
  // Fan bits are 0-3, mask out the rest
  uint8_t fanBitsMasked = modeFanBits & 0b00001111;

  switch (fanBitsMasked) {
    case 0x07:
      return FAN_LOW;
    case 0x0B:
      return FAN_MEDIUM;
    case 0x0D:
      return FAN_HIGH;
    case 0x0E:
      return FAN_AUTO;
    default:
      return FAN_INVALID;
  }
}

}
