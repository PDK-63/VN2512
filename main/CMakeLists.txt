idf_component_register(SRCS "main.c" "TM1638.c" "TCA9555PWR.c"
                    INCLUDE_DIRS "."
                    REQUIRES NTC TCA EC800M mqtt_service modem_service app_logic bsp_board)
