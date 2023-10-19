#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include "core.h"
#include "drivers/board.h"

void main(void)
{
    HardwareInit();
    CoreInit();

    if (IS_ENABLED(CONFIG_SETTINGS)) 
    {
        settings_load();
    }

    CoreStart();
}
