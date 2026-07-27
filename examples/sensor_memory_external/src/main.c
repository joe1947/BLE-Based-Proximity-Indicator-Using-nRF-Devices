#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include <hal/nrf_saadc.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include "my_lbs.h"

#define FLASH_START_ADDR 0x00000000
#define FLASH_PAGE_SIZE  4096
#define MAX_SIZE 10
#define STORE 1
#define CONNECT 2
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
int system_mode = STORE;
static const struct adc_dt_spec adc_channel =
    ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
struct adcData{
    int adc_buf;
};
static struct adcData data;
static struct adcData value;
// static int pressure_value;
const struct device *fa =
    DEVICE_DT_GET(DT_NODELABEL(pressure_partition));
uint32_t flash_write_addr = 0;
uint32_t flash_read_addr = 0;

static struct bt_conn *default_conn;
static struct adc_sequence sequence = {
    .channels = BIT(adc_channel.channel_id),          
    .buffer = &(data.adc_buf),
    .buffer_size = sizeof(data.adc_buf),
    .resolution = 12,
};

static bool notify_enabled;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

BT_GATT_SERVICE_DEFINE(pressure_svc,

    BT_GATT_PRIMARY_SERVICE(BT_UUID_MY_SERVICE),

    BT_GATT_CHARACTERISTIC(BT_UUID_MY_PRESSURE, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),

    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_WRITE),
);

int my_pressure_send_sensor_notify(struct adcData *sensor_value)
{
    if (!notify_enabled) {
        return -EACCES;
    }

    return bt_gatt_notify(NULL, &pressure_svc.attrs[2], sensor_value, sizeof(struct adcData));
}

static int init_adc(void)
{
    int err;

    struct adc_channel_cfg my_adc_cfg = {
    .gain = ADC_GAIN_1_6,
    .reference = ADC_REF_INTERNAL,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id = adc_channel.channel_id,
    .input_positive = NRF_SAADC_INPUT_AIN0,
	};

    if (!adc_is_ready_dt(&adc_channel)) {
        printk("ADC device not ready\n");
        return -ENODEV;
    }

    err = adc_channel_setup(adc_channel.dev, &my_adc_cfg);
    if (err < 0) {
        printk("ADC setup failed (%d)\n", err);
        return err;
    }

    printk("ADC initialized\n");

    return 0;
}

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}
	printk("Connected\n");
    default_conn = bt_conn_ref(conn);
    system_mode = CONNECT;
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
    bt_conn_unref(default_conn);
    default_conn = NULL;
    system_mode = STORE;
    notify_enabled = false;
}

struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
};

int main(void)
{
    int err;
    int count = 0;
    int index = 0;

    err = init_adc();
    if (err) {
        printk("ADC init failed\n");
        return -1;
    }

    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed\n");
        return -1;
    }

    printk("Bluetooth initialized\n");

    if (!device_is_ready(fa)) {
    printk("Flash device not ready!\n");
    return -1;
    }
    err = flash_erase(fa, 0, FLASH_PAGE_SIZE);
    if (err) {
        printk("Flash erase failed! Error: %d\n", err);
        return;
    }
    printk("Flash erase successful!\n");

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);

    if (err) {
        printk("Advertising failed (%d)\n", err);
        return -1;
    }
    bt_conn_cb_register(&connection_callbacks);
    printk("Advertising started\n");

    while (1) {

        if(system_mode == STORE)
        {
            if (flash_write_addr >= MAX_SIZE * sizeof(data))
            {
                // err = flash_erase(flash_dev, FLASH_START_ADDR, FLASH_PAGE_SIZE);

                // if (err) {
                //     printk("Flash erase failed! Error: %d\n", err);
                //     return;
                // }

                flash_write_addr = 0;
                flash_read_addr = 0;
                count = 0;
            }            
            index = (flash_write_addr) / sizeof(data);
            err = adc_read(adc_channel.dev, &sequence);
            if (err == 0) {
                printk("index %d : ADC Value = %d\n",index, data.adc_buf);
            }
            err = flash_write(fa, flash_write_addr, &data, sizeof(data));
            if (err) {
                printk("Flash write failed! Error: %d\n", err);
                return;
            }
            flash_write_addr += sizeof(data);
            count++;

            k_sleep(K_SECONDS(1));
        }
        else if(system_mode == CONNECT)

        {
            while (!notify_enabled && system_mode == CONNECT)
            {
                k_sleep(K_MSEC(100));
            }
            if((count > 0) && notify_enabled)
            {

                flash_read(fa, flash_read_addr, &value, sizeof(value));
                if (err) {
                    printk("Flash read failed! Error: %d\n", err);
                    return;
                }
                printk("Value read: %d\n", value.adc_buf);
                flash_read_addr += sizeof(value);

                my_pressure_send_sensor_notify(&value);

                count--;
                k_sleep(K_SECONDS(1));
            }
            else
            {
                bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

                while (system_mode == CONNECT)
                {
                    k_sleep(K_MSEC(10));
                }
            }
        }
    }
}