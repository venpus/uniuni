#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "drv_adc.h"
#include "../leds/drv_leds.h"

LOG_MODULE_REGISTER(drv_adc);

static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

int DriverADCInit(void)
{
    if (!device_is_ready(adc_channel.dev))
    {
        LOG_ERR("ADC Channel is not ready");
        return -ENODEV;
    }

    int err = adc_channel_setup_dt(&adc_channel);
    if (err < 0) 
    {
        LOG_ERR("ADC Channel can not be configured. Error: %d", err);
    }

    return err;
}

int DriverADCRead(void)
{
    int err;
    
    uint16_t buff;
    int32_t  raw;

    struct adc_sequence sequence = 
    {
        .buffer      = &buff,
        .buffer_size = sizeof(buff),
    };

    (void)adc_sequence_init_dt(&adc_channel, &sequence);
   
    err = adc_read(adc_channel.dev, &sequence);
    if (err < 0) 
    {
        LOG_ERR("Failed to read value from ADC Channel. Error: %d", err);
        return err;
    }

    if (adc_channel.channel_cfg.differential) 
    {
        raw = (int32_t)((int16_t)buff);
    } 
    else 
    {
        raw = (int32_t)buff;
    }

    err = adc_raw_to_millivolts_dt(&adc_channel, &raw);
    if (err < 0)
    {
        LOG_ERR("Failed to convert ADC results to mV. Error: %d", err);
        return err;
    }

    return (int)raw;
}