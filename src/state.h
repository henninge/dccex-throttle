#ifndef _STATE_H_
#define _STATE_H_

#include "zephyr/sys/clock.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct velocity_state {
    uint16_t speed;
    uint16_t direction;
    bool     stop;
} Velocity;

extern bool wait_velocity_change(Velocity *velocity, k_timeout_t timeout);

#endif
