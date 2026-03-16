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


// #include "TCA9555PWR.h"

// #define I2C_SCL 22
// #define I2C_SDA 21

// tca9555_t io;

// void i2c_init()
// {
//     i2c_config_t conf = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = I2C_SDA,
//         .scl_io_num = I2C_SCL,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = 400000};

//     i2c_param_config(I2C_NUM_0, &conf);
//     i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
// }

// void app_main()
// {
//     i2c_init();

//     tca9555_init(&io, I2C_NUM_0, 0x20, GPIO_NUM_19);

//     tca9555_set_pin_mode(&io, 0, 1); // input
//     tca9555_set_pin_mode(&io, 1, 0); // output

//     while (1)
//     {
//         tca9555_write_pin(&io, 1, 1);
//         vTaskDelay(pdMS_TO_TICKS(500));

//         tca9555_write_pin(&io, 1, 0);
//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }

// #include <stdio.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/uart.h"
// #include "esp_log.h"

// #define UART_PORT       UART_NUM_1
// #define UART_TX_PIN     17
// #define UART_RX_PIN     16
// #define UART_BAUD_RATE  115200
// #define BUF_SIZE        1024

// static const char *TAG = "EC800";

// static void ec800_send_cmd(const char *cmd)
// {
//     uart_write_bytes(UART_PORT, cmd, strlen(cmd));
//     uart_write_bytes(UART_PORT, "\r\n", 2);
//     ESP_LOGI(TAG, ">> %s", cmd);
// }

// static void ec800_read_response(void)
// {
//     uint8_t data[BUF_SIZE];
//     int len = uart_read_bytes(UART_PORT, data, BUF_SIZE - 1, pdMS_TO_TICKS(2000));
//     if (len > 0) {
//         data[len] = 0;
//         ESP_LOGI(TAG, "<< %s", (char *)data);
//     } else {
//         ESP_LOGW(TAG, "No response");
//     }
// }

// void app_main(void)
// {
//     uart_config_t uart_config = {
//         .baud_rate = UART_BAUD_RATE,
//         .data_bits = UART_DATA_8_BITS,
//         .parity    = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//         .source_clk = UART_SCLK_DEFAULT,
//     };

//     uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
//     uart_param_config(UART_PORT, &uart_config);
//     uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
//                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

//     vTaskDelay(pdMS_TO_TICKS(2000));

//     ec800_send_cmd("AT");
//     ec800_read_response();

//     ec800_send_cmd("ATE0");
//     ec800_read_response();

//     ec800_send_cmd("AT+CPIN?");
//     ec800_read_response();

//     ec800_send_cmd("AT+CSQ");
//     ec800_read_response();

//     ec800_send_cmd("AT+CREG?");
//     ec800_read_response();

//     // Example APN setup
//     ec800_send_cmd("AT+QICSGP=1,1,\"your_apn\",\"\",\"\",1");
//     ec800_read_response();

//     // Activate PDP context
//     ec800_send_cmd("AT+QIACT=1");
//     ec800_read_response();

//     // Query assigned IP
//     ec800_send_cmd("AT+QIACT?");
//     ec800_read_response();

//     while (1) {
//         vTaskDelay(pdMS_TO_TICKS(5000));
//         ec800_send_cmd("AT+CSQ");
//         ec800_read_response();
//     }
// }

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ec800.h"

static const char *TAG = "app";

static void modem_urc_cb(const char *line, void *ctx)
{
    ESP_LOGW(TAG, "URC: %s", line);
}

void app_main(void)
{
    ec800_config_t cfg = {
        .uart_num = UART_NUM_1,
        .tx_pin = 17,
        .rx_pin = 16,
        .baud_rate = 115200,
        .rx_buf_size = 2048,
        .tx_buf_size = 1024,
        .cmd_timeout_ms = 5000,
    };

    ESP_ERROR_CHECK(ec800_init(&cfg, modem_urc_cb, NULL));

    if (ec800_start() != EC800_OK) {
        ESP_LOGE(TAG, "modem start failed");
        return;
    }

    if (ec800_wait_for_network(30000) != EC800_OK) {
        ESP_LOGE(TAG, "network not ready");
        return;
    }

    if (ec800_send_sms("0963477510", "Hello from product firmware") != EC800_OK) {
        ESP_LOGE(TAG, "sms failed");
    }

    vTaskDelay(pdMS_TO_TICKS(2000));

    if (ec800_dial("0963477510") != EC800_OK) {
        ESP_LOGE(TAG, "call failed or unsupported");
    }

    vTaskDelay(pdMS_TO_TICKS(10000));
    ec800_hangup();
}