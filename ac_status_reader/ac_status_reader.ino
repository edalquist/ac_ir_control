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
#define BUFFER_LEN 6 * 5
#define UPDATE_TIME_MAX 500 // max time in micros the AC controller spends pushing data into the register

#define AC_STATES_LEN 5

// Variables used by the ISR to track the shift register
unsigned int cycleStart = 0;
volatile uint8_t byteBuffer[BUFFER_LEN];
volatile uint8_t* currentByte = byteBuffer;


// Overall state tracking
AcModels acModel = V1_2;
long lastUpdate = 0; // millis of successful data parse
long lastMessage = 0; // millis of of last message sent
int acStatesIndex = 0; // Circular buffer index into acStates array
struct AcState acStates[AC_STATES_LEN] = {
  {0, 0.0, FAN_OFF, MODE_OFF},
  {0, 0.0, FAN_OFF, MODE_OFF},
  {0, 0.0, FAN_OFF, MODE_OFF},
  {0, 0.0, FAN_OFF, MODE_OFF},
  {0, 0.0, FAN_OFF, MODE_OFF}
};


// Spark variables
int temp = 0;
double timer = 0;
char speed[2];
char mode[2];
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
  Spark.variable("temp", &temp, INT);
  Spark.variable("timer", &timer, DOUBLE);
  Spark.variable("speed", &speed, STRING);
  Spark.variable("mode", &mode, STRING);
  Spark.variable("data", &registerData, STRING);

  // Register control functions
  Spark.function("setAcModel", setAcModel);

  // Setup interrupt handler on rising edge of the register clock
  attachInterrupt(CLOCK_PIN, clock_Interrupt_Handler, RISING);
}

/**
 * Decode the number bits that control the 8 segment display
 */
int decodeDigit(uint8_t digitBits) {
  // Ensure the top bit is true to make the switch work no matter the state of the decimal place
  uint8_t decimalMasked = digitBits | 0x80;
  //Serial.print("masked: ");
  //Serial.print(digitBits, HEX);
  //Serial.print(" to ");
  //Serial.print(decimalMasked, HEX);
  //Serial.println();
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

double decodeDisplayNumber(uint8_t tensBits, uint8_t onesBits) {
  int tens = decodeDigit(tensBits);
  int ones = decodeDigit(onesBits);

  // One of the digits must be invalid, return -1
  if (tens == -1 || ones == -1) {
    return -1;
  }

  if ((tensBits & 0x80) == 0x80) {
    // If the top bit of the tens digit is set the display is an integer between 00 and 99
    return (tens * 10) + ones;
  } else {
    // If the top bit of the tens digit is not set the display is a decimal between 0.0 and 9.9
    return tens + (ones / 10.0);
  }
}

FanSpeeds decodeFanSpeed(uint8_t modeFanBits);
FanSpeeds decodeFanSpeed(uint8_t modeFanBits) {
  // Fan bits are 0-3
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
  // Mode bits are 4-6
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

int getStatusLength() {
  if (acModel == V1_2) {
    return 5;
  } else { // V1_4
    return 6;
  }
}


void updateState(double temp, String fan, String mode) {
  Serial.println("Updating State");

  /*String fanSpeedStr(fanSpeed);
  String acModeStr(acMode);

  // Record if any of the data changed, used to decide if an event should be published
  bool changed = temp != tempDisplay || fan != fanSpeedStr || mode != acModeStr;

  // All the data is good, update the published variables
  tempDisplay = temp;
  fan.toCharArray(fanSpeed, fan.length() + 1);
  mode.toCharArray(acMode, mode.length() + 1);
  lastUpdate = millis();

  if (changed || lastUpdate - lastMessage > 300000) {
    Serial.println("State Changed");
    char message[80];

    sprintf(message, "{\"temp\":%d,\"fan\":\"%s\",\"mode\":\"%s\"}", (int) tempDisplay, fanSpeed, acMode);
    Serial.println(message);
    Spark.publish("STATE_CHANGED", message);
    lastMessage = lastUpdate;
  }*/
}

void parseDataV14(uint8_t parseBuffer[], int pbLen) {
}

void parseDataV12(uint8_t parseBuffer[], int pbLen) {
  if (pbLen != getStatusLength()) {
    // Something is wrong, skip parsing
    return;
  }

  // If all 5 bytes are 0xFF the unit is off
  bool isOff = true;
  for (int i = 0; i < pbLen && isOff; i++) {
    isOff = parseBuffer[i] == 0xFF;
  }
  if (isOff) {
    acStates[acStatesIndex].temp = 0;
    acStates[acStatesIndex].timer = 0;
    acStates[acStatesIndex].speed = FAN_OFF;
    acStates[acStatesIndex].mode = MODE_OFF;
    acStatesIndex = ++acStatesIndex % AC_STATES_LEN;

    return;
  }
}

void parseData(uint8_t parseBuffer[], int pbLen) {
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
      parseBuffer[pb] = readBuffer[(rb + pb) % BUFFER_LEN];
    }
    parseData(parseBuffer, pbLen);
  }

  if (acModel == V1_2) {
    // V1.2 uses a 5 byte cycle
    // TODO decode v1.2 display data
  } else {
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
  }

  // If no update for 2 minutes complain
  if (millis() - lastUpdate > 120000) {
    Spark.publish("STATE_STALE", "TODO");
    lastUpdate += 30000; // wait 30s before complaining again
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
