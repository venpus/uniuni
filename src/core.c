#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "fwconfig.h"
#include "core.h"
#include "drivers/board.h"
#include "ble/bluetooth.h"


LOG_MODULE_REGISTER(core, LOG_LEVEL_DBG);


static void ShowConnectionStatus(void)
{
#ifdef ENABLE_DBG_GREEN_LED

    static uint8_t isNewConnection = 0;

    if (BluetoothGetStatus() == BLE_STATUS_CONNECTED)
    {
        DriverLedsSetState(LED_IDX_GREEN, LED_STATE_ON);
        if (isNewConnection)
        {
            DriverSoundPlay(100);
            isNewConnection = 0;
        }
    }
    else
    {
        isNewConnection = 1;
        DriverLedsStateToggle(LED_IDX_GREEN);
    }

#endif
}

static void AwaitButtonReleased(void)
{
    static int timeout = 0;

    while (1)
    {
        // wait when the button is release and enable it again

        if (DriverButtonIsReleased())
        {
            DriverLedsSetStateOffAll();
            DriverButtonEnable();
            return;
        }

        // show all leds in case after TIME_MSEC_ACTIVE_TIME sec 
        // the button is still pressed (emergency situation the button (we have a mechanical one) may
        // be got stuck and requires attation)

        if (timeout > TIME_MSEC_ACTIVE_TIME)
        {
            DriverLedsSetStateOnAll();
        }

        timeout += TIME_MSEC_CORE_PING;
        k_sleep(K_MSEC(TIME_MSEC_CORE_PING));
    }
}

static void CommonWakeupAction(void)
{
    static int timeout;

    // disable button to prevent unnecessary interruption

    DriverButtonDisable();

    // turn the Red LED on in case there is a problem with the battery level
    
    if (DirverPwrIsPowerGood() != PWR_STATUS_OK)
    {
        DriverLedsSetStateOn(LED_IDX_RED);
    }

    // send information via BLE (BLE advertising only)

    BluetoothAdvStart();

    timeout = 0;
    while (1)
    {
        if (BluetoothGetStatus() == BLE_STATUS_DISCONNECTED)
        {    
            if (timeout > TIME_MSEC_ACTIVE_TIME)
            {
                break;
            }
        }

        BluetoothAdvUpdate();
        ShowConnectionStatus();
        
        timeout += TIME_MSEC_CORE_PING;
        k_sleep(K_MSEC(TIME_MSEC_CORE_PING));
    }

    // and if there is no any connection within TIME_MSEC_ACTIVE_TIME
    // then stop any BLE activity in order to return to sleep mode

    BluetoothAdvStop();
    DriverLedsSetStateOffAll();

    // and enable button again once it's release

    AwaitButtonReleased();
}

static int      buttonPressEvent;
struct k_work   button_press_work;

static void button_press_handler(void)
{
    k_work_submit(&button_press_work);
}

static void HandleWakeupButtonPressEvent(struct k_work * work)
{
    DriverButtonDisable();

    k_sleep(K_MSEC(75));
    if (DriverButtonIsPressed())
    {
        // produce BEEP_COUNT beeps with BEEP_PERIOD_MS period
        
        for (int i = 0; i < BEEP_COUNT; i++)
        {
            DriverSoundPlay(BEEP_DURATION_MS);
            k_sleep(K_MSEC(BEEP_PERIOD_MS - BEEP_DURATION_MS));
        }

        // the button will be enabled later after sending info via BLE

        buttonPressEvent = 1;
        return;
    }

    // enable button here only if we detected the faulty press

    DriverButtonEnable();
}

K_WORK_DEFINE(button_press_work, HandleWakeupButtonPressEvent);

void CoreInit(void)
{
    PrintResetReason();
    BluetoothInit();

    buttonPressEvent = 0;

    SetButtonPressCallback(button_press_handler);
}

void CoreStart(void)
{
    // during start up process we need to show BLE activity
    // immideately, otherwise it's not clear if everything is okay or not
    // (especially if we have tried to update the code - OTA)

    CommonWakeupAction();

    static int timeout = 0;
    while (1)
    {
        if ( buttonPressEvent || ( timeout >= (TIME_SEC_WAKEUP_PERIOD * 1000) ) )
        {
            if (buttonPressEvent)
            {
                buttonPressEvent = 0;
            }

            CommonWakeupAction();
            timeout = 0;
        }

        timeout += TIME_MSEC_CORE_PING;
        k_sleep(K_MSEC(TIME_MSEC_CORE_PING));
    }
}
