#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include "drv_sound.h"

LOG_MODULE_REGISTER(drv_sound);

static const struct pwm_dt_spec buzzer = PWM_DT_SPEC_GET(DT_ALIAS(sound0));

int DriverSoundInit(void)
{
    if (!device_is_ready(buzzer.dev))
    {
        LOG_ERR("PWM module is not ready");
        return -ENODEV;
    }

    return 0;
}

int DriverSoundPlay(uint32_t duration_msec)
{
    // ATTENTION. We are using constant freq. when the buzzer can produce tha max
    // loud sound (accroding to the datasheet it's 4 kHz). So the period and pulse
    // lenght for pwm_set_dt() function is counted in the corresponding way.

    // 4 kHz and 50% duty cycle
    // period:      1 / 4 kHz = 0.25 msec = 250 usec
    // pulse width: 125 usec

    int err = 0;

    err = pwm_set_dt(&buzzer, PWM_USEC(250), PWM_USEC(125)); 
    if (err)
    {
        LOG_ERR("Failed to set the period and pulse width for PWM. Error: %d", err);
        return err;
    }  

    k_sleep(K_MSEC(duration_msec));

    err = pwm_set_dt(&buzzer, PWM_USEC(0), PWM_USEC(0)); 
    if (err)
    {
        LOG_ERR("Failed to set the period and pulse width for PWM. Error: %d", err);
        return err;
    }

    return err;
}