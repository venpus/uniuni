#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "drv_button.h"

LOG_MODULE_REGISTER(drv_button);

#define BUTTONS_NODE        DT_ALIAS(sw0)
#define GPIO_SPEC(node_id)  GPIO_DT_SPEC_GET(BUTTONS_NODE, gpios)

static const struct gpio_dt_spec button = GPIO_SPEC(button0);

static struct gpio_callback button_cb_data;

static int isEnabled;

static ButtonPressCallback button_press_cb;

static void DriverButtonISR(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if (isEnabled)
    {
        if (button_press_cb != NULL)
        {
            button_press_cb();
        }
    }
}

void SetButtonPressCallback(ButtonPressCallback cb)
{
    button_press_cb = cb;
}

int DriverButtonInit(void)
{
    int err = 0;

    if (!device_is_ready(button.port))
    {
        LOG_ERR("GPIO (Button) is not ready");
        return -ENODEV;
    }

    err = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (err) 
    {
        LOG_ERR("Failed to configure GPIO. Error: %d", err);
        return err;
    }

    err = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (err)
    {
        LOG_ERR("Failed to configure GPIO ISR. Error: %d", err);
        return err;
    }

    gpio_init_callback(&button_cb_data, DriverButtonISR, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    button_press_cb = NULL;
    isEnabled = 1;

    return err;
}

int DriverButtonGetState(void)
{
    if (gpio_pin_get_dt(&button))
        return BUTTON_STATE_PRESSED;

    return BUTTON_STATE_RELEASED;
}

int DriverButtonIsPressed(void)
{
    return (DriverButtonGetState() == BUTTON_STATE_PRESSED);
}

int DriverButtonIsReleased(void)
{
    return (DriverButtonGetState() == BUTTON_STATE_RELEASED);
}

void DriverButtonEnable(void)
{
    // we don't disable button, we simply enable/disable 
    // calling callback

    isEnabled = 1;
}

void DriverButtonDisable(void)
{
    // we don't disable button, we simply enable/disable 
    // calling callback

    isEnabled = 0;
}