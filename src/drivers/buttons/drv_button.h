#if !defined(drv_button_h)
#define drv_button_h

#ifdef __cplusplus
    extern "C" {
#endif

#include <zephyr/kernel.h>

enum
{
    BUTTON_STATE_RELEASED = 0,
    BUTTON_STATE_PRESSED,
};

typedef void (*ButtonPressCallback)(void);

int     DriverButtonInit        (void);

int     DriverButtonGetState    (void);
int     DriverButtonIsReleased  (void);
int     DriverButtonIsPressed   (void);

void    DriverButtonEnable      (void);
void    DriverButtonDisable     (void);

void    SetButtonPressCallback  (ButtonPressCallback cb);

#ifdef __cplusplus
    }
#endif

#endif