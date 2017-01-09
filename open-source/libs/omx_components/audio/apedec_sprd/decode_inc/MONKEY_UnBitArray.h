#ifndef APE_UNBITARRAY_H
#define APE_UNBITARRAY_H

#include "MONKEY_APECommon.h"
#define BitArray_RefillBitThreshold 512

typedef struct _RANGE_CODER_STRUCT_DECOMPRESS
{
    unsigned int low;       // low end of interval
    unsigned int range;     // length of interval
    unsigned int buffer;    // buffer for input/output
} RANGE_CODER_STRUCT_DECOMPRESS;

typedef struct _UNBIT_ARRAY_STATE
{
    uint32 k;
    uint32 nKSum;
} UNBIT_ARRAY_STATE;

enum DECODE_VALUE_METHOD
{
    DECODE_VALUE_METHOD_UNSIGNED_INT,
    DECODE_VALUE_METHOD_UNSIGNED_RICE,
    DECODE_VALUE_METHOD_X_BITS
};

typedef struct _CUnBitArray {
	
	int m_nVersion;

    uint32 m_nBytes;
    uint32 m_nLen;
    uint32 m_nCurrentBitIndex;
	uint32 m_nRefillBitThreshold;
	
    uint32 * m_pBitArray;

	int m_nFlushCounter;
    int m_nFinalizeCounter;

    RANGE_CODER_STRUCT_DECOMPRESS m_RangeCoderInfo;

	void *Input;
} CUnBitArray;

int MONKEY_CUnBitArray(CUnBitArray * CCUnBitArray, int nVersion);
int MONKEY_CUnBitArray_CreateHelper(CUnBitArray * CCUnBitArray, int nBytes, int nVersion);
int MONKEY_CUnBitArray_Finish(CUnBitArray * CCUnBitArray);

void MONKEY_CUnBitArray_FlushState(UNBIT_ARRAY_STATE * BitArrayState);
void MONKEY_CUnBitArray_FlushBitArray(CUnBitArray * CCUnBitArray);
void MONKEY_CUnBitArray_Finalize(CUnBitArray * CCUnBitArray);

int MONKEY_CUnBitArray_GenerateArray(CUnBitArray * CCUnBitArray, int * pOutputArray, int nElements);
int MONKEY_CUnBitArray_GenerateArrayRange(CUnBitArray * CCUnBitArray, int * pOutputArray, int nElements);
    
//__inline int MONKEY_CUnBitArray_RangeDecodeFast(CUnBitArray * CCUnBitArray, int nShift);
//__inline int MONKEY_CUnBitArray_RangeDecodeFastWithUpdate(CUnBitArray * CCUnBitArray, int nShift);
//__inline unsigned char MONKEY_CUnBitArray_GetC(CUnBitArray * CCUnBitArray);


int MONKEY_CUnBitArray_FillBitArray(CUnBitArray * CCUnBitArray);
int MONKEY_CUnBitArray_FillAndResetBitArray(CUnBitArray * CCUnBitArray);

void MONKEY_CUnBitArray_AdvanceToByteBoundary(CUnBitArray * CCUnBitArray);

int MONKEY_CUnBitArray_DecodeValue(CUnBitArray * CCUnBitArray, int DecodeMethod, unsigned int * value);
int MONKEY_CUnBitArray_DecodeValueXBits(CUnBitArray * CCUnBitArray, uint32 nBits, unsigned int * value);
int MONKEY_CUnBitArray_DecodeValueRange(CUnBitArray * CCUnBitArray, UNBIT_ARRAY_STATE * BitArrayState, int * value);


#endif // #ifndef APE_UNBITARRAY_H
