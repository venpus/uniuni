#if !defined(bluetooth_h)
#define bluetooth_h

#ifdef __cplusplus
    extern "C" {
#endif

#include "../core.h"

enum
{
    BLE_STATUS_DISCONNECTED = 0,
    BLE_STATUS_CONNECTED,
};

enum
{
    BLE_ADV_TYPE_PING = 0,
    BLE_ADV_TYPE_EMERGENCY,
};

int BluetoothInit       (void);

int BluetoothAdvStart   (int type);
int BluetoothAdvUpdate  (void);
int BluetoothAdvStop    (void);

int BluetoothGetStatus  (void);

#ifdef __cplusplus
    }
#endif

#endif