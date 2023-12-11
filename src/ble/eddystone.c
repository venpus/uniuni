#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "eddystone.h"
#include "uuid.h"

LOG_MODULE_REGISTER(eddystone);

#define EDS_URL_READ_OFFSET 2
#define EDS_URL_WRITE_OFFSET 4

static const struct bt_data ad[] = 
{
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    /* Eddystone Service UUID a3c87500-8ed3-4bdf-8a39-a01bebede295 */
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
            0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
            0xdf, 0x4b, 0xd3, 0x8e, 0x00, 0x75, 0xc8, 0xa3),
};

/* Eddystone Service Variables */

static struct bt_uuid_128 eds_uuid              = EDS_UUID;
static struct bt_uuid_128 eds_caps_uuid         = EDS_CAPS_UUID;
static struct bt_uuid_128 eds_slot_uuid         = EDS_SLOT_UUID;
static struct bt_uuid_128 eds_intv_uuid         = EDS_INTV_UUID;
static struct bt_uuid_128 eds_tx_uuid           = EDS_TX_UUID;
static struct bt_uuid_128 eds_adv_tx_uuid       = EDS_ADV_TX_UUID;
static struct bt_uuid_128 eds_lock_uuid         = EDS_LOCK_UUID;
static struct bt_uuid_128 eds_unlock_uuid       = EDS_UNLOCK_UUID;
static struct bt_uuid_128 eds_ecdh_uuid         = EDS_ECDH_UUID; 
static struct bt_uuid_128 eds_eid_uuid          = EDS_EID_UUID; 
static struct bt_uuid_128 eds_data_uuid         = EDS_DATA_UUID; 
static struct bt_uuid_128 eds_reset_uuid        = EDS_RESET_UUID;
static struct bt_uuid_128 eds_connectable_uuid  = EDS_CONNECTABLE_UUID;

static struct eds_capabilities eds_caps = 
{
    .version    = EDS_VERSION,
    .slots      = NUMBER_OF_SLOTS,
    .slot_types = EDS_SLOT_URL,
};

// default lock: SAVVY20211016000

struct eds_slot eds_slots[NUMBER_OF_SLOTS] = 
{
    [0 ... (NUMBER_OF_SLOTS - 1)] = {
        .type      = EDS_TYPE_NONE,         /* Start as disabled */
        .state     = EDS_UNLOCKED,          /* Start unlocked */
        .interval  = sys_cpu_to_be16(BT_GAP_ADV_FAST_INT_MIN_2),
        .lock      = { 'S', 'A', 'V', 'V', 'Y', '2', '0', '2', '1', '1','0', '1', '6', '0', '0', '0' },
        .challenge = {},
        .ad = {
            BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
            BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
            BT_DATA_BYTES(BT_DATA_SVC_DATA16,
                    0xaa, 0xfe, /* Eddystone UUID */
                    0x10,       /* Eddystone-URL frame type */
                    0x00,       /* Calibrated Tx power at 0m */
                    0x00,       /* URL Scheme Prefix http://www. */ /* .org */
                    'z', 'e', 'p', 'h', 'y', 'r', 'p', 'r', 'o', 'j', 'e', 'c', 't', 0x08) 
        },
    },
};

uint8_t eds_active_slot;

static uint8_t eds_ecdh[32] = {};
static uint8_t eds_eid[16]  = {};

static ssize_t read_caps(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                            void *buf, uint16_t len, uint16_t offset)
{
    const struct eds_capabilities *caps = attr->user_data;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, caps, sizeof(*caps));
}

static ssize_t read_slot(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                            void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &eds_active_slot, sizeof(eds_active_slot));
}

static ssize_t write_slot(struct bt_conn *conn,  const struct bt_gatt_attr *attr, 
                            const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    uint8_t value;

    if (offset + len > sizeof(value)) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(&value, buf, len);

    if (value + 1 > NUMBER_OF_SLOTS) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
    }

    eds_active_slot = value;

    return len;
}

static ssize_t read_tx_power(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                                void *buf, uint16_t len, uint16_t offset)
{
    struct eds_slot *slot = &eds_slots[eds_active_slot];

    if (slot->state == EDS_LOCKED) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
    }

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &slot->tx_power, sizeof(slot->tx_power));
}

static ssize_t write_tx_power(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                                const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    struct eds_slot *slot = &eds_slots[eds_active_slot];

    if (slot->state == EDS_LOCKED) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
    }

    if (offset + len > sizeof(slot->tx_power)) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(&slot->tx_power, buf, len);

    return len;
}

