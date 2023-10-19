#if !defined(driver_leds_h)
#define driver_leds_h

#ifdef __cplusplus
    extern "C" {
#endif

enum
{
    LED_STATE_OFF = 0,
    LED_STATE_ON  = 1,
};

enum
{
    LED_IDX_RED = 0,
    LED_IDX_GREEN,
};

int DriverLedsInit              (void);
int DriverLedsSetState          (uint8_t ledIdx, uint8_t state);
int DriverLedsSetStateOn        (uint8_t ledIdx);
int DriverLedsSetStateOff       (uint8_t ledIdx);
int DriverLedsStateToggle       (uint8_t ledIdx);
int DriverLedsSetStateOnAll     (void);
int DriverLedsSetStateOffAll    (void);

#ifdef __cplusplus
    }
#endif

#endif