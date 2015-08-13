#include "application.h"
#include "ac_parser.h"
#include "ac_parser_v12.h"
#include "ac_parser_v14.h"
#include "ac_parser_v18.h"
#include "ac_display_reader.h"
#include "ac_display_reader_p.h"

// Global config
int clockPin;
int inputPin;

// Variables used by the ISR to track the shift register
unsigned int cycleStart = 0;
volatile uint8_t byteBuffer[BUFFER_LEN];
volatile uint8_t* currentByte = byteBuffer;

// Overall state tracking
AcModels acModel = V1_4;
long lastUpdate = Time.now(); // unix seconds of successful data parse
long lastMessage = 0; // unix seconds of of last message sent
int acStatesIndex = 0; // Circular buffer index into acStates array
struct AcState currentAcState = {.timestamp = -1, .temp = -1, .timer = -1.0, .speed = FAN_INVALID, .mode = MODE_INVALID, .sleep = false};
struct AcState acStates[AC_STATES_LEN] = {
  {.timestamp = -1, .temp = -1, .timer = -1.0, .speed = FAN_INVALID, .mode = MODE_INVALID, .sleep = false},
  {.timestamp = -1, .temp = -1, .timer = -1.0, .speed = FAN_INVALID, .mode = MODE_INVALID, .sleep = false},
  {.timestamp = -1, .temp = -1, .timer = -1.0, .speed = FAN_INVALID, .mode = MODE_INVALID, .sleep = false},
  {.timestamp = -1, .temp = -1, .timer = -1.0, .speed = FAN_INVALID, .mode = MODE_INVALID, .sleep = false}
};
struct AcDisplayReaderConfig config;
char statusJson[sizeof(STATUS_TEMPLATE) * 2];
char registerData[(BUFFER_LEN * 3) + 1];

// Define versioned parsers
AcManager::AcParserV12 acParserV12;
AcManager::AcParserV14 acParserV14;
AcManager::AcParserV18 acParserV18;

enum AcModes getModeForName(String modeName) {
  if (modeName == "MODE_OFF") {
    return MODE_OFF;
  } else if (modeName == "MODE_FAN") {
    return MODE_FAN;
  } else if (modeName == "MODE_ECO") {
    return MODE_ECO;
  } else if (modeName == "MODE_COOL") {
    return MODE_COOL;
  } else {
    return MODE_INVALID;
  }
}

enum FanSpeeds getSpeedForName(String speedName) {
  if (speedName == "FAN_OFF") {
      return FAN_OFF;
  } else if (speedName == "FAN_LOW") {
      return FAN_LOW;
  } else if (speedName == "FAN_MEDIUM") {
      return FAN_MEDIUM;
  } else if (speedName == "FAN_HIGH") {
      return FAN_HIGH;
  } else if (speedName == "FAN_AUTO") {
      return FAN_AUTO;
  } else {
      return FAN_INVALID;
  }
}


void initAcDisplayReader(struct AcDisplayReaderConfig cfg) {
  config = cfg;
  clockPin = config.clockPin;
  inputPin = config.inputPin;

  pinMode(clockPin, INPUT);
  pinMode(inputPin, INPUT);

  // Register display status variables
  Spark.variable(config.statusVar, &statusJson, STRING);
  Spark.variable(config.dataVar, &registerData, STRING);

  // Register control functions
  Spark.function(config.setAcModelFuncName, setAcModel);

  // Setup interrupt handler on rising edge of the register clock
  attachInterrupt(clockPin, clock_Interrupt_Handler, RISING);

  loadAcModel();

  updateVariables(&currentAcState, true);
}

bool isAcOn() {
  return currentAcState.speed != FAN_OFF && currentAcState.speed != FAN_INVALID;
}

int getTemp() {
  return currentAcState.temp;
}

double getTimer() {
  return currentAcState.timer;
}

enum FanSpeeds getFanSpeed() {
  return currentAcState.speed;
}

enum AcModes getAcMode() {
  return currentAcState.mode;
}

/**
 * ISR that reads the shift register data, the next bit read from the inputPin whenever the clockPin
 * goes high
 */
