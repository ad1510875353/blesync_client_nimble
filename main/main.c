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
    for (uint8_t i = 0; i < PROFILE_NUM; i++){
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
    // 将连接时间发出去用于同步
    int i = 0;
    for (i = 0; i < PROFILE_NUM; i++)
    {
        if (peerinfos[i].conn_handle == conn_handle)
            break;
    }
    local_ts = peerinfos[i].connect_time;
    ESP_LOGE(TAG, "connect time = %lld, send to %s to sync ", local_ts, peerinfos[i].name);
    ble_gattc_write_flat(conn_handle, peerinfos[i].val_handle, &local_ts, sizeof(local_ts), NULL, NULL);
    // sync_num = sync_num + 1;
    // if (sync_num == PROFILE_NUM)
    // {
    //     xSemaphoreGive(printSemaphore);
    // }
    // ESP_LOGE(TAG, "sync num = %d ", sync_num);

    return 0;
}

// 将node发送过来的时间戳打印出来
int onNotifyRx(struct ble_gap_event *event, void *arg, uint16_t conn_handle)
{
    // static uint8_t data_idx[PROFILE_NUM] = {0};
    struct ble_gap_conn_desc conn_desc;
    for (uint8_t i = 0; i < PROFILE_NUM; i++)
        if (conn_handle == peerinfos[i].conn_handle)
        {
            if (event->notify_rx.attr_handle == peerinfos[i].val_handle)
            {
                if (OS_MBUF_PKTLEN(event->notify_rx.om) == 0)
                {
#ifdef NOTIFY_EVENT
                    gettimeofday(&timeval_s, NULL);
                    ble_gap_conn_find(conn_handle, &conn_desc);
                    local_ts = (uint64_t)timeval_s.tv_sec * 1000000L + (uint64_t)timeval_s.tv_usec + conn_desc.conn_itvl * 1250;
                    // 将连接时间发出去
                    ESP_LOGE(TAG, "Notify time = %lld, send to %s to sync ", local_ts, peerinfos[i].name);
                    ble_gattc_write_flat(conn_handle, peerinfos[i].val_handle, &local_ts, sizeof(local_ts), NULL, NULL);
                    // 全部同步完了再打印
                    sync_num = sync_num + 1;
                    if (sync_num == PROFILE_NUM)
                    {
                        xSemaphoreGive(printSemaphore);
                    }
                    ESP_LOGE(TAG, "sync num = %d ", sync_num);
#endif
                }
                else if (OS_MBUF_PKTLEN(event->notify_rx.om) == 8)
                {
                    node_ts = *(int64_t *)event->notify_rx.om->om_data; // 刚好是小端存储
                    printf("%s time : %lld\n", peerinfos[i].name, node_ts);
                }
            }
            break;
        }
    return 0;
}

// 生成下降沿打印输出
void print_time_task(void *pvParameters)
{
    for (;;)
    {
        if (xSemaphoreTake(printSemaphore, portMAX_DELAY))
        {
            // vTaskDelay(3000);
            // cnt++;
            // for (int i = 0; i < esp_random() % 1000; i++)
            //     ;
            // printf("\n%d sync print begin***********************\n", cnt);
            // set_led_state(cnt % 2);
            // sync_num = 0;
            // generate_falling_edge();
            vTaskDelay(10000);
            ble_gap_terminate(peerinfos[0].conn_handle, BLE_ERR_REM_USER_CONN_TERM);
            // printf(" sync num = %d \n", sync_num );
        }
    }
}

// 每秒发一个长消息，以求制造混论
void send_noise_task(void *pvParameters)
{
    static uint8_t buf[100];
    for (;;)
    {
        for (uint8_t i = 0; i < PROFILE_NUM; i++)
        {
            if (peerinfos[i].val_handle != 0)
            {
                // ble_gattc_write_flat(peerinfos[i].conn_handle, peerinfos[i].chr->chr.val_handle, buf, 20, NULL, NULL);
                ble_gattc_write_no_rsp_flat(peerinfos[i].conn_handle, peerinfos[i].val_handle, buf, 100);
                vTaskDelay(3000);
            }
        }
        // vTaskDelay(5);
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
    // xTaskCreate(send_noise_task, "send_noise_task", 2048, NULL, 1, NULL);
}
