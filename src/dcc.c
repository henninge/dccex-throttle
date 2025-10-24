#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include "leds.h"
#include "queue.h"
#include "thread_prios.h"
#include "state.h"

LOG_MODULE_REGISTER(DCC);

#define SEND_THREAD_STACK_SIZE 1024
#define RECV_THREAD_STACK_SIZE 1024

typedef struct dcc_connection {
	int sock;
} DccConnection;

typedef struct dcc_message {
	int addr;
	int speed;
	int direction;
} DccCommand;

DccConnection dcc_conn = {
	.sock = -1
};

DccCommand last_cmd = {
	.addr = 3,
	.speed = 0,
	.direction = 1
};

#define DCC_PING "<#>"
#define DCC_LOCO_STAT "<t 3>"

static int dcc_connect();
static void dcc_send_thread_entry(void *arg1, void *arg2, void *arg3);
static void dcc_recv_thread_entry(void *arg1, void *arg2, void *arg3);
static void update_cmd_from_state(VelocityState state, DccCommand *cmd);
static int dcc_send(DccConnection* conn, char* command);
static void dcc_format_command(char *cmd_buffer, DccCommand cmd);
static int recv_answer(DccConnection *conn, char* answer);
static VelocityState dcc_decode_answer(char* answer, VelocityState previous);
static VelocityState dcc_decode_velocity(int velocity);

SYS_INIT(dcc_connect, APPLICATION, DCC_CONNECT_PRIO);

K_THREAD_DEFINE(dcc_send_thread, SEND_THREAD_STACK_SIZE,
				dcc_send_thread_entry, &dcc_conn, NULL, NULL,
				SEND_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(dcc_recv_thread, RECV_THREAD_STACK_SIZE,
				dcc_recv_thread_entry, &dcc_conn, NULL, NULL,
				RECV_THREAD_PRIORITY, 0, 0);

int dcc_connect() {
	char *dcc_ip = CONFIG_DCC_IP;
	LOG_INF("Connecting to DCC at %s", dcc_ip);

	struct sockaddr_in addr4 = {
		.sin_family = AF_INET,
		.sin_port = htons(2560)
	};
	zsock_inet_pton(AF_INET, dcc_ip, &addr4.sin_addr);

	int sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		LOG_ERR("Creating socket failed: %d", errno);
		return -errno;
	}

	int ret;
	while (1) {
		ret = zsock_connect(sock, (struct sockaddr *)&addr4, sizeof(addr4));
		if (ret == 0) {
			LOG_INF("Connected to DCC at %s", dcc_ip);
			break;
		}
		LOG_ERR("Cannot connect to TCP remote: %s. Retrying.", strerror(errno));
	}

	dcc_conn.sock = sock;

	return 0;
}

static void dcc_send_thread_entry(void *arg1, void *arg2, void *arg3) {
	LOG_INF("send_thread starting");
	DccConnection *conn = (DccConnection *)arg1;
	char dcc_command[20];
	VelocityState current;
	VelocityState previous = {
		.speed = 0,
		.direction = DIR_FORWARD
	};
	dcc_send(conn, DCC_LOCO_STAT);
	int seconds_since_last_send = 0;
	while(1) {
		k_sleep(K_SECONDS(1));
		current = get_desired_velocity();
		if(has_velocity_state_changed(previous, current)) {
			previous = current;
			update_cmd_from_state(current, &last_cmd);
			dcc_format_command(dcc_command, last_cmd);
			dcc_send(conn, dcc_command);
			seconds_since_last_send = 0;
		} else if(seconds_since_last_send > 60) {
			dcc_send(conn, DCC_PING);
			seconds_since_last_send = 0;
		}
		seconds_since_last_send++;
	}
}

void update_cmd_from_state(VelocityState state, DccCommand *cmd) {
	cmd->speed = state.speed;
	cmd->direction = state.direction;
}

static void dcc_recv_thread_entry(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("recv_thread starting");
	DccConnection *conn = (DccConnection *)arg1;
	VelocityState current = {
		.speed = 0,
		.direction = DIR_FORWARD
	};
	char answer[101];
	while (1) {
		int ret = recv_answer(conn, answer);
		if(ret > 0) {
			current = dcc_decode_answer(answer, current);
			leds_update_from_state(current);
		}
	}
}

int recv_answer(DccConnection *conn, char* answer){
	int ret;

	LOG_DBG("Trying to recv...");

	ret = zsock_recv(conn->sock, answer, 100, 0);
	LOG_INF("Received: %d", ret);

	if (ret < 0) {
		LOG_ERR("Cannot recv from remote: %s", strerror(errno));
	} else 	if (ret == 0) {
		LOG_DBG("Received nothing");
		k_sleep(K_MSEC(100));
	} else {
		answer[ret] = '\0';
		LOG_INF("'%s'", answer);
	}
	return ret;
}

VelocityState dcc_decode_answer(char *answer, VelocityState previous) {
	int velocity, flags;
	int n_decoded = sscanf(answer, "<l 3 0 %d %d>", &velocity, &flags);
	if(n_decoded == 2) {
		LOG_INF("speed: %d, flags: %d ", velocity, flags);
		return dcc_decode_velocity(velocity);
	}
	return previous;
}

VelocityState dcc_decode_velocity(int velocity) {
	VelocityState new_velocity;
	if (velocity > 127 ) {
		new_velocity.direction = DIR_FORWARD;
		velocity -= 129;
	} else {
		new_velocity.direction = DIR_BACKWARD;
		velocity -= 1;
	}
	new_velocity.speed = velocity <= 0 ? 0 : velocity;
	return new_velocity;
}

int dcc_send(DccConnection* conn, char* command) {
	int ret;
	LOG_INF("Sending command '%s'", command);
	ret = zsock_send(conn->sock, command, strlen(command), 0);
	if (ret < 0) {
		LOG_ERR("Cannot send to remote: %s", strerror(errno));
		return -errno;
	}

	return 0;
}

void dcc_format_command(char *cmd_buffer, DccCommand cmd){
	sprintf(cmd_buffer, "<t %d %d %d>", cmd.addr, cmd.speed, cmd.direction);
}

// Ping (no of loco slots) "<#>"
// Track Info "<=>"
// Power on Track A "<1 A>"
// Power off Track A "<0 A>"
// Loco roster "<J R>"
// Loco 3 stop fwd "<F 3 0 0>"
// Loco 3 stop bwd "<F 3 0 1>"
// Loco 3 all off "<t 3 0 0>"
// Loco 3 Fn 0 on (?) "<t 3 0 1>"
