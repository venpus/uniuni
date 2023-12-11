#if !defined(eddystone_h)
#define eddystone_h

#ifdef __cplusplus
    extern "C" {
#endif

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>

#define NUMBER_OF_SLOTS     1
#define EDS_VERSION         0x00
#define EDS_IDLE_TIMEOUT    K_SECONDS(30)

enum 
{
    EDS_TYPE_UID  = 0x00,
    EDS_TYPE_URL  = 0x10,
    EDS_TYPE_TLM  = 0x20,
    EDS_TYPE_EID  = 0x30,
    EDS_TYPE_NONE = 0xff,
};

enum 
{
    EDS_SLOT_UID = sys_cpu_to_be16(BIT(0)),
    EDS_SLOT_URL = sys_cpu_to_be16(BIT(1)),
    EDS_SLOT_TLM = sys_cpu_to_be16(BIT(2)),
    EDS_SLOT_EID = sys_cpu_to_be16(BIT(3)),
};

struct eds_capabilities 
{
    uint8_t  version;
    uint8_t  slots;
    uint8_t  uids;
    uint8_t  adv_types;
    uint16_t slot_types;
    uint8_t  tx_power;
} __packed;

enum 
{
    EDS_LOCKED = 0x00,
    EDS_UNLOCKED = 0x01,
    EDS_UNLOCKED_NO_RELOCKING = 0x02,
};

struct eds_slot 
{
    uint8_t  type;
    uint8_t  state;
    uint8_t  connectable;
    uint16_t interval;
    uint8_t  tx_power;
    uint8_t  adv_tx_power;
    uint8_t  lock[16];
    uint8_t  challenge[16];
    struct bt_data ad[3];
};

extern struct eds_slot eds_slots[NUMBER_OF_SLOTS];
extern uint8_t eds_active_slot;

int eds_adv_start   (void);
int eds_slot_restart(struct eds_slot *slot, uint8_t type);

#ifdef __cplusplus
    }
#endif

#endif