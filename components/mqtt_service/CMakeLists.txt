idf_component_register(
    SRCS "mqtt_service.c"
    INCLUDE_DIRS "include"
     REQUIRES driver esp_adc esp_event esp_timer esp_netif esp_modem app_logic diag_service mqtt json
)