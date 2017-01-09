#ifndef APE_ROLLBUFFER_H
#define APE_ROLLBUFFER_H

#include "MONKEY_APECommon.h"

typedef struct _CRollBuffer {
    short * m_pData;
    short * m_pCurrent;
    int m_nHistoryElements;
    int m_nWindowElements;
} CRollBuffer;

void MONKEY_CRollBuffer(CRollBuffer * CCRollBuffer);
int MONKEY_CRollBuffer_Finish(CRollBuffer * CCRollBuffer);
void MONKEY_CRollBuffer_Flush(CRollBuffer * CCRollBuffer);
int MONKEY_CRollBuffer_Create(CRollBuffer * CCRollBuffer, int nWindowElements, int nHistoryElements);
void MONKEY_CRollBuffer_Roll(CRollBuffer * CCRollBuffer);

typedef struct _CRollBufferFast {
    int * m_pData;
    int * m_pCurrent;
	int m_nHistoryElements;
    int m_nWindowElements;
} CRollBufferFast;

int MONKEY_CRollBufferFast(CRollBufferFast * CCRollBufferFast, int WINDOW_ELEMENTS, int HISTORY_ELEMENTS);
int MONKEY_CRollBufferFast_Finish(CRollBufferFast * CCRollBufferFast);
void MONKEY_CRollBufferFast_Flush(CRollBufferFast * CCRollBufferFast);
void MONKEY_CRollBufferFast_Roll(CRollBufferFast * CCRollBufferFast);

#endif // #ifndef APE_ROLLBUFFER_H
