/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "queue.h"

LOG_MODULE_REGISTER(THROTTLE);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user))
#error "No suitable devicetree overlay specified"
#endif


#define THROTTLE_THREAD_STACK_SIZE 1024
#define THROTTLE_THREAD_PRIORITY 99

#define THROTTLE_RANGE (CONFIG_THROTTLE_MAX - CONFIG_THROTTLE_MIN)

static const struct adc_dt_spec adc_throttle = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static uint16_t samples[CONFIG_THROTTLE_SAMPLES];
static const struct adc_sequence_options options = {
	.extra_samplings = CONFIG_THROTTLE_SAMPLES - 1,
	.interval_us = 0,
};
static struct adc_sequence sequence = {
	.buffer = samples,
	.buffer_size = sizeof(samples),
	.options = &options,
};

uint16_t calc_avg(uint16_t *samples);
int32_t convert_to_speed(int32_t raw_val);
bool has_speed_changed(int32_t speed);
static int32_t throttle_read(const struct adc_dt_spec *adc_throttle);
static void throttle_handle(int32_t value);
static void throttle_poll_loop();
static bool throttle_setup();
static void throttle_thread_entry(void *arg1, void *arg2, void *arg3);


K_THREAD_DEFINE(throttle_thread, THROTTLE_THREAD_STACK_SIZE,
				throttle_thread_entry, NULL, NULL, NULL,
				THROTTLE_THREAD_PRIORITY, 0, 0);


static void throttle_thread_entry(void *arg1, void *arg2, void *arg3) {
	LOG_INF("throttle_thread starting");
	if (!throttle_setup()) {
		return;
	}
	throttle_poll_loop();
}

static bool throttle_setup() {
	int err;

	/* Configure channel prior to sampling. */
	if (!adc_is_ready_dt(&adc_throttle)) {
		LOG_ERR("ADC controller device %s not ready", adc_throttle.dev->name);
		return false;
	}

	err = adc_channel_setup_dt(&adc_throttle);
	if (err < 0) {
		LOG_ERR("Could not setup adcchannel (%d)", err);
		return false;
	}
	return true;
}

static void throttle_poll_loop() {
	int32_t value;
	
	while (1) {
		value = throttle_read(&adc_throttle);
		if (value < 0) {
			LOG_WRN("Could not read (%d)\n", value);
			continue;
		}
		throttle_handle(value);
		k_sleep(K_MSEC(CONFIG_THROTTLE_POLL_MSEC));
	}
}

static int32_t throttle_read(const struct adc_dt_spec *adc_throttle) {
	(void)adc_sequence_init_dt(adc_throttle, &sequence);

	int err = adc_read_dt(adc_throttle, &sequence);
	if (err < 0) {
		return (int32_t)err;
	}
	return calc_avg(samples);
}

uint16_t calc_avg(uint16_t *samples) {
	uint32_t sum = 0;
	for (int i = 0; i < CONFIG_THROTTLE_SAMPLES; i++) {
		sum += samples[i];
	}
	return (uint16_t)(sum / CONFIG_THROTTLE_SAMPLES);
}

static int32_t last_sent_speed;

static void throttle_handle(int32_t value) {
	int32_t speed = convert_to_speed(value);
	if (has_speed_changed(speed)) {
		printk("%d\n", speed);
		queue_send_speed(speed);
		last_sent_speed = speed;
	}
}

bool has_speed_changed(int32_t speed) {
	return speed != last_sent_speed && (
		speed == 0 || speed >= 127 || abs(speed-last_sent_speed) > 2
	);
}

int32_t convert_to_speed(int32_t raw_val) {
	if (raw_val < CONFIG_THROTTLE_MIN) {
		raw_val = 0;
	} else if (raw_val >= CONFIG_THROTTLE_MAX) {
		raw_val = THROTTLE_RANGE-1;
	} else {
		raw_val -= CONFIG_THROTTLE_MIN;
	}

	double ratio = ((double)raw_val)/THROTTLE_RANGE;

	return (int32_t)(ratio * 128.0);
}
