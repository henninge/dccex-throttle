#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "zephyr/sys/clock.h"
#include <stdint.h>
#include <stdbool.h>

#define MSG_SPEED 0
#define MSG_DIRECTION 1

#define DIR_FORWARD ((int16_t)1)
#define DIR_BACKWARD ((int16_t)0)

struct message {
	int16_t type;
	int16_t value;
};

extern void queue_send_speed(int16_t speed);
extern void queue_send_direction(int16_t direction);
extern struct message queue_receive();

extern bool btn_wait_stop(k_timeout_t timeout);
extern void btn_set_stop();

#endif