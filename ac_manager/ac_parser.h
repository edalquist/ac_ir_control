#include "ac_display_reader.h"

#ifndef AC_PARSER_H
#define AC_PARSER_H

/**
 * Structure that contains all of the displayed state of the AC unit
 * TODO namespace
 */
struct AcState {
  int timestamp;
  int temp;
  double timer;
  enum FanSpeeds speed;
  enum AcModes mode;
  bool sleep;
};

namespace AcManager {

class AcParser {
  public:
    AcParser(uint8_t hl, const uint8_t *ham, uint8_t dbm, const uint8_t *nb, uint8_t ambi, uint8_t ambm, const uint8_t *amb, uint8_t fsbi, uint8_t fsbm, const uint8_t *fsb) :
      headerLength(hl),
      headerAndMask(ham),
      decimalBitMask(dbm),
      numberBytes(nb),
      acModeByteIndex(ambi),
      acModeBitMask(ambm),
      acModeBytes(amb),
      fanSpeedByteIndex(fsbi),
      fanSpeedBitMask(fsbm),
      fanSpeedBytes(fsb) {}

    virtual ~AcParser() {};
    virtual int getDataLength() = 0;
    bool parseState(struct AcState* dest, uint8_t parseBuffer[], int pbLen);
  protected:
    const uint8_t headerLength; // Number of bytes in the header
    const uint8_t *headerAndMask; // Contains the header bytes and then "bits that must by 1" and-mask Size must be equal to getDataLength()
    const uint8_t decimalBitMask; // and-mask that denotes the decimal bit
    const uint8_t *numberBytes; // The ten number bytes to compare against, decimal bit must be 1
    const uint8_t acModeByteIndex; // The index of the byte that contains the ac mode data
    const uint8_t acModeBitMask; // The or-mask for the mode byte
    const uint8_t *acModeBytes; // 3 byte array in {MODE_COOL, MODE_ECO, MODE_FAN} order
    const uint8_t fanSpeedByteIndex; // The index of the byte that contains the fan speed data
    const uint8_t fanSpeedBitMask; // The or-mask for the mode byte
    const uint8_t *fanSpeedBytes; // 4 byte array in {FAN_LOW, FAN_MEDIUM, FAN_HIGH, FAN_AUTO} order

    virtual bool isTimer(uint8_t parseBuffer[], int pbLen) = 0;

  private:
    int decodeDigit(uint8_t digitBits, bool hasDecimal);
    double decodeDisplayNumber(uint8_t tensBits, uint8_t onesBits, bool isTimer);
    FanSpeeds decodeFanSpeed(uint8_t modeFanBits);
    AcModes decodeAcMode(uint8_t modeFanBits);
    void updateStates(struct AcState* dest, int temp, double timer, enum FanSpeeds speed, enum AcModes mode, bool isSleep);
};

}

#endif
