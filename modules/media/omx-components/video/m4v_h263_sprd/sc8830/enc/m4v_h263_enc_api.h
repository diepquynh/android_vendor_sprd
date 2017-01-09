/******************************************************************************
 ** File Name:    mpeg4enc.h                                                   *
 ** Author:                                     		                      *
 ** DATE:         3/15/2007                                                   *
 ** Copyright:    2007 Spreadtrum, Incorporated. All Rights Reserved.           *
 ** Description:  define data structures for Video Codec                      *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 3/15/2007     			      Create.                                     *
 *****************************************************************************/
#ifndef _MP4_H263_ENC_API_H_
#define _MP4_H263_ENC_API_H_

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
//typedef unsigned int		uint;

typedef signed char			int8;
typedef signed short		int16;
typedef signed int			int32;

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

// Encoder video capability structure
typedef struct
{
    int32 max_width;
    int32 max_height;
} MMEncCapability;

typedef enum
{
    MMENC_OK = 0,
    MMENC_ERROR = -1,
    MMENC_PARAM_ERROR = -2,
    MMENC_MEMORY_ERROR = -3,
    MMENC_INVALID_STATUS = -4,
    MMENC_OUTPUT_BUFFER_OVERFLOW = -5,
    MMENC_HW_ERROR = -6
} MMEncRet;

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

typedef MMCodecBuffer MMEncBuffer;


typedef enum
{
    MMENC_YUV420P_YU12 = 0,
    MMENC_YUV420P_YV12 = 1,
    MMENC_YUV420SP_NV12 = 2,   /*u/v interleaved*/
    MMENC_YUV420SP_NV21 = 3,   /*v/u interleaved*/
} MMENC_YUV_FORMAT_E;

// Encoder video format structure
typedef struct
{
    int32	is_h263;					// 1 : H.263, 0 : MP4
    int32	frame_width;				//frame width
    int32	frame_height;				//frame Height
    int32	time_scale;
    //int32 	uv_interleaved;				//tmp add
    int32    yuv_format;
    int32    b_anti_shake;
} MMEncVideoInfo;

// Encoder config structure
typedef struct
{
    uint32	RateCtrlEnable;            // 0 : disable  1: enable
    uint32	targetBitRate;             // 400 ~  (bit/s)
    uint32  FrameRate;
    uint32  PFrames;

    uint32	vbv_buf_size;				//vbv buffer size, to determine the max transfer delay

    uint32	QP_IVOP;     				// first I frame's QP; 1 ~ 31, default QP value if the Rate Control is disabled
    uint32	QP_PVOP;     				// first P frame's QP; 1 ~ 31, default QP value if the Rate Control is disabled

    uint32	h263En;            			// 1 : H.263, 0 : MP4

    uint32	profileAndLevel;
    uint32  EncSceneMode;
} MMEncConfig;

// Encoder input structure
typedef struct
{
    uint8   *p_src_y;
    uint8   *p_src_u;
    uint8   *p_src_v;

    uint8   *p_src_y_phy;
    uint8   *p_src_u_phy;
    uint8   *p_src_v_phy;

    bool	needIVOP;
    int32	time_stamp;					//time stamp
    int32   bs_remain_len;				//remained bitstream length
    int32 	channel_quality;			//0: good, 1: ok, 2: poor
    int32    org_img_width;
    int32    org_img_height;
    int32    crop_x;
    int32    crop_y;
} MMEncIn;

// Encoder output structure
typedef struct
{
    uint8	*pOutBuf;					//Output buffer
    int32	strmSize;					//encoded stream size, if 0, should skip this frame.
    int32	vopType;						//0: I VOP, 1: P VOP, 2: B VOP
} MMEncOut;


typedef struct tagMP4Handle
{
    void            *videoEncoderData;
//    int             videoEncoderInit;
    void *userData;

} MP4Handle;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
/*****************************************************************************/
//  Description:   Get capability of MPEG4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncGetCodecCapability(MP4Handle *mp4Handle, MMEncCapability *Capability);

/*****************************************************************************/
//  Description:   Pre-Init mpeg4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncPreInit(MP4Handle *mp4Handle, MMCodecBuffer *pInterMemBfr);

/*****************************************************************************/
//  Description:   Init mpeg4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncInit(MP4Handle *mp4Handle, MMCodecBuffer *pExtraMemBfr,MMCodecBuffer *pBitstreamBfr, MMEncVideoInfo *pVideoFormat);

/*****************************************************************************/
//  Description:   Generate mpeg4 header
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncGenHeader(MP4Handle *mp4Handle, MMEncOut *pOutput);

/*****************************************************************************/
//  Description:   Set mpeg4 encode config
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncSetConf(MP4Handle *mp4Handle, MMEncConfig *pConf);

/*****************************************************************************/
//  Description:   Get mpeg4 encode config
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncGetConf(MP4Handle *mp4Handle, MMEncConfig *pConf);

/*****************************************************************************/
//  Description:   Encode one vop
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncStrmEncode (MP4Handle *mp4Handle, MMEncIn *pInput, MMEncOut *pOutput);

/*****************************************************************************/
//  Description:   Close mpeg4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncRelease(MP4Handle *mp4Handle);

typedef MMEncRet (*FT_MP4EncGetCodecCapability)(MP4Handle *mp4Handle, MMEncCapability *Capability);
typedef MMEncRet (*FT_MP4EncPreInit)(MP4Handle *mp4Handle, MMCodecBuffer *pInterMemBfr);
typedef MMEncRet (*FT_MP4EncInit)(MP4Handle *mp4Handle, MMCodecBuffer *pExtraMemBfr,MMCodecBuffer *pBitstreamBfr, MMEncVideoInfo *pVideoFormat);
typedef MMEncRet (*FT_MP4EncSetConf)(MP4Handle *mp4Handle, MMEncConfig *pConf);
typedef MMEncRet (*FT_MP4EncGetConf)(MP4Handle *mp4Handle, MMEncConfig *pConf);
typedef MMEncRet (*FT_MP4EncStrmEncode) (MP4Handle *mp4Handle, MMEncIn *pInput, MMEncOut *pOutput);
typedef MMEncRet (*FT_MP4EncGenHeader)(MP4Handle *mp4Handle, MMEncOut *pOutput);
typedef MMEncRet (*FT_MP4EncRelease)(MP4Handle *mp4Handle);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
