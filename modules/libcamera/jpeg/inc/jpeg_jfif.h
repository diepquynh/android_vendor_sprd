#ifndef _JPEG_JFIF_H_
#define _JPEG_JFIF_H_

#include "sci_types.h"

#define JPEG_H_V_RATIO_YUV422			0x21
#define JPEG_H_V_RATIO_YUV422_1		0x12
#define JPEG_H_V_RATIO_YUV420			0x22
#define JPEG_H_V_RATIO_YUV444			0x11
#define JPEG_H_V_RATIO_YUV411			0x41
#define JPEG_H_V_RATIO_YUV411_1               0x14

/* JPEG marker codes */
typedef enum
{
	M_SOF0  = 0xc0,		  /* Baseline. */
	M_SOF1  = 0xc1,		  /* Extended sequential. */
	M_SOF2  = 0xc2,		  /* Progressive. */
	M_SOF3  = 0xc3,		  /* Lossless. */
	M_SOF5  = 0xc5,		  /* Differential sequential. */
	M_SOF6  = 0xc6,		  /* Differential progressive. */
	M_SOF7  = 0xc7,		  /* Differential lossless. */
	M_JPG   = 0xc8,
	M_SOF9  = 0xc9,		  /* Extended sequential, arithmetic coding. */
	M_SOF10 = 0xca,		  /* Progressive, arithmetic coding. */
	M_SOF11 = 0xcb,		  /* Lossless, arithmetic coding. */
	M_SOF13 = 0xcd,		  /* Differential sequential, arithmetic coding. */
	M_SOF14 = 0xce,		  /* Differential progressive, arithmetic coding. */
	M_SOF15 = 0xcf,		  /* Differential lossless, arithmetic coding. */

	M_DHT   = 0xc4,

	M_DAC   = 0xcc,

	M_RST0  = 0xd0,
	M_RST1  = 0xd1,
	M_RST2  = 0xd2,
	M_RST3  = 0xd3,
	M_RST4  = 0xd4,
	M_RST5  = 0xd5,
	M_RST6  = 0xd6,
	M_RST7  = 0xd7,

	M_SOI   = 0xd8,
	M_EOI   = 0xd9,
	M_SOS   = 0xda,
	M_DQT   = 0xdb,
	M_DNL   = 0xdc,
	M_DRI   = 0xdd,
	M_DHP   = 0xde,
	M_EXP   = 0xdf,

	M_APP0  = 0xe0,
	M_APP1  = 0xe1,
	M_APP2  = 0xe2,
	M_APP3   = 0xe3,
	M_APP4   = 0xe4,
	M_APP5   = 0xe5,
	M_APP6   = 0xe6,
	M_APP7   = 0xe7,
	M_APP8   = 0xe8,
	M_APP9   = 0xe9,
	M_APP10  = 0xea,
	M_APP11  = 0xeb,
	M_APP12  = 0xec,
	M_APP13  = 0xed,
	M_APP14  = 0xee,
	M_APP15 = 0xef,

	M_JPG0  = 0xf0,
	M_JPG13 = 0xfd,
	M_COM   = 0xfe,

	M_TEM   = 0x01,

	M_ERROR = 0x100,

	M_MARKER = 0xff
} JPEG_MARKER_E;




#endif