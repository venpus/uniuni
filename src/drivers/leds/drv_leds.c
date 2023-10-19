#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "drv_leds.h"

LOG_MODULE_REGISTER(drv_leds);

#define LEDS_NODE           DT_PATH(leds)
#define GPIO_SPEC(node_id)  GPIO_DT_SPEC_GET(node_id, gpios),

static const struct gpio_dt_spec leds[] = 
{
    #if DT_NODE_EXISTS(LEDS_NODE)
        DT_FOREACH_CHILD(LEDS_NODE, GPIO_SPEC)
    #endif
};

int DriverLedsInit(void)
{
    int err = 0;

    for (size_t i = 0; i < ARRAY_SIZE(leds); i++) 
    {
        if (!device_is_ready(leds[i].port))
        {
            LOG_ERR("GPIO (LED index: %d) is not ready", i);
            return -ENODEV;
        }

        err = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT);
        if (err) 
        {
            LOG_ERR("Failed to configure GPIO. LED index: %d. Error: %d", i, err);
            return err;
        }
    }

    return DriverLedsSetStateOffAll();
}

int DriverLedsSetState(uint8_t ledIdx, uint8_t state)
{
    int err = 0;

    if (ledIdx >= ARRAY_SIZE(leds)) 
    {
        LOG_ERR("LED index is out of the range. Received index: %d", ledIdx);
        return -EINVAL;
    }

    if (state != LED_STATE_ON && state != LED_STATE_OFF)
    {
        LOG_ERR("LED state is not correct. Received value: %d", state);
        return -EINVAL;
    }

    err = gpio_pin_set_dt(&leds[ledIdx], state);

    if (err) 
    {
        LOG_ERR("Failed to set GPIO. LED index: %d. Error: %d", ledIdx, err);
    }

    return err;
}

int DriverLedsSetStateOn(uint8_t ledIdx)
{
    return DriverLedsSetState(ledIdx, LED_STATE_ON);
}

int DriverLedsSetStateOff(uint8_t ledIdx)
{
    return DriverLedsSetState(ledIdx, LED_STATE_OFF);
}

int DriverLedsStateToggle(uint8_t ledIdx)
{
    int err = 0;

    if (ledIdx >= ARRAY_SIZE(leds)) 
    {
        LOG_ERR("LED index is out of the range. Received index: %d", ledIdx);
        return -EINVAL;
    } 

    err = gpio_pin_toggle_dt(&leds[ledIdx]);

    if (err) 
    {
        LOG_ERR("Failed to toggle GPIO. LED index: %d. Error: %d", ledIdx, err);
    }

    return err;
}

int DriverLedsSetStateOffAll(void)
{
    int err = 0;
    int last_err;

    for (size_t i = 0; i < ARRAY_SIZE(leds); i++) 
    {
        last_err = DriverLedsSetStateOff(i);
        if (last_err != 0)
            err = last_err;
    }

    return err;
}

int DriverLedsSetStateOnAll(void)
{
    int err = 0;
    int last_err;

    for (size_t i = 0; i < ARRAY_SIZE(leds); i++) 
    {
        last_err = DriverLedsSetStateOn(i);
        if (last_err != 0)
            err = last_err;
    }

    return err;
}