void clock_Interrupt_Handler() {
  // Track the start of each cycle
  // zero out the currentByte to start accumulating data
  int now = micros();
  if (cycleStart == 0 || (now - cycleStart) > UPDATE_TIME_MAX) {
    if (cycleStart != 0) {
      // New cycle means go to the next byte
      currentByte++;

      // Roll over to the start of the buffer once BUFFER_LEN is reached
      if (currentByte == (byteBuffer + BUFFER_LEN)) {
        currentByte = byteBuffer;
      }
    }

    // Record the update cycle start time for the current byte
    cycleStart = now;

    // Zero the current byte, some cycles don't push a full 8 bits into the register
    *currentByte = 0;
  }

  // On clock rise shift the data in the register by one
  *currentByte <<= 1;

  // If the input is high set the new bit to 1 otherwise leave it zero
  // faster way of reading pin D1: https://community.particle.io/t/reading-digits-from-dual-7-segment-10-pin-display/12989/6
  /*if ((GPIOB->IDR) & 0b01000000) {*/
  // Using digitalRead for core/photon cross compatibility
  if (digitalRead(inputPin) == HIGH) {
    *currentByte |= 1;
  }
}

void loadAcModel() {
  uint8_t modelFlag = EEPROM.read(1);
  switch (modelFlag) {
    default:
    case 12:
      acModel = V1_2;
      break;
    case 14:
      acModel = V1_4;
      break;
    case 18:
      acModel = V1_8;
      break;

  }
}

/**
 * Spark Function letting me switch AC Models while running
 * maybe read/write this in EEPROM
 *
 * http://docs.particle.io/core/firmware/#other-functions-eeprom
 */
int setAcModel(String acModelName) {
  if (acModelName == "V1_8") {
    acModel = V1_8;
    EEPROM.write(1, 18);
    return 18;
  } else if (acModelName == "V1_4") {
    acModel = V1_4;
    EEPROM.write(1, 14);
    return 14;
  } else {
    acModel = V1_2;
    EEPROM.write(1, 12);
    return 12;
  }
}

/**
 * Must be called in loop() to read the state of the AC display from the data collected by the ISR
 */
void processAcDisplayData() {
  uint8_t readBuffer[BUFFER_LEN];

  // Copy the volatile byteBuffer to a local buffer to minimize the time that interrupts are off
  noInterrupts();
  memcpy(readBuffer, (const uint8_t*) byteBuffer, BUFFER_LEN);
  // TODO use volatile uint8_t* currentByte = byteBuffer; to copy byteBuffer and fix ordering
  interrupts();


  // Dump the readBuffer to a the data variable each loop to make debugging easier
  for (int i = 0; i < BUFFER_LEN; i++) {
    sprintf(&registerData[i * 3], "%02x ", readBuffer[i]);
  }

  // Chunk the read buffer out into parse buffers and attempt to parse each one
  /*int pbLen = getStatusLength();*/
  AcManager::AcParser* acParser = getAcParser();
  int pbLen = acParser->getDataLength();

  uint8_t parseBuffer[pbLen];
  for (int rb = 0; rb < BUFFER_LEN; rb++) {
    for (int pb = 0; pb < pbLen; pb++) {
      parseBuffer[pb] = readBuffer[(rb + pb) % BUFFER_LEN];
    }
    struct AcState *acState = &acStates[acStatesIndex];
    bool parsed = acParser->parseState(acState, parseBuffer, pbLen);
    if (parsed) {
      // Skip next pbLen bytes, (the end of the successfull parsed buffer)
      rb = rb + pbLen - 1;

      // Update the shared variables based on the state
      updateStates();
    }
  }

  // If no update for staleInterval publish statusStale event
  int now = Time.now();
  if (now - lastMessage > config.staleInterval) {
    Spark.publish(config.statusStaleEventName, statusJson);
    lastMessage += now + config.staleInterval;
  }
}

