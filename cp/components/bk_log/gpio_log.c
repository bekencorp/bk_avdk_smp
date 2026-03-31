#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <os/os.h>

#if CONFIG_GPIO_LOG_ENABLE

#if CONFIG_GPIO_LOG_CLK_IDLE_STATE  // idle state is high
#define GPIO_LOG_CLK_RESET() GPIO_UP(CONFIG_GPIO_LOG_CLK_PIN)
#define GPIO_LOG_CLK_SET()   GPIO_DOWN(CONFIG_GPIO_LOG_CLK_PIN)
#else  // idle state is low
#define GPIO_LOG_CLK_RESET() GPIO_DOWN(CONFIG_GPIO_LOG_CLK_PIN)
#define GPIO_LOG_CLK_SET()   GPIO_UP(CONFIG_GPIO_LOG_CLK_PIN)
#endif

#define GPIO_LOG_DATA_SET_1() GPIO_UP(CONFIG_GPIO_LOG_DATA_PIN)
#define GPIO_LOG_DATA_SET_0() GPIO_DOWN(CONFIG_GPIO_LOG_DATA_PIN)

static void gpio_log_echo_bit(uint8_t bit)
{
    GPIO_LOG_CLK_RESET();
    if (bit) {
        GPIO_LOG_DATA_SET_1();
    } else {
        GPIO_LOG_DATA_SET_0();
    }
    GPIO_LOG_CLK_SET();
}

static inline void gpio_log_write_byte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        gpio_log_echo_bit(data & (1 << (7 - i)));
    }
}

static inline void gpio_log_write(const char *data, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        gpio_log_write_byte(data[i]);
    }
    GPIO_LOG_CLK_RESET();
}

void bk_gpio_log_print(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    gpio_log_write(buf, strlen(buf));
}

void bk_gpio_log_print_data(const char *data, size_t length)
{
    gpio_log_write(data, length);
}

#endif
