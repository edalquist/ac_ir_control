#include "ac_display_reader.h"

#ifndef AC_PARSER_H
#define AC_PARSER_H

/**
 * Structure that contains all of the displayed state of the AC unit
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
    virtual ~AcParser() {};
    virtual int getDataLength() = 0;
    virtual bool parseState(struct AcState* dest, uint8_t parseBuffer[], int pbLen) = 0;
  protected:
    virtual int decodeDigit(uint8_t digitBits, bool hasDecimal);
    virtual double decodeDisplayNumber(uint8_t tensBits, uint8_t onesBits, bool isTimer);
    virtual FanSpeeds decodeFanSpeed(uint8_t modeFanBits) = 0;
    virtual AcModes decodeAcMode(uint8_t modeFanBits);
    void updateStates(struct AcState* dest, int temp, double timer, enum FanSpeeds speed, enum AcModes mode, bool isSleep);
};

}

#endif
