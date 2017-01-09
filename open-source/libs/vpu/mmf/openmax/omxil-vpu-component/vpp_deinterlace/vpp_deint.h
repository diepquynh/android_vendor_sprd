#ifndef _DEINT_H_
#define _DEINT_H_

#include<stdio.h>
#include <utils/Log.h>

#define IMG_WIDTH 176
#define IMG_HEIGHT 144
#define INTERLEAVE 1
#define THRESHOLD 20
#define MAX_DECODE_FRAME_NUM 25

typedef unsigned char	BOOLEAN;
typedef unsigned char	uint8;
typedef signed char	int8;
typedef signed short	int16;
typedef unsigned short	uint16;
typedef signed int		int32;
typedef unsigned int	uint32;


//#define  SCI_TRACE_LOW(x...)   fprintf(stdout, ##x) 

//#define  SCI_TRACE_LOW(x...)   DEBUG(DEB_LEV_FUNCTION_NAME, ##x)

#define  SCI_TRACE_LOW ALOGI


//Deinterlace reg define
#define VPP_BASE_ADDR 0 

#define VPP_CFG     	(VPP_BASE_ADDR + 0x0000)
#define VPP_INT_STS 	(VPP_BASE_ADDR + 0x0010)
#define VPP_INT_MSK 	(VPP_BASE_ADDR + 0x0014)
#define VPP_INT_CLR 	(VPP_BASE_ADDR + 0x0018)
#define VPP_INT_RAW 	(VPP_BASE_ADDR + 0x001C)

#define AXIM_BURST_GAP   (VPP_BASE_ADDR + 0x0020)
#define AXIM_ENDIAN_SET  (VPP_BASE_ADDR + 0x0024)
#define AXIM_BURST_PAUSE (VPP_BASE_ADDR + 0x0028)
#define AXIM_STS         (VPP_BASE_ADDR + 0x002C)

#define DEINT_PATH_CFG   	(VPP_BASE_ADDR + 0x2000)
#define DEINT_PATH_START   	(VPP_BASE_ADDR + 0x2004)
#define DEINT_IMG_SIZE   	(VPP_BASE_ADDR + 0x2008)

#define DEINT_SRC_Y_ADDR  	(VPP_BASE_ADDR + 0x2010)
#define DEINT_SRC_UV_ADDR 	(VPP_BASE_ADDR + 0x2014)
#define DEINT_SRC_V_ADDR  	(VPP_BASE_ADDR + 0x2018)
#define DEINT_DST_Y_ADDR  	(VPP_BASE_ADDR + 0x201C)
#define DEINT_DST_UV_ADDR 	(VPP_BASE_ADDR + 0x2020)
#define DEINT_DST_V_ADDR  	(VPP_BASE_ADDR + 0x2024)
#define DEINT_REF_Y_ADDR  	(VPP_BASE_ADDR + 0x2028)
#define DEINT_REF_UV_ADDR 	(VPP_BASE_ADDR + 0x202C)
#define DEINT_REF_V_ADDR  	(VPP_BASE_ADDR + 0x2030)



typedef struct DeintParams{
    uint32 width;
    uint32 height;

    uint8  interleave;
    uint8  threshold;

    uint32 y_len;
    uint32 c_len;
}DEINT_PARAMS_T;


typedef enum
{
    SRC_FRAME_ADDR = 0,  
    REF_FRAME_ADDR, 
    DST_FRAME_ADDR,  
    MAX_FRAME_NUM
} CODEC_BUF_TYPE;

#endif
