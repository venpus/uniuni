ATTENTION. The project is created in VSCode. nRF Connect SDK v.2.4.0 is used.

-------------------------------
Current work flow
-------------------------------

The firmware is handling 2 different scenarios:

1. There are no button presses

    In this case the devices wakes up each 10 minutes (see TIME_MSEC_WAKEUP_PERIOD in fwconfig.h) and:

        - checks the battery level
            - the flashing RED led is used if the battery is to low

        - starts sending advertising packets:

            - advertising period is 500 msec 
            
            - advertising packet has manufacture data with the battery level (in mV) and current button state (0 or 1)
            
            - after 30 sec (see TIME_MSEC_ACTIVE_TIME in fwconfig.h) the device returns to the sleep mode


2. There is a button presson

    In this case the devices wake up immideately and:

        - reset timer clock for case 1 (10 minutes interval will be reseted to avoid unnecessary data transfer 
          in order to save power)

        - checks the battery level
            - the flashing RED led is used if the battery is to low

        - produce beep sound

        - starts sending advertising packets:

            - advertising period is 500 msec 
            
            - advertising packet has manufacture data with the battery level (in mV) and current button state (0 or 1)
            
            - after 30 sec (see TIME_MSEC_ACTIVE_TIME in fwconfig.h) the device returns to the sleep mode


ATTENTION. For debug purpose the Green LED is used to show BLE activity. Use ENABLE_DBG_GREEN_LED in fwconfig.h to
enabled/disable the Green LED.


-------------------------------
Advertising packet
-------------------------------

    Advertising packet has a manufacture data with the following structure:

        uint16_t company_code   - company ID code
        uint16_t battery_level  - current battery level in mV
        uint16_t button_state   - current button state (0/1 - release/pressed)

    There are 2 scenarios (see description above):

        - if we wake up due to timer (just to send information about battery status), 
          then button_state field will have information about current button state, so
          it may be chaged during advertising period in case someone has pressed the button

        - if we wake up due to button press, then button_state field will be equal to 1 during
          full advertising perdio even in the button has been released at some point.

    ATTENTION. Bluetooth LE uses little endianness to represent the data

    ATTENTION. Right now Company ID is 0x0059 (it's Nordic Semiconductor Company Identifier). 
    This ID must be changed before production. Please see note from Noridc Sem:
    
    >> Note: Bluetooth LE products are not allowed to broadcast a custom company ID without 
    >> being a Bluetooth SIG member. When you are ready to release your product, you would have 
    >> to apply for a Bluetooth SIG membership to get your own unique Company ID.

-------------------------------
Others
-------------------------------

1. The firmware supports 2 custom GATT services:

    ATTENTION. It makes sense to update UUIDs before production.

    Current Configuration (Read/Write)

        - BT_UUID_128_ENCODE(0xaa3dc387, 0x21fb, 0x5462, 0xabc2, 0xaa7c88669b18)

        Read:
            
            - it shows current wakeup interval as a string value

        Write:

            - it is not supported right now

    Current Status (Read Only):
    
        - BT_UUID_128_ENCODE(0x8eff2f37, 0x20fa, 0x506d, 0xb8ab, 0xb173d91675a6)

        Read:

            - it shows a string value with information about:

                - hardware version
                - firmware version
                - battery level in mV

2. The firmware support OTA (default Nordic solution)

    Unsigned image is used in the current version.
    Once the project is build ZIP file with image for OTA may be found here:

        build\zephyr\dfu_application.zip
