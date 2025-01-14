#include "common.h"
#include "blecent.h"
#include "device.h"
#include "services/gap/ble_svc_gap.h"

void ble_store_config_init(void);
static uint8_t own_addr_type;
static struct GapCallbacks_t gapcbs;
static struct ble_gap_conn_params conn_params = {
    .scan_itvl = 16,
    .scan_window = 16,
    .itvl_min = 80,
    .itvl_max = 80,
    .latency = 0,
    .supervision_timeout = 500,
    .min_ce_len = 0,
    .max_ce_len = 0};

static int blecent_on_subscribe(uint16_t conn_handle, const struct ble_gatt_error *error,
                                struct ble_gatt_attr *attr, void *arg)
{
    ESP_LOGI(TAG, "Subscribe complete; status=%d conn_handle=%d attr_handle=%d\n",
             error->status, conn_handle, attr->handle);

    if (gapcbs.OnSubscribe != NULL)
        gapcbs.OnSubscribe(NULL, NULL, conn_handle);
        
    return 0;
}

void blecent_on_disc_complete(const struct peer *peer, int status, void *arg)
{
    if (status != 0)
    {
        // 主动终止连接
        ESP_LOGE(TAG, "Error: Service discovery failed; status=%d, conn_handle=%d\n",
                 status, peer->conn_handle);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    int i = connstate.curr_id;

    // 找特征
    const struct peer_chr *chr;
    chr = peer_chr_find_uuid(peer, peerinfos[i].svc_uuid, peerinfos[i].chr_uuid);
    peerinfos[i].val_handle = chr->chr.val_handle;
    ESP_LOGI(TAG, "find target chr, val_handle is %d", peerinfos[i].val_handle);

    // 找到描述符
    const struct peer_dsc *dsc;
    dsc = peer_dsc_find_uuid(peer, peerinfos[i].svc_uuid, peerinfos[i].chr_uuid, BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_CFG_UUID16));
    ESP_LOGI(TAG, "find target dsc, handle is %d", dsc->dsc.handle);

    // 开启notify
    uint8_t value[2] = {1, 0};
    int rc = ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle, value, sizeof(value), blecent_on_subscribe, NULL);
    if (rc != 0)
        ESP_LOGE(TAG, "Error: Failed to subscribe to the subscribable characteristic;rc=%d\n ", rc);

    connstate.connecting = false;
    if (connstate.connected_num != PROFILE_NUM)
        if (!connstate.scanning)
            start_scan();
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc = 0;

    switch (event->type)
    {
    // 连接事件
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0)
        {
            if (gapcbs.OnConnect != NULL)
                rc = gapcbs.OnConnect(event, arg, event->connect.conn_handle);
        }
        else
        {
            connstate.connecting = false;
            if (!connstate.scanning)
                start_scan();
        }
        return rc;

    // 断开连接
    case BLE_GAP_EVENT_DISCONNECT:
        if (gapcbs.OnDisconnect != NULL)
            rc = gapcbs.OnDisconnect(event, arg, event->disconnect.conn.conn_handle);
        return rc;

    // 扫描到一个设备
    case BLE_GAP_EVENT_DISC:
    {
        if (connstate.connecting)
            return 0;

        if (connstate.connected_num == PROFILE_NUM)
        {
            stop_scan();
            return 0;
        }

        for (uint8_t i = 0; i < PROFILE_NUM; i++)
            if (memcmp(peerinfos[i].addr, event->disc.addr.val, sizeof(event->disc.addr.val)) == 0)
            {
                stop_scan();
                connstate.connecting = true;
                connstate.curr_id = i;
                ESP_LOGE(TAG, "device is connecting to the %s.....", (char *)peerinfos[i].name);
                ble_gap_connect(own_addr_type, &event->disc.addr, 10000, &conn_params, ble_gap_event, NULL);
                break;
            }
        return 0;
    }

    // 扫描停止
    case BLE_GAP_EVENT_DISC_COMPLETE:
        ESP_LOGI(TAG, "discovery complete; reason=%d\n", event->disc_complete.reason);
        return 0;

    // 收到通知
    case BLE_GAP_EVENT_NOTIFY_RX:
        if (gapcbs.OnNotifyRx != NULL)
            gapcbs.OnNotifyRx(event, arg, event->notify_rx.conn_handle);
        return 0;

    // 收到连接参数更新请求
    case BLE_GAP_EVENT_CONN_UPDATE_REQ:
        ESP_LOGE(TAG, "conn update request,conn_handle=%d,MIN=%d,MAX=%d,MIN=%d,MAX=%d \n",
                 event->conn_update_req.conn_handle,
                 event->conn_update_req.peer_params->itvl_min,
                 event->conn_update_req.peer_params->itvl_max,
                 event->conn_update_req.self_params->itvl_min,
                 event->conn_update_req.self_params->itvl_max);
        return 0;

    // 更新连接参数事件
    case BLE_GAP_EVENT_CONN_UPDATE:
        ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        ESP_LOGI(TAG,
                 "connection updated event, status=%d; conn_itvl=%d "
                 "conn_latency=%d supervision_timeout=%d ",
                 event->conn_update.status, desc.conn_itvl,
                 desc.conn_latency, desc.supervision_timeout);
        return 0;

    // 交换MTU事件
    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d",
                 event->mtu.conn_handle, event->mtu.channel_id,
                 event->mtu.value);
        return 0;

    default:
        return 0;
    }
}

