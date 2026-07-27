
#ifndef BT_LBS_H_
#define BT_LBS_H_

#include <zephyr/types.h>
#include <zephyr/bluetooth/uuid.h>

#define BT_UUID_TEMP BT_UUID_DECLARE_16(0x181A)
#define BT_UUID_TEMP_MYSENSOR_VAL BT_UUID_128_ENCODE(0x00001526, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

#define BT_UUID_TEMP_MYSENSOR BT_UUID_DECLARE_128(BT_UUID_TEMP_MYSENSOR_VAL)

struct nature{
	float temperature;
	float humidity;
	int value;
};

int my_lbs_send_sensor_notify(struct nature *sensor_value);

#endif 
