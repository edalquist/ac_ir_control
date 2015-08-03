#ifndef AC_DISPLAY_READER_P_H
#define AC_DISPLAY_READER_P_H

// The shift register sees 5 or 6 bytes repeatedly, 30 is a reasonable common multiplier
#define BUFFER_LEN 30
#define UPDATE_TIME_MAX 500 // max time in micros the AC controller spends pushing data into the register

#define AC_STATES_LEN 5
#define AC_STABLE_STATES 2

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

static const char STATUS_TEMPLATE[] = "{\"temp\":%d,\"fan\":\"%s\",\"mode\":\"%s\"}";

int setAcModel(String acModelName);
void clock_Interrupt_Handler();
bool parseData(uint8_t parseBuffer[], int pbLen);
bool parseDataV12(uint8_t parseBuffer[], int pbLen);
bool parseDataV14(uint8_t parseBuffer[], int pbLen);
int getStatusLength();
void updateStates(int temp, double timer, enum FanSpeeds speed, enum AcModes mode, bool isSleep);
AcModes decodeAcMode(uint8_t modeFanBits);
FanSpeeds decodeFanSpeed(uint8_t modeFanBits);
double decodeDisplayNumber(uint8_t tensBits, uint8_t onesBits, bool isTimer);
int decodeDigit(uint8_t digitBits, bool hasDecimal);
bool compareAcStates(struct AcState* s1, struct AcState* s2);
void updateVariables(struct AcState* acState);
void copyAcStates(struct AcState* from, struct AcState* to);
void loadAcModel();

#endif
