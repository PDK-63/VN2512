// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "ntc_driver.h"

// ntc_t ntc;

// void app_main()
// {
//     ntc_init(&ntc, ADC1_CHANNEL_6);   

//     while (1)
//     {
//         float temp = ntc_read_temperature(&ntc);

//         printf("Temperature: %.2f C\n", temp);

//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }


#include "TCA9555PWR.h"

#define I2C_SCL 22
#define I2C_SDA 21

tca9555_t io;

void i2c_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000};

    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

void app_main()
{
    i2c_init();

    tca9555_init(&io, I2C_NUM_0, 0x20, GPIO_NUM_19);

    tca9555_set_pin_mode(&io, 0, 1); // input
    tca9555_set_pin_mode(&io, 8, 0); // output

    while (1)
    {
        tca9555_write_pin(&io, 8, 1);
        vTaskDelay(pdMS_TO_TICKS(500));

        tca9555_write_pin(&io, 8, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}