#if !defined(bluetooth_h)
#define bluetooth_h

#ifdef __cplusplus
    extern "C" {
#endif

enum
{
    BLE_STATUS_DISCONNECTED = 0,
    BLE_STATUS_CONNECTED,
};

int BluetoothInit       (void);
int BluetoothAdvStart   (void);
int BluetoothGetStatus  (void);

#ifdef __cplusplus
    }
#endif

#endif