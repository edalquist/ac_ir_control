
#define CLOCK_PIN D0
#define INPUT_PIN D1

// The shift register sees 6 bytes repeatedly
#define STATUS_LEN 6
// Keep the last 6 status codes
#define CAPTURE_COUNT 6
#define BUFFER_LEN 6 * 6
#define UPDATE_TIME_MAX 500 // max time in micros the AC controller spends pushing data into the register

unsigned int cycleStart = 0;
volatile uint8_t byteBuffer[BUFFER_LEN];
volatile uint8_t* currentByte = byteBuffer;

void setup() {
  Serial.begin(115200); //change BAUD rate as required
  pinMode(CLOCK_PIN, INPUT);
  pinMode(INPUT_PIN, INPUT);
  attachInterrupt(CLOCK_PIN, clock_Interrupt_Handler, RISING); //set up ISR for receiving shift register clock signal

}

void loop() {
  uint8_t readBuffer[BUFFER_LEN];

  // Copy the volatile byteBuffer to a local buffer to minimize the time that interrupts are off
  noInterrupts();
  memcpy(readBuffer, (const uint8_t*) byteBuffer, BUFFER_LEN);
  interrupts();

  // TODO read the array and keep the 6 bytes that happen most frequently

  // Dump the input data
  for (int i = 0; i < BUFFER_LEN - 6; i++) {
    Serial.print(readBuffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();


  delay(2000); // pause 2s
}

void clock_Interrupt_Handler() {
  // Track the start of each cycle
  // zero out the shiftReg to start accumulating data
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
  if (((GPIOB->IDR) & 0b01000000) == 0b01000000) {
    *currentByte |= 1;
  }
}
