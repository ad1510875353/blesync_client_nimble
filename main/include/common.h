#ifndef __COMMON_H__
#define __COMMON_H__

/* STD APIs */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* ESP APIs */
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "console/console.h"
#include <sys/time.h>

/* FreeRTOS APIs */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

/* NimBLE stack APIs */
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "esp_central.h"

#define BT_BD_ADDR_STR "%02x:%02x:%02x:%02x:%02x:%02x"
#define BT_BD_ADDR_HEX(addr) addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]

#define TAG "TimeSync_Client"
#define DEVICE_NAME "ESP32_Client"
#define DATA_BUFF_SIZE 128

#define SVC_UUID BLE_UUID128_DECLARE(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x00, 0x6E)
#define CHR_UUID BLE_UUID128_DECLARE(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x00, 0x6E)

// #define FOUR_NODE

struct PeerInfo
{
    const char *name;
    const uint8_t(*addr);
    const ble_uuid_t *svc_uuid;
    const ble_uuid_t *chr_uuid;
    bool connected;
    uint16_t conn_handle;
    uint16_t val_handle;
    uint64_t connect_time;
};

struct ConnState{
    bool scanning;
    bool connecting;
    int connected_num;
    int curr_id;
};

extern struct ConnState connstate;
extern struct timeval timeval_s;

#endif


// 使用cdc虚拟串口的方法
// 1.
// esp_log_buffer_hex(tag, (char *)data_to_send, data_size);
// 2.
// #include "hal/usb_serial_jtag_ll.h"
// usb_serial_jtag_ll_write_txfifo(data_to_send, data_size);
// usb_serial_jtag_ll_txfifo_flush();