static ssize_t read_adv_tx_power(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                                    void *buf, uint16_t len, uint16_t offset)
{
    struct eds_slot *slot = &eds_slots[eds_active_slot];

    if (slot->state == EDS_LOCKED) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
    }

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &slot->tx_power, sizeof(slot->tx_power));
}

static ssize_t write_adv_tx_power(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                                    const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    struct eds_slot *slot = &eds_slots[eds_active_slot];

    if (slot->state == EDS_LOCKED) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
    }

    if (offset + len > sizeof(slot->adv_tx_power)) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(&slot->adv_tx_power, buf, len);

    return len;
}

static ssize_t read_interval(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                                void *buf, uint16_t len, uint16_t offset)
{
    struct eds_slot *slot = &eds_slots[eds_active_slot];

    if (slot->state == EDS_LOCKED) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
    }

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &slot->interval, sizeof(slot->interval));
}

static ssize_t read_lock(struct bt_conn *conn, const struct bt_gatt_attr *attr,  
                            void *buf, uint16_t len, uint16_t offset)
{
    struct eds_slot *slot = &eds_slots[eds_active_slot];

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &slot->state, sizeof(slot->state));
}

static ssize_t write_lock(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                            const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    struct eds_slot *slot = &eds_slots[eds_active_slot];
    uint8_t value;

    if (slot->state == EDS_LOCKED) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
    }

    if (offset) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if (len != 1U) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    memcpy(&value, buf, sizeof(value));

    if (value > EDS_UNLOCKED_NO_RELOCKING) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
    }

    slot->state = value;

    return len;
}

static ssize_t read_unlock(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                                void *buf, uint16_t len, uint16_t offset)
{
    struct eds_slot *slot = &eds_slots[eds_active_slot];

    if (slot->state != EDS_LOCKED) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
    }

    if (bt_rand(slot->challenge, sizeof(slot->challenge))) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }

    return bt_gatt_attr_read(conn, attr, buf, len, offset, slot->challenge, sizeof(slot->challenge));
}

static ssize_t write_unlock(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                                const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    struct eds_slot *slot = &eds_slots[eds_active_slot];

    if (slot->state != EDS_LOCKED) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
    }

    return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
}

static ssize_t read_ecdh(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                            void *buf, uint16_t len, uint16_t offset)
{
    uint8_t *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(eds_ecdh));
}

static ssize_t read_eid(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                            void *buf, uint16_t len, uint16_t offset)
{
    uint8_t *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(eds_eid));
}

static ssize_t read_adv_data(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                                void *buf, uint16_t len, uint16_t offset)
{
    struct eds_slot *slot = &eds_slots[eds_active_slot];

    if (slot->state == EDS_LOCKED) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
    }

    if (slot->type == EDS_TYPE_NONE) 
    {
        return 0;
    }

    return bt_gatt_attr_read(conn, attr, buf, len, offset, 
                                slot->ad[2].data + EDS_URL_READ_OFFSET, 
                                slot->ad[2].data_len - EDS_URL_READ_OFFSET);
}

int eds_slot_restart(struct eds_slot *slot, uint8_t type)
{
    int  err;
    char addr_s[BT_ADDR_LE_STR_LEN];

    bt_addr_le_t addr = {0};

    bt_le_adv_stop();

    if (type == EDS_TYPE_NONE) 
    {
        struct bt_le_oob oob;
        if (bt_le_oob_get_local(BT_ID_DEFAULT, &oob) == 0) 
        {
            addr = oob.addr;
        }

        err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    } 
    else 
    {
        size_t count = 1;

        bt_id_get(&addr, &count);

        err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, slot->ad, ARRAY_SIZE(slot->ad), NULL, 0);
    }

    if (err) 
    {
        LOG_ERR("Advertising failed to start (err %d)\n", err);
        return err;
    }

    bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

    LOG_INF("Advertising as %s\n", addr_s);

    slot->type = type;

    return 0;
}

static ssize_t write_adv_data(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                                const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    struct eds_slot *slot = &eds_slots[eds_active_slot];

    uint8_t type;

    if (slot->state == EDS_LOCKED) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
    }

    if (offset) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    /* Writing an empty array, clears the slot and stops Tx. */

    if (!len) 
    {
        eds_slot_restart(slot, EDS_TYPE_NONE);
        return len;
    }

    /* Write length: 17 bytes (UID), 19 bytes (URL), 1 byte (TLM), 34 or 18 bytes (EID) */

    if (len > 19) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    memcpy(&type, buf, sizeof(type));

    switch (type) 
    {
    case EDS_TYPE_URL:

        /* written data is just the frame type and any ID-related
        * information, and doesn't include the Tx power since that is
        * controlled by characteristics 4 (Radio Tx Power) and
        * 5 (Advertised Tx Power).
        */

        slot->ad[2].data_len = MIN(slot->ad[2].data_len, len + EDS_URL_WRITE_OFFSET);

        memcpy((uint8_t *) slot->ad[2].data + EDS_URL_WRITE_OFFSET, buf, slot->ad[2].data_len - EDS_URL_WRITE_OFFSET);

        /* Restart slot */

        if (eds_slot_restart(slot, type) < 0) 
        {
            return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
        }

        return len;

    case EDS_TYPE_UID:
    case EDS_TYPE_TLM:
    case EDS_TYPE_EID:
    default:
        return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
    }
}

