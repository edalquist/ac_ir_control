
enum AcModels {
  V1_2,
  V1_4
};

// Data structures to track AC state
enum FanSpeeds {
  FAN_OFF,
  FAN_LOW,
  FAN_MEDIUM,
  FAN_HIGH,
  FAN_AUTO,
  FAN_INVALID
};

enum AcModes {
  MODE_OFF,
  MODE_FAN,
  MODE_ECO,
  MODE_COOL,
  MODE_INVALID
};

struct AcState {
  int temp;
  double timer;
  enum FanSpeeds speed;
  enum AcModes mode;
};
