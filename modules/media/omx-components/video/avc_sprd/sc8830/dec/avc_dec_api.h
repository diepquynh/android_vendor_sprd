/******************************************************************************
 ** File Name:    h264dec.h                                                   *
 ** Author:                                     		                      *
 ** DATE:         3/15/2007                                                   *
 ** Copyright:    2007 Spreadtrum, Incorporated. All Rights Reserved.         *
 ** Description:  define data structures for Video Codec                      *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 3/15/2007     			      Create.                                     *
 *****************************************************************************/
#ifndef _H264_DEC_H_
#define _H264_DEC_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
//#include "mmcodec.h"
/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

typedef unsigned char		BOOLEAN;
//typedef unsigned char		Bool;
typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
typedef unsigned long long 	uint64;
typedef signed long long 	int64;

typedef signed char			int8;
typedef signed short		int16;
typedef signed int			int32;

/**
This enumeration is for profiles. The value follows the profile_idc  in sequence
parameter set rbsp. See Annex A.
@publishedAll
*/
typedef enum
{
    AVC_BASELINE = 66,
    AVC_MAIN = 77,
    AVC_EXTENDED = 88,
    AVC_HIGH = 100,
    AVC_HIGH10 = 110,
    AVC_HIGH422 = 122,
    AVC_HIGH444 = 144
} AVCProfile;

/**
This enumeration is for levels. The value follows the level_idc in sequence
parameter set rbsp. See Annex A.
@published All
*/
typedef enum
{
    AVC_LEVEL_AUTO = 0,
    AVC_LEVEL1_B = 9,
    AVC_LEVEL1 = 10,
    AVC_LEVEL1_1 = 11,
    AVC_LEVEL1_2 = 12,
    AVC_LEVEL1_3 = 13,
    AVC_LEVEL2 = 20,
    AVC_LEVEL2_1 = 21,
    AVC_LEVEL2_2 = 22,
    AVC_LEVEL3 = 30,
    AVC_LEVEL3_1 = 31,
    AVC_LEVEL3_2 = 32,
    AVC_LEVEL4 = 40,
    AVC_LEVEL4_1 = 41,
    AVC_LEVEL4_2 = 42,
    AVC_LEVEL5 = 50,
    AVC_LEVEL5_1 = 51
} AVCLevel;

// Decoder video capability structure
typedef struct
{
    AVCProfile profile;
    AVCLevel   level;
    uint32 max_width;
    uint32 max_height;
} MMDecCapability;

typedef enum
{
    MMDEC_OK = 0,
    MMDEC_ERROR = -1,
    MMDEC_PARAM_ERROR = -2,
    MMDEC_MEMORY_ERROR = -3,
    MMDEC_INVALID_STATUS = -4,
    MMDEC_STREAM_ERROR = -5,
    MMDEC_OUTPUT_BUFFER_OVERFLOW = -6,
    MMDEC_HW_ERROR = -7,
    MMDEC_NOT_SUPPORTED = -8,
    MMDEC_FRAME_SEEK_IVOP = -9,
    MMDEC_MEMORY_ALLOCED = -10
} MMDecRet;

/*standard*/
typedef enum {
    ITU_H263 = 0,
    MPEG4,
    JPEG,
    FLV_V1,
    H264,
    RV8,
    RV9
} VIDEO_STANDARD_E;

typedef enum
{
    YUV420P_YU12 = 0,
    YUV420P_YV12 = 1,
    YUV420SP_NV12 = 2,   /*u/v interleaved*/
    YUV420SP_NV21 = 3,   /*v/u interleaved*/
} MM_YUV_FORMAT_E;

// decoder video format structure
typedef struct
{
    int32 	video_std;			//video standard, 0: VSP_ITU_H263, 1: VSP_MPEG4, 2: VSP_JPEG, 3: VSP_FLV_V1
    int32	frame_width;
    int32	frame_height;
    int32	i_extra;
    void 	*p_extra;
    unsigned long p_extra_phy;
    //int32	uv_interleaved;
    int32   yuv_format;
} MMDecVideoFormat;

// Decoder buffer for decoding structure
typedef struct
{
    uint8	*common_buffer_ptr;     // Pointer to buffer used when decoding
    unsigned long common_buffer_ptr_phy;
    uint32	size;            		// Number of bytes decoding buffer

    int32 	frameBfr_num;			//YUV frame buffer number

    uint8   *int_buffer_ptr;		// internal memory address
    int32 	int_size;				//internal memory size
} MMCodecBuffer;

typedef struct
{
    uint16 start_pos;
    uint16 end_pos;
} ERR_POS_T;

#define MAX_ERR_PKT_NUM		30

// Decoder input structure
typedef struct
{
    uint8		*pStream;          	// Pointer to stream to be decoded
    unsigned long pStream_phy;          	// Pointer to stream to be decoded, phy
    uint32		dataLen;           	// Number of bytes to be decoded
    int32		beLastFrm;			// whether the frame is the last frame.  1: yes,   0: no

    int32		expected_IVOP;		// control flag, seek for IVOP,
    uint64		nTimeStamp;                // time stamp, it is PTS or DTS

    int32		beDisplayed;		// whether the frame to be displayed    1: display   0: not //display

    int32		err_pkt_num;		// error packet number
    ERR_POS_T	err_pkt_pos[MAX_ERR_PKT_NUM];		// position of each error packet in bitstream
} MMDecInput;

// Decoder output structure
typedef struct
{
    uint8	*pOutFrameY;     //Pointer to the recent decoded picture
    uint8	*pOutFrameU;
    uint8	*pOutFrameV;

    uint32	frame_width;
    uint32	frame_height;

    int32   is_transposed;	//the picture is transposed or not, in 8800S4, it should always 0.

    uint64	pts;            //presentation time stamp
    int32	frameEffective;

    int32	err_MB_num;		//error MB number
    void *pBufferHeader;
    int reqNewBuf;
    int32 mPicId;

    BOOLEAN sawSPS;
    BOOLEAN sawPPS;
} MMDecOutput;

