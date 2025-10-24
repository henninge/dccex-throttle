#ifndef _STATE_H_
#define _STATE_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct desired_speed {
    uint16_t speed;
    uint16_t direction;
} DesiredSpeed;

extern DesiredSpeed get_desired_speed();
extern bool has_desired_speed_changed(DesiredSpeed old, DesiredSpeed new);

#endif