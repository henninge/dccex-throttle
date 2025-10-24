#include <stdint.h>
#include <zephyr/kernel.h>

#include "thread_prios.h"
#include "queue.h"
#include "state.h"

#define STATE_THREAD_STACK_SIZE 1024

K_MUTEX_DEFINE(desired_speed_mutex);
VelocityState desired_speed = {
	.speed = 0,
	.direction = DIR_FORWARD
};

static void update_desired_speed_from_message(struct message msg);
static void desired_state_thread_entry(void *arg1, void *arg2, void *arg3);

K_THREAD_DEFINE(desired_state_thread, STATE_THREAD_STACK_SIZE,
				desired_state_thread_entry, NULL, NULL, NULL,
				STATE_THREAD_PRIORITY, 0, 0);

static void desired_state_thread_entry(void *arg1, void *arg2, void *arg3) {
	struct message msg;
	while(1) {
		msg = queue_receive();
		update_desired_speed_from_message(msg);
	}
}

void update_desired_speed_from_message(struct message msg) {
	k_mutex_lock(&desired_speed_mutex, K_FOREVER);
	switch(msg.type) {
	case MSG_SPEED:
		desired_speed.speed = msg.value;
		break;
	case MSG_DIRECTION:
		if (msg.value != desired_speed.direction) {
			desired_speed.speed = 0;
			desired_speed.direction = msg.value;
		}
		break;
	}
	k_mutex_unlock(&desired_speed_mutex);
}

VelocityState get_desired_velocity() {
	k_mutex_lock(&desired_speed_mutex, K_FOREVER);
	VelocityState current_val = desired_speed;
	k_mutex_unlock(&desired_speed_mutex);
	return current_val;
}

bool has_velocity_state_changed(VelocityState old, VelocityState new) {
	return !(old.speed == new.speed && old.direction == new.direction);
}
