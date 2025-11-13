#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "queue.h"

LOG_MODULE_REGISTER(QUEUE);

K_MSGQ_DEFINE(ctrl_queue, sizeof(struct message), 20, 1);


static void queue_send(struct message *msg) {
	while (k_msgq_put(&ctrl_queue, msg, K_NO_WAIT) != 0) {
		/* message queue is full: purge old data & try again */
		k_msgq_purge(&ctrl_queue);
	}
}

static void queue_send_first(struct message *msg) {
	while (k_msgq_put_front(&ctrl_queue, msg) != 0) {
		/* message queue is full: purge old data & try again */
		k_msgq_purge(&ctrl_queue);
	}
}

void queue_send_speed(int16_t speed) {
	struct message speed_msg = {
		.type = MSG_SPEED,
		.value = speed
	};
	queue_send(&speed_msg);
}

void queue_send_direction(int16_t direction) {
	struct message direction_msg = {
		.type = MSG_DIRECTION,
		.value = direction
	};
	queue_send(&direction_msg);
}

void queue_send_stop() {
	struct message stop_msg = {
		.type = MSG_STOP,
		.value = 0
	};
	queue_send_first(&stop_msg);
}

struct message queue_wait_receive() {
	struct message received_msg;
	int err;
	do {
		err = k_msgq_get(&ctrl_queue, &received_msg, K_FOREVER);
	} while(err != 0);
	return received_msg;
}
