#include <stdint.h>
#include <zephyr/kernel.h>

#include "thread_prios.h"
#include "queue.h"
#include "state.h"

#define STATE_THREAD_STACK_SIZE 1024

enum state {
	halted,
	driving,
	reverting,
	stopping,
};

K_MUTEX_DEFINE(state_mutex);

Velocity current_velocity = {
	.speed = 0,
	.direction = DIR_FORWARD,
	.stop = false
};
static enum state current_state = halted;

static Velocity new_velocity_from_message(struct message msg);
static void desired_state_thread_entry(void *arg1, void *arg2, void *arg3);
static enum state step_state(enum state state, Velocity new_v);

K_THREAD_DEFINE(desired_state_thread, STATE_THREAD_STACK_SIZE,
				desired_state_thread_entry, NULL, NULL, NULL,
				STATE_THREAD_PRIORITY, 0, 0);

static void desired_state_thread_entry(void *arg1, void *arg2, void *arg3) {
	struct message msg;
	Velocity new_velocity;
	while(1) {
		msg = queue_receive();
		new_velocity = new_velocity_from_message(msg);
		current_state = step_state(current_state, new_velocity);
	}
}

Velocity new_velocity_from_message(struct message msg) {
	Velocity new_velocity = current_velocity;
	switch(msg.type) {
	case MSG_SPEED:
		new_velocity.speed = msg.value;
		break;
	case MSG_DIRECTION:
		new_velocity.direction = msg.value;
		break;
	}
	return new_velocity;
}

typedef enum state (*StepFunction)(Velocity new_v);

enum state step_state_halted(Velocity new_v) {
	current_velocity.speed = 0;
	current_velocity.direction = new_v.direction;
	if (new_v.stop) {
		current_velocity.stop = true;
		return stopping;
	}
	if (new_v.speed > 0) {
		current_velocity.speed = new_v.speed;
		return driving;
	}
	return halted;
}

enum state step_state_driving(Velocity new_v) {
	current_velocity.speed = new_v.speed;
	if (new_v.stop) {
		current_velocity.stop = true;
		return stopping;
	}
	if (new_v.speed == 0) {
		return halted;
	}
	if (current_velocity.direction != new_v.direction) {
		current_velocity.speed = 0;
		return reverting;
	}
	return driving;
}

enum state step_state_reverting(Velocity new_v) {
	current_velocity.speed = 0;
	if (new_v.stop) {
		current_velocity.stop = true;
		return stopping;
	}
	if (new_v.speed == 0) {
		return halted;
	}
	return reverting;
}

enum state step_state_stopping(Velocity new_v) {
	current_velocity.stop = true;
	if (new_v.speed == 0) {
		current_velocity.stop = false;
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

enum state step_state(enum state state, Velocity new_v) {
	return steps[state](new_v);
}
