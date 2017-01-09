
#include "nvitem_common.h"

#ifndef _NVITEM_PACKET_H_
#define _NVITEM_PACKET_H_


void _initPacket(void);

#define _PACKET_START		0	// new packet group
#define _PACKET_CONTINUE	1	// next packet in current group
#define _PACKET_SKIP		2	// non control packet
#define _PACKET_FAIL		3	// channel fail
uint32 _getPacket(uint8** buf, uint32* size);

void _sendPacktRsp(uint8 rsp);

#endif

