#include "application.h"
#include "ac_display_reader.h"
#include "ac_ir_controller.h"

#define IR_LED   D6   //IR carrier output pin


void setup() {
  initIrController("sendNEC", IR_LED);
}

void loop() {
}
