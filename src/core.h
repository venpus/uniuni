#if !defined(core_h)
#define core_h

#ifdef __cplusplus
    extern "C" {
#endif

#include <zephyr/kernel.h>

#define DEVICE_NAME             "BLE DBG"
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

void CoreInit(void);
void CoreStart(void);

#ifdef __cplusplus
    }
#endif

#endif