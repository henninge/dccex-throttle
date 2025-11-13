#ifndef _LEDS_H_
#define _LEDS_H_

#include "state.h"

#define STAGE_GREEN 3
#define STAGE_RED 1
#define STAGE_BLUE 2

extern void leds_update_from_state(Velocity velocity);
extern void leds_set_stage(int stage);

#endif