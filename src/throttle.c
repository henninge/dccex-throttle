/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(THROTTLE);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user))
#error "No suitable devicetree overlay specified"
#endif


#define THROTTLE_THREAD_STACK_SIZE 1024
#define THROTTLE_THREAD_PRIORITY 99

#define THROTTLE_RANGE (CONFIG_THROTTLE_MAX - CONFIG_THROTTLE_MIN)

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_throttle = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);


uint16_t calc_avg(uint16_t *samples) {
	uint32_t sum = 0;
	for (int i = 0; i < CONFIG_THROTTLE_SAMPLES; i++) {
		sum += samples[i];
	}
	return (uint16_t)(sum / CONFIG_THROTTLE_SAMPLES);
}

int32_t throttle_read(const struct adc_dt_spec *adc_throttle) {
	uint16_t samples[CONFIG_THROTTLE_SAMPLES];

	/* Options for the sequence sampling. */
	const struct adc_sequence_options options = {
		.extra_samplings = CONFIG_THROTTLE_SAMPLES - 1,
		.interval_us = 0,
	};

	/* Configure the sampling sequence to be made. */
	struct adc_sequence sequence = {
		.buffer = samples,
		.buffer_size = sizeof(samples),
		.options = &options,
	};
	(void)adc_sequence_init_dt(adc_throttle, &sequence);

	int err = adc_read_dt(adc_throttle, &sequence);
	if (err < 0) {
		return (int32_t)err;
	}
	return calc_avg(samples);
}

int32_t convert_val(int32_t raw_val) {
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

static void throttle_thread_entry(void *arg1, void *arg2, void *arg3) {
	LOG_INF("throttle_thread starting");
	int err;

	/* Configure channel prior to sampling. */
	if (!adc_is_ready_dt(&adc_throttle)) {
		LOG_ERR("ADC controller device %s not ready", adc_throttle.dev->name);
		return;
	}

	err = adc_channel_setup_dt(&adc_throttle);
	if (err < 0) {
		LOG_ERR("Could not setup adcchannel (%d)", err);
		return;
	}

	while (1) {
		int32_t value;

		value = throttle_read(&adc_throttle);
		if (value < 0) {
			LOG_WRN("Could not read (%d)\n", err);
			continue;
		}

		printk("%d\n", convert_val(value));

		k_sleep(K_MSEC(CONFIG_THROTTLE_POLL_MSEC));
	}
}

K_THREAD_DEFINE(throttle_thread, THROTTLE_THREAD_STACK_SIZE,
				throttle_thread_entry, NULL, NULL, NULL,
				THROTTLE_THREAD_PRIORITY, 0, 0);