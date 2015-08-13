#include "application.h"
#include "ac_parser.h"
#include "ac_parser_v14.h"

namespace AcManager {

bool AcParserV14::parseState(struct AcState* dest, uint8_t parseBuffer[], int pbLen) {
/*

  if (pbLen != getStatusLength()) {
    Spark.publish(config.parseErrorEventName, "BAD LENGTH");
    // Something is wrong, skip parsing
    return false;
  }

  // Ensure the first two bytes are the header
  if (parseBuffer[0] != 0x7F || parseBuffer[1] != 0x7F) {
    return false;
  }

  // If th next 4 bytes are 0xFF the unit is off
  bool isOff = true;
  for (int i = 2; i < pbLen && isOff; i++) {
    isOff = parseBuffer[i] == 0xFF;
  }
  if (isOff) {
    updateStates(0, 0, FAN_OFF, MODE_OFF, false);
    return false;
  }

  // All valid buffers have the FD for the last byte
  if (parseBuffer[5] != 0xFD) {
    return false;
  }

  // If bit 7 of byte 4 is false timer is on
  bool isTimer = (parseBuffer[4] & 0b1000000) != 0b1000000;

  double display = decodeDisplayNumber(parseBuffer[2], parseBuffer[3], isTimer);
  if (display == -1) {
    // Display digits were invalid, ignore buffer
    char msg[20];
    sprintf(msg, "INVALID DISPLAY: %02x %02x", parseBuffer[2], parseBuffer[3]);
    Spark.publish(config.parseErrorEventName, msg);
    return false;
  }

  enum AcModes acMode = decodeAcMode(parseBuffer[4]);
  if (acMode == MODE_INVALID) {
    // AC Mode was invalid, ignore buffer
    char msg[20];
    sprintf(msg, "INVALID MODE: %02x", parseBuffer[4]);
    Spark.publish(config.parseErrorEventName, msg);
    return false;
  }

  enum FanSpeeds fanSpeed = decodeFanSpeed(parseBuffer[4]);
  if (fanSpeed == FAN_INVALID) {
    // Fan Speed was invalid, ignore buffer
    char msg[20];
    sprintf(msg, "INVALID FAN: %02x", parseBuffer[4]);
    Spark.publish(config.parseErrorEventName, msg);
    return false;
  }

  if (isTimer) {
    updateStates(0, display, fanSpeed, acMode, false);
  } else {
    updateStates((int) display, 0, fanSpeed, acMode, false);
  }

  return true;
*/
  return false;
}


FanSpeeds AcParserV14::decodeFanSpeed(uint8_t modeFanBits) {
  // Fan bits are 0-3, mask out the rest
  uint8_t fanBitsMasked = modeFanBits & 0b00001111;

  switch (fanBitsMasked) {
    case 0x07:
      return FAN_LOW;
    case 0x0B:
      return FAN_HIGH;
    case 0x0D:
      return FAN_AUTO;
    default:
      return FAN_INVALID;
  }
}

}
