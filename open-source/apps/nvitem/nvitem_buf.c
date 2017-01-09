
#include "nvitem_common.h"
#include "nvitem_os.h"
#include "nvitem_config.h"
#include "nvitem_buf.h"
#include "nvitem_fs.h"

typedef struct
{
	uint8*	diskbuf;								// ramdisk buf
	uint32	dirty[RAMNV_DIRTYTABLE_MAXSIZE];	// dirty bits, one bits indicate one sect
}_RAMNV_BUF_CTRL;

typedef struct
{
// disk partition id
	uint32	partId;
	uint32	sctNum;								// total number of sector in current disk
// disk buffer
	_RAMNV_BUF_CTRL	fromChannel;
	_RAMNV_BUF_CTRL	backup;
	_RAMNV_BUF_CTRL	toDisk;
// fs handle
	RAMDISK_HANDLE	fdhandle;
}_RAMNV_PART_CTRL;

typedef struct
{
	uint32 partNum;
	_RAMNV_PART_CTRL part[RAMNV_NUM];
}_RAMNV_CTL;

static _RAMNV_CTL ramNvCtl;
extern BOOLEAN is_cali_mode;


//----------------------------
//	init buffer module
//----------------------------
void initBuf(void)
{
	uint32 i,k;
	const RAM_NV_CONFIG* config;

	ramNvCtl.partNum = 0;
	for(i = 0; i < RAMNV_NUM; i++)
	{
		ramNvCtl.part[i].sctNum = 0;
		for(k = 0; k < RAMNV_DIRTYTABLE_MAXSIZE; k++)
		{
			ramNvCtl.part[i].fromChannel.dirty[k] = 0;
			ramNvCtl.part[i].backup.dirty[k] = 0;
			ramNvCtl.part[i].toDisk.dirty[k] = 0;
		}
	}

	config = ramDisk_Init();

	i = 0;
	while(config->partId)
	{
//------------------------------------------------------------
		ramNvCtl.part[i].fdhandle = ramDisk_Open(config->partId);
//------------------------------------------------------------
		if(0 == ramNvCtl.part[i].fdhandle)
		{
			config++;
			continue;
		}
		ramNvCtl.part[i].partId					= config->partId;
		ramNvCtl.part[i].sctNum				= config->image_size/RAMNV_SECT_SIZE;
		ramNvCtl.part[i].fromChannel.diskbuf		= malloc(config->image_size+4);
		ramNvCtl.part[i].backup.diskbuf			= malloc(config->image_size+4);
		ramNvCtl.part[i].toDisk.diskbuf			= malloc(config->image_size+4);
//---for test---
		memset(ramNvCtl.part[i].fromChannel.diskbuf, 0, config->image_size);
//------------
//------------------------------------------------------------
		if(!ramDisk_Read(ramNvCtl.part[i].fdhandle, ramNvCtl.part[i].fromChannel.diskbuf, config->image_size))
//------------------------------------------------------------
		{
			free(ramNvCtl.part[i].fromChannel.diskbuf);
			free(ramNvCtl.part[i].backup.diskbuf);
			free(ramNvCtl.part[i].toDisk.diskbuf);
			config++;
			continue;
		}
		memcpy(ramNvCtl.part[i].backup.diskbuf,ramNvCtl.part[i].fromChannel.diskbuf,config->image_size+4);
		memcpy(ramNvCtl.part[i].toDisk.diskbuf,ramNvCtl.part[i].fromChannel.diskbuf,config->image_size+4);
//		backupData(i);
		ramNvCtl.partNum++;
		i++;
		config++;
	}
}

//----------------------------
//	uninit buffer module
//----------------------------
void uninitBuf(void)
{
	uint32 i;

	for(i = 0; i < ramNvCtl.partNum; i++)
	{
		ramDisk_Close(ramNvCtl.part[i].fdhandle);
		ramNvCtl.part[i].sctNum = 0;
//		for(k = 0; k < RAMNV_DIRTYTABLE_MAXSIZE; k++)
//		{
//			ramNvCtl.part[i].fromChannel.dirty[k] = 0;
//			ramNvCtl.part[i].backup.dirty[k] = 0;
//			ramNvCtl.part[i].toDisk.dirty[k] = 0;
//		}
		if(ramNvCtl.part[i].fromChannel.diskbuf)	{free(ramNvCtl.part[i].fromChannel.diskbuf);}
		if(ramNvCtl.part[i].backup.diskbuf)		{free(ramNvCtl.part[i].backup.diskbuf);}
		if(ramNvCtl.part[i].toDisk.diskbuf)		{free(ramNvCtl.part[i].toDisk.diskbuf);}
	}
	ramNvCtl.partNum = 0;
}

