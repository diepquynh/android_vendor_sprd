/*-------------------------------------------------------------------*/
/*  Copyright(C) 2015 by Spreadtrum                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/
/*
    Face Detection Library API
*/

#ifndef __SPRD_FDAPI_H__
#define __SPRD_FDAPI_H__

#if (defined( WIN32 ) || defined( WIN64 )) && (defined FDAPI_EXPORTS)
#define FD_EXPORTS __declspec(dllexport)
#else
#define FD_EXPORTS
#endif

#ifndef FDAPI
#define FDAPI(rettype) extern FD_EXPORTS rettype
#endif

/* The error codes */
#define FD_OK                   0     /* Ok!                                      */
#define FD_ERROR_INTERNAL       -1    /* Error: Unknown internal error            */
#define FD_ERROR_NOMEMORY       -2    /* Error: Memory allocation error           */
#define FD_ERROR_INVALIDARG     -3    /* Error: Invalid argument                  */

/* Face direction definitions */
#define FD_DIRECTION_0          0x01  /* Face direction: upright direction        */
#define FD_DIRECTION_90         0x02  /* Face direction: right-rotate 90 degrees  */
#define FD_DIRECTION_180        0x04  /* Face direction: right-rotate 180 degrees */
#define FD_DIRECTION_270        0x08  /* Face direction: right-rotate 270 degrees */
#define FD_DIRECTION_ALL        0x0F  /* Face direction: all directions           */

/* Face angle range on each face direction */
#define FD_ANGLE_NONE           0x00  /* Each face direction cover 0 degreess, which means not to do face detection */
#define FD_ANGLE_RANGE_30       0x01  /* Each face direction cover 30 degrees:[-15,+15] */
#define FD_ANGLE_RANGE_90       0x02  /* Each face direction cover 90 degrees:[-45,+45] */

#define FD_WORKMODE_STILL       0x00  /* Still mode: only detection               */
#define FD_WORKMODE_MOVIE       0x01  /* Movie mode: detection + tracking         */
#define FD_WORKMODE_DEFAULT     FD_WORKMODE_STILL /* the default work mode        */

/* The gray-scale image structure */
typedef struct
{
    unsigned char *data;              /* Image data                               */
    int width;                        /* Image width                              */
    int height;                       /* Image height                             */
    int step;                         /* The byte count per scan line             */
}FD_IMAGE;

/* The face information structure */
typedef struct {
    int x, y, width, height;          /* Face rectangle                           */
    int yawAngle;                     /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees;   */
    int rollAngle;                    /* In-plane rotation angle (Roll); In (-180, +180] degrees;   */
    int score;                        /* Confidence score; In [0, 1000]           */
    int id;                           /* Human ID Number                          */
} FD_FACEINFO;

/* Face Detection option */
typedef struct {
    unsigned int workMode;           /* Work mode: FD_WORKMODE_STILL or FD_WORKMODE_MOVIE           */

    /* options for both still mode and movie mode */
    unsigned int maxFaceNum;         /* Maximum face number to detect. (In [1, 1024])               */
    unsigned int minFaceSize;        /* Minimum face size to detect.(In [20, 8192], default:20)     */
    unsigned int maxFaceSize;        /* Maximum face size to detect.(In [20, 8192], default:8192)   */
    unsigned int directions;         /* Face directions to detect.(default: FD_DIRECTION_0)    */
    unsigned int angleFrontal;       /* Frontal face: angle range for each direction. (default: FD_ANGLE_RANGE_90) */
    unsigned int angleHalfProfile;   /* Half profile face: angle range for each direction. (default: FD_ANGLE_RANGE_90) */
    unsigned int angleFullProfile;   /* Full profile face: angle range for each direction. (default: FD_ANGLE_RANGE_90) */
    unsigned int detectDensity;      /* Face detection density.(In [3, 10]);                        */
    unsigned int scoreThreshold;     /* The threshold for confidence score. (In [0, 1000], default:0)  */

    /* options for movie mode only */
    unsigned int initFrames;         /* Divide the initial face searching task into N frames; (In [1, 3])                    */
    unsigned int detectFrames;       /* Divide the new face searching task into N frames; (In [1, 3])                        */
    unsigned int detectInterval;     /* The frame intervals between the searches for new faces during tracking; (In [0, 50]) */
    unsigned int trackDensity;       /* Face tracking density.(In [3, 10])                                                   */
    unsigned int lostRetryCount;     /* The number of frames to try to search a face that was lost during tracking. (In [0, 10]) */
    unsigned int lostHoldCount;      /* The number of frames to keep outputting a lost face. (In [0, 10]);                   */
    unsigned int holdPositionRate;   /* If the displacement rate during tracking is below the rate, the face center will be corrected back to the previous one. (In [0, 20]) */
    unsigned int holdSizeRate;       /* If the size change during tracking is below the rate, the face size will be corrected back to the previous one. (In [0, 30]) */
    unsigned int swapFaceRate;       /* When the detected face count is larger than "maxFaceNum", only if the new face is larger than the old face by the rate, the old face is replaced by the new face. */
    unsigned int guessFaceDirection; /* 1-->TRUE; 0 --> FALSE; If set as TRUE, new face search will only be performed on the guessed directions, which can speed up the detection */
} FD_OPTION;

/* Face Detector handle */
typedef void * FD_DETECTOR_HANDLE;


#ifdef  __cplusplus
extern "C" {
#endif

/* Get the software version */
FDAPI(const char *) FdGetVersion();

/* Init the FD_OPTION structure by default values */
FDAPI(void) FdInitOption(FD_OPTION *option);

/* Create a Face Detector handle according to the input option */
FDAPI(int)  FdCreateDetector(FD_DETECTOR_HANDLE *hDT, const FD_OPTION *option);

/* Release the Face Detector handle */
FDAPI(void) FdDeleteDetector(FD_DETECTOR_HANDLE *hDT);

/* Detect face on the input gray-scale image */
FDAPI(int)  FdDetectFace(FD_DETECTOR_HANDLE hDT, const FD_IMAGE *grayImage);

/* Clear the faces detected in the previous image */
FDAPI(int) FdClearFace(FD_DETECTOR_HANDLE hDT);

/* Get the detected face count */
FDAPI(int)  FdGetFaceCount(const FD_DETECTOR_HANDLE hDT);

/* Get the face information at the specified index */
FDAPI(int)  FdGetFaceInfo(const FD_DETECTOR_HANDLE hDT, int faceIndex, FD_FACEINFO *faceInfo);


#ifdef  __cplusplus
}
#endif

#endif /* __SPRD_FDAPI_H__ */
