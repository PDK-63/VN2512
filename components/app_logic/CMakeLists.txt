idf_component_register(
    SRCS "app_logic.c"
    INCLUDE_DIRS "include"
     REQUIRES driver esp_adc bsp_board modem_service mqtt_service diag_service 
)