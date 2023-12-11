#if !defined(board_h)
#define board_h

#ifdef __cplusplus
    extern "C" {
#endif

#include "buttons/drv_button.h"
#include "leds/drv_leds.h"
#include "pwr/drv_pwr.h"
#include "sound/drv_sound.h"
#include "adc/drv_adc.h"

int     HardwareInit        (void);
void    PrintResetReason    (void);
void    SystemShutdown      (void);

int     GetSerialNumber     (void);

#ifdef __cplusplus
    }
#endif

#endif