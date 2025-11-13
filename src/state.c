#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "thread_prios.h"
#include "queue.h"
#include "state.h"

LOG_MODULE_REGISTER(STATE);

#define STATE_THREAD_STACK_SIZE 1024

enum state {
	halted_fwd,
	halted_bwd,
	driving,
	reverting,
	stopping,
	stopped,
};

const char* state_strings[] = {
	"Halted Forward",
	"Halted Backward",
	"Driving",
	"Reverting",
	"Stopping",
	"Stopped",
};

K_MSGQ_DEFINE(state_changed, sizeof(Velocity), 20, 1);

Velocity dcc_velocity = {
	.speed = 0,
	.direction = DIR_FORWARD,
	.stop = false
};

static Velocity new_velocity_from_message(struct message msg);
static void desired_state_thread_entry(void *arg1, void *arg2, void *arg3);
static bool step_state(enum state *state, Velocity new_v);
static void queue_send(Velocity *current_v);

K_THREAD_DEFINE(desired_state_thread, STATE_THREAD_STACK_SIZE,
				desired_state_thread_entry, NULL, NULL, NULL,
				STATE_THREAD_PRIORITY, 0, 0);

static void desired_state_thread_entry(void *arg1, void *arg2, void *arg3) {
	enum state current_state = halted_fwd;
	struct message msg;
	Velocity new_velocity;
	while(1) {
		msg = queue_wait_receive();
		new_velocity = new_velocity_from_message(msg);
		if(step_state(&current_state, new_velocity)){
			LOG_INF("New state (speed): %s (%d)", state_strings[current_state], dcc_velocity.speed);
			queue_send(&dcc_velocity);
		}
		k_yield();
	}
}

void queue_send(Velocity *current_v) {
	while (k_msgq_put(&state_changed, current_v, K_NO_WAIT) != 0) {
		/* message queue is full: purge old data & try again */
		k_msgq_purge(&state_changed);
	}
}

bool wait_velocity_change(Velocity *velocity, k_timeout_t timeout) {
//	printk(" waiting state_changed\n");
	int result = k_msgq_get(&state_changed, velocity, timeout);
	bool has_changed = result == 0;
//	printk("state_changed: %d (%d)\n",has_changed, result);
	return has_changed;
};

Velocity new_velocity_from_message(struct message msg) {
	Velocity new_velocity = dcc_velocity;
	switch(msg.type) {
	case MSG_SPEED:
		new_velocity.speed = msg.value;
		break;
	case MSG_DIRECTION:
		new_velocity.direction = msg.value;
		break;
	case MSG_STOP:
		new_velocity.stop = true;
		break;
	}
	return new_velocity;
}

#define get_halted_state(v) (v.direction == DIR_FORWARD ? halted_fwd : halted_bwd)

typedef enum state (*StepFunction)(Velocity new_v);

enum state step_state_halted(Velocity new_v) {
	dcc_velocity.speed = 0;
	dcc_velocity.direction = new_v.direction;
	if (new_v.stop) {
		dcc_velocity.stop = true;
		// Trigger a return to halted.
		queue_send_speed(0);
		return stopped;
	}
	if (new_v.speed > 0) {
		dcc_velocity.speed = new_v.speed;
		return driving;
	}
	return get_halted_state(new_v);
}

enum state step_state_driving(Velocity new_v) {
	dcc_velocity.speed = new_v.speed;
	if (new_v.stop) {
		dcc_velocity.stop = true;
		return stopping;
	}
	if (new_v.speed == 0) {
		return get_halted_state(new_v);
	}
	if (dcc_velocity.direction != new_v.direction) {
		dcc_velocity.speed = 0;
		dcc_velocity.direction = new_v.direction;
		return reverting;
	}
	return driving;
}

enum state step_state_reverting(Velocity new_v) {
	dcc_velocity.speed = 0;
	if (new_v.stop) {
		dcc_velocity.stop = true;
		return stopping;
	}
	if (new_v.speed == 0) {
		dcc_velocity.direction = new_v.direction;
		return get_halted_state(new_v);
	}
	return reverting;
}

enum state step_state_stopping(Velocity new_v) {
	dcc_velocity.stop = true;
	if (new_v.speed == 0) {
		dcc_velocity.stop = false;
		dcc_velocity.speed = 0;
		dcc_velocity.direction = new_v.direction;
		return get_halted_state(new_v);
	}
	return stopping;
}

StepFunction steps[] = {
	step_state_halted,
	step_state_halted,
	step_state_driving,
	step_state_reverting,
	step_state_stopping,
	step_state_stopping,
};

bool step_state(enum state *state, Velocity new_v) {
	bool has_speed_changed = new_v.speed != dcc_velocity.speed;

	enum state new_state = steps[*state](new_v);
	bool has_state_changed = new_state != *state;
	*state = new_state;

	return *state == driving ? has_speed_changed : has_state_changed;
}
