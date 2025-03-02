#include <cstdint>
#include <zephyr/logging/log.h>

#include "middleware.hpp"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);


int main(void)
{
	int ret;
	ReadClass readobj;
	ReactClass reactobj;

	ret = readobj.setup_input_gpio();
	if (ret) {
		LOG_ERR("Failed to setup input GPIO");
		return 0;
	}

	ret = readobj.register_cb();
	if (ret) {
		LOG_ERR("Failed to register input interrupt callback");
		return 0;
	}

	ret = reactobj.setup_output_gpio();
	if (ret){
		LOG_ERR("Failed to setup output GPIO");
		return 0;
	}

	ret = reactobj.register_observer();
	if (ret) {
		LOG_ERR("Failed to register observer");
		return 0;
	}

	LOG_INF("App initialized successfully");

	return 0;
}