/**
 * Code to read the data being sent to a shift register and decode it. In this case decoding the
 * display status of a Frigidaire AC unit.
 *
 * See for context:
 * https://community.particle.io/t/reading-digits-from-dual-7-segment-10-pin-display/12989
 */

#include "types.h"

#define CLOCK_PIN D2
#define INPUT_PIN D1

// The shift register sees 5 or 6 bytes repeatedly, 30 is a reasonable common multiplier
#define BUFFER_LEN 30
#define UPDATE_TIME_MAX 500 // max time in micros the AC controller spends pushing data into the register

#define AC_STATES_LEN 5
#define AC_STABLE_STATES 2

// Variables used by the ISR to track the shift register
unsigned int cycleStart = 0;
volatile uint8_t byteBuffer[BUFFER_LEN];
volatile uint8_t* currentByte = byteBuffer;


// Overall state tracking
AcModels acModel = V1_2;
long lastUpdate = 0; // unix seconds of successful data parse
long lastMessage = 0; // unix seconds of of last message sent
int acStatesIndex = 0; // Circular buffer index into acStates array
struct AcState currentAcState;
struct AcState acStates[AC_STATES_LEN] = {
  {0, 0, 0.0, FAN_OFF, MODE_OFF},
  {0, 0, 0.0, FAN_OFF, MODE_OFF},
  {0, 0, 0.0, FAN_OFF, MODE_OFF},
  {0, 0, 0.0, FAN_OFF, MODE_OFF},
  {0, 0, 0.0, FAN_OFF, MODE_OFF}
};


static const char STATUS_TEMPLATE[] = "{\"temp\":%d,\"fan\":\"%s\",\"mode\":\"%s\"}";
// Spark variables
char statusJson[sizeof(STATUS_TEMPLATE) * 2];
char registerData[(BUFFER_LEN * 3) + 1];

/**
 * Spark Function letting me switch AC Models while running
 * maybe read/write this in EEPROM
 *
 * http://docs.particle.io/core/firmware/#other-functions-eeprom
 */
int setAcModel(String acModelName) {
  if (acModelName == "V1_2") {
    acModel = V1_2;
    return 12;
  } else {
    // Default to V1_4
    acModel = V1_4;
    return 14;
  }
}

void setup() {
  Serial.begin(115200); //change BAUD rate as required

  pinMode(CLOCK_PIN, INPUT);
  pinMode(INPUT_PIN, INPUT);

  // Register display status variables
  Spark.variable("status", &statusJson, STRING);
  Spark.variable("data", &registerData, STRING);

  // Register control functions
  Spark.function("setAcModel", setAcModel);

  // Setup interrupt handler on rising edge of the register clock
  attachInterrupt(CLOCK_PIN, clock_Interrupt_Handler, RISING);
}

/**
 * Decode the number bits that control the 8 segment display
 */
int decodeDigit(uint8_t digitBits, bool hasDecimal) {
  if (hasDecimal && (digitBits & 0b10000000) == 0b10000000) {
    // should have decimal but doesn't
    return -1;
  } else if (!hasDecimal && (digitBits & 0b10000000) == 0) {
    // Shouldn't have a decimal but does
    return -1;
  }

  // Ensure the top bit is true to make the switch work no matter the state of the decimal place
  uint8_t decimalMasked = digitBits | 0x80;
  switch (decimalMasked) {
    case 0x90:
      return 9;
    case 0x80:
      return 8;
    case 0xF8:
      return 7;
    case 0x82:
      return 6;
    case 0x92:
      return 5;
    case 0x99:
      return 4;
    case 0xB0:
      return 3;
    case 0xA4:
      return 2;
    case 0xF9:
      return 1;
    case 0xC0:
      return 0;
    default:
      return -1;
  }
}

double decodeDisplayNumber(uint8_t tensBits, uint8_t onesBits, bool isTimer) {
  int tens = decodeDigit(tensBits, isTimer);
  int ones = decodeDigit(onesBits, false);

  // One of the digits must be invalid, return -1
  if (tens == -1 || ones == -1) {
    return -1;
  }

  if (isTimer) {
    // For the timer the number is a double between 0.0 and 9.9
    return tens + (ones / 10.0);
  } else {
    // Non timer the number is an int between 00 and 99
    return (tens * 10) + ones;
  }
}

FanSpeeds decodeFanSpeed(uint8_t modeFanBits);
FanSpeeds decodeFanSpeed(uint8_t modeFanBits) {
  // Fan bits are 0-3, mask out the rest
  uint8_t fanBitsMasked = modeFanBits & 0b00001111;

  if (acModel == V1_2) {
    switch (fanBitsMasked) {
      case 0x07:
        return FAN_LOW;
      case 0x0B:
        return FAN_MEDIUM;
      case 0x0D:
        return FAN_HIGH;
      case 0x0E:
        return FAN_AUTO;
      default:
        return FAN_INVALID;
    }
  } else { // V1_4
    switch (fanBitsMasked) {
      case 0x07:
        return FAN_LOW;
      case 0x0B:
        return FAN_HIGH;
      case 0x0D:
        return FAN_AUTO;
      default:
        return FAN_INVALID;
    }
  }
}

