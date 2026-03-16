#pragma once
#include <stdbool.h>
#include "esp_err.h"

typedef void (*mqtt_rpc_led_cb_t)(int state);
typedef void (*mqtt_rpc_interval_cb_t)(int interval_ms);
typedef void (*mqtt_rpc_status_req_cb_t)(void);

typedef struct {
    const char *broker_uri;
    const char *access_token;
    mqtt_rpc_led_cb_t on_set_led;
    mqtt_rpc_interval_cb_t on_set_interval;
} mqtt_service_config_t;

esp_err_t mqtt_service_init(const mqtt_service_config_t *cfg);
esp_err_t mqtt_service_start(void);
esp_err_t mqtt_service_stop(void);

bool mqtt_service_is_connected(void);
int mqtt_service_publish_telemetry(const char *json);
int mqtt_service_get_outbox_size(void);