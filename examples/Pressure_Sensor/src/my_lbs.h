
#ifndef BT_LBS_H_
#define BT_LBS_H_

#include <zephyr/types.h>
#include <zephyr/bluetooth/uuid.h>

#define BT_UUID_MY_SERVICE BT_UUID_DECLARE_128(BT_UUID_128_ENCODE( 0x12345678, 0x1234, 0x5678, 0x1234, 0x56789ABCDEF0))
#define BT_UUID_MY_PRESSURE BT_UUID_DECLARE_128(BT_UUID_128_ENCODE( 0x12345679, 0x1234, 0x5678, 0x1234, 0x56789ABCDEF0))

int my_pressure_send_sensor_notify(int *sensor_value);

#endif 
