
#include "nvitem_common.h"

#ifndef _NVITEM_BUF_H_
#define _NVITEM_BUF_H_

//----------------------------
//	init buffer module
//----------------------------
void initBuf(void);

//----------------------------
//	uninit buffer module
//----------------------------
void uninitBuf(void);

//----------------------------
//	partId to index of control array
//----------------------------
uint32 getCtlId(uint32 partId);

//----------------------------
// to "fromChannel" buffer
// unit is sector
//----------------------------
void _markDirtyInfo(uint32 id,  uint32 start, uint32 scts);

//----------------------------
// to "fromChannel" buffer
// unit is bytes
//----------------------------
void writeData(uint32 id,  uint32 start, uint32 bytesLen, uint8* buf);

//----------------------------
// fromChannel -> backup
//----------------------------
BOOLEAN backupData(uint32 id);

//----------------------------
// backup -> fromChannel
//----------------------------
void restoreData(uint32 id);

//----------------------------
// backup -> toDisk
//----------------------------
BOOLEAN saveToDisk(void);

#endif

