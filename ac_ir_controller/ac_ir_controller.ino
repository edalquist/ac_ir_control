/*
Author: AnalysIR
Revision: 1.0
*/

// From https://github.com/shirriff/Arduino-IRremote/blob/master/IRremoteInt.h
#define NEC_BITS 32
#define NEC_HDR_MARK	9000
#define NEC_HDR_SPACE	4500
#define NEC_BIT_MARK	560
#define NEC_ONE_SPACE	1690
#define NEC_ZERO_SPACE	560
#define NEC_RPT_SPACE	2250

#define MARK_EXCESS 100

#define Duty_Cycle 50  //in percent (10->50), usually 33 or 50
//actual duty cycle will be relative close to set value, depending on carrier freq

#define Carrier_Frequency 38000   //usually one of 38000, 40000, 36000, 56000, 33000, 30000


#define PERIOD    (1000000+Carrier_Frequency/2)/Carrier_Frequency
#define HIGHTIME  PERIOD*Duty_Cycle/100
#define LOWTIME   PERIOD - HIGHTIME
#define txPinIR   D6   //IR carrier output pin

#define LED D7

unsigned long sigTime = 0; //use in mark & space functions to keep track of time
unsigned int lastCode = 0;

unsigned int decodeNECHex(String codeHex) {
  unsigned int h;
  char code[9];
  codeHex.toCharArray(code, 9);
  sscanf(code, "%x", &h);
  return h;
}

int sendNECCode(unsigned int codeBin) {
  Serial.print("Sending: ");
  Serial.print(codeBin, HEX);

  digitalWrite(LED, HIGH);
  lastCode = codeBin;

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

  digitalWrite(LED, LOW);

  Serial.println(" Sent!");
  return 1;
}

int sendNEC(String command) {
  if (command.length() != 8) {
    Serial.println("ERR: Command must be 8 characters (32 bit hex string)");
    return -1;
  }

  unsigned int codeBin = decodeNECHex(command);
  return sendNECCode(codeBin);
}

int sendLast(String command) {
  return sendNECCode(lastCode);
}

int incrCode(String command) {
  Serial.print("Incrementing code from ");
  Serial.print(lastCode, HEX);
  lastCode++;
  Serial.print(" to ");
  Serial.println(lastCode, HEX);
  return 1;
}

void setup() {
  Serial.begin(57600);
  pinMode(txPinIR, OUTPUT);
  pinMode(LED, OUTPUT);

  Spark.function("sendNEC", sendNEC);
  Spark.function("incrCode", incrCode);
  Spark.function("sendLast", sendLast);
}

void loop() {
  Serial.print("Last Code: ");
  Serial.println(lastCode, HEX);
  delay(5000);
}

void mark(unsigned int mLen) { //uses sigTime as end parameter
  /*Serial.print(" m");
  Serial.print(mLen);*/

  sigTime+= mLen; //mark ends at new sigTime
  unsigned long now = micros();
  if (now >= sigTime) {
    Serial.println("Skipping mark");
    return;
  }
  unsigned long dur = sigTime - now; //allows for rolling time adjustment due to code execution delays
  while ((micros() - now) < dur) { //just wait here until time is up
    digitalWrite(txPinIR, HIGH);
    delayMicroseconds(HIGHTIME - 3);
    digitalWrite(txPinIR, LOW);
    delayMicroseconds(LOWTIME - 4);
  }
}

void space(unsigned int sLen) { //uses sigTime as end parameter
  /*Serial.print(" s");
  Serial.print(sLen);*/

  sigTime+= sLen; //space ends at new sigTime
  unsigned long now = micros();
  if (now >= sigTime) {
    Serial.println("Skipping space");
    return;
  }
  unsigned long dur = sigTime - now; //allows for rolling time adjustment due to code execution delays
  while ((micros() - now) < dur) ; //just wait here until time is up
}
