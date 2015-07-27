
#define CLOCK_PIN D0
#define INPUT_PIN D1

// I think the state is sent 6 bytes with each byte lasting ~2.886ms and then repeating.
// This should give us 10 copies of the pattern
#define MAX_LEN 60
#define CYCLE_MAX 500 // max time in micros the AC controller spends pushing data into the register

volatile bool firstCycle = true;
volatile unsigned int cycleStart = 0;

volatile unsigned int registerCount = 0;
volatile unsigned int shiftRegBuffer[MAX_LEN];

void setup() {
  Serial.begin(115200); //change BAUD rate as required
  pinMode(CLOCK_PIN, INPUT);
  pinMode(INPUT_PIN, INPUT);
  attachInterrupt(CLOCK_PIN, clock_Interrupt_Handler, RISING); //set up ISR for receiving shift register clock signal
}

void loop() {
  noInterrupts();
  if (firstCycle) {
    Serial.println("Waiting on first cycle");
    Serial.println(cycleStart);
    Serial.println(cycleStart);
  } else {
    // Dump the input data
    for (int i = 0; i < MAX_LEN; i++) {
      Serial.print(shiftRegBuffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    firstCycle = true;
    cycleStart = 0;
    registerCount = 0;
  }
  interrupts();

  delay(2000); // pause 2s
}

void clock_Interrupt_Handler() {
  int input = digitalRead(INPUT_PIN);

  // Track the start of each cycle
  // zero out the shiftReg to start accumulating data
  int now = micros();
  int cycleDiff = now - cycleStart;
  if (cycleStart == 0 || cycleDiff > CYCLE_MAX) {
    cycleStart = now;

    registerCount++;

    // Roll over to the start of the buffer once MAX_LEN is reached
    if (registerCount == MAX_LEN) {
      registerCount = 0;
    }

    shiftRegBuffer[registerCount] = 0;
  }

  // Skip the first cycle data on the off chance that we started listing during a register update
  if (firstCycle) {
    if (cycleDiff <= CYCLE_MAX) {
      return;
    } else {
      firstCycle = false;
    }
  }

  // On clock rise shift the data in the shift register by one
  shiftRegBuffer[registerCount] = shiftRegBuffer[registerCount] << 1;
  shiftRegBuffer[registerCount] &= 0x000000FF;

  // If the input is high set the new bit to 1
  if (input == HIGH) {
    shiftRegBuffer[registerCount] |= 1;
  }
}
