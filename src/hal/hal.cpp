#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "hal.hpp"

LOG_MODULE_REGISTER(hal, LOG_LEVEL_DBG);

#define LED0_NODE DT_ALIAS(led0)
#if !DT_NODE_HAS_STATUS_OKAY(LED0_NODE)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#define BTN0_NODE DT_ALIAS(btn0)
#if !DT_NODE_HAS_STATUS_OKAY(BTN0_NODE)
#error "Unsupported board: btn0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(BTN0_NODE, gpios);

static struct gpio_callback input_cb_data;

int configure_input_gpio()
{
	if (!gpio_is_ready_dt(&btn))
	{
		return -ENODEV;
	}

	return gpio_pin_configure_dt(&btn, GPIO_INPUT | GPIO_PULL_UP);
}

int configure_input_interrupt()
{
	return gpio_pin_interrupt_configure_dt(&btn, GPIO_INT_EDGE_BOTH);
}

int register_interrupt_cb(gpio_callback_handler_t handler)
{
	gpio_init_callback(&input_cb_data, handler, BIT(btn.pin));

	return gpio_add_callback_dt(&btn, &input_cb_data);
}

int get_input_gpio_state(bool *active)
{
	int ret;

	ret = gpio_pin_get_dt(&btn);
	if ((ret != 0) && (ret != 1))
		return -1;

	if (ret == 1)
		*active = true;
	else
		*active = false;

	return 0;
}

int configure_output_gpio()
{
	if (!gpio_is_ready_dt(&led))
	{
		return ENODEV;
	}

	return gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
}

int turn_on_led()
{
	return gpio_pin_set_dt(&led, 1);
}

int turn_off_led()
{
	return gpio_pin_set_dt(&led, 0);
}

int toggle_led()
{
	return gpio_pin_toggle_dt(&led);
}
