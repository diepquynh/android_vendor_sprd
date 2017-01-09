/******************************************************************************
 ** File Name:    mpeg4dec.h                                                  *
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
#ifndef _MP4_H263_DEC_API_H_
#define _MP4_H263_DEC_API_H_

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
    INTER_MEM = 0,  /*internal memory, only for software writing and reading and initialized when initialize decoder*/
    HW_NO_CACHABLE, /*physical continuous and no-cachable, only for VSP writing and reading */
    HW_CACHABLE,    /*physical continuous and cachable, for software writing and VSP reading */
    SW_CACHABLE,    /*only for software writing and reading*/
    MAX_MEM_TYPE
} CODEC_BUF_TYPE;

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
    uint8 *p_extra;
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

typedef MMCodecBuffer MMDecBuffer;

typedef struct
{
    uint16 start_pos;
    uint16 end_pos;
} ERR_POS_T;

#define MAX_ERR_PKT_NUM		30

// Decoder input structure
typedef struct
{
    uint8		*pStream;          	// Pointer to stream to be decoded. Virtual address.
    unsigned long pStream_phy;          	// Pointer to stream to be decoded. Physical address.
    uint32		dataLen;           	// Number of bytes to be decoded
    int32		beLastFrm;			// whether the frame is the last frame.  1: yes,   0: no

    int32		expected_IVOP;		// control flag, seek for IVOP,
    uint64		nTimeStamp;                // time stamp, it maybe PTS or DTS

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

    int32   is_transposed;	//the picture is transposed or not, in 8800H5, it should always 0.

    uint64	pts;            //presentation time stamp
    int32	frameEffective;

    int32	err_MB_num;		//error MB number
    void *pBufferHeader;
    int VopPredType;
} MMDecOutput;

typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader,int flag);
typedef int (*FunctionType_MemAllocCB)(void *userData, unsigned int extra_mem_size);

/* Application controls, this structed shall be allocated */
/*    and initialized in the application.                 */
typedef struct tagMP4Handle
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
//    int nLayers;
    /* pointers to VOL data for frame-based decoding. */
//    uint8 *volbuf[2];           /* maximum of 2 layers for now */
//    int32 volbuf_size[2];

    void *userdata;

    FunctionType_BufCB VSP_bindCb;
    FunctionType_BufCB VSP_unbindCb;
    FunctionType_MemAllocCB VSP_extMemCb;

    int g_mpeg4_dec_err_flag;
} MP4Handle;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
void MP4DecReleaseRefBuffers(MP4Handle *mp4Handle);
int MP4DecGetLastDspFrm(MP4Handle *mp4Handle, void **pOutput);
void MP4DecSetCurRecPic(MP4Handle *mp4Handle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader);
void MP4DecSetReferenceYUV(MP4Handle *mp4Handle, uint8 *pFrameY);

void Mp4GetVideoDimensions(MP4Handle *mp4Handle, int32 *display_width, int32 *display_height);
void Mp4GetBufferDimensions(MP4Handle *mp4Handle, int32 *width, int32 *height);
MMDecRet MP4GetCodecCapability(MP4Handle *mp4Handle, int32 *max_width, int32 *max_height);

/*****************************************************************************/
//  Description: Init mpeg4 decoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet MP4DecInit(MP4Handle *mp4Handle, MMCodecBuffer * pBuffer);

MMDecRet MP4DecVolHeader(MP4Handle *mp4Handle, MMDecVideoFormat *video_format_ptr);

/*****************************************************************************/
//  Description: Init mpeg4 decoder	memory
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet MP4DecMemInit(MP4Handle *mp4Handle, MMCodecBuffer *pBuffer);

/*****************************************************************************/
//  Description: Decode one vop
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet MP4DecDecode(MP4Handle *mp4Handle, MMDecInput *pInput,MMDecOutput *pOutput);

/*****************************************************************************/
//  Description: Close mpeg4 decoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet MP4DecRelease(MP4Handle *mp4Handle);

typedef void (*FT_MP4DecSetReferenceYUV)(MP4Handle *mp4Handle, uint8 *pFrameY);
typedef void (*FT_MP4DecReleaseRefBuffers)(MP4Handle *mp4Handle);
typedef int (*FT_MP4DecGetLastDspFrm)(MP4Handle *mp4Handle,void **pOutput);
typedef void (*FT_MP4DecSetCurRecPic)(MP4Handle *mp4Handle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader);
typedef void (*FT_MP4DecSetReferenceYUV)(MP4Handle *mp4Handle, uint8 *pFrameY);
typedef void (*FT_Mp4GetVideoDimensions)(MP4Handle *mp4Handle, int32 *display_width, int32 *display_height);
typedef void (*FT_Mp4GetBufferDimensions)(MP4Handle *mp4Handle, int32 *width, int32 *height);
typedef MMDecRet (*FT_MP4GetCodecCapability)(MP4Handle *mp4Handle, int32 *max_width, int32 *max_height);

typedef MMDecRet (*FT_MP4DecInit)(MP4Handle *mp4Handle, MMCodecBuffer * pBuffer);
typedef MMDecRet (*FT_MP4DecVolHeader)(MP4Handle *mp4Handle, MMDecVideoFormat *video_format_ptr);
typedef MMDecRet (*FT_MP4DecMemInit)(MP4Handle *mp4Handle, MMCodecBuffer *pBuffer);
typedef MMDecRet (*FT_MP4DecDecode)(MP4Handle *mp4Handle, MMDecInput *pInput,MMDecOutput *pOutput);
typedef MMDecRet (*FT_MP4DecRelease)(MP4Handle *mp4Handle);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
