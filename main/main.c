#include "common.h"
#include "blecent.h"
#include "esp_random.h"
#include "device.h"
#include "GPIOcontrol.h"

int64_t local_ts;
static int64_t node_ts;
struct timeval timeval_s;
struct ConnState connstate;
static SemaphoreHandle_t printSemaphore;

int cnt;

volatile int sync_num = 0;

int onConnect(struct ble_gap_event *event, void *arg, uint16_t conn_handle)
{
    struct ble_gap_conn_desc conn_desc;
    int i = connstate.curr_id;
    gettimeofday(&timeval_s, NULL);
    ble_gap_conn_find(conn_handle, &conn_desc);
    ESP_LOGI(TAG, "%s connect successed, connect handle=%d, interval=%d",
             peerinfos[i].name, conn_handle, conn_desc.conn_itvl);
    peerinfos[i].conn_handle = conn_handle;
    peerinfos[i].connected = true;
    peerinfos[i].connect_time = (uint64_t)timeval_s.tv_sec * 1000000L + (uint64_t)timeval_s.tv_usec;
    connstate.connected_num = connstate.connected_num + 1;
    peer_add(conn_handle);
    ble_gattc_exchange_mtu(conn_handle, NULL, NULL);
    peer_disc_all(conn_handle, blecent_on_disc_complete, NULL);
    return 0;
}

int onDisconnect(struct ble_gap_event *event, void *arg, uint16_t conn_handle)
{
    sync_num = sync_num - 1;
    connstate.connected_num = connstate.connected_num - 1;
    connstate.connecting = false;
    for (uint8_t i = 0; i < PROFILE_NUM; i++)
    {
        if (conn_handle == peerinfos[i].conn_handle)
        {
            ESP_LOGE(TAG, "%s disconnect,connect handle = %d,reason = %d", peerinfos[i].name, conn_handle, event->disconnect.reason);
            peerinfos[i].conn_handle = -1;
            peerinfos[i].connected = false;
            peerinfos[i].val_handle = 0;
            break;
        }
    }
    peer_delete(conn_handle);
    if (!connstate.scanning)
        start_scan();
    return 0;
}

int onSubscribe(struct ble_gap_event *event, void *arg, uint16_t conn_handle)
{
    ESP_LOGI(TAG, "自定义回调，订阅成功");
    return 0;
}

int onNotifyRx(struct ble_gap_event *event, void *arg, uint16_t conn_handle)
{
    for (uint8_t i = 0; i < PROFILE_NUM; i++)
        if (conn_handle == peerinfos[i].conn_handle)
        {
            if (event->notify_rx.attr_handle == peerinfos[i].val_handle)
            {
                ble_gattc_write_flat(conn_handle, peerinfos[i].val_handle, &local_ts, sizeof(local_ts), NULL, NULL);
                set_led_state(1);
                ESP_LOGI(TAG, "收到通知后立刻进行写操作");
                xSemaphoreGive(printSemaphore);
            }
            break;
        }
    return 0;
}

// 5秒后关闭led,等待下一次通知的到来
void print_time_task(void *pvParameters)
{
    for (;;)
    {
        if (xSemaphoreTake(printSemaphore, portMAX_DELAY))
        {
            vTaskDelay(5000);
            set_led_state(0);
        }
    }
}

void app_main(void)
{
    struct GapCallbacks_t gapcbs = {
        .OnConnect = onConnect,
        .OnDisconnect = onDisconnect,
        .OnNotifyRx = onNotifyRx,
        .OnSubscribe = onSubscribe};
    set_gap_callbacks(gapcbs);
    printSemaphore = xSemaphoreCreateBinary();
    vTaskDelay(3000);
    ble_init();
    gpio_init();
    xTaskCreate(print_time_task, "print_time_task", 2048, NULL, 2, NULL);
}