#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "queue.h"

LOG_MODULE_REGISTER(QUEUE);

K_MSGQ_DEFINE(message_queue, sizeof(struct message), 20, 1);

void queue_send_speed(int32_t speed) {
	struct message speed_msg = {
		.speed = speed
	};
	while (k_msgq_put(&message_queue, &speed_msg, K_NO_WAIT) != 0) {
		/* message queue is full: purge old data & try again */
		k_msgq_purge(&message_queue);
	}
}

struct message queue_receive() {
	struct message received_msg;
	int err;
	do {
		err = k_msgq_get(&message_queue, &received_msg, K_FOREVER);
	} while(err != 0);
	return received_msg;
}
