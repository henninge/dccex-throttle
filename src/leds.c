#include <sys/_types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "queue.h"
#include "thread_prios.h"
#include "state.h"

LOG_MODULE_REGISTER(LEDS);

#define LED_FORWARD DT_ALIAS(led_forward)
#define LED_BACKWARD DT_ALIAS(led_backward)

#if !DT_NODE_HAS_STATUS(LED_FORWARD, okay)
#error "Unsupported board: led-forward devicetree alias is not defined"
#endif
#if !DT_NODE_HAS_STATUS(LED_BACKWARD, okay)
#error "Unsupported board: led-backward devicetree alias is not defined"
#endif

struct leds
{
	struct gpio_dt_spec forward;
	struct gpio_dt_spec backward;
};

static struct leds leds = {
	.forward = GPIO_DT_SPEC_GET(LED_FORWARD, gpios),
	.backward = GPIO_DT_SPEC_GET(LED_BACKWARD, gpios)
};

static int leds_init();

SYS_INIT(leds_init, APPLICATION, LEDS_INIT_PRIO);

static int leds_init()
{
	gpio_pin_configure_dt(&leds.forward, GPIO_OUTPUT | DT_GPIO_FLAGS(LED_FORWARD, gpios ) | GPIO_OUTPUT_INACTIVE);
	if (!gpio_is_ready_dt(&leds.forward)) {
		LOG_ERR("Error: forward led device not ready");
	}
	gpio_pin_configure_dt(&leds.backward, GPIO_OUTPUT | DT_GPIO_FLAGS(LED_BACKWARD, gpios ) | GPIO_OUTPUT_INACTIVE);
	if (!gpio_is_ready_dt(&leds.backward)) {
		LOG_ERR("Error: backward led device not ready");
	}

	LOG_INF("LEDs initialized");
	return 0;
}

void leds_update_from_state(VelocityState velocity) {
	gpio_pin_set_dt(&leds.forward, velocity.direction == DIR_FORWARD ? 1 : 0);
	gpio_pin_set_dt(&leds.backward, velocity.direction == DIR_BACKWARD ? 1 : 0);
}
