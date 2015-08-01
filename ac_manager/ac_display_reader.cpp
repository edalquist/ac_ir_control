#include "application.h"
#include "ac_display_reader.h"

// Global config
int clockPin;
int inputPin;

// State tracking
AcModels acModel = V1_2;
char statusJson[sizeof(STATUS_TEMPLATE) * 2];
char registerData[(BUFFER_LEN * 3) + 1];

// Variables used by the ISR to track the shift register
unsigned int cycleStart = 0;
volatile uint8_t byteBuffer[BUFFER_LEN];
volatile uint8_t* currentByte = byteBuffer;


void initAcDisplayReader(int cp, int ip, String statusVar, String dataVar, String setAcModelFuncName) {
  clockPin = cp;
  inputPin = ip;

  pinMode(clockPin, INPUT);
  pinMode(inputPin, INPUT);

  // Register display status variables
  Spark.variable(statusVar, &statusJson, STRING);
  Spark.variable(dataVar, &registerData, STRING);

  // Register control functions
  Spark.function(setAcModelFuncName, setAcModel);

  // Setup interrupt handler on rising edge of the register clock
  //attachInterrupt(clockPin, clock_Interrupt_Handler, RISING);
}

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
