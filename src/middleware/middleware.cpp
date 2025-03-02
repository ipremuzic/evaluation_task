#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "middleware.hpp"
#include "hal.hpp"

LOG_MODULE_REGISTER(middleware, LOG_LEVEL_DBG);

struct input_event_msg {
	bool input_active;
};									

ZBUS_CHAN_DEFINE(input_event_chan,					/* Name */
				 struct input_event_msg,			/* Message type */

				 NULL,								/* Validator */
				 NULL,								/* User data */
				 ZBUS_OBSERVERS_EMPTY,				/* observers */
				 ZBUS_MSG_INIT(0) 					/* Initial value {0} */
);

static void turn_off_led_dwork_handler(struct k_work *work)
{
	int ret;

	ret = turn_off_led();
	if (ret)
		LOG_ERR("Failed to turn on LED, %d", ret);
}

static K_WORK_DELAYABLE_DEFINE(turn_off_led_dwork, turn_off_led_dwork_handler);

static void turn_on_led_dwork_handler(struct k_work *work)
{
	int ret;
	ret = turn_on_led();
	if(ret)
		LOG_ERR("Failed to turn off LED %d", ret);
}

static K_WORK_DELAYABLE_DEFINE(turn_on_led_dwork, turn_on_led_dwork_handler);

static uint32_t toggle_count = 0;

static void blink_led_dwork_handler(struct k_work *work)
{
	int ret;
	ret = toggle_led();
	if (ret) {
		LOG_ERR("Failed to toggle LED, %d", ret);
		return;
	}

	toggle_count++;

	/* Toogle on and off 3 times */
	if (toggle_count % 6 == 0)
		return;

	k_work_reschedule(k_work_delayable_from_work(work), K_MSEC(100));
}

static K_WORK_DELAYABLE_DEFINE(blink_led_dwork, blink_led_dwork_handler);

static void listener_cb(const struct zbus_channel *chan)
{
	const struct input_event_msg *msg =
					(const struct input_event_msg *) zbus_chan_const_msg(chan);

	if (msg->input_active) {
		/* If the state is high, blink the LED 3 times with 100ms between each blink */
		k_work_reschedule(&blink_led_dwork, K_NO_WAIT);
	} else {
		/* If the state is low, turn the LED on and keep it on for 500ms */
		k_work_cancel_delayable(&blink_led_dwork);
		k_work_reschedule(&turn_on_led_dwork, K_NO_WAIT);
		k_work_reschedule(&turn_off_led_dwork, K_MSEC(500));
	}
}

ZBUS_LISTENER_DEFINE(input_lis, listener_cb);

int ReadClass::setup_input_gpio()
{
	int ret = 0;

	ret = configure_input_gpio();
	if (ret) {
		LOG_ERR("Input GPIO config failed, %d", ret);
		return ret;
	}

	ret = configure_input_interrupt();
	if (ret)
		LOG_ERR("Input GPIO interrupt config failed %d", ret);

	return ret;
}

static void input_delayed_work_handler(struct k_work *work)
{
	struct input_event_msg msg;
	bool active;
	int ret;

	(void)work;

	ret = get_input_gpio_state(&active);
	if (ret) {
		LOG_ERR("Failed to get input pin logical state, %d", ret);
		return;
	}

	if (active)
		LOG_INF("Input GPIO is active");
	else
		LOG_INF("Input GPIO is inactive");

	/* Publish a change of state via Zbus */
	msg.input_active = active;
	ret = zbus_chan_pub(&input_event_chan, &msg, K_NO_WAIT);
	if (ret)
		LOG_ERR("Failed to publish to Zbus channel, %d", ret);

}

static K_WORK_DELAYABLE_DEFINE(input_delayed_work, input_delayed_work_handler);

void input_cb_handler(const struct device *port, struct gpio_callback *cb,
					  gpio_port_pins_t pins)
{
	/* Trigger a k_work callback after debounce period of 30ms*/
	k_work_reschedule(&input_delayed_work, K_MSEC(30));
}

int ReadClass::register_cb()
{
	int ret;

	ret = register_interrupt_cb(input_cb_handler);
	if (ret)
		LOG_ERR("Failed to register interrupt callback, %d", ret);

	return 0;
}

int ReactClass::setup_output_gpio()
{
	int ret = 0;

	ret = configure_output_gpio();
	if (ret)
		LOG_ERR("Failed to configure output GPIO, %d", ret);

	return ret;
}

int ReactClass::register_observer()
{
	int ret;

	ret = zbus_chan_add_obs(&input_event_chan, &input_lis, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to register ZBUS observer, %d", ret);
		return ret;
	}

	return 0;
}
