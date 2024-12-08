#ifndef __BLECENT_H__
#define __BLECENT_H__

#include "common.h"
typedef int (*GapCallback)(struct ble_gap_event *event, void *arg, uint16_t conn_handle);

struct GapCallbacks_t
{
    GapCallback OnConnect;
    GapCallback OnDisconnect;
    GapCallback OnNotifyRx;
    GapCallback OnSubscribe;
};

void start_scan(void);
void stop_scan(void);
void set_gap_callbacks(struct GapCallbacks_t cbs);
void ble_init(void);
void blecent_on_disc_complete(const struct peer *peer, int status, void *arg);

#endif