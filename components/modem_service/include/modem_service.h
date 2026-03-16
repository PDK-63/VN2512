#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include "modem_state.h"

typedef void (*modem_net_ready_cb_t)(bool ready);

typedef struct {
    const char *apn;
    int uart_port;
    int tx_pin;
    int rx_pin;
    int baudrate;
    modem_net_ready_cb_t on_net_ready;
} modem_service_config_t;

esp_err_t modem_service_init(const modem_service_config_t *cfg);
esp_err_t modem_service_start(void);
esp_err_t modem_service_restart_ppp(void);

bool modem_service_is_ip_ready(void);
modem_state_t modem_service_get_state(void);