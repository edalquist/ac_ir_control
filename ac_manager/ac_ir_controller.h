#include "application.h"

#ifndef AC_IR_CONTROLLER_H
#define AC_IR_CONTROLLER_H

void initIrController(String funcKey, int irLedPin);
int sendNEC(String command);

#endif
