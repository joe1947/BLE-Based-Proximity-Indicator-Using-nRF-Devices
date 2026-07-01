#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>
#define CONNECTION_STATUS_LED   DK_LED2
#define DISTANCE DK_LED1


static void start_scan(void);

static struct bt_conn *default_conn;

static const bt_addr_le_t target_addr = {
    .type = BT_ADDR_LE_RANDOM,
    .a.val = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (default_conn) {
		return;
	}

	if (bt_addr_le_cmp(addr, &target_addr) != 0) {
		return;
	}
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	if (rssi < -50) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &default_conn);
	if (err) {
		printk("Create conn to %s failed (%d)\n", addr_str, err);
		start_scan();
	}
}

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s %u %s\n", addr, err, bt_hci_err_to_str(err));

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	if (conn != default_conn) {
		return;
	}

	printk("Connected: %s\n", addr);

	dk_set_led(CONNECTION_STATUS_LED, 1);
	
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));
	dk_set_led(CONNECTION_STATUS_LED, 0);
	dk_set_led(DISTANCE, 0);
	bt_conn_unref(default_conn);
	default_conn = NULL;

	start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void)
{
	int err;
	printk("Entered Main\n");

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return -1;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	start_scan();
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
						dk_set_led(DISTANCE, 1);
					else
					dk_set_led(DISTANCE, 0);					

                    net_buf_unref(rsp);
                }
            }
        }

        k_sleep(K_SECONDS(1));
    }

}
