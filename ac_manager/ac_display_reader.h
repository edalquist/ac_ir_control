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

struct AcDisplayReaderConfig {
  int clockPin;
  int inputPin;
  String statusVar;
  String dataVar;
  String setAcModelFuncName;
  int refreshInterval;
  int staleInterval;
  String statusChangeEventName;
  String statusRefreshEventName;
  String statusStaleEventName;
  String parseErrorEventName;
};

const struct AcDisplayReaderConfig AC_DISPLAY_READER_CONFIG_DEFAULTS {
  .clockPin = D2,
  .inputPin = D1,
  .statusVar = "status",
  .dataVar = "data",
  .setAcModelFuncName = "setAcModel",
  .refreshInterval = 300,
  .staleInterval = 120,
  .statusChangeEventName = "STATUS_CHANGE",
  .statusRefreshEventName = "STATUS_REFRESH",
  .statusStaleEventName = "STATUS_STALE",
  .parseErrorEventName = "PARSE_ERROR"
};

void initAcDisplayReader(struct AcDisplayReaderConfig config);
void processAcDisplayData();

bool isAcOn();
int getTemp();
double getTimer();
enum FanSpeeds getFanSpeed();
enum AcModes getAcMode();
enum AcModels getAcModel();
enum AcModes getModeForName(String modeName);
enum FanSpeeds getSpeedForName(String speedName);

#endif
