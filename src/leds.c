#include <sys/_types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "queue.h"
#include "thread_prios.h"
#include "state.h"
#include "leds.h"

LOG_MODULE_REGISTER(LEDS);

#define LED_FORWARD DT_ALIAS(led_forward)
#define LED_BACKWARD DT_ALIAS(led_backward)
#define LED_STOP DT_ALIAS(led_stop)
#define LED_RED DT_ALIAS(led_red)
#define LED_GREEN DT_ALIAS(led_green)
#define LED_BLUE DT_ALIAS(led_blue)

#if !DT_NODE_HAS_STATUS(LED_FORWARD, okay)
#error "Unsupported board: led-forward devicetree alias is not defined"
#endif
#if !DT_NODE_HAS_STATUS(LED_BACKWARD, okay)
#error "Unsupported board: led-backward devicetree alias is not defined"
#endif
#if !DT_NODE_HAS_STATUS(LED_STOP, okay)
#error "Unsupported board: led-stop devicetree alias is not defined"
#endif
#if !DT_NODE_HAS_STATUS(LED_RED, okay)
#error "Unsupported board: led-red devicetree alias is not defined"
#endif
#if !DT_NODE_HAS_STATUS(LED_GREEN, okay)
#error "Unsupported board: led-green devicetree alias is not defined"
#endif
#if !DT_NODE_HAS_STATUS(LED_BLUE, okay)
#error "Unsupported board: led-blue devicetree alias is not defined"
#endif

struct leds
{
	struct gpio_dt_spec forward;
	struct gpio_dt_spec backward;
	struct gpio_dt_spec stop;
	struct gpio_dt_spec red;
	struct gpio_dt_spec green;
	struct gpio_dt_spec blue;
};

static struct leds leds = {
	.forward = GPIO_DT_SPEC_GET(LED_FORWARD, gpios),
	.backward = GPIO_DT_SPEC_GET(LED_BACKWARD, gpios),
	.stop = GPIO_DT_SPEC_GET(LED_STOP, gpios),
	.red = GPIO_DT_SPEC_GET(LED_RED, gpios),
	.green = GPIO_DT_SPEC_GET(LED_GREEN, gpios),
	.blue = GPIO_DT_SPEC_GET(LED_BLUE, gpios)
};

static int leds_init();
static void leds_init_led(struct gpio_dt_spec *spec, const char *led_name);

SYS_INIT(leds_init, APPLICATION, LEDS_INIT_PRIO);

static int leds_init()
{
	leds_init_led(&leds.forward, "forward");
	leds_init_led(&leds.backward, "backward");
	leds_init_led(&leds.stop, "stop");
	leds_init_led(&leds.red, "red");
	leds_init_led(&leds.green, "green");
	leds_init_led(&leds.blue, "blue");

	LOG_INF("LEDs initialized");
	leds_set_stage(STAGE_RED);
	return 0;
}

static void leds_init_led(struct gpio_dt_spec *spec, const char *led_name) {
	gpio_pin_configure_dt(spec, GPIO_OUTPUT | DT_GPIO_FLAGS(LED_FORWARD, gpios ) | GPIO_OUTPUT_INACTIVE);
	if (!gpio_is_ready_dt(spec)) {
		LOG_ERR("Error: %s led device not ready", led_name);
	}
}

void leds_update_from_state(VelocityState velocity) {
	gpio_pin_set_dt(&leds.forward, velocity.direction == DIR_FORWARD ? 1 : 0);
	gpio_pin_set_dt(&leds.backward, velocity.direction == DIR_BACKWARD ? 1 : 0);
	gpio_pin_set_dt(&leds.stop, velocity.stop ? 1 : 0);
}

void leds_set_stage(int stage) {
	gpio_pin_set_dt(&leds.red, 0);
	gpio_pin_set_dt(&leds.green, 0);
	gpio_pin_set_dt(&leds.blue, 0);
	struct gpio_dt_spec *spec = &leds.green;
	switch(stage) {
	case STAGE_RED:
		spec = &leds.red;
		break;
	case STAGE_GREEN:
		spec = &leds.green;
		break;
	case STAGE_BLUE:
		spec = &leds.blue;
		break;
	}
	gpio_pin_set_dt(spec, 1);
}
