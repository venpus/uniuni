
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/gpio.h>
#include <hal/nrf_gpio.h>
#include <nrfx.h>
#include <helpers/nrfx_reset_reason.h>
#include <zephyr/logging/log.h>

#include "board.h"

LOG_MODULE_REGISTER(board);

int HardwareInit(void)
{
    int err = 0;
    int last_err;

    // ATTENTION. Each driver has own debug output in case there is a problem
    // So there is no need to have extra logs here (just return the latest error
    // code in case there is something)

    last_err = DriverPwrInit();
    if (last_err != 0)
        err = last_err;

    last_err = DriverLedsInit();
    if (last_err != 0)
        err = last_err;

    last_err = DriverButtonInit();
    if (last_err != 0)
        err = last_err;   

    last_err = DriverSoundInit();
    if (last_err != 0)
        err = last_err;

    last_err = DriverADCInit();
    if (last_err != 0)
        err = last_err;

    return err;
}

void PrintResetReason(void)
{
    uint32_t reason;
    reason = nrfx_reset_reason_get();

    LOG_INF("Reset reasons: %d", reason);
}

void SystemShutdown(void)
{
    const struct pm_state_info info = {
        PM_STATE_SOFT_OFF, 
        0, 
        0, 
        0
    };

    // shutdown LED, sensors and memory

    DriverLedsSetStateOffAll();

    // and only after that configure and shutdown MCU

    nrfx_reset_reason_clear(nrfx_reset_reason_get());

    // configure to generate PORT event (wakeup) on button press

    nrf_gpio_cfg_sense_set(NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(sw0), gpios), NRF_GPIO_PIN_SENSE_HIGH);

    // and activate the shutdown state
    
    if (!pm_state_force(0, &info))
    {
        // next part should not be execuded
        // if we have it, then something goes totaly wrong

        DriverLedsSetState(LED_IDX_RED,   LED_STATE_ON);
        DriverLedsSetState(LED_IDX_GREEN, LED_STATE_ON);

        while (1)
        {

        }
    }
}