AcModes decodeAcMode(uint8_t modeFanBits);
AcModes decodeAcMode(uint8_t modeFanBits) {
  // Mode bits are 4-6, mask out the rest
  uint8_t modeBitsMasked = modeFanBits & 0b01110000;

  // Logic is the same for V1_2 & V1_4
  switch (modeBitsMasked) {
    case 0x60: // 0110 0000
      return MODE_COOL;
    case 0x50: // 0101 0000
      return MODE_ECO;
    case 0x30: // 0011 0000
      return MODE_FAN;
    default:
      return MODE_INVALID;
  }
}

/**
 * Number of bytes in the AC display status update
 */
int getStatusLength() {
  if (acModel == V1_2) {
    return 5;
  } else { // V1_4
    return 6;
  }
}

void updateVariables(struct AcState acState) {
  // Record if any of the data changed, used to decide if an event should be published
  bool changed =
    currentAcState.temp != acState.temp ||
    currentAcState.timer != acState.timer ||
    currentAcState.speed != acState.speed ||
    currentAcState.mode != acState.mode;

  // Copy the new state struct to the current state
  currentAcState = acState;

  char vSpeed[2];
  char vMode[2];

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

  lastUpdate = currentAcState.timestamp;

  if (changed || lastUpdate - lastMessage > 300) {
    sprintf(statusJson, STATUS_TEMPLATE, currentAcState.temp, vSpeed, vMode);
    if (changed) {
      Spark.publish("STATE_CHANGED", statusJson);
    } else {
      Spark.publish("STATE_REFRESH", statusJson);
    }
    lastMessage = Time.now();
  }
}

bool compareAcStates(struct AcState s1, struct AcState s2) {
  return s1.temp == s2.temp &&
    s1.timer == s2.timer &&
    s1.speed == s2.speed &&
    s1.mode == s2.mode &&
    s1.sleep == s2.sleep;
}

void updateStates(int temp, double timer, enum FanSpeeds speed, enum AcModes mode, bool isSleep);
void updateStates(int temp, double timer, enum FanSpeeds speed, enum AcModes mode, bool isSleep) {
  // Update the next index in the states array with the pushed data
  acStates[acStatesIndex].timestamp = Time.now();
  acStates[acStatesIndex].temp = temp;
  acStates[acStatesIndex].timer = timer;
  acStates[acStatesIndex].speed = speed;
  acStates[acStatesIndex].mode = mode;
  acStates[acStatesIndex].sleep = isSleep;

  // Increment states index to the next position
  acStatesIndex = (acStatesIndex + 1) % AC_STATES_LEN;

  // Track the entry with the most matches, the index it exists at and the match count for every
  // entry
  int maxMatches = 0;
  int maxIndex = -1;
  int equivalentStates[AC_STATES_LEN] = { 0 };

  // iterate through acStates comparing each entry to each other entry
  for (int i = 0; i < AC_STATES_LEN; i++) {
    for (int o = i + 1; o < AC_STATES_LEN; o++) {
      if (compareAcStates(acStates[i], acStates[o])) {
        if (++equivalentStates[i] > maxMatches) {
          maxMatches = equivalentStates[i];
          maxIndex = i;
        }
        if (++equivalentStates[o] > maxMatches) {
          maxMatches = equivalentStates[o];
          maxIndex = o;
        }
      }
    }
  }

  if (maxMatches != AC_STATES_LEN - 1) {
    char message[80];
    sprintf(message, "mm:%d, mi:%d, [%d, %d, %d, %d, %d]", maxMatches, maxIndex, equivalentStates[0], equivalentStates[1], equivalentStates[2], equivalentStates[3], equivalentStates[4]);
    Spark.publish("EQUIV_STATES", message);
  }

  if (maxMatches >= AC_STABLE_STATES && maxIndex >= 0 && maxIndex < AC_STATES_LEN) {
    updateVariables(acStates[maxIndex]);
  }
}

void parseDataV14(uint8_t parseBuffer[], int pbLen) {
}

