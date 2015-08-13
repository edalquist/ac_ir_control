#ifndef AC_PARSER_V12_H
#define AC_PARSER_V12_H

#include "application.h"
#include "ac_parser.h"

namespace AcManager {

class AcParserV12 : public AcParser {
  public:
    int getDataLength() {
      // 5 byte status for V1.2
      return 5;
    };
    bool parseState(struct AcState* dest, uint8_t parseBuffer[], int pbLen);
  protected:
    FanSpeeds decodeFanSpeed(uint8_t modeFanBits);

};

}

#endif
