#include "mqtt_service.h"
#include "app_config.h"
#include "diag_service.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char *TAG = "mqtt_service";

static esp_mqtt_client_handle_t s_mqtt = NULL;
static bool s_connected = false;
static mqtt_service_config_t s_cfg = {0};

static bool extract_rpc_req_id(const char *topic, int topic_len, char *out_id, size_t out_sz)
{
    const char *prefix = "v1/devices/me/rpc/request/";
    size_t prefix_len = strlen(prefix);

    if ((size_t)topic_len < prefix_len) return false;
    if (strncmp(topic, prefix, prefix_len) != 0) return false;

    int id_len = topic_len - (int)prefix_len;
    if (id_len <= 0) return false;
    if ((size_t)id_len >= out_sz) id_len = (int)out_sz - 1;

    memcpy(out_id, topic + prefix_len, (size_t)id_len);
    out_id[id_len] = '\0';
    return true;
}

static void publish_rpc_response(const char *req_id, const char *json_resp)
{
    if (!s_mqtt || !req_id || !json_resp) return;

    char topic[128];
    snprintf(topic, sizeof(topic), APP_TB_TOPIC_RPC_RESP_PREFIX "%s", req_id);
    esp_mqtt_client_publish(s_mqtt, topic, json_resp, 0, 1, 0);
}

static void handle_rpc(const char *topic, int topic_len, const char *data, int data_len)
{
    char req_id[32];
    if (!extract_rpc_req_id(topic, topic_len, req_id, sizeof(req_id))) return;

    char *payload = malloc((size_t)data_len + 1);
    if (!payload) return;

    memcpy(payload, data, (size_t)data_len);
    payload[data_len] = '\0';

    cJSON *root = cJSON_Parse(payload);
    free(payload);

    if (!root) {
        publish_rpc_response(req_id, "{\"error\":\"invalid_json\"}");
        return;
    }

    cJSON *method = cJSON_GetObjectItem(root, "method");
    cJSON *params = cJSON_GetObjectItem(root, "params");

    if (!cJSON_IsString(method) || !method->valuestring) {
        cJSON_Delete(root);
        publish_rpc_response(req_id, "{\"error\":\"missing_method\"}");
        return;
    }

    if (strcmp(method->valuestring, "setLed") == 0) {
        int new_state = 0;

        if (cJSON_IsBool(params)) {
            new_state = cJSON_IsTrue(params) ? 1 : 0;
        } else if (cJSON_IsNumber(params)) {
            new_state = params->valueint ? 1 : 0;
        } else {
            cJSON_Delete(root);
            publish_rpc_response(req_id, "{\"error\":\"params_expected_bool_or_number\"}");
            return;
        }

        if (s_cfg.on_set_led) s_cfg.on_set_led(new_state);
        cJSON_Delete(root);
        publish_rpc_response(req_id, new_state ? "{\"ok\":true,\"led\":1}" : "{\"ok\":true,\"led\":0}");
        return;
    }

    if (strcmp(method->valuestring, "setInterval") == 0) {
        if (!cJSON_IsNumber(params)) {
            cJSON_Delete(root);
            publish_rpc_response(req_id, "{\"error\":\"params_expected_number_ms\"}");
            return;
        }

        int ms = params->valueint;
        if (s_cfg.on_set_interval) s_cfg.on_set_interval(ms);

        char resp[96];
        snprintf(resp, sizeof(resp), "{\"ok\":true,\"interval_ms\":%d}", ms);

        cJSON_Delete(root);
        publish_rpc_response(req_id, resp);
        return;
    }

    cJSON_Delete(root);
    publish_rpc_response(req_id, "{\"error\":\"unknown_method\"}");
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;

    esp_mqtt_event_handle_t e = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "MQTT connected");
        esp_mqtt_client_subscribe(s_mqtt, APP_TB_TOPIC_RPC_REQ, 1);
        esp_mqtt_client_publish(s_mqtt, APP_TB_TOPIC_ATTR,
                                "{\"fw\":\"pppos-service\",\"net\":\"ppp\"}",
                                0, 1, 0);
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "MQTT disconnected");
        break;

    case MQTT_EVENT_DATA:
        if (e->topic && e->data && e->topic_len > 0) {
            handle_rpc(e->topic, e->topic_len, e->data, e->data_len);
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        break;

    default:
        break;
    }
}

esp_err_t mqtt_service_init(const mqtt_service_config_t *cfg)
{
    if (!cfg || !cfg->broker_uri || !cfg->access_token) return ESP_ERR_INVALID_ARG;
    s_cfg = *cfg;
    return ESP_OK;
}

esp_err_t mqtt_service_start(void)
{
    if (s_mqtt) return ESP_OK;

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    char client_id[64];
    snprintf(client_id, sizeof(client_id),
             "ppp_%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = s_cfg.broker_uri,
        .credentials.username = s_cfg.access_token,
        .credentials.client_id = client_id,
        .session.keepalive = 60,
        .network.disable_auto_reconnect = false,
    };

    s_mqtt = esp_mqtt_client_init(&cfg);
    if (!s_mqtt) return ESP_FAIL;

    esp_mqtt_client_register_event(s_mqtt, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_mqtt);
    return ESP_OK;
}

esp_err_t mqtt_service_stop(void)
{
    if (!s_mqtt) return ESP_OK;
    s_connected = false;
    esp_mqtt_client_stop(s_mqtt);
    esp_mqtt_client_destroy(s_mqtt);
    s_mqtt = NULL;
    return ESP_OK;
}

bool mqtt_service_is_connected(void)
{
    return s_connected;
}

int mqtt_service_publish_telemetry(const char *json)
{
    if (!s_mqtt || !s_connected || !json) return -1;
    return esp_mqtt_client_publish(s_mqtt, APP_TB_TOPIC_TELEMETRY, json, 0, 1, 0);
}

int mqtt_service_get_outbox_size(void)
{
    if (!s_mqtt) return -1;
    return esp_mqtt_client_get_outbox_size(s_mqtt);
}