typedef enum
{
    INTER_MEM = 0,  /*internal memory, only for software writing and reading and initialized when initialize decoder*/
    HW_NO_CACHABLE, /*physical continuous and no-cachable, only for VSP writing and reading */
    HW_CACHABLE,    /*physical continuous and cachable, for software writing and VSP reading */
    SW_CACHABLE,    /*only for software writing and reading*/
    MAX_MEM_TYPE
} CODEC_BUF_TYPE;

typedef struct
{
    uint32 cropLeftOffset;
    uint32 cropOutWidth;
    uint32 cropTopOffset;
    uint32 cropOutHeight;
} CropParams;

typedef struct
{
    uint32 profile;
    uint32 picWidth;
    uint32 picHeight;
    uint32 videoRange;
    uint32 matrixCoefficients;
    uint32 parWidth;
    uint32 parHeight;
    uint32 croppingFlag;
    CropParams cropParams;
    uint32 numRefFrames;
    uint32 has_b_frames;
} H264SwDecInfo;

typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader);
typedef int (*FunctionType_MallocCB)(void* aUserData, unsigned int size_extra);
typedef int (*FunctionType_MbinfoMallocCB)(void* aUserData, unsigned int size_mbinfo, unsigned long *pPhyAddr);

/* Application controls, this structed shall be allocated */
/*    and initialized in the application.                 */
typedef struct tagAVCHandle
{
    /* The following fucntion pointer is copied to BitstreamDecVideo structure  */
    /*    upon initialization and never used again. */
//    int (*readBitstreamData)(uint8 *buf, int nbytes_required, void *appData);
//    applicationData appData;

//    uint8 *outputFrame;
    void *videoDecoderData;     /* this is an internal pointer that is only used */
    /* in the decoder library.   */
#ifdef PV_MEMORY_POOL
    int32 size;
#endif

    void *userdata;

    FunctionType_BufCB VSP_bindCb;
    FunctionType_BufCB VSP_unbindCb;
    FunctionType_MallocCB VSP_extMemCb;
    FunctionType_MbinfoMallocCB VSP_mbinfoMemCb;
} AVCHandle;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

MMDecRet H264DecGetNALType(AVCHandle *avcHandle, uint8 *bitstream, int size, int *nal_type, int *nal_ref_idc);
MMDecRet H264DecGetInfo(AVCHandle *avcHandle, H264SwDecInfo *pDecInfo);
MMDecRet H264GetCodecCapability(AVCHandle *avcHandle, MMDecCapability *Capability);
MMDecRet H264DecSetParameter(AVCHandle *avcHandle, MMDecVideoFormat * pVideoFormat);

/*****************************************************************************/
//  Description: Init h264 decoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet H264DecInit(AVCHandle *avcHandle, MMCodecBuffer * pBuffer,MMDecVideoFormat * pVideoFormat);

/*****************************************************************************/
//  Description: Init h264 decoder	memory
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet H264DecMemInit(AVCHandle *avcHandle, MMCodecBuffer *pBuffer);

/*****************************************************************************/
//  Description: Decode one vop
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet H264DecDecode(AVCHandle *avcHandle, MMDecInput *pInput,MMDecOutput *pOutput);

/*****************************************************************************/
//  Description: Close h264 decoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet H264DecRelease(AVCHandle *avcHandle);

void H264Dec_ReleaseRefBuffers(AVCHandle *avcHandle);
MMDecRet H264Dec_GetLastDspFrm(AVCHandle *avcHandle, void **pOutput, int32 *picId, uint64 *pts);
void H264Dec_SetCurRecPic(AVCHandle *avcHandle, uint8 *pFrameY,uint8 *pFrameY_phy,void *pBufferHeader, int32 picId);


typedef MMDecRet (*FT_H264DecGetNALType)(AVCHandle *avcHandle, uint8 *bitstream, int size, int *nal_type, int *nal_ref_idc);
typedef void (*FT_H264GetBufferDimensions)(AVCHandle *avcHandle, int32 *aligned_width, int32 *aligned_height) ;
typedef MMDecRet (*FT_H264DecInit)(AVCHandle *avcHandle, MMCodecBuffer * pBuffer,MMDecVideoFormat * pVideoFormat);
typedef MMDecRet (*FT_H264DecGetInfo)(AVCHandle *avcHandle, H264SwDecInfo *pDecInfo);
typedef MMDecRet (*FT_H264GetCodecCapability)(AVCHandle *avcHandle, MMDecCapability *Capability);
typedef MMDecRet (*FT_H264DecMemInit)(AVCHandle *avcHandle, MMCodecBuffer *pBuffer);
typedef MMDecRet (*FT_H264DecDecode)(AVCHandle *avcHandle, MMDecInput *pInput,MMDecOutput *pOutput);
typedef MMDecRet (*FT_H264DecRelease)(AVCHandle *avcHandle);
typedef void (*FT_H264Dec_SetCurRecPic)(AVCHandle *avcHandle, uint8 *pFrameY,uint8 *pFrameY_phy,void *pBufferHeader, int32 picId);
typedef MMDecRet (*FT_H264Dec_GetLastDspFrm)(AVCHandle *avcHandle, void **pOutput, int32 *picId, uint64 *pts);
typedef void (*FT_H264Dec_ReleaseRefBuffers)(AVCHandle *avcHandle);
typedef MMDecRet (*FT_H264DecSetparam)(AVCHandle *avcHandle, MMDecVideoFormat * pVideoFormat);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif //_H264_DEC_H_
// End
