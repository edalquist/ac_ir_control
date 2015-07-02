
#define Duty_Cycle 50  //in percent (10->50), usually 33 or 50
//actual duty cycle will be relative close to set value, depending on carrier freq

#define Carrier_Frequency 38000   //usually one of 38000, 40000, 36000, 56000, 33000, 30000


#define PERIOD    (1000000+Carrier_Frequency/2)/Carrier_Frequency
#define HIGHTIME  PERIOD*Duty_Cycle/100
#define LOWTIME   PERIOD - HIGHTIME
#define txPinIR   D6   //IR carrier output pin

#define LED D7


int slowCount = 0;
unsigned long sigTime = 0; //use in mark & space functions to keep track of time

//RAW NEC signal -32 bit with 1 repeat - make sure buffer starts with a Mark
// AC On/Off Button
unsigned int NEC_RAW[] = {9026, 4474, 601, 527, 590, 530, 586, 543, 576, 1653, 592, 531, 586, 538, 585, 534, 582, 537, 585, 1650, 596, 522, 594, 1646, 601, 529, 589, 1645, 596, 1648, 610, 1634, 600, 1643, 597, 1647, 599, 530, 586, 542, 576, 542, 578, 1645, 600, 530, 586, 549, 574, 539, 578, 542, 580, 1644, 598, 1646, 598, 1645, 601, 530, 588, 1645, 598, 1646, 600, 1636, 599, 40842, 8999, 2234, 599};

void setup() {
  Serial.begin(57600);
  pinMode(txPinIR, OUTPUT);
  pinMode(LED, OUTPUT);

  pinMode(10, OUTPUT);
  digitalWrite(10, LOW);

  //make sure there is enough time to reflash.
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);

}

void loop() {

  //First send the NEC RAW signal above
  slowCount = 0;
  Serial.print("Start at ");
  digitalWrite(LED,HIGH);
  sigTime = micros(); //keeps rolling track of signal time to avoid impact of loop & code execution delays
  /*Serial.println(sigTime);*/
  for (int i = 0; i < sizeof(NEC_RAW) / sizeof(NEC_RAW[0]); i++) {
    mark(NEC_RAW[i++]); //also move pointer to next position
    if (i < sizeof(NEC_RAW) / sizeof(NEC_RAW[0])) space(NEC_RAW[i]); //pointer will be moved by for loop
  }
  digitalWrite(LED,LOW);
  Serial.print("End ");
  Serial.println(slowCount);
  delay(10000); //wait 5 seconds between each signal (change to suit)

}

void mark(unsigned int mLen) { //uses sigTime as end parameter
  /*Serial.print("mark(");
  Serial.print(mLen);
  Serial.print(") ");*/

  sigTime+= mLen; //mark ends at new sigTime
  unsigned long now = micros();
  unsigned long dur = sigTime - now; //allows for rolling time adjustment due to code execution delays
  if (now > sigTime) {
    slowCount++;
    dur = 0;
  }
  /*Serial.print(sigTime);
  Serial.print(",");
  Serial.print(now);
  Serial.print(",");
  Serial.println(dur);*/
  if (dur == 0) return;
  while ((micros() - now) < dur) { //just wait here until time is up
    digitalWrite(txPinIR, HIGH);
    delayMicroseconds(HIGHTIME - 3);
    digitalWrite(txPinIR, LOW);
    delayMicroseconds(LOWTIME - 4);
  }
}

void space(unsigned int sLen) { //uses sigTime as end parameter
  /*Serial.print("space(");
  Serial.print(sLen);
  Serial.print(") ");*/

  sigTime+= sLen; //space ends at new sigTime
  unsigned long now = micros();
  unsigned long dur = sigTime - now; //allows for rolling time adjustment due to code execution delays
  if (now > sigTime) {
    slowCount++;
    dur = 0;
  }
  /*Serial.print(sigTime);
  Serial.print(",");
  Serial.print(now);
  Serial.print(",");
  Serial.println(dur);*/
  if (dur == 0) return;
  while ((micros() - now) < dur) ; //just wait here until time is up
}