static void blecent_on_reset(int reason)
{
    ESP_LOGE(TAG, "Resetting state; reason=%d\n", reason);
}

static void blecent_on_sync(void)
{
    // 确保设置了合适的ip地址
    int rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    // 获取地址类型
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "error determining address type; rc=%d\n", rc);
        return;
    }

    // 开始扫描设备
    start_scan();
}

static void blecent_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void set_gap_callbacks(struct GapCallbacks_t cbs)
{
    gapcbs.OnConnect = cbs.OnConnect;
    gapcbs.OnDisconnect = cbs.OnDisconnect;
    gapcbs.OnNotifyRx = cbs.OnNotifyRx;
    gapcbs.OnSubscribe = cbs.OnSubscribe;
}

void ble_init(void)
{
    int rc;

    // 初始化nvs存储器
    rc = nvs_flash_init();
    if (rc == ESP_ERR_NVS_NO_FREE_PAGES || rc == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        rc = nvs_flash_init();
    }
    ESP_ERROR_CHECK(rc);

    // 初始化BT控制器和nimble堆栈
    rc = nimble_port_init();
    if (rc != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to init nimble %d ", rc);
        return;
    }

    // 设置回调
    ble_hs_cfg.reset_cb = blecent_on_reset;
    ble_hs_cfg.sync_cb = blecent_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_store_config_init();

    // 设置peer队列的内存大小
    rc = peer_init(MYNEWT_VAL(BLE_MAX_CONNECTIONS), 64, 64, 64);
    assert(rc == 0);

    // 设置名字
    rc = ble_svc_gap_device_name_set("ESP32-Client");
    assert(rc == 0);

    // 启动协议栈
    nimble_port_freertos_init(blecent_host_task);
}

void start_scan(void)
{
    struct ble_gap_disc_params disc_params = {
        .filter_duplicates = 0,
        .passive = 1,
        .itvl = 0,
        .window = 0,
        .filter_policy = 0,
        .limited = 0,
    };

    int rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "error initiating GAP discovery procedure; rc=%d\n", rc);
        return;
    }
    connstate.scanning = true;
    ESP_LOGI(TAG, "start scan successed\n");
}

void stop_scan(void)
{
    int rc = ble_gap_disc_cancel();
    if (rc != 0)
    {
        ESP_LOGE(TAG, "error stopping GAP discovery procedure; rc=%d\n", rc);
        return;
    }
    connstate.scanning = false;
    ESP_LOGI(TAG, "stop scan successed\n");
}