#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <dk_buttons_and_leds.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>
#include "my_lbs.h"

static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM
	(BT_LE_ADV_OPT_CONNECTABLE, 800, 801, NULL);

LOG_MODULE_REGISTER(Lesson4_Exercise2, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define LED DK_LED1
#define NOTIFY_INTERVAL 1000

static bool notify_mysensor_enabled;
int8_t rssi;
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x1A, 0x18),
};

struct nature sensor = {
	.temperature = 24.5,
	.humidity = 63,
	.value = 0,
};

static void mytemp_ccc_mysensor_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify_mysensor_enabled = (value == BT_GATT_CCC_NOTIFY);
}

BT_GATT_SERVICE_DEFINE(
	my_temp_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_TEMP),

	BT_GATT_CHARACTERISTIC(BT_UUID_TEMP_MYSENSOR, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),

	BT_GATT_CCC(mytemp_ccc_mysensor_cfg_changed, BT_GATT_PERM_WRITE),

);

int my_temp_send_sensor_notify(struct nature *sensor_value)
{
	if (!notify_mysensor_enabled) {
		return -EACCES;
	}

	return bt_gatt_notify(NULL, &my_temp_svc.attrs[2], sensor_value, sizeof(struct nature));
}

static void simulate_data(struct nature *sensor)
{
	(sensor -> value)++;
	if ((sensor -> value) == 200) {
		(sensor -> value) = 0;
	}
}

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");

}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

}

struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
};


int main(void)
{
	int err;
    err = dk_leds_init();
    if (err) {
        LOG_ERR("LEDs init failed (err %d)", err);
        return -1;
    }

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}
	bt_conn_cb_register(&connection_callbacks);

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
	printk("Advertising successfully started\n");

	while (1) {
		simulate_data(&sensor);

		if(my_temp_send_sensor_notify(&sensor))
		{
			printk("Notify failed\n\r");
		}
		
		k_sleep(K_MSEC(NOTIFY_INTERVAL));
	}
}