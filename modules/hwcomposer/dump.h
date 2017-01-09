#ifndef _DUMP_HWCOMPOSER_BMP_H_
#define _DUMP_HWCOMPOSER_BMP_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <system/graphics.h>
#include<stdlib.h>
#include <cutils/log.h>

//LOCAL_SHARED_LIBRARIES := libcutils
#include <cutils/properties.h>
#include <hardware/hwcomposer.h>
#include <hardware/hardware.h>
#include "gralloc_priv.h"
#include "sprd_fb.h"

#define BI_RGB          0
#define BI_BITFIELDS    3
#define MAX_DUMP_PATH_LENGTH 100
#define MAX_DUMP_FILENAME_LENGTH 100
typedef unsigned char BYTE, *PBYTE, *LPBYTE;
typedef unsigned short WORD, *PWORD, *LPWORD;
typedef int32_t DWORD, *PDWORD, *LPDWORD;

typedef int32_t LONG, *PLONG, *LPLONG;

typedef BYTE  U8;
typedef WORD  U16;
typedef DWORD U32;

#pragma pack()
enum {
    DUMP_AT_HWCOMPOSER_HWC_SET,
    DUMP_AT_HWCOMPOSER_HWC_PREPARE
};
typedef struct tagBITMAPFILEHEADER {
  DWORD bfSize;
  WORD  bfReserved1;
  WORD  bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    union {
        struct {
          DWORD rgbBlueMask;
          DWORD rgbGreenMask;
          DWORD rgbRedMask;
          DWORD rgbReservedMask;
        };
        struct {
          BYTE rgbBlue;
          BYTE rgbGreen;
          BYTE rgbRed;
          BYTE rgbReserved;
        } table[256];
    };
} RGBQUAD;

typedef struct tagBITMAPINFO {
  BITMAPFILEHEADER bmfHeader;
  BITMAPINFOHEADER bmiHeader;
} BITMAPINFO;
typedef enum
{
    HWCOMPOSER_DUMP_ORIGINAL_LAYERS = 0x01,
    HWCOMPOSER_DUMP_ORIGINAL_VD_LAYERS = 0x10,
    HWCOMPOSER_DUMP_VIDEO_OVERLAY_FLAG = 0x2,
    HWCOMPOSER_DUMP_OSD_OVERLAY_FLAG = 0x4,
    HWCOMPOSER_DUMP_FRAMEBUFFER_FLAG = 0x8,
    HWCOMPOSER_DUMP_VD_OVERLAY_FLAG = 0x20,
    HWCOMPOSER_DUMP_MULTI_LAYER_FLAG = 0x40 // when GSP process multi-layer by multi-times GSP calling, dump the middle result
} dump_type;


extern void queryDebugFlag(int *debugFlag);

extern void queryDumpFlag(int *dumpFlag);
extern void queryIntFlag(const char* strProperty,int *IntFlag);


extern int dumpImage(hwc_display_contents_1_t *list);

extern int dumpOverlayImage(private_handle_t* buffer, const char* name);

void dumpFrameBuffer(char *virAddr, const char* ptype, int width, int height, int format);
#endif
