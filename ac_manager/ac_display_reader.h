#ifndef AC_DISPLAY_READER_H
#define AC_DISPLAY_READER_H

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

struct AcDisplayReaderConfig {
  int clockPin;
  int inputPin;
  String statusVar;
  String dataVar;
  String setAcModelFuncName;
  int refreshInterval;
  String statusChangeEventName;
  String statusRefreshEventName;
  String parseErrorEventName;
};

const struct AcDisplayReaderConfig AC_DISPLAY_READER_CONFIG_DEFAULTS {
  .clockPin = D2,
  .inputPin = D1,
  .statusVar = "status",
  .dataVar = "data",
  .setAcModelFuncName = "setAcModel",
  .refreshInterval = 300,
  .statusChangeEventName = "STATUS_CHANGE",
  .statusRefreshEventName = "STATUS_REFRESH",
  .parseErrorEventName = "PARSE_ERROR"
};

void initAcDisplayReader(int cp, int ip, String statusVar, String dataVar, String setAcModelFuncName);
void processDisplay();

bool isOn();
int getTemp();
double getTimer();
enum FanSpeeds getFanSpeed();
enum AcModes getAcMode();
enum AcModels getAcModel();

#endif
