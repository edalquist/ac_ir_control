#ifndef AC_PARSER_V14_H
#define AC_PARSER_V14_H

#include "application.h"
#include "ac_parser.h"

namespace AcManager {

class AcParserV14 : public AcParser {
  static const uint8_t HEADER_LENGTH = 2;
  static const uint8_t DATA_LENGTH = 6;
  static const uint8_t HEADER_AND_MASK[DATA_LENGTH];
  static const uint8_t DECIMAL_BIT_MASK = 0b10000000;
  static const uint8_t NUMBER_BYTES[10];
  static const uint8_t AC_MODE_BYTE_INDEX = 4;
  static const uint8_t AC_MODE_BIT_MASK = 0b10001111;
  static const uint8_t AC_MODE_BYTES[3];
  static const uint8_t FAN_SPEED_BYTE_INDEX = 4;
  static const uint8_t FAN_SPEED_BIT_MASK = 0b11110001;
  static const uint8_t FAN_SPEED_BYTES[4];

  public:
    AcParserV14() : AcParser(
      HEADER_LENGTH,
      HEADER_AND_MASK,
      DECIMAL_BIT_MASK,
      NUMBER_BYTES,
      AC_MODE_BYTE_INDEX,
      AC_MODE_BIT_MASK,
      AC_MODE_BYTES,
      FAN_SPEED_BYTE_INDEX,
      FAN_SPEED_BIT_MASK,
      FAN_SPEED_BYTES
    ) {};
    int getDataLength() {
      return DATA_LENGTH;
    };
  protected:
    bool isTimer(uint8_t parseBuffer[], int pbLen);
};

}

#endif
