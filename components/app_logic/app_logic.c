#include "app_logic.h"
#include "bsp_board.h"

#include "mqtt_service.h"
#include "modem_service.h"
#include "diag_service.h"
#include "app_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "app_logic";

static volatile int s_pub_period_ms = APP_DEFAULT_PUB_MS;
static volatile int s_led_state = 0;

static void app_set_led(int state)
{
    s_led_state = state ? 1 : 0;
    bsp_led_set(s_led_state);
}

static void app_set_interval(int interval_ms)
{
    if (interval_ms < 1000) interval_ms = 1000;
    if (interval_ms > 3600000) interval_ms = 3600000;
    s_pub_period_ms = interval_ms;
}

static void publish_task(void *arg)
{
    (void)arg;

    char payload[256];
    int cnt = 0;
    TickType_t last_health = xTaskGetTickCount();

    while (1) {
        if ((xTaskGetTickCount() - last_health) >= pdMS_TO_TICKS(300000)) {
            diag_log_health("periodic",
                            modem_service_is_ip_ready(),
                            mqtt_service_is_connected(),
                            mqtt_service_get_outbox_size());
            last_health = xTaskGetTickCount();
        }

        if (modem_service_is_ip_ready() && mqtt_service_is_connected()) {
            snprintf(payload, sizeof(payload),
                     "{\"cnt\":%d,\"led\":%d,\"interval_ms\":%d}",
                     cnt++,
                     s_led_state,
                     s_pub_period_ms);

            int mid = mqtt_service_publish_telemetry(payload);
            ESP_LOGI(TAG, "Publish mid=%d payload=%s", mid, payload);
        } else {
            ESP_LOGW(TAG, "Skip publish: IP=%d MQTT=%d",
                     modem_service_is_ip_ready(),
                     mqtt_service_is_connected());
        }

        int delay_ms = s_pub_period_ms;
        if (delay_ms < 1000) delay_ms = 1000;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

esp_err_t app_logic_init(void)
{
    app_set_led(0);
    app_set_interval(APP_DEFAULT_PUB_MS);

    mqtt_service_config_t mqtt_cfg = {
        .broker_uri = APP_TB_URI,
        .access_token = APP_TB_TOKEN,
        .on_set_led = app_set_led,
        .on_set_interval = app_set_interval,
    };

    return mqtt_service_init(&mqtt_cfg);
}

esp_err_t app_logic_start(void)
{
    BaseType_t ok = xTaskCreate(publish_task, "publish_task", 4096, NULL, 8, NULL);
    return ok == pdPASS ? ESP_OK : ESP_FAIL;
}