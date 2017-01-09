#ifndef APE_NNFILTER_H
#define APE_NNFILTER_H

#include "MONKEY_APECommon.h"
#include "MONKEY_RollBuffer.h"

#define NN_WINDOW_ELEMENTS    512
//#define NN_TEST_MMX

typedef struct _CNNFilter {
    int m_nOrder;
    int m_nShift;
    int m_nVersion;
    int m_bMMXAvailable;
    int m_nRunningAverage;

    CRollBuffer m_rbInput;
    CRollBuffer m_rbDeltaM;

    short * m_paryM;
	int * m_paryM0;
} CNNFilter;

int MONKEY_CNNFilter(CNNFilter * CCNNFilter, int nOrder, int nShift, int nVersion);

int MONKEY_CNNFilter_Finish(CNNFilter * CCNNFilter);

int MONKEY_CNNFilter_Compress(CNNFilter * CCNNFilter, int nInput);
    
int MONKEY_CNNFilter_Decompress(CNNFilter * CCNNFilter, int nInput);
    
void MONKEY_CNNFilter_Flush(CNNFilter * CCNNFilter);

static __inline short GetSaturatedShortFromInt(int nValue)
{
   return (short) (nValue == (short)nValue) ? nValue : (nValue >> 31) ^ 0x7FFF;
}

#ifdef ARM9E
void MONKEY_CNNFilter_Adapt_ASM(short * pM, short * pAdapt, int nDirection, int nOrder);
int MONKEY_CNNFilter_CalcDotProduct_ASM(short * pA, short * pB, int nOrder);
#endif

#endif // #ifndef APE_NNFILTER_H
