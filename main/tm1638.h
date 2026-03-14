#pragma once
#include "driver/gpio.h"
#include <stdint.h>

typedef struct
{
    gpio_num_t dio;
    gpio_num_t clk;
    gpio_num_t stb;
} tm1638_t;

void tm1638_init(tm1638_t *dev, gpio_num_t dio, gpio_num_t clk, gpio_num_t stb);
void tm1638_write_digit(tm1638_t *dev, uint8_t pos, uint8_t seg);
void tm1638_display_raw(tm1638_t *dev, uint8_t *data);
void tm1638_display_number(tm1638_t *dev, int num);