/**
 * @file launcher.h
 * @brief Application launcher (horizontal carousel menu) public interface.
 */
#ifndef LAUNCHER_H
#define LAUNCHER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and show the carousel home page.
 *
 * Call only after LVGL is initialized and while holding the display lock
 * (lv_vendor_disp_lock()).
 */
void launcher_start(void);

/**
 * @brief Delete all carousel objects and timers.
 *
 * Used when leaving the page to start another app. All internal pointers are
 * cleared, so launcher_start() can rebuild the page later.
 */
void launcher_stop(void);

/**
 * @brief Clear internal pointers only.
 *
 * For use after LVGL has been deinitialized (lv_vendor_deinit). Does not touch
 * any LVGL object.
 */
void launcher_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* LAUNCHER_H */