//----------------------------
//	partId to index of control array
//----------------------------
uint32 getCtlId(uint32 partId)
{
	uint32 i;

	for(i = 0; i < ramNvCtl.partNum; i++)
	{
		if(partId == ramNvCtl.part[i].partId)
		{
			return i;
		}
	}
	return (uint32)-1;
}
//----------------------------
// to "fromChannel" buffer
// unit is sector
//----------------------------
/*PASS*/
#define _U32MASK ((uint32)(-1))			// 0xFFFFFFFF
#define _DIRTY_TABLE		(ramNvCtl.part[id].fromChannel.dirty)
void _markDirtyInfo(uint32 id, /*IN*/uint32 start, /*IN*/uint32 scts)
{
	uint32 _bytes;
	uint32 _bits;

	_bytes = start >> 5;				// _bytes = start/32;
	_bits = start - (_bytes<<5);		// _bits = start%32;

	while((32-_bits) < scts)
	{
		_DIRTY_TABLE[_bytes] |= (_U32MASK << _bits);
		scts -= (32-_bits);
		_bytes++;
		_bits = 0;
	}
	_DIRTY_TABLE[_bytes] |= ((_U32MASK>>(32-scts))<<_bits);
}
#undef _DIRTY_TABLE

#define _DIRTY_TABLE		(ramNvCtl.part[id].backup.dirty)
static BOOLEAN __ifHasDirty(uint32 id)
{
	uint32 i;
	for(i = 0; i < RAMNV_DIRTYTABLE_MAXSIZE; i++)
	{
		if(_DIRTY_TABLE[i])
		{
			return 1;
		}
	}
	return 0;
}
#undef _DIRTY_TABLE

//----------------------------
// to "fromChannel" buffer
// unit is bytes
//----------------------------
void writeData(uint32 id,  uint32 start, uint32 bytesLen, uint8* buf)
{
	if(bytesLen)
	{
		memcpy(&ramNvCtl.part[id].fromChannel.diskbuf[start], buf, bytesLen);
	}
}

//----------------------------
// fromChannel -> backup
//----------------------------
BOOLEAN backupData(uint32 id)
{
	uint32 i;

	getMutex();

	NVITEM_PRINT("backupData is_cali_mode 0x%x\n",is_cali_mode);
	memcpy(ramNvCtl.part[id].backup.diskbuf, ramNvCtl.part[id].fromChannel.diskbuf, RAMNV_SECT_SIZE*ramNvCtl.part[id].sctNum);
	for(i = 0; i < RAMNV_DIRTYTABLE_MAXSIZE; i++)
	{
		ramNvCtl.part[id].backup.dirty[i] |= ramNvCtl.part[id].fromChannel.dirty[i];
	}

	putMutex();
	if(saveToDisk()){
		return 1;
	}
	else{
		return 0;
	}

//---for test---
//	__getData(id);
//	return ramDisk_Write(ramNvCtl.part[id].fdhandle, ramNvCtl.part[id].toDisk.diskbuf, RAMNV_SECT_SIZE*ramNvCtl.part[id].sctNum);
//------------
}

//----------------------------
// backup -> fromChannel
//----------------------------
void restoreData(uint32 id)
{
	getMutex();

	memcpy(ramNvCtl.part[id].fromChannel.diskbuf, ramNvCtl.part[id].backup.diskbuf, RAMNV_SECT_SIZE*ramNvCtl.part[id].sctNum);
	memcpy(ramNvCtl.part[id].fromChannel.dirty, ramNvCtl.part[id].backup.dirty, RAMNV_DIRTYTABLE_MAXSIZE*sizeof(uint32));

	putMutex();
	return;
}

//----------------------------
// backup -> toDisk
//----------------------------
void __getData(uint32 id)
{
	uint32 i;

	getMutex();

	memcpy(ramNvCtl.part[id].toDisk.diskbuf, ramNvCtl.part[id].backup.diskbuf, RAMNV_SECT_SIZE*ramNvCtl.part[id].sctNum);
	for(i = 0; i < RAMNV_DIRTYTABLE_MAXSIZE; i++)
	{
		ramNvCtl.part[id].toDisk.dirty[i] |= ramNvCtl.part[id].backup.dirty[i] ;
		ramNvCtl.part[id].backup.dirty[i]  = 0;
	}

	putMutex();
	return;
}

BOOLEAN saveToDisk(void)
{
	uint32 id,i;

	for(id = 0; id <ramNvCtl.partNum; id++)
	{
		if(__ifHasDirty(id))
		{
			__getData(id);
			if(ramDisk_Write(ramNvCtl.part[id].fdhandle, ramNvCtl.part[id].toDisk.diskbuf, RAMNV_SECT_SIZE*ramNvCtl.part[id].sctNum))
			{
				// clean dirty bit of toDiskBuf
					for(i = 0; i < RAMNV_DIRTYTABLE_MAXSIZE; i++)
					{
						ramNvCtl.part[id].toDisk.dirty[i]  = 0;
					}
					NVITEM_PRINT("saveToDisk success id = %d\n",id);
					continue;
			}
			else{
				NVITEM_PRINT("saveToDisk failed id = %d\n",id);
				return 0;
			}
		}
		else{
			NVITEM_PRINT("saveToDisk no dirty to save id = %d\n",id);
		}
	}
	return 1;
}
	
