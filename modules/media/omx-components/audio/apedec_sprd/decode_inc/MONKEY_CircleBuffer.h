#ifndef APE_CIRCLEBUFFER_H
#define APE_CIRCLEBUFFER_H

#include "MONKEY_APECommon.h"

typedef struct _CircleBuffer {
	int m_nTotal;
    int m_nMaxDirectWriteBytes;
    int m_nEndCap;
    int m_nHead;
    int m_nTail;
    unsigned char * m_pBuffer;
} CircleBuffer;

// construction / destruction
void MONKEY_CircleBuffer_CircleBuffer(CircleBuffer * CCircleBuffer);
   
// create the buffer
int MONKEY_CircleBuffer_CreateBuffer(CircleBuffer * CCircleBuffer, int nBytes, int nMaxDirectWriteBytes);

int MONKEY_CircleBuffer_Finish(CircleBuffer * CCircleBuffer);

// query
int MONKEY_CircleBuffer_MaxAdd(CircleBuffer * CCircleBuffer);
int MONKEY_CircleBuffer_MaxGet(CircleBuffer * CCircleBuffer);

// direct writing    
static __inline unsigned char * MONKEY_CircleBuffer_GetDirectWritePointer(CircleBuffer * CCircleBuffer)
{
        // return a pointer to the tail -- note that it will always be safe to write
        // at least m_nMaxDirectWriteBytes since we use an end cap region
        return &(CCircleBuffer->m_pBuffer[CCircleBuffer->m_nTail]);
}

    
static __inline void MONKEY_CircleBuffer_UpdateAfterDirectWrite(CircleBuffer * CCircleBuffer, int nBytes)
{
        // update the tail
        CCircleBuffer->m_nTail += nBytes;

        // if the tail enters the "end cap" area, set the end cap and loop around
        if (CCircleBuffer->m_nTail >= (CCircleBuffer->m_nTotal - CCircleBuffer->m_nMaxDirectWriteBytes))
        {
            CCircleBuffer->m_nEndCap = CCircleBuffer->m_nTail;
            CCircleBuffer->m_nTail = 0;
        }
}

    
// get data
int MONKEY_CircleBuffer_Get(CircleBuffer * CCircleBuffer, unsigned char * pBuffer, int nBytes);

// remove / empty
void MONKEY_CircleBuffer_Empty(CircleBuffer * CCircleBuffer);
int MONKEY_CircleBuffer_RemoveHead(CircleBuffer * CCircleBuffer, int nBytes);
int MONKEY_CircleBuffer_RemoveTail(CircleBuffer * CCircleBuffer, int nBytes);

#endif // #ifndef APE_CIRCLEBUFFER_H
