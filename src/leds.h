#ifndef _LEDS_H_
#define _LEDS_H_

#include "state.h"

#define STAGE_GREEN 1
#define STAGE_RED 2
#define STAGE_BLUE 3

extern void leds_update_from_state(VelocityState velocity);
extern void leds_set_stage(int stage);

#endif