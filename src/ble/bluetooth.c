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

#include "bluetooth.h"
#include "eddystone.h"
#include "uuid.h"

#include "../drivers/board.h"

LOG_MODULE_REGISTER(bluetooth);

struct k_work_delayable idle_work;
static int connectionStatus;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) 
    {
		LOG_ERR("Connection failed. Error: 0x%02x", err);
	} 
    else 
    {
        LOG_INF("Connected");

        connectionStatus = BLE_STATUS_CONNECTED;
		k_work_cancel_delayable(&idle_work);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	LOG_INF("Disconnected. Reason: 0x%02x", reason);

    connectionStatus = BLE_STATUS_DISCONNECTED;

	if (!slot->connectable) 
    {
		k_work_reschedule(&idle_work, K_NO_WAIT);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = 
{
	.connected    = connected,
	.disconnected = disconnected,
};

static void idle_timeout(struct k_work *work)
{
	if (eds_slots[eds_active_slot].type == EDS_TYPE_NONE) 
    {
		LOG_INF("Switching to Beacon mode %u", eds_active_slot);
		eds_slot_restart(&eds_slots[eds_active_slot], EDS_TYPE_URL);
	}
}

static void set_device_name(void)
{
    // SVBeacon_0000000

    int err = 0;

    char name[16];
    uint32_t serialNumber;

    serialNumber = GetSerialNumber();
    sprintf(name, "SVBeacon_%x", serialNumber);

    LOG_INF("Bluetooth Device Name: %s", name);

    err = bt_set_name(name);
    if (err)
    {
        LOG_ERR("Failed to set device name. Error: %d", err);
    }
}

int BluetoothInit(void)
{
    int err = 0;

    connectionStatus = BLE_STATUS_DISCONNECTED;

    k_work_init_delayable(&idle_work, idle_timeout);

    err = bt_enable(NULL);

    set_device_name();

    if (err) 
    {
        LOG_ERR("Bluetooth initialization failed. Error: %d", err);
    }  

    return err;
}

int BluetoothAdvStart(void)
{    
    int err = 0;

    char addr_s[BT_ADDR_LE_STR_LEN];
    
    struct bt_le_oob oob;

    err = eds_adv_start();

    if (err) 
    {
        LOG_ERR("Advertising failed to start. Error: %d", err);
        return err;
    }

    /* Restore connectable if slot */

    bt_le_oob_get_local(BT_ID_DEFAULT, &oob);
    bt_addr_le_to_str(&oob.addr, addr_s, sizeof(addr_s));

    LOG_INF("Initial advertising as %s", addr_s);

    k_work_schedule(&idle_work, EDS_IDLE_TIMEOUT);

    LOG_INF("Configuration mode: waiting connections...");

    return err;
}

int BluetoothGetStatus(void)
{
    return connectionStatus;
}
