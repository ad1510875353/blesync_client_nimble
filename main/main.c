#include "common.h"
#include "blecent.h"
#include "esp_random.h"
#include "device.h"
#include "GPIOcontrol.h"
#include "esp_rom_sys.h"

static SemaphoreHandle_t printSemaphore;

struct timeval timeval_s;
struct ConnState connstate;

volatile int sync_num = 0;

int onConnect(struct ble_gap_event *event, void *arg, uint16_t conn_handle)
{
    gettimeofday(&timeval_s, NULL);
    struct ble_gap_conn_desc conn_desc;
    int i = connstate.curr_id;
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
    xSemaphoreGive(printSemaphore);
    return 0;
}

// 将node发送过来的时间戳打印出来
int onNotifyRx(struct ble_gap_event *event, void *arg, uint16_t conn_handle)
{
    int rc = 0;
    uint8_t data_buf[10];
    for (uint8_t i = 0; i < PROFILE_NUM; i++)
    {
        if (conn_handle == peerinfos[i].conn_handle)
        {
            if (event->notify_rx.attr_handle == peerinfos[i].val_handle)
            {
                if (OS_MBUF_PKTLEN(event->notify_rx.om) == 9)
                {
                    rc = ble_hs_mbuf_to_flat(event->notify_rx.om, data_buf, event->notify_rx.om->om_len, NULL);
                    uint8_t data_type = data_buf[0];
                    int64_t node_ts = *(int64_t *)(data_buf + 1); // 刚好是小端存储
                    printf("%s time : %lld\n", peerinfos[i].name, node_ts);
                    xSemaphoreGive(printSemaphore);
                }
            }
            break;
        }
    }
    return 0;
}

// 延迟一定时间后将当前时间戳发送给对方用于同步，然后生成下降沿输出时间
void print_time_task(void *pvParameters)
{
    static int cnt = 0;
    for (;;)
    {
        if (xSemaphoreTake(printSemaphore, portMAX_DELAY))
        {
            // 延迟随机的毫秒和微秒数目,这里要有微秒级别的延迟，不然保证不了是任意时间放入协议栈队列
            vTaskDelay(500 + esp_random() % 100);
            esp_rom_delay_us(esp_random() % 1000);
            // 将当前时间发出去用于同步
            gettimeofday(&timeval_s, NULL);
            int64_t local_ts = (int64_t)timeval_s.tv_sec * 1000000L + (uint64_t)timeval_s.tv_usec;
            ble_gattc_write_flat(peerinfos[0].conn_handle, peerinfos[0].val_handle, &local_ts, sizeof(local_ts),NULL,NULL);
            // 延迟等待对方收到
            vTaskDelay(120);
            cnt++;
            printf("\n%d sync print begin***********************\n", cnt);
            set_led_state(cnt % 2);
            generate_falling_edge();
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
