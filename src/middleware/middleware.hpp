#ifndef MIDDLEWARE_HPP
#define MIDDLEWARE_HPP


/**
 * @class ReadClass
 * @brief Class that handles GPIO input. TODO: Add description
 */
class ReadClass
{
public:
    /**
     * @brief Sets up input GPIO
     * @retval 0 - success, !0 - fail
     */
	int setup_input_gpio();
    /**
     * @brief Registers an interrupt callback on GPIO change
     * @retval 0 - success, !0 - fail
     */
	int register_cb();
};

/**
 * @class ReactClass
 * @brief Class that reacts to GPIO input. TODO: Add description
 */
class ReactClass
{
public:
    /**
     * @brief Sets up output GPIO
     * @retval 0 - success, !0 - fail
     */
	int setup_output_gpio();
    /**
     * @brief Registers an Zbus observer observing the GPIO change topic
     * @retval 0 - success, !0 - fail
     */
	int register_observer();
};

#endif