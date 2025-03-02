#ifndef HAL_HPP
#define HAL_HPP

/**
 * @brief Configure input GPIO
 * 
 * Validates that input GPIO is ready and configures it as input pullup
 * 
 * @retval 0 - success, !0 - fail
 */
int configure_input_gpio();

/**
 * @brief Configure input GPIO interrupt
 * 
 * Configures input GPIO interrupt on both edges
 * 
 * @retval 0 - success, !0 - fail
 */
int configure_input_interrupt();

/**
 * @brief Register input GPIO interrupt callback
 * 
 * @retval 0 - success, !0 - fail
 */
int register_interrupt_cb(gpio_callback_handler_t handler);

/**
 * @brief Get input GPIO state
 * 
 * @param[in,out] pointer to boolean indicating if gpio is active or not
 * @retval 0 - success, !0 - fail
 */
int get_input_gpio_state(bool *active);

/**
 * @brief Configure output GPIO
 * 
 * Validates that ouptut GPIO is ready and configures it as output and inactive
 * 
 * @retval 0 - success, !0 - fail
 */
int configure_output_gpio();

/**
 * @brief Turn on LED wrapper
 * 
 * @retval 0 - success, !0 - fail
 */
int turn_on_led();

/**
 * @brief Turn off LED wrapper
 * 
 * @retval 0 - success, !0 - fail
 */
int turn_off_led();

/**
 * @brief Toggle LED wrapper
 * 
 * @retval 0 - success, !0 - fail
 */
int toggle_led();

#endif