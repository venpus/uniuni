#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "../core.h"
#include "../version.h"
#include "../fwconfig.h"
#include "bluetooth.h"
#include "uuid.h"

#include "../drivers/pwr/drv_pwr.h"
#include "../drivers/buttons/drv_button.h"

LOG_MODULE_REGISTER(bluetooth);

/* Internal module information */

static int connectionStatus;

/* Service Declaration */

static ssize_t onSentStatus(struct bt_conn *conn,  const struct bt_gatt_attr *attr,
                                void *buf, uint16_t len, uint16_t offset);
static ssize_t onSentConfig(struct bt_conn *conn,  const struct bt_gatt_attr *attr,
                                void *buf, uint16_t len, uint16_t offset);
static ssize_t onReceiveConfig(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len, uint16_t offset, uint8_t flags);                                                                

BT_GATT_SERVICE_DEFINE(custom_service,

    BT_GATT_PRIMARY_SERVICE(BT_UUID_SERVICE),

    // device status (battery, fw/hw version and etc.)

    BT_GATT_CHARACTERISTIC(BT_UUID_STATUS,
                            BT_GATT_CHRC_READ,
                            BT_GATT_PERM_READ, 
                            onSentStatus, NULL, NULL),

    // device configuration (ping interval in sec)

    BT_GATT_CHARACTERISTIC(BT_UUID_CONFIG,
                            BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
                            BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, 
                            onSentConfig, onReceiveConfig, NULL),
);

static struct bt_le_adv_param * adv_param = BT_LE_ADV_PARAM(
        BT_LE_ADV_OPT_CONNECTABLE,
        800,                        // min. advertising interval 500 ms (800 * 0.625)
        801,                        // max. advertising interval 500.625 ms
        NULL
);

static struct adv_mfg_data {
    uint16_t company_code;  // company ID
    uint16_t battery_level; // battery level in mV
    uint16_t button_state;  // current button state (0 - release, 1 - pressed)
} adv_mfg_data = 
{
    .company_code  = BT_COMPANY_ID_CODE,
    .battery_level = 0,
    .button_state  = 0
};

static const struct bt_data ad[] = 
{
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, (unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),
};

static const struct bt_data sd[] = 
{
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_SERVICE_VAL),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) 
    {
        LOG_ERR("Connection failed. Error: %u", err);
		return;
	}

    LOG_INF("Connected");
    connectionStatus = BLE_STATUS_CONNECTED;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected. Reason: %u", reason);
    connectionStatus = BLE_STATUS_DISCONNECTED;
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err)
    {
        LOG_ERR("Security failed: %s level %u. Error: err %d", addr, level, err);
        return;
    }

    LOG_INF("Security changed: %s level %u", addr, level);
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Passkey for %s: %06u", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Pairing cancelled: %s", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_ERR("Pairing failed conn: %s, reason %d\n", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = 
{
	.passkey_display = auth_passkey_display,
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks =
{
	.pairing_complete = pairing_complete,
	.pairing_failed   = pairing_failed
};

BT_CONN_CB_DEFINE(conn_callbacks) = 
{
	.connected        = connected,
	.disconnected     = disconnected,
	.security_changed = security_changed,
};


static void InitBluetoothContext(void)
{
    connectionStatus = BLE_STATUS_DISCONNECTED;
}

static void UpdateAdvManufactureData(void)
{
    int err = 0;

    uint16_t raw_mv = 0;

    err = DriverPwrBatteryLevel(&raw_mv);
    if (err < 0)
    {
        LOG_ERR("Failed to get battery level. Error: %d", err);
    }

    adv_mfg_data.battery_level = raw_mv;
    adv_mfg_data.button_state  = DriverButtonGetState();
}

// Characterisitc callbacks for reading/writing

static ssize_t onSentStatus(struct bt_conn *conn,  const struct bt_gatt_attr *attr,
                                void *buf, uint16_t len, uint16_t offset)
{
    char txBuffer[64];

    UpdateAdvManufactureData();

    int ret = sprintf( txBuffer, "hw: %s; fw: %s; bat: %d mV",
                                    hwVersion,
                                    fwVersion,
                                    adv_mfg_data.battery_level );

    strcpy(buf, txBuffer);
    
    return (ret + 1); 
}

static ssize_t onSentConfig(struct bt_conn *conn,  const struct bt_gatt_attr *attr,
                                void *buf, uint16_t len, uint16_t offset)
{
    char txBuffer[64];

    int ret = sprintf( txBuffer, "wakeup interval: %d sec", TIME_SEC_WAKEUP_PERIOD);

    strcpy(buf, txBuffer);
    
    return (ret + 1); 
}

static ssize_t onReceiveConfig(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    return len;
}

int BluetoothInit(void)
{
    int err = 0;

    InitBluetoothContext();

    err = bt_conn_auth_cb_register(&conn_auth_callbacks);
    if (err) 
    {
        LOG_ERR("Failed to register authorization callbacks. Error: %d", err);
        return err;
    }

    err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
    if (err) 
    {
        LOG_ERR("Failed to register authorization info callbacks. Error: %d", err);
        return err;
    }

    err = bt_enable(NULL);

    if (err) 
    {
        LOG_ERR("Bluetooth initialization failed. Error: %d", err);
    }  

    return err;
}

int BluetoothAdvStart(int type)
{
    int err = 0;

    UpdateAdvManufactureData();

    if (type == BLE_ADV_TYPE_PING)
    {
        adv_mfg_data.button_state = BUTTON_STATE_RELEASED;
    }
    else if (type == BLE_ADV_TYPE_EMERGENCY)
    {
        // if we start advertising due to emergency case we need to ensure that
        // the button_state field equals to BUTTON_STATE_PRESSED even if the button
        // release at some point

        adv_mfg_data.button_state = BUTTON_STATE_PRESSED;
    }
    else
    {
        LOG_ERR("Unexpected type for bluetooth advertising");
    }

    err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

    if (err) 
    {
        LOG_ERR("Failed to start bluetooth advertising. Error: %d", err);
    } 

    return err;
}

int BluetoothAdvUpdate(void)
{
    int err = 0;

    // it doesn't make sense to update advertising data if we have a connection

    if (BluetoothGetStatus() == BLE_STATUS_CONNECTED)
        return 0;

    UpdateAdvManufactureData();

    LOG_DBG("Update advertising data:");
    LOG_DBG("   company ID:         %d", adv_mfg_data.company_code);
    LOG_DBG("   battery level (mV): %d", adv_mfg_data.battery_level);
    LOG_DBG("   button state:       %d", adv_mfg_data.button_state);

    err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

    if (err) 
    {
        LOG_ERR("Failed to update bluetooth advertising data. Error: %d", err);
    } 

    return err;  
}

int BluetoothAdvStop(void)
{
    int err = 0;

    err = bt_le_adv_stop();

    if (err) 
    {
        LOG_ERR("Failed to stop bluetooth advertising. Error: %d", err);
    } 

    return err;
}

int BluetoothGetStatus(void)
{
    return connectionStatus;
}
