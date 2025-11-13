#include <stdint.h>
#include <sys/_types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "thread_prios.h"
#include "queue.h"

LOG_MODULE_REGISTER(BUTTONS);

#define BTN_FORWARD DT_ALIAS(btn_forward)
#define BTN_BACKWARD DT_ALIAS(btn_backward)
#define BTN_STOP DT_ALIAS(btn_stop)
#define BTN_WHITE_LEFT DT_ALIAS(btn_white_left)
#define BTN_WHITE_RIGHT DT_ALIAS(btn_white_right)
#define BTN_YELLOW DT_ALIAS(btn_yellow)
#define BTN_BLACK DT_ALIAS(btn_black)
#define BTN_GREEN DT_ALIAS(btn_green)
#define BTN_BLUE DT_ALIAS(btn_blue)

static int btn_init();
static void btn_ctrl_init_callback(void);
static void btn_func_init_callback(void);
static void btn_ctrl_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
static void btn_func_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

SYS_INIT(btn_init, APPLICATION, BTN_INIT_PRIO);

struct buttons
{
	struct gpio_dt_spec forward;
	struct gpio_dt_spec backward;
	struct gpio_dt_spec stop;
	struct gpio_dt_spec white_left;
	struct gpio_dt_spec white_right;
	struct gpio_dt_spec yellow;
	struct gpio_dt_spec black;
	struct gpio_dt_spec green;
	struct gpio_dt_spec blue;
};

static struct buttons buttons = {
	.forward = GPIO_DT_SPEC_GET(BTN_FORWARD, gpios),
	.backward = GPIO_DT_SPEC_GET(BTN_BACKWARD, gpios),
	.stop = GPIO_DT_SPEC_GET(BTN_STOP, gpios),
	.white_left = GPIO_DT_SPEC_GET(BTN_WHITE_LEFT, gpios),
	.white_right = GPIO_DT_SPEC_GET(BTN_WHITE_RIGHT, gpios),
	.yellow = GPIO_DT_SPEC_GET(BTN_YELLOW, gpios),
	.black = GPIO_DT_SPEC_GET(BTN_BLACK, gpios),
	.green = GPIO_DT_SPEC_GET(BTN_GREEN, gpios),
	.blue = GPIO_DT_SPEC_GET(BTN_BLUE, gpios)
};

static int btn_init()
{
	gpio_pin_configure_dt(&buttons.forward, GPIO_INPUT);
	gpio_pin_configure_dt(&buttons.backward, GPIO_INPUT);
	gpio_pin_configure_dt(&buttons.stop, GPIO_INPUT);
	gpio_pin_configure_dt(&buttons.white_left, GPIO_INPUT);
	gpio_pin_configure_dt(&buttons.white_right, GPIO_INPUT);
	gpio_pin_configure_dt(&buttons.yellow, GPIO_INPUT);
	gpio_pin_configure_dt(&buttons.black, GPIO_INPUT);
	gpio_pin_configure_dt(&buttons.green, GPIO_INPUT);
	gpio_pin_configure_dt(&buttons.blue, GPIO_INPUT);

	btn_ctrl_init_callback();
	btn_func_init_callback();

	LOG_INF("Buttons initialized");
	return 0;
}

static struct gpio_callback buttons_ctrl_cb_data;
static struct gpio_callback buttons_func_cb_data;

static void btn_ctrl_init_callback(void)
{
	unsigned int pin_mask = BIT(buttons.forward.pin) | BIT(buttons.backward.pin) | BIT(buttons.stop.pin);
	gpio_pin_interrupt_configure_dt(&buttons.forward, GPIO_INT_EDGE_FALLING);
	gpio_pin_interrupt_configure_dt(&buttons.backward, GPIO_INT_EDGE_FALLING);
	gpio_pin_interrupt_configure_dt(&buttons.stop, GPIO_INT_EDGE_FALLING);
	gpio_init_callback(&buttons_ctrl_cb_data, btn_ctrl_pressed, pin_mask);
	gpio_add_callback(buttons.forward.port, &buttons_ctrl_cb_data);
	LOG_INF("Set up callback for control buttons");
}

static void btn_func_init_callback(void)
{
	unsigned int pin_mask = 0;
	pin_mask |= BIT(buttons.white_left.pin);
	pin_mask |= BIT(buttons.white_right.pin);
	pin_mask |= BIT(buttons.yellow.pin);
	pin_mask |= BIT(buttons.black.pin);
	pin_mask |= BIT(buttons.green.pin);
	pin_mask |= BIT(buttons.blue.pin);

	gpio_pin_interrupt_configure_dt(&buttons.white_left, GPIO_INT_EDGE_FALLING);
	gpio_pin_interrupt_configure_dt(&buttons.white_right, GPIO_INT_EDGE_FALLING);
	gpio_pin_interrupt_configure_dt(&buttons.yellow, GPIO_INT_EDGE_FALLING);
	gpio_pin_interrupt_configure_dt(&buttons.black, GPIO_INT_EDGE_FALLING);
	gpio_pin_interrupt_configure_dt(&buttons.green, GPIO_INT_EDGE_FALLING);
	gpio_pin_interrupt_configure_dt(&buttons.blue, GPIO_INT_EDGE_FALLING);

	gpio_init_callback(&buttons_func_cb_data, btn_func_pressed, pin_mask);
	gpio_add_callback(buttons.white_left.port, &buttons_func_cb_data);
	LOG_INF("Set up callback for function buttons");
}

static void btn_ctrl_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (pins & BIT(buttons.forward.pin))
	{
		int val = gpio_pin_get(dev, buttons.forward.pin);
		LOG_DBG("Forward button pressed, val= %d", val);
		queue_send_direction(DIR_FORWARD);
	}
	if (pins & BIT(buttons.backward.pin))
	{
		int val = gpio_pin_get(dev, buttons.backward.pin);
		LOG_DBG("Backward button pressed, val= %d", val);
		queue_send_direction(DIR_BACKWARD);
	}
	if (pins & BIT(buttons.stop.pin))
	{
		int val = gpio_pin_get(dev, buttons.stop.pin);
		LOG_DBG("STOP button pressed, val= %d", val);
		queue_send_stop();
	}
}

void btn_func_handle(const char* name, const struct device *dev, uint32_t pins, gpio_pin_t pin) {
	if (pins & BIT(pin))
	{
		int val = gpio_pin_get(dev, pin);
		LOG_DBG("%s button pressed, val= %d", name, val);
	}

}

static void btn_func_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	btn_func_handle("White left", dev, pins, buttons.white_left.pin);
	btn_func_handle("White right", dev, pins, buttons.white_right.pin);
	btn_func_handle("Yellow", dev, pins, buttons.yellow.pin);
	btn_func_handle("Black", dev, pins, buttons.black.pin);
	btn_func_handle("Green", dev, pins, buttons.green.pin);
	btn_func_handle("Blue", dev, pins, buttons.blue.pin);
}
