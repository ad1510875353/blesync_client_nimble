#include "device.h"
#include "common.h"

#ifdef FOUR_NODE
const uint8_t node1_addr[6] = {0x1a, 0xaf, 0x75, 0xf9, 0x55, 0x60};
const uint8_t node2_addr[6] = {0xd6, 0xa8, 0x75, 0xf9, 0x55, 0x60};
const uint8_t node3_addr[6] = {0xb2, 0xc1, 0x75, 0xf9, 0x55, 0x60};
const uint8_t node4_addr[6] = {0xce, 0x88, 0x75, 0xf9, 0x55, 0x60};
struct PeerInfo peerinfos[PROFILE_NUM] = {
    [0] = {
        .name = "Node1",
        .addr = node1_addr,
        .svc_uuid = SVC_UUID,
        .chr_uuid = CHR_UUID,
        .find_chr = false,
        .is_connect = false,
    },
    [1] = {
        .name = "Node2",
        .addr = node2_addr,
        .svc_uuid = SVC_UUID,
        .chr_uuid = CHR_UUID,
        .find_chr = false,
        .is_connect = false,
    },
    [2] = {
        .name = "Node3",
        .addr = node3_addr,
        .svc_uuid = SVC_UUID,
        .chr_uuid = CHR_UUID,
        .find_chr = false,
        .is_connect = false,
    },
    [3] = {
        .name = "Node4",
        .addr = node4_addr,
        .svc_uuid = SVC_UUID,
        .chr_uuid = CHR_UUID,
        .find_chr = false,
        .is_connect = false,
    },
};
#else
const uint8_t node1_addr[6] = {0x1a, 0xaf, 0x75, 0xf9, 0x55, 0x60};
struct PeerInfo peerinfos[PROFILE_NUM] = {
    [0] = {
        .name = "Node1",
        .addr = node1_addr,
        .svc_uuid = SVC_UUID,
        .chr_uuid = CHR_UUID,
        .connected = false,
        .val_handle = 0,
    },
};
#endif