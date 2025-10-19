#include <sys/_types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "thread_prios.h"
#include "queue.h"

LOG_MODULE_REGISTER(BUTTONS);

#define BTN_FORWARD DT_ALIAS(btn_forward)
#define BTN_BACKWARD DT_ALIAS(btn_backward)

#if !DT_NODE_HAS_STATUS(BTN_FORWARD, okay)
#error "Unsupported board: btn-forward devicetree alias is not defined"
#endif
#if !DT_NODE_HAS_STATUS(BTN_BACKWARD, okay)
#error "Unsupported board: btn-backward devicetree alias is not defined"
#endif

static int btn_init();
static void btn_init_callback(void);
static void btn_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

SYS_INIT(btn_init, APPLICATION, BTN_INIT_PRIO);

struct buttons
{
	struct gpio_dt_spec forward;
	struct gpio_dt_spec backward;
};

static struct buttons buttons = {
	.forward = GPIO_DT_SPEC_GET(BTN_FORWARD, gpios),
	.backward = GPIO_DT_SPEC_GET(BTN_BACKWARD, gpios)
};

static int btn_init()
{
	gpio_pin_configure_dt(&buttons.forward, GPIO_INPUT);
	if (!gpio_is_ready_dt(&buttons.forward)) {
		LOG_ERR("Error: forward buttons device not ready");
	}
	gpio_pin_configure_dt(&buttons.backward, GPIO_INPUT);
	if (!gpio_is_ready_dt(&buttons.backward)) {
		LOG_ERR("Error: backward buttons device not ready");
	}

	btn_init_callback();
	LOG_INF("Buttons initialized");
	return 0;
}

static struct gpio_callback buttons_cb_data;

static void btn_init_callback(void)
{
	unsigned int pin_mask = BIT(buttons.forward.pin) | BIT(buttons.backward.pin);
	gpio_pin_interrupt_configure_dt(&buttons.forward, GPIO_INT_EDGE_FALLING);
	gpio_pin_interrupt_configure_dt(&buttons.backward, GPIO_INT_EDGE_FALLING);
	gpio_init_callback(&buttons_cb_data, btn_pressed, pin_mask);
	gpio_add_callback(buttons.forward.port, &buttons_cb_data);
	LOG_INF("Set up callback for buttonsd");
}

static void btn_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (pins & BIT(buttons.forward.pin))
	{
		int val = gpio_pin_get(dev, buttons.forward.pin);
		LOG_INF("Forward button pressed, val= %d", val);
		queue_send_direction(DIR_FORWARD);
	}
	if (pins & BIT(buttons.backward.pin))
	{
		int val = gpio_pin_get(dev, buttons.backward.pin);
		LOG_INF("Backward button pressed, val= %d", val);
		queue_send_direction(DIR_BACKWARD);
	}
}
