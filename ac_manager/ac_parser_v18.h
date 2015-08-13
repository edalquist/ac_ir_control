#ifndef AC_PARSER_V18_H
#define AC_PARSER_V18_H

#include "application.h"
#include "ac_parser.h"

namespace AcManager {

class AcParserV18 : public AcParser {
  public:
    int getDataLength() {
      // 6 byte status for V1.8
      return 6;
    };
    bool parseState(struct AcState* dest, uint8_t parseBuffer[], int pbLen);
  protected:
    FanSpeeds decodeFanSpeed(uint8_t modeFanBits);
};

}

#endif
