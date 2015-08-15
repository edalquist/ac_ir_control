#include "application.h"
#include "ac_parser.h"
#include "ac_parser_v12.h"

namespace AcManager {

const uint8_t AcParserV12::HEADER_AND_MASK[DATA_LENGTH] = {0xFF, 0x00, 0x80, 0x5E, 0x00};
const uint8_t AcParserV12::NUMBER_BYTES[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
const uint8_t AcParserV12::AC_MODE_BYTES[3] = {0xEF, 0xDF, 0xBF};
const uint8_t AcParserV12::FAN_SPEED_BYTES[4] = {0xF7, 0xFB, 0xFD, 0xFE};

bool AcParserV12::isTimer(uint8_t parseBuffer[], int pbLen) {
  return !(parseBuffer[1] & 0b10000000) && //bit 7 on byte 1 == 0
    !(parseBuffer[3] & 0b00100000) && //bits 5 on byte 3 == 0
    (parseBuffer[3] & 0b10000000); //bits 7 on byte 3 == 1
}

}
