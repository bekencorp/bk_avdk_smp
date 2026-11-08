/**
 * @file juice_rush.h
 * @brief Juice Rush: slice-fruit game public interface (LVGL v9, plain C,
 *        no JS binding layer).
 */
#ifndef JUICE_RUSH_H
#define JUICE_RUSH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the game: build the scene and start the update timer.
 *
 * Call only after LVGL is initialized and while holding the display lock
 * (lv_vendor_disp_lock()).
 */
void juice_rush_start(void);

/**
 * @brief Stop the game and delete the scene (timer plus all objects).
 *
 * Used to return to the app carousel; juice_rush_start() can be called again.
 */
void juice_rush_stop(void);

/**
 * @brief Clear internal pointers only.
 *
 * For use after LVGL has been deinitialized (lv_vendor_deinit). Touches no
 * LVGL object; only clears the pointers so the scene can be rebuilt later.
 */
void juice_rush_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* JUICE_RUSH_H */
