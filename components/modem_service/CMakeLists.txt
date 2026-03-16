idf_component_register(
    SRCS "modem_service.c"
    INCLUDE_DIRS "include"
     REQUIRES driver esp_adc diag_service esp_event esp_timer esp_netif esp_modem
)