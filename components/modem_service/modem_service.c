#include "modem_service.h"
#include "diag_service.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_modem_api.h"
#include "esp_modem_config.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include <string.h>
#include "esp_netif.h"
#include "esp_netif_ppp.h"

#include "esp_err.h"
#include "esp_check.h"


static const char *TAG = "modem_service";

#define MODEM_IP_READY_BIT  BIT0

static modem_service_config_t s_cfg = {0};
static esp_modem_dce_t *s_dce = NULL;
static esp_netif_t *s_ppp_netif = NULL;
static EventGroupHandle_t s_ev = NULL;
static modem_state_t s_state = MODEM_STATE_OFF;

static void modem_escape_data_mode_uart(int uart_num)
{
    vTaskDelay(pdMS_TO_TICKS(1200));
    uart_write_bytes(uart_num, "+++", 3);
    uart_wait_tx_done(uart_num, pdMS_TO_TICKS(200));
    vTaskDelay(pdMS_TO_TICKS(1200));

    uart_flush(uart_num);
    uart_write_bytes(uart_num, "AT\r\n", 4);
    uart_wait_tx_done(uart_num, pdMS_TO_TICKS(200));
    vTaskDelay(pdMS_TO_TICKS(200));
}

static esp_err_t modem_sync_with_recover(esp_modem_dce_t *dce, int uart_num)
{
    for (int attempt = 0; attempt < 8; attempt++) {
        esp_err_t err = esp_modem_sync(dce);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Sync OK");
            return ESP_OK;
        }

        diag_inc_sync_fail();
        ESP_LOGW(TAG, "Sync TIMEOUT attempt=%d", attempt + 1);
        modem_escape_data_mode_uart(uart_num);
    }
    return ESP_ERR_TIMEOUT;
}

static void on_ip_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;

    if (base == IP_EVENT && id == IP_EVENT_PPP_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;

        ESP_LOGI(TAG, "PPP GOT IP: " IPSTR, IP2STR(&e->ip_info.ip));
        xEventGroupSetBits(s_ev, MODEM_IP_READY_BIT);
        s_state = MODEM_STATE_RUNNING;

        if (s_cfg.on_net_ready) s_cfg.on_net_ready(true);
    }
    else if (base == IP_EVENT && id == IP_EVENT_PPP_LOST_IP) {
        ESP_LOGW(TAG, "PPP LOST IP");
        xEventGroupClearBits(s_ev, MODEM_IP_READY_BIT);
        s_state = MODEM_STATE_RECOVERING;

        if (s_cfg.on_net_ready) s_cfg.on_net_ready(false);

        esp_err_t err = esp_modem_set_mode(s_dce, ESP_MODEM_MODE_DATA);
        ESP_LOGW(TAG, "Re-enter DATA mode => %s", esp_err_to_name(err));
    }
}

static esp_err_t configure_dce(void)
{
    esp_modem_dte_config_t dte_cfg = ESP_MODEM_DTE_DEFAULT_CONFIG();
    dte_cfg.uart_config.port_num = s_cfg.uart_port;
    dte_cfg.uart_config.tx_io_num = s_cfg.tx_pin;
    dte_cfg.uart_config.rx_io_num = s_cfg.rx_pin;
    dte_cfg.uart_config.rts_io_num = UART_PIN_NO_CHANGE;
    dte_cfg.uart_config.cts_io_num = UART_PIN_NO_CHANGE;
    dte_cfg.uart_config.flow_control = ESP_MODEM_FLOW_CONTROL_NONE;
    dte_cfg.uart_config.baud_rate = s_cfg.baudrate;
    dte_cfg.uart_config.rx_buffer_size = 4096;
    dte_cfg.uart_config.tx_buffer_size = 2048;
    dte_cfg.uart_config.event_queue_size = 30;
    dte_cfg.task_stack_size = 4096;
    dte_cfg.task_priority = 10;
    dte_cfg.dte_buffer_size = 1024;

    esp_modem_dce_config_t dce_cfg = ESP_MODEM_DCE_DEFAULT_CONFIG(s_cfg.apn);

    s_dce = esp_modem_new(&dte_cfg, &dce_cfg, s_ppp_netif);
    return s_dce ? ESP_OK : ESP_FAIL;
}

esp_err_t modem_service_init(const modem_service_config_t *cfg)
{
    if (!cfg || !cfg->apn) return ESP_ERR_INVALID_ARG;

    s_cfg = *cfg;

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, NULL));

    s_ev = xEventGroupCreate();
    if (!s_ev) return ESP_FAIL;

    esp_netif_config_t ppp_cfg = ESP_NETIF_DEFAULT_PPP();
    s_ppp_netif = esp_netif_new(&ppp_cfg);
    if (!s_ppp_netif) return ESP_FAIL;

    s_state = MODEM_STATE_INIT;
    return configure_dce();
}

esp_err_t modem_service_start(void)
{
    char tmp[128] = {0};

    s_state = MODEM_STATE_SYNC;
    ESP_RETURN_ON_ERROR(modem_sync_with_recover(s_dce, s_cfg.uart_port), TAG, "sync fail");

    (void)esp_modem_at(s_dce, "AT+CSCLK=0", tmp, 1000);

    s_state = MODEM_STATE_DATA;
    ESP_RETURN_ON_ERROR(esp_modem_set_mode(s_dce, ESP_MODEM_MODE_DATA), TAG, "set DATA mode fail");

    s_state = MODEM_STATE_WAIT_IP;
    return ESP_OK;
}

esp_err_t modem_service_restart_ppp(void)
{
    diag_inc_ppp_restart();

    if (s_cfg.on_net_ready) s_cfg.on_net_ready(false);
    xEventGroupClearBits(s_ev, MODEM_IP_READY_BIT);

    s_state = MODEM_STATE_RECOVERING;
    modem_escape_data_mode_uart(s_cfg.uart_port);

    return esp_modem_set_mode(s_dce, ESP_MODEM_MODE_DATA);
}

bool modem_service_is_ip_ready(void)
{
    if (!s_ev) return false;
    EventBits_t bits = xEventGroupGetBits(s_ev);
    return (bits & MODEM_IP_READY_BIT) != 0;
}

modem_state_t modem_service_get_state(void)
{
    return s_state;
}