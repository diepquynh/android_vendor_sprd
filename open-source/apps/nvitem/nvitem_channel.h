
#include "nvitem_common.h"

#ifndef _NVITEM_CHANNEL_H_
#define _NVITEM_CHANNEL_H_

void channel_open(void);

BOOLEAN channel_read(uint8* buf, int32 size, int32* hasRead);

BOOLEAN channel_write(uint8* buf, uint32 size, int32* hasWrite);

void channel_close(void);

#endif

