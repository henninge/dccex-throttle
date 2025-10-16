#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdint.h>

struct message {
    int32_t speed;
};

extern void queue_send_speed(int32_t speed);
extern struct message queue_receive();

#endif