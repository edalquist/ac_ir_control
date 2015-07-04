/*
Author: AnalysIR
Revision: 1.0

This code is provided to overcome an issue with Arduino IR libraries
It allows you to capture raw timings for signals longer than 255 marks & spaces.
Typical use case is for long Air conditioner signals.

You can use the output to plug back into IRremote, to resend the signal.

This Software was written by AnalysIR.

Usage: Free to use, subject to conditions posted on blog below.
Please credit AnalysIR and provide a link to our website/blog, where possible.

Copyright AnalysIR 2014

Please refer to the blog posting for conditions associated with use.
http://www.analysir.com/blog/2014/03/19/air-conditioners-problems-recording-long-infrared-remote-control-signals-arduino/

Connections:
IR Receiver      Arduino
V+          ->  +5v
GND          ->  GND
Signal Out   ->  Digital Pin 2
(If using a 3V Arduino, you may connect V+ to +3V)
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

#define LEDPIN A0
#define IRPIN D0
//you may increase this value on Arduinos with greater than 2k SRAM
#define maxLen 800

volatile  unsigned int irBuffer[maxLen]; //stores timings - volatile because changed by ISR
volatile unsigned int x = 0; //Pointer thru irBuffer - volatile because changed by ISR

int MATCH_MARK(int measured_us, int desired_us) {
  return measured_us >= (desired_us - MARK_EXCESS) && measured_us <= (desired_us + MARK_EXCESS);
}

int MATCH_SPACE(int measured_us, int desired_us) {
  return measured_us >= (desired_us - MARK_EXCESS) && measured_us <= (desired_us + MARK_EXCESS);
}

// NECs have a repeat only 4 items long
long decodeNEC(int *irData) {
  long data = 0;
  int offset = 0;
  // Initial mark
  if (!MATCH_MARK(irData[offset], NEC_HDR_MARK)) {
    Serial.println("ERR No NEC Header");
    return -1;
  }
  offset++;
  // Check for repeat
  if (x == 4 &&
    MATCH_SPACE(irData[offset], NEC_RPT_SPACE) &&
    MATCH_MARK(irData[offset+1], NEC_BIT_MARK)) {
    /*results->bits = 0;
    results->value = REPEAT;
    results->decode_type = NEC;*/
    Serial.println("NEC Repeat");
    return data;
  }
  if (x < 2 * NEC_BITS + 4) {
    Serial.println("ERR not enough data");
    return -1;
  }
  // Initial space
  if (!MATCH_SPACE(irData[offset], NEC_HDR_SPACE)) {
    Serial.println("ERR No NEC Header");
    return -1;
  }
  offset++;
  for (int i = 0; i < NEC_BITS; i++) {
    if (!MATCH_MARK(irData[offset], NEC_BIT_MARK)) {
      Serial.println("ERR Expected a Mark");
      return -1;
    }
    offset++;
    if (MATCH_SPACE(irData[offset], NEC_ONE_SPACE)) {
      data = (data << 1) | 1;
    }
    else if (MATCH_SPACE(irData[offset], NEC_ZERO_SPACE)) {
      data <<= 1;
    }
    else {
      Serial.println("ERR Expected a Space");
      return -1;
    }
    offset++;
  }
  // Success
  /*results->bits = NEC_BITS; */ //32 bits
  return data;
  /*results->decode_type = NEC;*/
  /*return DECODED;*/
}


void setup() {
  Serial.begin(115200); //change BAUD rate as required
  pinMode(LEDPIN, OUTPUT);
  pinMode(IRPIN, INPUT);
  attachInterrupt(IRPIN, rxIR_Interrupt_Handler, CHANGE);//set up ISR for receiving IR signal
}

void loop() {
  // put your main code here, to run repeatedly:

  Serial.println(F("Press the button on the remote now - once only"));
  delay(5000); // pause 5 secs
  if (x) { //if a signal is captured
    detachInterrupt(IRPIN);//stop interrupts & capture until finshed here
    digitalWrite(LEDPIN, HIGH);//visual indicator that signal received

    int irData[x];
    Serial.println();
    for (int i = 1; i < x; i++) { //now dump the times
      int timing = irBuffer[i] - irBuffer[i - 1];
      irData[i-1] = timing;
      Serial.print(irData[i-1]);
      if (i + 1 < x) Serial.print(F("\t"));
    }

    Serial.println();
    long necCode = decodeNEC(irData);
    Serial.print(F("NEC: "));
    Serial.println(necCode, HEX);

    x = 0;
    Serial.println();
    digitalWrite(LEDPIN, LOW);//end of visual indicator, for this time
    attachInterrupt(IRPIN, rxIR_Interrupt_Handler, CHANGE);//re-enable ISR for receiving IR signal
  }

}

void rxIR_Interrupt_Handler() {
  if (x > maxLen) return; //ignore if irBuffer is already full
  irBuffer[x++] = micros(); //just continually record the time-stamp of signal transitions

}
