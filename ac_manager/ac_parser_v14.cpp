#include "application.h"
#include "ac_parser.h"
#include "ac_parser_v14.h"

namespace AcManager {

const uint8_t AcParserV14::HEADER_AND_MASK[DATA_LENGTH] = {0x7F, 0x7F, 0x00, 0x80, 0x01, 0xFD};
const uint8_t AcParserV14::NUMBER_BYTES[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
const uint8_t AcParserV14::AC_MODE_BYTES[3] = {0xEF, 0xDF, 0xBF};
const uint8_t AcParserV14::FAN_SPEED_BYTES[4] = {0xF7, 0x00, 0xFB, 0xFD};

bool AcParserV14::isTimer(uint8_t parseBuffer[], int pbLen) {
  return !(parseBuffer[2] & 0b10000000) && //bit 7 on byte 2 == 0
    !(parseBuffer[4] & 0b10000000); //bits 7 on byte 4 == 0
}

}
