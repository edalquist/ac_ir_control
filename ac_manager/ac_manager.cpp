#include "application.h"
#include "ac_display_reader.h"
#include "ac_ir_controller.h"

#define IR_LED   D6   //IR carrier output pin

#define CLOCK_PIN D2
#define INPUT_PIN D1

void setup() {
  initIrController("sendNEC", IR_LED);

  struct AcDisplayReaderConfig acConfig = AC_DISPLAY_READER_CONFIG_DEFAULTS;
  initAcDisplayReader(acConfig);
}

void loop() {
  processAcDisplayData();

  // process display data here
  delay(1000); // Sleep 1 second
}
