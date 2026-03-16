#pragma once
#include "esp_err.h"

esp_err_t diag_service_init(void);

void diag_inc_ppp_restart(void);
void diag_inc_mqtt_restart(void);
void diag_inc_sync_fail(void);

void diag_log_health(const char *tag, int got_ip, int mqtt_ok, int mqtt_outbox);