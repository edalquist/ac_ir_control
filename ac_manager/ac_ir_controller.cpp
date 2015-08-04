#include "application.h"
#include "ac_ir_controller.h"
#include "ac_ir_controller_p.h"

unsigned long sigTime = 0; //use in mark & space functions to keep track of time
int txPinIR;
int lastStartMillis = 0;

void initIrController(String funcKey, int irLedPin) {
  txPinIR = irLedPin;
  pinMode(txPinIR, OUTPUT);
  Spark.function(funcKey, sendNEC);
}

int sendNEC(String command) {
  if (command.length() != 8) {
    return -1;
  }

  unsigned int codeBin = decodeNECHex(command);
  return sendNECCode(codeBin);
}

unsigned int decodeNECHex(String codeHex) {
  unsigned int h;
  char code[9];
  codeHex.toCharArray(code, 9);
  sscanf(code, "%x", &h);
  return h;
}

int sendNECCode(unsigned int codeBin) {
  // Don't send IR codes more than once every 110ms
  int sleepTime = min(110, max(0, 110 - (millis() - lastStartMillis)));
  if (sleepTime > 0) {
    delay(sleepTime);
  }

  // Disable interrupts while sending IR code to not break the timing
  noInterrupts();

  sigTime = micros(); //keeps rolling track of signal time to avoid impact of loop & code execution delays
  mark(NEC_HDR_MARK);
  space(NEC_HDR_SPACE);
  for (int i = NEC_BITS - 1; i >= 0; i--) {
    mark(NEC_BIT_MARK);
    if (codeBin & (1<<i)) {
      space(NEC_ONE_SPACE);
    } else {
      space(NEC_ZERO_SPACE);
    }
  }
  mark(NEC_BIT_MARK);

  interrupts();

  return 1;
}

void mark(unsigned int mLen) { //uses sigTime as end parameter
  sigTime+= mLen; //mark ends at new sigTime
  unsigned long now = micros();
  if (now >= sigTime) return;

  unsigned long dur = sigTime - now; //allows for rolling time adjustment due to code execution delays
  while ((micros() - now) < dur) { //just wait here until time is up
    digitalWrite(txPinIR, HIGH);
    delayMicroseconds(HIGHTIME - 3);
    digitalWrite(txPinIR, LOW);
    delayMicroseconds(LOWTIME - 4);
  }
}

void space(unsigned int sLen) { //uses sigTime as end parameter
  sigTime+= sLen; //space ends at new sigTime
  unsigned long now = micros();
  if (now >= sigTime) return;

  unsigned long dur = sigTime - now; //allows for rolling time adjustment due to code execution delays
  while ((micros() - now) < dur) ; //just wait here until time is up
}
