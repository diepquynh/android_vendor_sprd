#ifndef APE_API_H
#define APE_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MONKEY_APEHeader.h"
#include "MONKEY_APECommon.h"
#include "MONKEY_APEDecompress.h"

typedef struct APE__StreamDecoder_tag
{
	//APE_FILE_INFO INFO;
	MONKEY_APE_DEC_CONFIG CONFIG;
	MONKEY_APE_DEC_HANDLE HANDLE;
	MONKEY_APE_DEC_DATA INPUT;
	MONKEY_APE_DEC_DATA OUTPUT;
}APE__StreamDecoder;

int MONKEY_APEDecompress_Init(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo);
int MONKEY_APEDecompress_Term(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo);
int MONKEY_APEDecompress_Dec(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo,
	MONKEY_APE_DEC_DATA * Input, MONKEY_APE_DEC_DATA * Output);

typedef int (*FT_APEDecompress_Init)(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo);
typedef int (*FT_APEDecompress_Term)(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo);
typedef int (*FT_APEDecompress_Dec)(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo,
    MONKEY_APE_DEC_DATA * Input, MONKEY_APE_DEC_DATA * Output);

#ifdef __cplusplus
}
#endif
#endif
