#include "application.h"

#ifndef AC_IR_CONTROLLER_P_H
#define AC_IR_CONTROLLER_P_H

// From https://github.com/shirriff/Arduino-IRremote/blob/master/IRremoteInt.h
#define NEC_BITS        32
#define NEC_HDR_MARK	  9000
#define NEC_HDR_SPACE	  4500
#define NEC_BIT_MARK	  560
#define NEC_ONE_SPACE	  1690
#define NEC_ZERO_SPACE  560
#define NEC_RPT_SPACE   2250

#define MARK_EXCESS 100

#define Duty_Cycle 50  //in percent (10->50), usually 33 or 50
//actual duty cycle will be relative close to set value, depending on carrier freq

#define Carrier_Frequency 38000   //usually one of 38000, 40000, 36000, 56000, 33000, 30000

#define PERIOD    (1000000+Carrier_Frequency/2)/Carrier_Frequency
#define HIGHTIME  PERIOD*Duty_Cycle/100
#define LOWTIME   PERIOD - HIGHTIME

unsigned int decodeNECHex(String codeHex);
int sendNECCode(unsigned int codeBin);
void mark(unsigned int mLen);
void space(unsigned int sLen);

#endif
