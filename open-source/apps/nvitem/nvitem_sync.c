
#include "nvitem_common.h"
#include "nvitem_buf.h"
#include "nvitem_sync.h"
#include "nvitem_packet.h"

#define SYNC_REQ_HEAD 0
#define SYNC_REQ_BODY 1
#define SYNC_REQ_TAIL 2

typedef struct
{
	uint32	packet_type;				// headd,body,tail
	uint32	reqIdx;						// ++operation
	uint32	partId;						// parition Id
	uint32	sctSize;						// size of sector
}SYNC_HEAD;

typedef struct
{
	uint32	packet_type;				// headd,body,tail
	uint32	reqIdx;						// ++operation
	uint32	partId;						// parition Id
	uint32	start;						// start position of dirty data
	uint32	num;						// number of dirty data, unit is sector
}SYNC_BODY;

typedef struct
{
	uint32	packet_type;				// headd,body,tail
	uint32	reqIdx;						// ++operation
	uint32	partId;						// parition Id
}SYNC_TAIL;

typedef union
{
	SYNC_HEAD	head;
	SYNC_BODY	body;
	SYNC_TAIL	tail;
}SYNC_REQ;


uint32 _syncState;	// 0: analyzer head ,1 analyzer body, 2 annalyzer tail
uint32 _syncBusy	= 0;	// 0: idle,  1: busy

static	uint32	syncReqFrame = 0;
static	uint8*	syncReqBuf[sizeof(SYNC_REQ)];
static	uint32	_ofst = 0;
static	uint32	_id = (uint32)(-1);
static	uint32	_start = 0;
static	uint32	_len	= 0;

//----------------------------
//	Init Sync module
//----------------------------
void _syncInit(void)
{
	_syncState = 0;
	_syncBusy = 0;
	syncReqFrame = 0;
	//syncReqBuf;
	_ofst = 0;
	_id = (uint32)(-1);
	_start = 0;
	_len	= 0;
}
//----------------------------
//	Reset Sync module
//----------------------------
/*PASS*/
#define SYNC_FAIL_RESET	0
#define SYNC_SUC_RESET	1
static BOOLEAN _syncReset(uint32 ifSuc)
{
	BOOLEAN ret;

	ret = 1;
	if(ifSuc)
	{
		ret = backupData(_id);
	}
	else
	{
		if(_syncState)	// in this state , fromchannel buffer maybe changed. So we must restore it.
		{
			restoreData(_id);
		}
	}
	_syncInit();
	return ret;
}

//--------------------------------------------------
//	TRUE: have getted
//	FALSE: not getted, need more information
/*PASS*/
static uint32 __getToken(uint8* *buf, uint32* size)
{
	if((sizeof(SYNC_REQ)-_ofst) > *size)
	{
		if(*size)
		{
			return 0;
		}
		memcpy(syncReqBuf+_ofst, *buf, *size);
		_ofst += *size;
		*buf += *size;
		*size = 0;
		return 0;
	}
	else
	{
		if(0 == (sizeof(SYNC_REQ)-_ofst))
		{
			_ofst = 0;
			return 1;
		}
		memcpy(syncReqBuf+_ofst, *buf,(sizeof(SYNC_REQ)-_ofst));
		*buf += (sizeof(SYNC_REQ)-_ofst);
		*size -= (sizeof(SYNC_REQ)-_ofst);
		_ofst = 0;
		return 1;
	}
}


#define SYNC_CONTINUE	0		// analyzer not complete, information is not enough, wait for continuous information
#define SYNC_NEXT		1		// one token analyzer complete,  should analyzer next token of request
#define SYNC_DONE		2		// all token analyzer is all complete
#define SYNC_FAIL		3		// analyzer fail, infomation error, should reset
/*PASS*/
static uint32 _syncGetHead(uint8** buf, uint32 *size)
{
	SYNC_REQ* head = (SYNC_REQ*)syncReqBuf;

	if(!__getToken(buf,size))
	{
		return SYNC_CONTINUE;
	}
	if(SYNC_REQ_HEAD != head->head.packet_type)
	{
		return SYNC_FAIL;
	}
	syncReqFrame = head->head.reqIdx;
	_id = getCtlId(head->head.partId);
	if(((uint32)-1) == _id)
	{
		return SYNC_FAIL;
	}
	return SYNC_NEXT;
}

