
enum AcModels {
  V1_2,
  V1_4
};

// Data structures to track AC state
enum FanSpeeds {
  FAN_LOW,
  FAN_MEDIUM,
  FAN_HIGH,
  FAN_AUTO,
  FAN_INVALID
};

enum AcModes {
  MODE_FAN,
  MODE_ECO,
  MODE_COOL,
  MODE_INVALID
};

struct AcState {
  int temp;
  double timer;
  FanSpeeds speed;
  AcModes mode;
};


FanSpeeds decodeFanSpeed(uint8_t modeFanBits);

AcModes decodeAcMode(uint8_t modeFanBits);
