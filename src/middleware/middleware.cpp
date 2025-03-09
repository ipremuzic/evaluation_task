#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "middleware.hpp"
#include "hal.hpp"

LOG_MODULE_REGISTER(middleware, LOG_LEVEL_DBG);

#define LED_CTRL_THRD_STACKSZ 512

struct input_event_msg {
	bool input_active;
};

struct fifo_data_item {
	void *fifo_reserved;
	bool input_state;
};

K_FIFO_DEFINE(led_ctrl_fifo);

static void led_ctrl_thrd(void *thrd_ctx, void *unused1, void *unused2)
{
	struct fifo_data_item *fifo_data;
	int ret;

	while (1) {
		fifo_data =
				(struct fifo_data_item *) k_fifo_get(&led_ctrl_fifo, K_FOREVER);

		if (fifo_data->input_state) {
			/* If the state is high, blink the LED 3 times with 100ms between each blink */
			for (int i = 0; i < 3; i++) {
				ret = turn_on_led();
				if (ret)
					LOG_ERR("Failed to turn on LED %d", ret);

				k_msleep(100);

				ret = turn_off_led();
				if (ret)
					LOG_ERR("Failed to turn off LED %d", ret);

				k_msleep(100);
			}
		} else {
			/* If the state is low, turn the LED on and keep it on for 500ms */
			ret = turn_on_led();
			if (ret)
				LOG_ERR("Failed to turn on LED %d", ret);

			k_msleep(500);

			ret = turn_off_led();
			if (ret)
				LOG_ERR("Failed to turn off LED %d", ret);
		}

		k_free(fifo_data);
	}
}

K_THREAD_DEFINE(lec_ctrl_tid, LED_CTRL_THRD_STACKSZ, led_ctrl_thrd, NULL, NULL,
				NULL, K_PRIO_PREEMPT(1), 0, 0);

ZBUS_CHAN_DEFINE(input_event_chan,					/* Name */
				 struct input_event_msg,			/* Message type */

				 NULL,								/* Validator */
				 NULL,								/* User data */
				 ZBUS_OBSERVERS_EMPTY,				/* observers */
				 ZBUS_MSG_INIT(0) 					/* Initial value {0} */
);

static void listener_cb(const struct zbus_channel *chan)
{
	const struct input_event_msg *msg;
	struct fifo_data_item *fifo_data;

	msg = (const struct input_event_msg *) zbus_chan_const_msg(chan);
	fifo_data = (struct fifo_data_item *) k_malloc(sizeof(fifo_data));
	fifo_data->input_state = msg->input_active;
	k_fifo_put(&led_ctrl_fifo, fifo_data);
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
