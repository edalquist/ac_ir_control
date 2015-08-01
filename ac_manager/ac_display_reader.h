#ifndef AC_DISPLAY_READER_H
#define AC_DISPLAY_READER_H

// The shift register sees 5 or 6 bytes repeatedly, 30 is a reasonable common multiplier
#define BUFFER_LEN 30
#define UPDATE_TIME_MAX 500 // max time in micros the AC controller spends pushing data into the register

#define AC_STATES_LEN 5
#define AC_STABLE_STATES 2


/**
 * The different Frigidaire AC models supported
 */
enum AcModels {
  V1_2, //
  V1_4  //
};

/**
 * Various fan speed settings
 */
enum FanSpeeds {
  FAN_OFF,
  FAN_LOW,
  FAN_MEDIUM,
  FAN_HIGH,
  FAN_AUTO,
  FAN_INVALID
};

/**
 * Various ac modes
 */
enum AcModes {
  MODE_OFF,
  MODE_FAN,
  MODE_ECO,
  MODE_COOL,
  MODE_INVALID
};

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

void initAcDisplayReader(int cp, int ip, String statusVar, String dataVar, String setAcModelFuncName);
int setAcModel(String acModelName);

#endif
