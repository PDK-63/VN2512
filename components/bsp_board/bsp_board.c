#include "bsp_board.h"
#include "app_config.h"
#include "driver/gpio.h"

esp_err_t bsp_board_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << APP_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_level(APP_LED_GPIO, 0));
    return ESP_OK;
}

esp_err_t bsp_led_set(int level)
{
    return gpio_set_level(APP_LED_GPIO, level ? 1 : 0);
}