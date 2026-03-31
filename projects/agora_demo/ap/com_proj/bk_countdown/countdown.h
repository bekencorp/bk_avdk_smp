#pragma once

/**
 * @brief Start the countdown timer
 * 
 * Creates and starts a oneshot timer. When the countdown expires, the system
 * will automatically enter deep sleep mode. If the timer already exists, it
 * will be reloaded with the new timeout value.
 * 
 * @param time_ms Countdown duration in milliseconds
 * 
 * @note When the countdown expires, it triggers a forced deep sleep 
 *       (RESET_SOURCE_FORCE_DEEPSLEEP)
 */
void start_countdown(uint32_t time_ms);

/**
 * @brief Stop the countdown timer
 * 
 * Stops and deletes the countdown timer, releasing associated resources.
 * This function is safe to call even if the timer is not initialized or 
 * not running.
 * 
 * @note After calling this function, the timer is completely destroyed. 
 *       You need to call start_countdown() again to recreate it.
 */
void stop_countdown();