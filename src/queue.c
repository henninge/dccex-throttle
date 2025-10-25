#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "queue.h"
#include "zephyr/sys/clock.h"

LOG_MODULE_REGISTER(QUEUE);

K_MSGQ_DEFINE(message_queue, sizeof(struct message), 20, 1);

void queue_send_speed(int16_t speed) {
	struct message speed_msg = {
		.type = MSG_SPEED,
		.value = speed
	};
	while (k_msgq_put(&message_queue, &speed_msg, K_NO_WAIT) != 0) {
		/* message queue is full: purge old data & try again */
		k_msgq_purge(&message_queue);
	}
}

void queue_send_direction(int16_t direction) {
	struct message direction_msg = {
		.type = MSG_DIRECTION,
		.value = direction
	};
	while (k_msgq_put(&message_queue, &direction_msg, K_NO_WAIT) != 0) {
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

K_SEM_DEFINE(btn_stop, 0, 1);

bool btn_wait_stop(k_timeout_t timeout) {
	return k_sem_take(&btn_stop, timeout) == 0;
}

void btn_set_stop() {
	k_sem_give(&btn_stop);
}
