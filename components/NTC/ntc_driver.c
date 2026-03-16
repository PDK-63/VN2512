#include <math.h>
#include "ntc_driver.h"

#define DEFAULT_VREF 1100
#define ADC_SAMPLES 64
#define VCC 3.3

static uint32_t ntc_adc_read(adc1_channel_t channel)
{
    uint32_t sum = 0;

    for(int i = 0; i < ADC_SAMPLES; i++)
    {
        sum += adc1_get_raw(channel);
    }

    return sum / ADC_SAMPLES;
}

void ntc_init(ntc_t *ntc, adc1_channel_t channel)
{
    ntc->channel = channel;

    ntc->r_pullup = 10000.0;
    ntc->beta = 3950.0;
    ntc->r0 = 10000.0;
    ntc->t0 = 298.15;

    ntc->alpha = 0.1;
    ntc->temp_filtered = 25.0;

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(channel, ADC_ATTEN_DB_11);

    esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        DEFAULT_VREF,
        &ntc->adc_chars);
}

float ntc_read_temperature(ntc_t *ntc)
{
    uint32_t adc_raw = ntc_adc_read(ntc->channel);

    uint32_t voltage_mv =
        esp_adc_cal_raw_to_voltage(adc_raw, &ntc->adc_chars);

    float v = voltage_mv / 1000.0;

    // divider: pull-up trên, NTC dưới
    float r_ntc = ntc->r_pullup * v / (VCC - v);

    float tempK =
        1.0 /
        ((1.0 / ntc->t0) +
         (1.0 / ntc->beta) * log(r_ntc / ntc->r0));

    float tempC = tempK - 273.15;

    // EMA filter
    ntc->temp_filtered =
        ntc->temp_filtered +
        ntc->alpha * (tempC - ntc->temp_filtered);

    return ntc->temp_filtered;
}