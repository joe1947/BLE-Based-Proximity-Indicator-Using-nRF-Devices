#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/hci.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/sys/byteorder.h>

#define CONNECTION_STATUS_LED DK_LED2
#define DISTANCE_LED DK_LED1
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
static struct bt_conn *default_conn;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),

};

void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection error %d", err);
		return;
	}
	printk("Connected");
	default_conn = bt_conn_ref(conn);

	dk_set_led(CONNECTION_STATUS_LED, 1);
}

void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected. Reason %d", reason);
	bt_conn_unref(default_conn);

	dk_set_led(CONNECTION_STATUS_LED, 0);
	dk_set_led(DISTANCE_LED, 0);
	default_conn = NULL;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void)
{
	int err;
	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return -1;
	}

	bt_addr_le_t addr;
	err = bt_addr_le_from_str("FF:EE:DD:CC:BB:AA", "random", &addr);
	if (err) {
		printk("Invalid BT address (err %d)\n", err);
	}

	err = bt_id_create(&addr, NULL);
	if (err < 0) {
		printk("Creating new ID failed (err %d)\n", err);
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return -1;
	}

	printk("Bluetooth initialized\n");
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
	printk("Advertising successfully started\n");

	while (1) {

        if (default_conn) {

            struct net_buf *buf;
            struct net_buf *rsp;
            struct bt_hci_cp_read_rssi *cp;
            struct bt_hci_rp_read_rssi *rp;
            uint16_t handle;

            buf = bt_hci_cmd_create(BT_HCI_OP_READ_RSSI, sizeof(struct bt_hci_cp_read_rssi));

            cp = net_buf_add(buf, sizeof(struct bt_hci_cp_read_rssi));

            err = bt_hci_get_conn_handle(default_conn, &handle);
            if (err) 
			{
                printk("Failed to get handle (%d)\n", err);
            }
			else {

                cp->handle = sys_cpu_to_le16(handle);

                err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_RSSI, buf, &rsp);

                if (!err) {

                    rp = (struct bt_hci_rp_read_rssi *)rsp->data;

                    printk("RSSI = %d dBm\n", rp->rssi);

					if((rp->rssi) < -50)
						dk_set_led(DISTANCE_LED, 1);
					else
					dk_set_led(DISTANCE_LED, 0);

                    net_buf_unref(rsp);
                }
            }
        }

        k_sleep(K_SECONDS(1));
    }
}
