#ifndef APE_APEDECOMPRESS_H
#define APE_APEDECOMPRESS_H

#include "MONKEY_APECommon.h"

#define BLOCKS_PER_DECODE               1024
#define BLOCKS_TEMP_OUTPUT              BLOCKS_PER_DECODE*2
#define BUFFER_TEMP_INPUT_BYTES			BLOCKS_TEMP_OUTPUT*2//BLOCKS_TEMP_OUTPUT*4*2
#define BUFFER_PCMOUTPUT_BYTES			BLOCKS_PER_DECODE*4

//#define MV_APE_API
#ifdef MV_APE_USE_API
typedef struct _CircleBuffer {
	int m_nTotal;
    int m_nMaxDirectWriteBytes;
    int m_nEndCap;
    int m_nHead;
    int m_nTail;
    unsigned char * m_pBuffer;
} CircleBuffer;

typedef struct _RANGE_CODER_STRUCT_DECOMPRESS
{
    unsigned int low;       // low end of interval
    unsigned int range;     // length of interval
    unsigned int buffer;    // buffer for input/output
} RANGE_CODER_STRUCT_DECOMPRESS;

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

typedef struct _UNBIT_ARRAY_STATE
{
    uint32 k;
    uint32 nKSum;
} UNBIT_ARRAY_STATE;
#else
#include "MONKEY_UnBitArray.h"
#include "MONKEY_CircleBuffer.h"
#endif

typedef struct _MONKEY_APE_DEC_DATA
{
 	int		nLen;                   // bytes of the effective data pointed by pchData
 	int		counter;				// (inbuf)bytes of the data decoded/(outbuf)number of blocks decoded
    int		nLenAlloc;              // bytes of the allocated memory pointed by pchData
    char	*pchData;               // pointer of the allocated memory
} MONKEY_APE_DEC_DATA;

typedef struct _MONKEY_APE_DEC_CONFIG
{
	int nVersion;                                   // file version number * 1000 (3.93 = 3930)
    int nCompressionLevel;                          // the compression level
    int nFormatFlags;                               // format flags
    int nTotalFrames;                               // the total number frames (frames are used internally)
    int nBlocksPerFrame;                            // the samples in a frame (frames are used internally)
    int nFinalFrameBlocks;                          // the number of samples in the final frame
    int nChannels;                                  // audio channels
    int nSampleRate;                                // audio samples per second
    int nBitsPerSample;                             // audio bits per sample
    int nBytesPerSample;                            // audio bytes per sample

} MONKEY_APE_DEC_CONFIG;

typedef struct _MONKEY_APE_DEC_HANDLE 
{
    // start / finish information
    int m_nStartBlock;
    int m_nFinishBlock;
    int m_bIsRanged;
    int m_bDecompressorInitialized;

	int m_nCurrentFrame;
	int m_nCurrentBlock;
    int m_nCurrentFrameBufferBlock;
	int m_bErrorDecodingCurrentFrame;
	int64_t m_nCurrentTimeUs;

    // decoding tools
    unsigned int m_nCRC;
    unsigned int m_nStoredCRC;
    int m_nSpecialCodes;

	int nBlocksToOutput;
    
    // more decoding components
    CUnBitArray m_spUnBitArray;
    UNBIT_ARRAY_STATE m_BitArrayStateX;
    UNBIT_ARRAY_STATE m_BitArrayStateY;

    void * m_spNewPredictorX;
    void * m_spNewPredictorY;

    int m_nLastX;
    
    // decoding buffer
    CircleBuffer m_cbFrameBufferX;
	CircleBuffer m_cbFrameBufferY;
} MONKEY_APE_DEC_HANDLE;


int MONKEY_APEDecompress_Init(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo);
int MONKEY_APEDecompress_Term(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo);
int MONKEY_APEDecompress_Dec(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo, 
	MONKEY_APE_DEC_DATA * Input, MONKEY_APE_DEC_DATA * Output);
int MONKEY_APEDecompress_Dec_sprd(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo, int64_t TimeUs, char remainder, MONKEY_APE_DEC_DATA *input,MONKEY_APE_DEC_DATA *output);

#endif // #ifndef APE_APEDECOMPRESS_H
