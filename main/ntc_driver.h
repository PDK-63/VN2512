#ifndef NTC_DRIVER_H
#define NTC_DRIVER_H

#include "driver/adc.h"
#include "esp_adc_cal.h"

typedef struct
{
    adc1_channel_t channel;

    float r_pullup;
    float beta;
    float r0;
    float t0;

    float alpha;
    float temp_filtered;

    esp_adc_cal_characteristics_t adc_chars;

} ntc_t;

void ntc_init(ntc_t *ntc, adc1_channel_t channel);

float ntc_read_temperature(ntc_t *ntc);

#endif