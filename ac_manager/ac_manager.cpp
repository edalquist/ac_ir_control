#include "application.h"
#include "ac_display_reader.h"
#include "ac_ir_controller.h"
#include "ac_manager.h"

#define IR_LED   D6   //IR carrier output pin

#define CLOCK_PIN D2
#define INPUT_PIN D1

#define AC_CMD__ON_OFF        "10AF8877"
#define AC_CMD__TIMER         "10AF609F"
#define AC_CMD__FAN_SPEED_U   "10AF807F"
#define AC_CMD__FAN_SPEED_D   "10AF20DF"
#define AC_CMD__TEMP_TIMER_U  "10AF708F"
#define AC_CMD__TEMP_TIMER_D  "10AFB04F"
#define AC_CMD__COOL          "10AF906F"
#define AC_CMD__ENERGY_SAVER  "10AF40BF"
#define AC_CMD__AUTO_FAN      "10AFF00F"
#define AC_CMD__FAN_ONLY      "10AFE01F"
#define AC_CMD__SLEEP         "10AF00FF"

void setup() {
  initIrController("sendNEC", IR_LED);

  struct AcDisplayReaderConfig acConfig = AC_DISPLAY_READER_CONFIG_DEFAULTS;
  initAcDisplayReader(acConfig);

  Spark.function("setState", setState);
}

/**
 * Expects one of:
 *  70,MODE_ECO,FAN_AUTO (temp,acMode,fanSpeed)
 *  OFF
 *
 */
int setState(String command) {
  int start = Time.now();

  int temp;
  enum AcModes mode;
  enum FanSpeeds speed;

  if (command == "OFF") {
    temp = 0;
    mode = MODE_OFF;
    speed = FAN_OFF;
  } else {
    int firstComma = command.indexOf(",");
    if (firstComma == -1) {
      return 2;
    }

    int secondComma = command.indexOf(",", firstComma + 1);
    if (secondComma == -1) {
      return 3;
    }

    temp = command.substring(0, firstComma).toInt();
    if (temp <= 60 || temp >= 80) {
      return 4;
    }

    mode = getModeForName(command.substring(firstComma + 1, secondComma));
    if (mode == MODE_INVALID) {
      Spark.publish("MODE", command.substring(firstComma + 1, secondComma));
      return 5;
    }

    speed = getSpeedForName(command.substring(secondComma + 1));
    if (speed == FAN_INVALID) {
      Spark.publish("SPEED", command.substring(secondComma + 1));
      return 6;
    }
  }

  // Try to get AC to the correct state for up to 10 seconds
  while (Time.now() - start < 10) {
    // Special handling for OFF
    if (mode == MODE_OFF) {
      if (isAcOn()) {
        sendNEC(AC_CMD__ON_OFF);
      } else {
        break;
      }
    } else if (!isAcOn()) {
      // First turn it on if it is off
      sendNEC(AC_CMD__ON_OFF);
    } else {
      bool stable = true;
      if (mode != getAcMode()) {
        stable = false;
        switch (mode) {
          case MODE_FAN:
            Spark.publish("MODE", "MODE_FAN");
            sendNEC(AC_CMD__FAN_ONLY);
            break;
          case MODE_ECO:
            Spark.publish("MODE", "MODE_ECO");
            sendNEC(AC_CMD__ENERGY_SAVER);
            break;
          case MODE_COOL:
            Spark.publish("MODE", "MODE_COOL");
            sendNEC(AC_CMD__COOL);
            break;
        }
      }

      if (speed != getFanSpeed()) {
        stable = false;
        switch (speed) {
          case FAN_AUTO:
            Spark.publish("SPEED", "FAN_AUTO");
            sendNEC(AC_CMD__AUTO_FAN);
            break;
          case FAN_LOW:
            Spark.publish("SPEED", "FAN_LOW");
            sendNEC(AC_CMD__FAN_SPEED_D);
            sendNEC(AC_CMD__FAN_SPEED_D);
            sendNEC(AC_CMD__FAN_SPEED_D);
            break;
          case FAN_MEDIUM:
            Spark.publish("SPEED", "FAN_MEDIUM");
            sendNEC(AC_CMD__FAN_SPEED_D);
            sendNEC(AC_CMD__FAN_SPEED_D);
            sendNEC(AC_CMD__FAN_SPEED_D);
            sendNEC(AC_CMD__FAN_SPEED_U);
            break;
          case FAN_HIGH:
            Spark.publish("SPEED", "FAN_HIGH");
            sendNEC(AC_CMD__FAN_SPEED_U);
            sendNEC(AC_CMD__FAN_SPEED_U);
            break;
        }
      }

      if (mode != MODE_FAN && temp != getTemp()) {
        stable = false;
        int tempDiff = temp - getTemp();
        if (tempDiff < 0) {
          for (int i = 0; i < abs(tempDiff); i++) {
            sendNEC(AC_CMD__TEMP_TIMER_D);
          }
        } else {
          for (int i = 0; i < abs(tempDiff); i++) {
            sendNEC(AC_CMD__TEMP_TIMER_U);
          }
        }
      }

      // Yay at a stable state!
      if (stable) {
        return 0;
      }
    }

    // Wait for the AC to handle IR codes and the display to update
    delay(1000);
    processAcDisplayData();
  }

  if (Time.now() - start >= 30) {
    // Return 1000 for timeout
    return 1000;
  } else {
    // Return 0 for success
    return 0;
  }
}

void loop() {
  processAcDisplayData();

  // process display data here
  delay(5000); // Sleep 1 second
}