static ssize_t write_reset(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
}

static ssize_t read_connectable(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                                        void *buf, uint16_t len, uint16_t offset)
{
    uint8_t connectable = 0x01;

    /* Returning a non-zero value indicates that the beacon is capable of becoming non-connectable */

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &connectable, sizeof(connectable));
}

static ssize_t write_connectable(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                    const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    struct eds_slot *slot = &eds_slots[eds_active_slot];

    if (slot->state == EDS_LOCKED) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
    }

    if (offset) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if (len > sizeof(slot->connectable)) 
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    /* If any non-zero value is written, the beacon shall remain in its
    * connectable state until any other value is written. */
   
    memcpy(&slot->connectable, buf, len);

    return len;
}

/* Eddystone Configuration Service Declaration */
BT_GATT_SERVICE_DEFINE(eds_svc,
    BT_GATT_PRIMARY_SERVICE(&eds_uuid),
    /* Capabilities: Readable only when unlocked. Never writable. */
    BT_GATT_CHARACTERISTIC(&eds_caps_uuid.uuid, BT_GATT_CHRC_READ,
                BT_GATT_PERM_READ, read_caps, NULL, &eds_caps),
    /* Active slot: Must be unlocked for both read and write. */
    BT_GATT_CHARACTERISTIC(&eds_slot_uuid.uuid,
                BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                read_slot, write_slot, NULL),
    /* Advertising Interval: Must be unlocked for both read and write. */
    BT_GATT_CHARACTERISTIC(&eds_intv_uuid.uuid, BT_GATT_CHRC_READ,
                BT_GATT_PERM_READ, read_interval, NULL, NULL),
    /* Radio TX Power: Must be unlocked for both read and write. */
    BT_GATT_CHARACTERISTIC(&eds_tx_uuid.uuid,
                BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                read_tx_power, write_tx_power, NULL),
    /* Advertised TX Power: Must be unlocked for both read and write. */
    BT_GATT_CHARACTERISTIC(&eds_adv_tx_uuid.uuid,
                BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                read_adv_tx_power, write_adv_tx_power, NULL),
    /* Lock State:
    * Readable in locked or unlocked state.
    * Writeable only in unlocked state.
    */
    BT_GATT_CHARACTERISTIC(&eds_lock_uuid.uuid,
                BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                read_lock, write_lock, NULL),
    /* Unlock:
    * Readable only in locked state.
    * Writeable only in locked state.
    */
    BT_GATT_CHARACTERISTIC(&eds_unlock_uuid.uuid,
                BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                read_unlock, write_unlock, NULL),
    /* Public ECDH Key: Readable only in unlocked state. Never writable. */
    BT_GATT_CHARACTERISTIC(&eds_ecdh_uuid.uuid, BT_GATT_CHRC_READ,
                BT_GATT_PERM_READ, read_ecdh, NULL, &eds_ecdh),
    /* EID Identity Key:Readable only in unlocked state. Never writable. */
    BT_GATT_CHARACTERISTIC(&eds_eid_uuid.uuid, BT_GATT_CHRC_READ,
                BT_GATT_PERM_READ, read_eid, NULL, eds_eid),
    /* ADV Slot Data: Must be unlocked for both read and write. */
    BT_GATT_CHARACTERISTIC(&eds_data_uuid.uuid,
                BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                read_adv_data, write_adv_data, NULL),
    /* ADV Factory Reset: Must be unlocked for write. */
    BT_GATT_CHARACTERISTIC(&eds_reset_uuid.uuid,  BT_GATT_CHRC_WRITE,
                BT_GATT_PERM_WRITE, NULL, write_reset, NULL),
    /* ADV Remain Connectable: Must be unlocked for write. */
    BT_GATT_CHARACTERISTIC(&eds_connectable_uuid.uuid,
                BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                read_connectable, write_connectable, NULL),
);

int eds_adv_start(void)
{
    return bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
}