void updateStates() {
  // Update most recent state with the current timestamp
  acStates[acStatesIndex].timestamp = Time.now();

  // Increment states index to the next position
  acStatesIndex = (acStatesIndex + 1) % AC_STATES_LEN;

  // Track the entry with the most matches, the index it exists at and the match count for every
  // entry
  int maxMatches = 0;
  int equivalentStates[AC_STATES_LEN] = { 0 };

  // iterate through acStates comparing each entry to each other entry
  for (int i = 0; i < AC_STATES_LEN; i++) {
    for (int o = i + 1; o < AC_STATES_LEN; o++) {
      if (compareAcStates(&acStates[i], &acStates[o])) {
        maxMatches = max(maxMatches, max(++equivalentStates[i], ++equivalentStates[o]));
      }
    }
  }

  // TODO figure out better way to add this debuggging
  /*if (maxMatches != AC_STATES_LEN - 1) {
    char message[80];
    sprintf(message, "mm:%d, [%d, %d, %d, %d, %d]", maxMatches, equivalentStates[0], equivalentStates[1], equivalentStates[2], equivalentStates[3], equivalentStates[4]);
    Spark.publish("EQUIV_STATES", message);
  }*/

  if (maxMatches >= AC_STABLE_STATES) {
    // Find the stable state with the most recent timestamp
    int mostRecent = 0;
    int maxIndex = -1;
    for (int i = 0; i < AC_STATES_LEN; i++) {
      if (equivalentStates[i] == maxMatches && acStates[i].timestamp > mostRecent) {
        mostRecent = acStates[i].timestamp;
        maxIndex = i;
      }
    }

    // Protect against OOB and then update the variables
    if (maxIndex >= 0 && maxIndex < AC_STATES_LEN) {
      updateVariables(&acStates[maxIndex], false);
    }
  }
}

void updateVariables(struct AcState* acState, bool force) {
  // Record if any of the data changed, used to decide if an event should be published
  bool match = compareAcStates(&currentAcState, acState);
  if (!force && match) {
    return;
  }

  // Copy the new state struct to the current state
  copyAcStates(acState, &currentAcState);

  char vSpeed[2];
  char vMode[2];
  char version[4];

  // TODO turn this into a function
  switch (currentAcState.speed) {
    case FAN_OFF:
      strncpy(vSpeed, "X", 2);
      break;
    case FAN_LOW:
      strncpy(vSpeed, "L", 2);
      break;
    case FAN_MEDIUM:
      strncpy(vSpeed, "M", 2);
      break;
    case FAN_HIGH:
      strncpy(vSpeed, "H", 2);
      break;
    case FAN_AUTO:
      strncpy(vSpeed, "A", 2);
      break;
    case FAN_INVALID:
    default:
      strncpy(vSpeed, "?", 2);
      break;
  }
  // TODO turn this into a function
  switch (currentAcState.mode) {
    case MODE_OFF:
      strncpy(vMode, "X", 2);
      break;
    case MODE_FAN:
      strncpy(vMode, "F", 2);
      break;
    case MODE_ECO:
      strncpy(vMode, "E", 2);
      break;
    case MODE_COOL:
      strncpy(vMode, "C", 2);
      break;
    case MODE_INVALID:
    default:
      strncpy(vMode, "?", 2);
      break;
  }
  // TODO turn this into a function
  switch (acModel) {
    case V1_2:
      strncpy(version, "1.2", 4);
      break;
    case V1_4:
      strncpy(version, "1.4", 4);
      break;
    case V1_8:
      strncpy(version, "1.8", 4);
      break;
    default:
      strncpy(version, "?.?", 4);
      break;
  }

  lastUpdate = currentAcState.timestamp;

  sprintf(statusJson, STATUS_TEMPLATE, currentAcState.temp, vSpeed, vMode, version);
  if (lastUpdate - lastMessage > 300) {
    Spark.publish(config.statusRefreshEventName, statusJson);
    lastMessage = Time.now();
  } else {
    Spark.publish(config.statusChangeEventName, statusJson);
    lastMessage = Time.now();
  }
}

AcManager::AcParser* getAcParser() {
  switch (acModel) {
    default:
    case V1_2:
      return &acParserV12;
    case V1_4:
      return &acParserV14;
    case V1_8:
      return &acParserV18;
  }
}

bool compareAcStates(struct AcState* s1, struct AcState* s2) {
  return s1->temp == s2->temp &&
    s1->timer == s2->timer &&
    s1->speed == s2->speed &&
    s1->mode == s2->mode &&
    s1->sleep == s2->sleep;
}

void copyAcStates(struct AcState* from, struct AcState* to) {
  to->temp = from->temp;
  to->timer = from->timer;
  to->speed = from->speed;
  to->mode = from->mode;
  to->sleep = from->sleep;
}
