#include <stdint.h>
#include <zephyr/kernel.h>

#include "thread_prios.h"
#include "queue.h"
#include "state.h"

#define STATE_THREAD_STACK_SIZE 1024

K_MUTEX_DEFINE(desired_state_mutex);
Velocity desired_state = {
	.speed = 0,
	.direction = DIR_FORWARD,
	.stop = false
};

Velocity actual_state = {
	.speed = 0,
	.direction = DIR_FORWARD,
	.stop = false
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
	k_mutex_lock(&desired_state_mutex, K_FOREVER);
	switch(msg.type) {
	case MSG_SPEED:
		desired_state.speed = msg.value;
		break;
	case MSG_DIRECTION:
		if (msg.value != desired_state.direction) {
			desired_state.speed = 0;
			desired_state.direction = msg.value;
		}
		break;
	}
	k_mutex_unlock(&desired_state_mutex);
}

Velocity get_desired_state() {
	k_mutex_lock(&desired_state_mutex, K_FOREVER);
	Velocity current_val = desired_state;
	k_mutex_unlock(&desired_state_mutex);
	return current_val;
}

bool has_velocity_state_changed(Velocity old, Velocity new) {
	return !(old.speed == new.speed && old.direction == new.direction);
}

Velocity set_actual_state(Velocity new_state) {
	actual_state = new_state;
	return actual_state;
}

enum state {
	halted,
	driving,
	reverting,
	stopping,
};

typedef enum state (*StepFunction)(Velocity *current_v, Velocity new_v);

enum state step_state_halted(Velocity *current_v, Velocity new_v) {
	current_v->speed = 0;
	current_v->direction = new_v.direction;
	if (new_v.stop) {
		current_v->stop = true;
		return stopping;
	}
	if (new_v.speed > 0) {
		current_v->speed = new_v.speed;
		return driving;
	}
	return halted;
}

enum state step_state_driving(Velocity *current_v, Velocity new_v) {
	current_v->speed = new_v.speed;
	if (new_v.stop) {
		current_v->stop = true;
		return stopping;
	}
	if (new_v.speed == 0) {
		return halted;
	}
	if (current_v->direction != new_v.direction) {
		current_v->speed = 0;
		return reverting;
	}
	return driving;
}

enum state step_state_reverting(Velocity *current_v, Velocity new_v) {
	current_v->speed = 0;
	if (new_v.stop) {
		current_v->stop = true;
		return stopping;
	}
	if (new_v.speed == 0) {
		return halted;
	}
	return reverting;
}

enum state step_state_stopping(Velocity *current_v, Velocity new_v) {
	current_v->stop = true;
	if (new_v.speed == 0) {
		current_v->stop = false;
		return halted;
	}
	return stopping;
}

StepFunction steps[] = {
	step_state_halted,
	step_state_driving,
	step_state_reverting,
	step_state_stopping,
};

enum state step_state(enum state state, Velocity *current_v, Velocity new_v) {
	return steps[state](current_v, new_v);
}
