#ifndef _STATE_H_
#define _STATE_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct velocity_state {
    uint16_t speed;
    uint16_t direction;
} VelocityState;

extern VelocityState get_desired_velocity();
extern bool has_velocity_state_changed(VelocityState old, VelocityState new);

#endif