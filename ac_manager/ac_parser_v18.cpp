#include "application.h"
#include "ac_parser.h"
#include "ac_parser_v18.h"

namespace AcManager {

bool AcParserV18::parseState(struct AcState* dest, uint8_t parseBuffer[], int pbLen) {
  return false;
}

FanSpeeds AcParserV18::decodeFanSpeed(uint8_t modeFanBits) {
  //TODO
  return FAN_OFF;
}

}
