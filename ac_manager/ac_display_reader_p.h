#include "application.h"
#include "ac_parser.h"

#ifndef AC_DISPLAY_READER_P_H
#define AC_DISPLAY_READER_P_H

// The shift register sees 5 or 6 bytes repeatedly, 30 is a reasonable common multiplier
#define BUFFER_LEN 30
#define UPDATE_TIME_MAX 500 // max time in micros the AC controller spends pushing data into the register

#define AC_STATES_LEN 5
#define AC_STABLE_STATES 2

static const char STATUS_TEMPLATE[] = "{\"temp\":%d,\"fan\":\"%s\",\"mode\":\"%s\",\"version\":\"%s\"}";

int setAcModel(String acModelName);
void clock_Interrupt_Handler();
void loadAcModel();
AcManager::AcParser* getAcParser();
void updateStates();
bool compareAcStates(struct AcState* s1, struct AcState* s2);
void copyAcStates(struct AcState* from, struct AcState* to);
void updateVariables(struct AcState* acState, bool force);

AcModes decodeAcMode(uint8_t modeFanBits);
FanSpeeds decodeFanSpeed(uint8_t modeFanBits);
double decodeDisplayNumber(uint8_t tensBits, uint8_t onesBits, bool isTimer);
int decodeDigit(uint8_t digitBits, bool hasDecimal);

#endif