void parseDataV12(uint8_t parseBuffer[], int pbLen) {
  if (pbLen != getStatusLength()) {
    Spark.publish("PARSE_V12", "BAD LENGTH");
    // Something is wrong, skip parsing
    return;
  }

  // If all 5 bytes are 0xFF the unit is off
  bool isOff = true;
  for (int i = 0; i < pbLen && isOff; i++) {
    isOff = parseBuffer[i] == 0xFF;
  }
  if (isOff) {
    updateStates(0, 0, FAN_OFF, MODE_OFF, false);
    return;
  }

  // All valid buffers have the FF for the first byte
  if (parseBuffer[0] != 0xFF) {
    return;
  }

  // If bit 5 of byte 3 is false timer is on
  bool isTimer;
  uint8_t timerMasked = parseBuffer[3] & 0b11110000;
  switch (timerMasked) {
    case 0xD0:
      isTimer = true;
      break;
    case 0x70:
      isTimer = false;
      break;
    default:
      // Timer bits are invalid
      char msg[20];
      sprintf(msg, "INVALID TIMER: %02x", parseBuffer[3]);
      Spark.publish("PARSE_V12", msg);
      return;
  }

  // If bit 5 of byte 3 is false timer is on
  bool isSleep;
  uint8_t sleepMasked = parseBuffer[3] & 0b00001111;
  switch (sleepMasked) {
    case 0x0E:
      isSleep = true;
      break;
    case 0x0F:
      isSleep = false;
      break;
    default:
      // Timer bits are invalid
      char msg[20];
      sprintf(msg, "INVALID SLEEP: %02x", parseBuffer[3]);
      Spark.publish("PARSE_V12", msg);
      return;
  }

  double display = decodeDisplayNumber(parseBuffer[1], parseBuffer[2], isTimer);
  if (display == -1) {
    // Display digits were invalid, ignore buffer
    char msg[20];
    sprintf(msg, "INVALID DISPLAY: %02x %02x", parseBuffer[1], parseBuffer[2]);
    Spark.publish("PARSE_V12", msg);
    return;
  }

  enum AcModes acMode = decodeAcMode(parseBuffer[4]);
  if (acMode == MODE_INVALID) {
    // AC Mode was invalid, ignore buffer
    char msg[20];
    sprintf(msg, "INVALID MODE: %02x", parseBuffer[4]);
    Spark.publish("PARSE_V12", msg);
    return;
  }

  enum FanSpeeds fanSpeed = decodeFanSpeed(parseBuffer[4]);
  if (fanSpeed == FAN_INVALID) {
    // Fan Speed was invalid, ignore buffer
    char msg[20];
    sprintf(msg, "INVALID FAN: %02x", parseBuffer[4]);
    Spark.publish("PARSE_V12", msg);
    return;
  }

  if (isTimer) {
    updateStates(0, display, fanSpeed, acMode, isSleep);
  } else {
    updateStates((int) display, 0, fanSpeed, acMode, isSleep);
  }
}

void parseData(uint8_t parseBuffer[], int pbLen) {
  char dataDebugBuff[(pbLen * 3) + 1];
  for (int i = 0; i < pbLen; i++) {
    sprintf(&dataDebugBuff[i * 3], "%02x ", parseBuffer[i]);
  }
  Serial.println(dataDebugBuff);

  if (acModel == V1_2) {
    parseDataV12(parseBuffer, pbLen);
  } else {
  }
}

void loop() {
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
  int pbLen = getStatusLength();
  uint8_t parseBuffer[pbLen];
  for (int rb = 0; rb < BUFFER_LEN; rb++) {
    for (int pb = 0; pb < pbLen; pb++) {
      int readPos = (rb + pb) % BUFFER_LEN;
      /*if (readPos >= BUFFER_LEN) {
        readPos -= BUFFER_LEN;
      }*/
      Serial.print(rb);
      Serial.print(" ");
      Serial.print(pb);
      Serial.print(" ");
      Serial.println(readPos);
      parseBuffer[pb] = readBuffer[readPos];
    }
    parseData(parseBuffer, pbLen);
  }
    /*// Dump the input data
    for (int i = 0; i < BUFFER_LEN - 6; i++) {
      // Look for the header bytes
      if (readBuffer[i] == 0x7F && readBuffer[i+1] == 0x7F) {
        if (readBuffer[i+2] == 0xFF && readBuffer[i+3] == 0xFF && readBuffer[i+4] == 0xFF && readBuffer[i+5] == 0xFF) {
          // If the next 4 bytes are FF the unit is off
          updateState(0, "X", "X");
          break; // Read a successfull data frame, stop looping
        } else if (readBuffer[i+5] == 0xFD) {
          // If the last byte is FD we should have a 6 byte status
          double td = decodeDisplayNumber(readBuffer[i+2], readBuffer[i+3]);
          Serial.println(td);
          if (td == -1) {
            // If decoded number is -1 then the data must be bad, give up and let the loop keep iterating
            continue;
          }

          String fs = decodeFanSpeed(readBuffer[i+4]);
          Serial.println(fs);
          if (fs == "") {
            // If decoded String is "" then the data must be bad, give up and let the loop keep iterating
            continue;
          }

          String am = decodeAcMode(readBuffer[i+4]);
          Serial.println(am);
          if (am == "") {
            // If decoded String is "" then the data must be bad, give up and let the loop keep iterating
            continue;
          }

          updateState(td, fs, am);
          break; // Read a successfull data frame, stop looping
        }
      }
    }*/

  // If no update for 2 minutes complain
  if (Time.now() - lastUpdate > 120) {
    Spark.publish("STATE_STALE", "TODO");
    lastUpdate += 30; // wait 30s before complaining again
  }
  /*//Serial.println();*/


  delay(5000); // pause 5s
}

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
  if (digitalRead(INPUT_PIN) == HIGH) {
    *currentByte |= 1;
  }
}
 
