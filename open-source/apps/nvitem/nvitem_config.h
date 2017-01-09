
#include "nvitem_common.h"

#ifndef _NVITEM_CONFIG_H_
#define _NVITEM_CONFIG_H_


#define RAMDISK_CFG	"/nvitem.cfg"

//---------------------------------------------------------
//				Const config: can not be changed
//---------------------------------------------------------
//NOTE:
//	1: 0 and 0xFFFFFFFF is reserved partition id
//	2: high 24 bit is reserved bit, so only have 255 id that can be used
//	3: So don't waste id number when you add new partition id here
//---------------------------------------------------------
#define RAMBSD_FIXNV_ID			1
#define RAMBSD_RUNNV_ID			2
#define RAMBSD_PRODUCTINFO_ID	3


#endif

