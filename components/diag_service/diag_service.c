#include "diag_service.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "diag_service";

static int s_ppp_restart_count = 0;
static int s_mqtt_restart_count = 0;
static int s_sync_fail_count = 0;

esp_err_t diag_service_init(void)
{
    s_ppp_restart_count = 0;
    s_mqtt_restart_count = 0;
    s_sync_fail_count = 0;
    return ESP_OK;
}

void diag_inc_ppp_restart(void)
{
    s_ppp_restart_count++;
}

void diag_inc_mqtt_restart(void)
{
    s_mqtt_restart_count++;
}

void diag_inc_sync_fail(void)
{
    s_sync_fail_count++;
}

void diag_log_health(const char *tag, int got_ip, int mqtt_ok, int mqtt_outbox)
{
    size_t free8  = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t min8   = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    size_t free32 = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    size_t min32  = heap_caps_get_minimum_free_size(MALLOC_CAP_32BIT);

    ESP_LOGI(TAG,
             "[%s] GOT_IP=%d MQTT_OK=%d outbox=%d "
             "heap8 free=%u min=%u heap32 free=%u min=%u "
             "ppp_restart=%d mqtt_restart=%d sync_fail=%d",
             tag ? tag : "health",
             got_ip, mqtt_ok, mqtt_outbox,
             (unsigned)free8, (unsigned)min8,
             (unsigned)free32, (unsigned)min32,
             s_ppp_restart_count,
             s_mqtt_restart_count,
             s_sync_fail_count);
}