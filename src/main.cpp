#include <stdio.h>
#include <zephyr/sys/printk.h> //TODO: Remove later if not needed
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define BUTTON_PRESSED		1
#define BUTTON_NOT_PRESSED	0

// TODO: Refactor, maybe move HW things to service/library/middleware layer
#define LED0_NODE	DT_ALIAS(led0)
#if !DT_NODE_HAS_STATUS_OKAY(LED0_NODE)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

static struct gpio_callback btn_cb_data;

static void delayed_work_handler(struct k_work *work)
{
	(void)work;

	int btn_state = gpio_pin_get_dt(&btn);

	if (btn_state == BUTTON_PRESSED) {
		LOG_INF("Button pressed");
	} else if (btn_state == BUTTON_NOT_PRESSED){
		LOG_INF("Button released");
	} else {
		LOG_ERR("Invalid button state, %d", btn_state);
		return;
	}

	//TODO: Publish a change of state vie Zbus

}

static K_WORK_DELAYABLE_DEFINE(delayed_work, delayed_work_handler);

void btn_int_handler(const struct device *port, struct gpio_callback *cb,
					 gpio_port_pins_t pins)
{
	/* Trigger a k_work callback after debounce period */
	k_work_reschedule(&delayed_work, K_MSEC(30));								//TODO: Optimize maybe
}

/*
• Setup a GPIO as input pullup
• Register a interrupt callback which will trigger on GPIO input change
• Register a k_work callback and trigger it from interrupt
• Inside k_work interrupt publish a change of state via Zbus
• Log the change
• Debounce the GPIO input
*/
/**
 * @class Class that handles GPIO input
 */
class ReadClass
{
public:
	ReadClass();
	~ReadClass() {}
	int setup();
	int register_cb();
};

/*
• Setup a GPIO as output (LED)
• Register an observer observing the GPIO change topic
• If the state is high, blink the LED 3 times with 100ms between each blink
• If the state is low, turn the LED on and keep it on for 500ms.
• Use either `k_work_delayable` or a thread to control the LED state - do not control it directly from observer callback.
*/
/**
 * @class Class that reacts to reacts to GPIO input
 */
class ReactClass
{
public:
	ReactClass();
	~ReactClass() {}
	int setup();
	int register_observer();
};

/*
 * @brief ReadClass constructor
 *
 * TODO: describe
 *
 */
ReadClass::ReadClass()
{
	LOG_INF("Contruct ReadClass");
}

int ReadClass::setup()
{
	int ret;

	LOG_INF("Setup input");

	if (!gpio_is_ready_dt(&btn)) {
		return -ENODEV;
	}

	/* Setup a GPIO as input pullup */
	ret = gpio_pin_configure_dt(&btn, GPIO_INPUT | GPIO_PULL_UP);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&btn, GPIO_INT_EDGE_BOTH);
	if (ret) {
		return ret;
	}

	return 0;
}

int ReadClass::register_cb()
{
	int ret;
	
	LOG_INF("Register input interrupt cb");

	/* Register a interrupt callback which will trigger on GPIO input change */
	gpio_init_callback(&btn_cb_data, btn_int_handler, BIT(btn.pin));
	ret = gpio_add_callback_dt(&btn, &btn_cb_data);
	if (ret) {
		return ret;
	}

	return 0;
}

ReactClass::ReactClass()
{
	LOG_INF("Contruct ReactClass");
}

int ReactClass::setup()
{
	int ret;

	LOG_INF("Setup output");

	if (!gpio_is_ready_dt(&led)) {
		return ENODEV;
	}

	/* Setup GPIO as output (LED) */
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Failed to configure GPIO pin, %d", ret);
		return ret;
	}

	return 0;
}

int ReactClass::register_observer()
{
	LOG_INF("Register observer");

	/* Register an ZBUS observer observing the GPIO change topic */

	return 0;
}

static int try_led_blink()
{
	int ret;
	bool led_state = true;

	LOG_INF("LED state: %s\n", gpio_pin_get_dt(&led) ? "ON" : "OFF");
	k_msleep(1000);

	for (uint32_t i = 0; i < 10; i++) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret) {
			LOG_ERR("gpio_pin_toggle_dt failed");
			return ret;
		}

		led_state = !led_state;
		LOG_INF("LED state: %s\n", gpio_pin_get_dt(&led) ? "ON" : "OFF");
		k_msleep(1000);
	}

	return 0;
}

int main(void)
{
	ReadClass readobj;
	ReactClass reactobj;

	printk("%s: Hello from %s\n", __FUNCTION__, CONFIG_BOARD);

	if (readobj.setup()) {
		LOG_ERR("Setup of readobj failed");
	}

	if (readobj.register_cb()) {
		LOG_ERR("Failed to register readobj callback");
	}

	reactobj.setup();
	reactobj.register_observer();

	// TEST led blinking
	try_led_blink();

	LOG_INF("Successful initialization and led test");

	return 0;
}