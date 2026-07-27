/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include<zephyr/bluetooth/bluetooth.h>
#include<zephyr/bluetooth/gap.h>
/* STEP 3 - Include the header file of the Bluetooth LE stack */

#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(Lesson2_Exercise1, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000

/* STEP 4.1.1 - Declare the advertising packet */
const static struct bt_data ad[] = { BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN) 
};

/* STEP 4.2.2 - Declare the URL data to include in the scan response */
static unsigned char url_data[] = {0x17, '/','/','a','c','a','d','e','m','y','.','n','o','r','d','i','c','s','e','m','i','.','c','o','m'};

/* STEP 4.2.1 - Declare the scan response packet */
const static struct bt_data sd[] = { BT_DATA(BT_DATA_URI, url_data, sizeof(url_data))};

int main(void)
{
	int blink_status = 0;
	int err;

	LOG_INF("Starting Lesson 2 - Exercise 1 \n");

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return -1;
	}
	/* STEP 5 - Enable the Bluetooth LE stack */
	err = bt_enable(NULL);
	if(err)
	{
		LOG_ERR("Bluetooth init failed 	(err %d)\n", err);
		return -1;
	}
	LOG_INF("Blurtooth initialised\n");

	/* STEP 6 - Start advertising */

	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
	LOG_ERR("Advertising failed to start (err %d)\n", err);
	return -1;
}

	LOG_INF("Advertising successfully started\n");

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
