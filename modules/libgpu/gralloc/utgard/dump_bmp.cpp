#include <stdio.h>
#include <assert.h>

#include <hardware/gralloc.h>
#include <ui/GraphicBuffer.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <linux/fb.h>
#include <sys/stat.h>
#include "gralloc_priv.h"


#define BI_RGB          0
#define BI_BITFIELDS    3

typedef unsigned char BYTE, *PBYTE, *LPBYTE; 
typedef unsigned short WORD, *PWORD, *LPWORD; 
typedef unsigned long DWORD, *PDWORD, *LPDWORD;

typedef long LONG, *PLONG, *LPLONG; 

typedef BYTE  U8;
typedef WORD  U16;
typedef DWORD U32;

#pragma pack()

typedef unsigned char BYTE, *PBYTE, *LPBYTE; 
typedef unsigned short WORD, *PWORD, *LPWORD; 
typedef unsigned long DWORD, *PDWORD, *LPDWORD;

typedef long LONG, *PLONG, *LPLONG; 

typedef WORD  U16;
typedef DWORD U32;

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

typedef struct region{
    int left;
    int top;
    int right;
    int bottom;
}REGION, *PREGION;

typedef struct bufferInfo{
    int width;
    int height;
    int format;
}BINFO, *PBINFO;

//@function: dump bmp to a specific file
//@param: filename: the path of the file
//        addr: the address of the framebuffer
//        pBInfo: pointer to buffer info including width height and format
//        pScissor: pointer to the scissor region
//@return: void
void dump_bmp(const char* filename, void* addr, PBINFO pBInfo, PREGION pScissor)
{
    FILE* fp;
    WORD bfType;
    BITMAPINFO bmInfo;
    RGBQUAD quad;
    int pixel_size=0,i=0;
    void* tempAddr=NULL;

    fp = fopen(filename, "wb");
    if(!fp) goto fail_open;

    bfType = 0x4D42;
    memset(&bmInfo, 0, sizeof(BITMAPINFO));

    bmInfo.bmfHeader.bfOffBits = sizeof(WORD) + sizeof(BITMAPINFO);
    bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmInfo.bmiHeader.biWidth = pScissor->right-pScissor->left;
    bmInfo.bmiHeader.biHeight = pScissor->bottom-pScissor->top;
    bmInfo.bmiHeader.biPlanes = 1;

    switch (pBInfo->format)
    {
    case HAL_PIXEL_FORMAT_RGB_565:
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 16;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x001F;
        quad.rgbGreenMask    = 0x07E0;
        quad.rgbBlueMask     = 0xF800;
        quad.rgbReservedMask = 0;
        pixel_size=sizeof(U16);
        bmInfo.bmiHeader.biSizeImage = bmInfo.bmiHeader.biWidth * bmInfo.bmiHeader.biHeight * pixel_size;
        break;

    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 32;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x00FF0000;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x000000FF;
        quad.rgbReservedMask = 0xFF000000;
        pixel_size=sizeof(U32);
        bmInfo.bmiHeader.biSizeImage = bmInfo.bmiHeader.biWidth * bmInfo.bmiHeader.biHeight * pixel_size;
        break;

    case HAL_PIXEL_FORMAT_YCbCr_420_888:
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
        bmInfo.bmfHeader.bfOffBits += 256*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 8;
        bmInfo.bmiHeader.biCompression = BI_RGB;
        {
            for(int i=0; i<256; i++)
            {
                quad.table[i].rgbRed      = i;
                quad.table[i].rgbGreen    = i;
                quad.table[i].rgbBlue     = i;
                quad.table[i].rgbReserved = 0;
            }
        }
        pixel_size=sizeof(U8);
        bmInfo.bmiHeader.biSizeImage = bmInfo.bmiHeader.biWidth * bmInfo.bmiHeader.biHeight * pixel_size;
        break;

    default:
        assert(false);
    }

    bmInfo.bmfHeader.bfSize = bmInfo.bmfHeader.bfOffBits + bmInfo.bmiHeader.biSizeImage;

    fwrite(&bfType, sizeof(WORD), 1, fp);
    fwrite(&bmInfo, sizeof(BITMAPINFO), 1, fp);

    switch (pBInfo->format)
    {
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
      fwrite(&quad, 4*sizeof(U32), 1, fp);
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_888:
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
        fwrite(&quad, 256*sizeof(U32), 1, fp);
        break;
    }

    
    for(i=pScissor->bottom; i>pScissor->top; i--){
        tempAddr=(char*)addr + (i-1)*(pBInfo->width*pixel_size);
        fwrite((char*)tempAddr+(pScissor->left*pixel_size), bmInfo.bmiHeader.biWidth*pixel_size , 1, fp);
    }
    fclose(fp);
    return;

fail_open:    
    ALOGD("could not open dump_fb file");
}


//@function: dump framebuffer content
//@params: addr: the address of the buffer
//         info: frambuffer info
//         format: pixel format
//@return: void
void dump_fb(void* addr, struct fb_var_screeninfo * info , int format)
{
    static int i=0;
    REGION region={0,0,0,0};
    BINFO bInfo={0,0,0};
    FILE* fp=NULL;
    if((fp=fopen("/data/dump/fb_scissor","r"))){
        fscanf(fp,"%d,%d,%d,%d",&region.left,&region.top,&region.right,&region.bottom);
        fclose(fp);
        bInfo.width=info->xres;
        bInfo.height=info->yres;
        bInfo.format=format;
        if((region.left<0)||(region.right<0)||(region.top<0)||(region.bottom<0)){
            ALOGD("the params in fb_scissor should be positive");
            return;
        }
        if((region.left>bInfo.width)||(region.right>bInfo.width)||(region.top>bInfo.height)||(region.bottom>bInfo.height)){
            ALOGD("scissor region should be in [0,%d,0,%d]",bInfo.width,bInfo.height);
            return;
        }
        if((region.left>region.right)||(region.top>region.bottom)){
            ALOGD("the params in fb_scissor left < right, top < bottom");
            return;
        }
        i++;
        char filename[256];
        sprintf(filename, "/data/dump/fb_%04d.bmp",i);
        dump_bmp(filename, addr , &bInfo , &region);
    }
}