/*PASS*/
static uint32 _syncGetBody(uint8** buf, uint32* size)
{
	SYNC_REQ* req = (SYNC_REQ*)syncReqBuf;

	if(!__getToken(buf,size))
	{
		return SYNC_CONTINUE;
	}
	if(SYNC_REQ_BODY == req->body.packet_type)
	{
		if((syncReqFrame+1) != req->body.reqIdx)
		{
			return SYNC_FAIL;
		}
		syncReqFrame++;
		_markDirtyInfo(_id,  req->body.start, req->body.num);
		_start	= req->body.start*RAMNV_SECT_SIZE;
		_len	= req->body.num*RAMNV_SECT_SIZE;
		return SYNC_NEXT;
	}
	else if(SYNC_REQ_TAIL == req->tail.packet_type)
	{
		if((syncReqFrame+1) != req->tail.reqIdx)
		{
			return SYNC_FAIL;
		}
		syncReqFrame++;
		return SYNC_DONE;
	}
	else
	{
		return SYNC_FAIL;
	}
}

/*PASS*/
static uint32 _syncGetData(uint8** buf, uint32* size)
{
	if(_len <= *size)
	{
		writeData(_id,  _start, _len, *buf);
		*buf	+=_len;
		*size	-=_len;
		_start	+= _len;
		_len	= 0;
		return SYNC_NEXT;
	}
	else
	{
		writeData(_id,  _start, *size, *buf);
		_start	+= *size;
		_len	-= *size;
		*buf	+= *size;
		*size	= 0;
		return SYNC_CONTINUE;
	}
}


//----------------------------
//	analyzer packet buf
//----------------------------
/*PASS*/
void syncAnalyzer(void)
{
	uint32 syncRet;
	uint32 packetState;
	uint8* buf = 0;
	uint32 size = 0;

do
{
	packetState = _getPacket(&buf, &size);
	if(_PACKET_FAIL == packetState)
	{
		NVITEM_PRINT("packetState fail\n");
		// packet module connection fail. teminate current req process
		_syncReset(SYNC_FAIL_RESET);
		return;
	}
	else if((_PACKET_START == packetState)&&_syncBusy)
	{
		NVITEM_PRINT("packetState no req tail\n");
		// 1 == _syncBusy, means current req not finish
		// _PACKET_START means next req has come. so teminate current req process, and start new process
		_syncReset(SYNC_FAIL_RESET);
	}
	else if((_PACKET_CONTINUE == packetState)&&(!_syncBusy))
	{
		NVITEM_PRINT("packetState no req head\n");
		// 0 == _syncBusy, means no req is process.
		// 1 _PACKET_CONTINUE means body of req. So has no head, skip it.
		_sendPacktRsp(0);
		continue;
	}
	else if(_PACKET_SKIP == packetState)
	{
		NVITEM_PRINT("packetState skip\n");
		// invalid packet, skip it
		continue;
	}

	_syncBusy = 1;

	while(0 != size)
	{
		if(0 == _syncState)							// head analyzer
		{
			syncRet = _syncGetHead(&buf, &size);
			if(SYNC_FAIL == syncRet)
			{
				NVITEM_PRINT("_syncGetHead fail\n");
				_syncReset(SYNC_FAIL_RESET);
				_sendPacktRsp(0);
				break;
			}
			else if(SYNC_NEXT == syncRet)
			{
				_syncState = 1;
				continue;
			}
			else if(SYNC_CONTINUE == syncRet)
			{
				_sendPacktRsp(1);
				break;
			}
		}
		else if(1 == _syncState)						// body and tail analyzer
		{
			syncRet = _syncGetBody(&buf, &size);
			if(SYNC_FAIL == syncRet)
			{
				NVITEM_PRINT("_syncGetBody fail\n");
				_syncReset(SYNC_FAIL_RESET);
				_sendPacktRsp(0);
				break;
			}
			else if(SYNC_NEXT == syncRet)
			{
				_syncState = 2;
				continue;
			}
			else if(SYNC_CONTINUE == syncRet)
			{
				_sendPacktRsp(1);
				break;
			}
			else if(SYNC_DONE == syncRet)
			{
				NVITEM_PRINT("sync _syncGetTail success\n");
				_sendPacktRsp(_syncReset(SYNC_SUC_RESET));
				break;
			}
		}
		else // (2 == _syncState)						// data analyzer
		{
			syncRet =  _syncGetData(&buf, &size);
			if(SYNC_CONTINUE == syncRet)
			{
				_sendPacktRsp(1);
				break;
			}
			else if(SYNC_NEXT == syncRet)
			{
				_syncState = 1;
				continue;
			}
		}
	}
}while(1);
}

