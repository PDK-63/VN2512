idf_component_register(
    SRCS "ec800.c"
    INCLUDE_DIRS "include"
     REQUIRES driver esp_driver_uart freertos
)