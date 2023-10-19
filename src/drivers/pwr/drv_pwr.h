#if !defined(drv_pwr_h)
#define drv_pwr_h

#ifdef __cplusplus
    extern "C" {
#endif

#define PWR_BATTERY_THRESHOLD_MV    (1900)

enum
{
    PWR_STATUS_UNKNONW = 0,
    PWR_STATUS_NOK,
    PWR_STATUS_OK,
};

int DriverPwrInit           (void);
int DriverPwrBatteryLevel   (uint16_t * level_mv);
int DirverPwrIsPowerGood    (void);

#ifdef __cplusplus
    }
#endif

#endif