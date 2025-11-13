#ifndef _STATE_H_
#define _STATE_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct velocity_state {
    uint16_t speed;
    uint16_t direction;
    bool     stop;
} Velocity;

extern Velocity get_desired_state();
extern bool has_velocity_state_changed(Velocity old, Velocity new);
extern Velocity set_actual_state(Velocity new);

#endif
