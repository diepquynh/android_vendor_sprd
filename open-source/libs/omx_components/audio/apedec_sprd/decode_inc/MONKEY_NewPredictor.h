#ifndef APE_NEWPREDICTOR_H
#define APE_NEWPREDICTOR_H

#include "MONKEY_RollBuffer.h"
#include "MONKEY_NNFilter.h"
#include "MONKEY_ScaledFirstOrderFilter.h"


#define WINDOW_BLOCKS           512
#define BUFFER_COUNT            1
#define HISTORY_ELEMENTS        8
#define M_COUNT                 8

typedef struct _CPredictorCompressNormal {
    // buffer information
    CRollBufferFast m_rbPrediction;
    CRollBufferFast m_rbAdapt;

    CScaledFirstOrderFilter m_Stage1FilterA;
    CScaledFirstOrderFilter m_Stage1FilterB;

    // adaption
    int m_aryM[9];
    
    // other
    int m_nCurrentIndex;
    CNNFilter * m_pNNFilter;
    CNNFilter * m_pNNFilter1;
    CNNFilter * m_pNNFilter2;
} CPredictorCompressNormal;

int MONKEY_CPredictorCompressNormal(CPredictorCompressNormal * CCPredictorCompressNormal, int nCompressionLevel);
int MONKEY_CPredictorCompressNormal_Finish(CPredictorCompressNormal * CCPredictorCompressNormal);
int MONKEY_CPredictorCompressNormal_CompressValue(CPredictorCompressNormal * CCPredictorCompressNormal, int nA, int nB);
int MONKEY_CPredictorCompressNormal_Flush(CPredictorCompressNormal * CCPredictorCompressNormal);


typedef struct _CPredictorDecompressNormal3930to3950 {
    // buffer information
    int * m_pBuffer[BUFFER_COUNT];

    // adaption
    int m_aryM[M_COUNT];
    
    // buffer pointers
    int * m_pInputBuffer;

    // other
    int m_nCurrentIndex;
    int m_nLastValue;
    CNNFilter * m_pNNFilter;
    CNNFilter * m_pNNFilter1;
} CPredictorDecompressNormal3930to3950;

int MONKEY_CPredictorDecompressNormal3930to3950(CPredictorDecompressNormal3930to3950 * CCPredictorDecompressNormal3930to3950, int nCompressionLevel, int nVersion);
int MONKEY_CPredictorDecompressNormal3930to3950_Finish(CPredictorDecompressNormal3930to3950 * CCPredictorDecompressNormal3930to3950);
int MONKEY_CPredictorDecompressNormal3930to3950_DecompressValue(CPredictorDecompressNormal3930to3950 * CCPredictorDecompressNormal3930to3950, int nInput);
int MONKEY_CPredictorDecompressNormal3930to3950_Flush(CPredictorDecompressNormal3930to3950 * CCPredictorDecompressNormal3930to3950);



typedef struct _CPredictorDecompress3950toCurrent {
	// adaption
    int m_aryMA[M_COUNT];
    int m_aryMB[M_COUNT];
    
    // buffer pointers
    CRollBufferFast m_rbPredictionA;
    CRollBufferFast m_rbPredictionB;

    CRollBufferFast m_rbAdaptA;
    CRollBufferFast m_rbAdaptB;

    CScaledFirstOrderFilter m_Stage1FilterA;
    CScaledFirstOrderFilter m_Stage1FilterB;

    // other
    int m_nCurrentIndex;
    int m_nLastValueA;
    int m_nVersion;
    CNNFilter * m_pNNFilter;
    CNNFilter * m_pNNFilter1;
    CNNFilter * m_pNNFilter2;
} CPredictorDecompress3950toCurrent;


int MONKEY_CPredictorDecompress3950toCurrent(CPredictorDecompress3950toCurrent * CCPredictorDecompress3950toCurrent, int nCompressionLevel, int nVersion);
int MONKEY_CPredictorDecompress3950toCurrent_Finish(CPredictorDecompress3950toCurrent * CCPredictorDecompress3950toCurrent);
int MONKEY_CPredictorDecompress3950toCurrent_DecompressValue(CPredictorDecompress3950toCurrent * CCPredictorDecompress3950toCurrent, int nA, int nB);
int MONKEY_CPredictorDecompress3950toCurrent_Flush(CPredictorDecompress3950toCurrent * CCPredictorDecompress3950toCurrent);


#endif // #ifndef APE_NEWPREDICTOR_H
