#pragma once

/* ===== MODEM ===== */
#define APP_MODEM_UART_PORT      UART_NUM_1
#define APP_MODEM_UART_TX        16
#define APP_MODEM_UART_RX        15
#define APP_MODEM_BAUDRATE       115200
#define APP_MODEM_APN            "m3-world"

/* ===== RS485 / MODBUS ===== */
#define APP_RS485_UART_PORT      UART_NUM_2
#define APP_RS485_TX             17
#define APP_RS485_RX             18
#define APP_RS485_BAUDRATE       9600
#define APP_RS485_BUF_SIZE       1024

#define APP_MB_SLAVE_ID          1
#define APP_MB_REG_START         0x0000
#define APP_MB_REG_QTY           2

/* ===== APP ===== */
#define APP_LED_GPIO             2
#define APP_DEFAULT_PUB_MS       10000

/* ===== THINGSBOARD ===== */
#define APP_TB_URI               "mqtt://demo.thingsboard.io:1883"
#define APP_TB_TOKEN             "YOUR_TOKEN_HERE"

#define APP_TB_TOPIC_TELEMETRY   "v1/devices/me/telemetry"
#define APP_TB_TOPIC_ATTR        "v1/devices/me/attributes"
#define APP_TB_TOPIC_RPC_REQ     "v1/devices/me/rpc/request/+"
#define APP_TB_TOPIC_RPC_RESP_PREFIX "v1/devices/me/rpc/response/"