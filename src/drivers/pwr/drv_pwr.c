#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_power.h>

#include "drv_pwr.h"
#include "../adc/drv_adc.h"

LOG_MODULE_REGISTER(drv_pwr);

static void InitEmbVoltageRegulator(void)
{
    // we are using high voltage mode
    // GPIO output voltage is set to 1.8 volts by default and that is not enough to turn the LEDs on.
    // so we need to increase GPIO voltage to 3.0 volts.

    if ( (nrf_power_mainregstatus_get(NRF_POWER) == NRF_POWER_MAINREGSTATUS_HIGH) &&
            ( (NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) == (UICR_REGOUT0_VOUT_DEFAULT << UICR_REGOUT0_VOUT_Pos) ) ) 

    {

        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;

        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) 
        {
            ;
        }

        NRF_UICR->REGOUT0 = (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) | (UICR_REGOUT0_VOUT_3V0 << UICR_REGOUT0_VOUT_Pos);
        NRF_NVMC->CONFIG  = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;

        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) 
        {
            ;
        }

        // a reset is required for changes to take effect

        NVIC_SystemReset();
    }
}

int DriverPwrInit(void)
{
    InitEmbVoltageRegulator();
    return 0;
}

int DriverPwrBatteryLevel(uint16_t * level_mv)
{
    int value = DriverADCRead();

    if (value > 0)
    {
        (*level_mv) = (uint16_t)value * 2;
        return 0;
    }
    else
    {
        (*level_mv) = 0;
        return value;
    }
}

int DirverPwrIsPowerGood(void)
{
    int err = 0;
    uint16_t raw_mv = 0;

    err = DriverPwrBatteryLevel(&raw_mv);
    if (err < 0)
    {
        LOG_ERR("Failed to get battery level. Error: %d", err);
        return PWR_STATUS_UNKNONW;
    }

    if (raw_mv <= PWR_BATTERY_THRESHOLD_MV)
        return PWR_STATUS_NOK;

    return PWR_STATUS_OK;
}
