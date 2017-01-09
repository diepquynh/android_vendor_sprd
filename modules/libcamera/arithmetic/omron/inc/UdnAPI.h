/*-------------------------------------------------------------------*/
/*  Copyright(C) 2003-2012 by UDN Corporation                      */
/*  All Rights Reserved.                                             */
/*                                                                   */
/*   This source code is the Confidential and Proprietary Property   */
/*   of UDN Corporation.  Any unauthorized use, reproduction or    */
/*   transfer of this software is strictly prohibited.               */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* 
    UDN_SDK Library API
*/
#ifndef UDNCOMAPI_H__
#define UDNCOMAPI_H__

#define UDN_API
#include "UdnDef.h"
#include "DetectionInfo.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*----- Get Version -----*/
UDN_API INT32      UDN_GetVersion(UINT8 *pbyMajor, UINT8 *pbyMinor);

/*----- Memory Management -----*/
UDN_API INT32      UDN_SetWMemoryArea(VOID *pMemoryAddr, UINT32 unSize);                      /* Work Memory Area Setting */
UDN_API INT32      UDN_SetBMemoryArea(VOID *pMemoryAddr, UINT32 unSize);                      /* Backup Memory Area Setting */
UDN_API INT32      UDN_FreeWMemoryArea(void);             /* Release Work Memory Area */
UDN_API INT32      UDN_FreeBMemoryArea(void);             /* Release Backup Memory Area */

/*----- BGR Data --> GRAY Data -----*/
UDN_API INT32 UDN_ImgConvertBGRtoGRAY(
    RAWIMAGE    *pSrcImage,         /* (I) BGR Image Data       */
    INT32       nWidth,             /* (I) Image Width          */
    INT32       nHeight,            /* (I) Image Height         */
    RAWIMAGE    *pGrayImage);       /* (O) GRAY Scale Data      */

/*----- YUV422 Data --> GRAY Data -----*/
UDN_API INT32 UDN_ImgConvertYUV422toGRAY(
    RAWIMAGE    *pSrcImage,         /* (I) YUV422 Image Data    */
    INT32       nWidth,             /* (I) Image Width          */
    INT32       nHeight,            /* (I) Image Height         */
    RAWIMAGE    *pGrayImage);       /* (O) GRAY Scale Data      */

/*----- BGR Data --> YUV422 Data -----*/
UDN_API INT32 UDN_ImgConvertBGRtoYUV422(
    RAWIMAGE    *pSrcImage,         /* (I) BGR Image Data       */
    INT32       nWidth,             /* (I) Image Width          */
    INT32       nHeight,            /* (I) Image Height         */
    RAWIMAGE    *pDstImage);        /* (O) YUV422 Image Data    */

/*----- YUV422 Data --> BGR Data -----*/
UDN_API INT32 UDN_ImgConvertYUV422toBGR(
    RAWIMAGE    *pSrcImage,         /* (I) YUV422 Image Data    */
    INT32       nWidth,             /* (I) Image Width          */
    INT32       nHeight,            /* (I) Image Height         */
    RAWIMAGE    *pDstImage);        /* (O) BGR Image Data       */

/*----- RGBAlpha Data --> GRAY Data -----*/
UDN_API INT32 UDN_ImgConvertRGBALPHAtoGRAY(
    RAWIMAGE    *pSrcImage,         /* (I) RGBAlpha Image Data  */
    INT32       nWidth,             /* (I) Image Width          */
    INT32       nHeight,            /* (I) Image Height         */
    RAWIMAGE    *pGrayImage);       /* (O) GRAY Scale Data      */

/*----- RGB565 Data --> YUV422 Data -----*/
UDN_API INT32 UDN_ImgConvertRGB565toYUV422(
    RAWIMAGE    *pSrcImage,         /* (I) RGB565 Image Data    */
    INT32       nWidth,             /* (I) Image Width          */
    INT32       nHeight,            /* (I) Image Height         */
    RAWIMAGE    *pDstImage);        /* (O) YUV422 Image Data    */

/*----- Conversion of Detection Information -----*/
UDN_API INT32 UDN_GetRectangleFromInfo(
    DETECTION_INFO *psDetectionInfo,/* (I) Detection Information */
    POINT *ptLeftTop,               /* (O) Top-left Corner       */
    POINT *ptRightTop,              /* (O) Top-right Corner      */
    POINT *ptLeftBottom,            /* (O) Bottom-left Corner    */
    POINT *ptRightBottom);          /* (O) Bottom-right Corner   */

/*----- Conversion of "coordinates of the center, size, and angle" from coordinates of the rectangle. -----*/
UDN_API INT32 UDN_ConvertRectToCenter(
    POINT ptLeftTop,                /* (I) Top-left Corner     */
    POINT ptRightTop,               /* (I) Top-right Corner    */
    POINT ptLeftBottom,             /* (I) Bottom-left Corner  */
    POINT ptRightBottom,            /* (I) Bottom-right Corner */
    POINT *pptCenter,               /* (O) Center Position     */
    INT32 *pnSize,                  /* (O) Size                */
    INT32 *pnAngle);                /* (O) Angle               */

#ifdef  __cplusplus
}
#endif

#endif  /* UDNCOMAPI_H__ */
