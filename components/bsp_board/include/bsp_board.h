#pragma once
#include "esp_err.h"

esp_err_t bsp_board_init(void);
esp_err_t bsp_led_set(int level);