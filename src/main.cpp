#include <stdio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

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

static int turn_on_led()
{
	int ret;

	ret = gpio_pin_set_dt(&led, 1);
	if (ret) {
		LOG_ERR("Failed to turn on LED, %d", ret);
		return ret;
	}

	LOG_INF("LED turned on");

	return 0;
}

static int turn_off_led()
{
	int ret;

	ret = gpio_pin_set_dt(&led, 0);
	if (ret) {
		LOG_ERR("Failed to turn off LED, %d", ret);
		return ret;
	}

	LOG_INF("LED turned off");

	return 0;
}

static int toggle_led()
{
	int ret;

	ret = gpio_pin_toggle_dt(&led);
	if (ret) {
		LOG_ERR("Failed to toggle LED, %d", ret);
		return ret;
	}

	LOG_INF("LED toggled");

	return 0;
}

/* ZBUS */
struct btn_event_msg {
	bool btn_pressed;
};

ZBUS_CHAN_DEFINE(btn_event_chan,					/* Name */
				 struct btn_event_msg,				/* Message type */

				 NULL,								/* Validator */					//TODO: Maybe add validator, just for exercise
				 NULL,								/* User data */
				 ZBUS_OBSERVERS(btn_lis),			/* observers */
				 ZBUS_MSG_INIT(0) 					/* Initial value {0} */
);

static void turn_off_led_dwork_handler(struct k_work *work)
{
	turn_off_led();
}

static K_WORK_DELAYABLE_DEFINE(turn_off_led_dwork, turn_off_led_dwork_handler);

static void turn_on_led_dwork_handler(struct k_work *work)
{
	turn_on_led();
}

static K_WORK_DELAYABLE_DEFINE(turn_on_led_dwork, turn_on_led_dwork_handler);

static uint32_t toggle_count = 0;

static void blink_led_dwork_handler(struct k_work *work)
{
	if (toggle_led())
		return;

	toggle_count++;

	/* Toogle on and toggle off 3 times */
	if (toggle_count % 6 == 0)
		return;

	k_work_reschedule(k_work_delayable_from_work(work), K_MSEC(100));
}

static K_WORK_DELAYABLE_DEFINE(blink_led_dwork, blink_led_dwork_handler);

static void listener_cb(const struct zbus_channel *chan)
{
	const struct btn_event_msg *msg =
					(const struct btn_event_msg *) zbus_chan_const_msg(chan);

	LOG_INF("From listener: Button is %s", msg->btn_pressed? "pressed": "released");		//TODO: Remove

	if (msg->btn_pressed) {
		/* If the state is high, blink the LED 3 times with 100ms between each blink */
		k_work_reschedule(&blink_led_dwork, K_NO_WAIT);

	} else {
		/* If the state is low, turn the LED on and keep it on for 500ms */
		k_work_cancel_delayable(&blink_led_dwork);
		k_work_reschedule(&turn_on_led_dwork, K_NO_WAIT);
		k_work_reschedule(&turn_off_led_dwork, K_MSEC(500));
	}
}

ZBUS_LISTENER_DEFINE(btn_lis, listener_cb);										//TODO: Move zbus observer init to ReactClass:register_observer
/* ZBUS END */

static void input_delayed_work_handler(struct k_work *work)
{
	struct btn_event_msg msg;
	int btn_state;
	int ret;

	(void)work;

	ret = gpio_pin_get_dt(&btn);
	if ((ret != 0) && (ret != 1)) {
		LOG_ERR("Failed to get pin logical state");
		return;
	}

	btn_state = ret;

	if (btn_state == BUTTON_PRESSED) {
		LOG_INF("Button pressed");
		msg.btn_pressed = true;
	} else if (btn_state == BUTTON_NOT_PRESSED){
		LOG_INF("Button released");
		msg.btn_pressed = false;
	} else {
		LOG_ERR("Invalid button state, %d", btn_state);
		return;
	}

	/* Publish a change of state vie Zbus */
	ret = zbus_chan_pub(&btn_event_chan, &msg, K_NO_WAIT);
	if (ret)
		LOG_ERR("Failed to publish to Zbus channel, %d", ret);

}

static K_WORK_DELAYABLE_DEFINE(input_delayed_work, input_delayed_work_handler);

void btn_int_handler(const struct device *port, struct gpio_callback *cb,
					 gpio_port_pins_t pins)
{
	/* Trigger a k_work callback after debounce period */
	k_work_reschedule(&input_delayed_work, K_MSEC(30));
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

int main(void)
{
	ReadClass readobj;
	ReactClass reactobj;

	printk("%s: Hello from %s\n", __FUNCTION__, CONFIG_BOARD);

	if (readobj.setup()) {
		LOG_ERR("Setup readobj failed");
		return 0;
	}

	if (readobj.register_cb()) {
		LOG_ERR("Failed to register readobj callback");
		return 0;
	}

	//TODO: Init zbus channel here maybe, make it a part of readobj?
	//TODO: Log info about channels using iterator funciton

	if (reactobj.setup()){
		LOG_ERR("Setup reactobj failed");
		return 0;
	}

	if (reactobj.register_observer()) {
		LOG_ERR("Failed to register observer");
		return 0;
	}

	LOG_INF("Successful initialization and led test");

	return 0;
}