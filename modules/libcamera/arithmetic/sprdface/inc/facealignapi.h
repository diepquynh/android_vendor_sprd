/*-------------------------------------------------------------------*/
/*  Copyright(C) 2015 by Spreadtrum                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/
/*
    Face Alignment Library API
*/

#ifndef __SPRD_FACE_ALIGN_H__
#define __SPRD_FACE_ALIGN_H__

#if (defined( WIN32 ) || defined( WIN64 )) && (defined FAAPI_EXPORTS)
#define FA_EXPORTS __declspec(dllexport)
#else
#define FA_EXPORTS
#endif

#ifndef FAAPI
#define FAAPI(rettype) extern FA_EXPORTS rettype
#endif

#define FA_SHAPE_POINTNUM  7 /* Number of vertex on the face shape 68/49/7*/

/* The error codes */
#define FA_OK                   0   /* Ok!                                      */
#define FA_ERROR_INTERNAL       -1  /* Error: Unknown internal error            */
#define FA_ERROR_NOMEMORY       -2  /* Error: Memory allocation error           */
#define FA_ERROR_INVALIDARG     -3  /* Error: Invalid argument                  */

/* The gray-scale image structure */
typedef struct
{
    unsigned char *data;        /* Image data                        */
    int width;                  /* Image width                       */
    int height;                 /* Image height                      */
    int step;                   /* The byte count per scan line      */
}FA_IMAGE;

/* The face information structure */
typedef struct
{
    int x, y, width, height;    /* Face rectangle                    */
    int yawAngle;               /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees;   */
    int rollAngle;               /* In-plane rotation angle (Roll);   In (-180, +180] degrees; */
} FA_FACEINFO;


typedef struct
{
    int data[FA_SHAPE_POINTNUM*2];         /*Coordinates of landmarks [x0,y0,x1,y1, ..., xn,yn]*/
    int score;                              /*Confidence score of face shape*/
} FA_SHAPE;

/* The facle alignment detector handle*/
typedef void * FA_ALIGN_HANDLE;

#ifdef  __cplusplus
extern "C" {
#endif

/* Get the software version */
FAAPI(const char *) FaGetVersion();

/* Create a Face Alignment Detector handle*/
FAAPI(int) FaCreateAlignHandle(FA_ALIGN_HANDLE *hFA);

/* Release the Face Alignment Detector handle */
FAAPI(void) FaDeleteAlignHandle(FA_ALIGN_HANDLE *hFA);

/* Detect face landmarks based on face rectangle*/
FAAPI(int)  FaFaceAlign(FA_ALIGN_HANDLE hFA,
                        const FA_IMAGE *grayImage,
                        const FA_FACEINFO *face,
                        FA_SHAPE *shape);

/* Track face landmarks based on previous landamrks*/
FAAPI(int) FaFaceTrack(FA_ALIGN_HANDLE hFA,
                       const FA_IMAGE *grayImage,
                       const FA_SHAPE *preShape,
                       FA_SHAPE *curShape);

#ifdef  __cplusplus
}
#endif

#endif /* __SPRD_FACE_ALIGN_H__ */