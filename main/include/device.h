#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "common.h"

#ifndef FOUR_NODE
#define PROFILE_NUM 1
#else
#define PROFILE_NUM 4
#endif

extern struct PeerInfo peerinfos[PROFILE_NUM];


#endif 