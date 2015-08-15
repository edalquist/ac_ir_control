#include "application.h"
#include "ac_parser.h"
#include "ac_parser_v18.h"

namespace AcManager {

const uint8_t AcParserV18::HEADER_AND_MASK[DATA_LENGTH] = {0xFF, 0xFF, 0x00, 0x20, 0x98, 0xA0};
const uint8_t AcParserV18::NUMBER_BYTES[10] = {0x30, 0xFC, 0xA2, 0xA4, 0x6C, 0x25, 0x21, 0xBC, 0x20, 0x24};
const uint8_t AcParserV18::AC_MODE_BYTES[3] = {0xFB, 0xFD, 0xFE};
const uint8_t AcParserV18::FAN_SPEED_BYTES[4] = {0xBF, 0xFD, 0xFB, 0xF7};

bool AcParserV18::isTimer(uint8_t parseBuffer[], int pbLen) {
  return !(parseBuffer[2] & 0b00100000) && //bit 5 on byte 2 == 0
    !(parseBuffer[4] & 0b01100000) && //bits 5 & 6 on byte 4 == 0
    (parseBuffer[5] & 0b00000001); //bit 0 on byte 5 == 1
}

}
