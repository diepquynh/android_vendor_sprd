//--=========================================================================--
//  This implements some useful common functionalities 
//  for handling the register files used in Bellagio
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2015  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

#ifndef _OMX_VPUDEC_COMPONENT_H_
#define _OMX_VPUDEC_COMPONENT_H_

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <pthread.h>
//#include <linux/videodev.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <omx_base_filter.h>
#include <omx_base_video_port.h>

#include "OMX_VPU_Video.h"

#if ANDROID_PLATFORM_SDK_VERSION >= 19
#include <OMX_VideoExt.h>
#endif
#include "../../../../vpuapi/vpuconfig.h"
#include "../../../../vpuapi/vpuapi.h"
#include "../../../../vpuapi/vpuapifunc.h"
#include "../../../../include/vpuio.h"

//#define WORKAROUND_OMX_USE_INPUT_HEADER_BUFFER_AS_DECODER_BITSTREAM_BUFFER_NOT_REFORMATED

#define WORKAROUND_VP8_CTS_TEST

#define SUPPORT_ONE_BUFFER_CONTAIN_MULTIPLE_FRAMES


#define MAX_DEC_BITSTREAM_BUFFER_COUNT		(16)


#define VIDEO_DEC_NUM 2
#define VIDEO_DEC_BASE_NAME "OMX.vpu.video_decoder"
#define VIDEO_DEC_MPEG2_NAME  "OMX.vpu.video_decoder.mpeg2"
#define VIDEO_DEC_MPEG2_ROLE  "video_decoder.mpeg2"
#define VIDEO_DEC_MPEG4_NAME "OMX.vpu.video_decoder.mpeg4"
#define VIDEO_DEC_MPEG4_ROLE "video_decoder.mpeg4"
#define VIDEO_DEC_H264_NAME "OMX.vpu.video_decoder.avc"
#define VIDEO_DEC_H264_ROLE "video_decoder.avc"
#define VIDEO_DEC_RV_NAME	"OMX.vpu.video_decoder.rv"
#define VIDEO_DEC_RV_ROLE	"video_decoder.rv"
#define VIDEO_DEC_WMV_NAME    "OMX.vpu.video_decoder.wmv"
#define VIDEO_DEC_WMV_ROLE    "video_decoder.wmv"
#define VIDEO_DEC_H263_NAME   "OMX.vpu.video_decoder.h263"
#define VIDEO_DEC_H263_ROLE   "video_decoder.h263"
#define VIDEO_DEC_MSMPEG_NAME "OMX.vpu.video_decoder.msmpeg"
#define VIDEO_DEC_MSMPEG_ROLE "video_decoder.msmpeg"
#define VIDEO_DEC_AVS_NAME "OMX.vpu.video_decoder.avs"
#define VIDEO_DEC_AVS_ROLE "video_decoder.avs"
#define VIDEO_DEC_VP8_NAME "OMX.vpu.video_decoder.vp8"
#define VIDEO_DEC_VP8_ROLE "video_decoder.vp8"
#define VIDEO_DEC_THO_NAME "OMX.vpu.video_decoder.tho"
#define VIDEO_DEC_THO_ROLE "video_decoder.tho"
#define VIDEO_DEC_JPG_NAME "OMX.vpu.image_decoder.jpg"
#define VIDEO_DEC_JPG_ROLE "image_decoder.jpg"
#define VIDEO_DEC_VC1_NAME	"OMX.vpu.video_decoder.vc1"
#define VIDEO_DEC_VC1_ROLE	"video_decoder.vc1"
#define VIDEO_DEC_HEVC_NAME "OMX.vpu.video_decoder.hevc"
#define VIDEO_DEC_HEVC_ROLE "video_decoder.hevc"
#define MAX_PPU_SRC_NUM 2
#define IS_STATE_EMPTYTHISBUFFER(pInputBuffer)      (pInputBuffer != NULL)
#define IS_STATE_FILLTHISBUFFER(pOutBuffer)      (pOutBuffer != NULL)
typedef struct vpu_frame_t
{
	int format;
	int index;
	int stride;
	vpu_buffer_t vbY;
	vpu_buffer_t vbCb;
	vpu_buffer_t vbCr;
	vpu_buffer_t vbMvCol;
} vpu_frame_t;


typedef enum {
	OMX_BUFFER_OWNER_NOBODY = 0,
	OMX_BUFFER_OWNER_CLIENT,
	OMX_BUFERR_OWNER_COMPONENT,
} omx_buffer_owner;

typedef struct {
	omx_buffer_owner owner;
	OMX_TICKS nInputTimeStamp;
} omx_display_flag;


typedef struct {
    DecOutputInfo* buffer;
    int  size;
    int  count;
    int  front;
    int  rear;
} omx_outinfo_queue_item_t;


typedef struct {
    OMX_BUFFERHEADERTYPE* buffer;
    int  size;
    int  count;
    int  front;
    int  rear;
	pthread_mutex_t mutex; \
} omx_bufferheader_queue_item_t;


typedef struct omx_timestamp_correction_smooth_t
{
	OMX_TICKS LeftTimeUs;
	OMX_TICKS AnchorTimeUs;
	OMX_TICKS TimePerFrameUs;
} omx_timestamp_correction_smooth_t;

typedef struct omx_timestamp_correction_t
{
	OMX_BOOL mTimeStampCalcuteMode;
	OMX_BOOL mTimeStampDirectMode;
	OMX_TICKS mDurationPerFrameUs;
	OMX_TICKS mFrameNumber;
	OMX_TICKS mPreviousTimeStampUs;
	OMX_TICKS mAnchorTimeUs;

	omx_timestamp_correction_smooth_t mSmoothTime;
} omx_timestamp_correction_t;



typedef struct vpu_dec_context_t
{
	DecHandle handle;
	DecOpenParam decOP;
	DecInitialInfo initialInfo;
	DecOutputInfo dispOutputInfo;
	DecOutputInfo decOutputInfos[MAX_REG_FRAME];
	DecParam decParam;
	SecAxiUse secAxiUse;
	DRAMConfig dramCfg;
	FrameBufferAllocInfo fbAllocInfo;
	FrameBuffer fbPPU[MAX_PPU_SRC_NUM];
	FrameBuffer fbUser[MAX_REG_FRAME];
	vpu_buffer_t vbUserFb[MAX_REG_FRAME];	// consist of all buffers context information and not for freeing
	vpu_buffer_t vbAllocFb[MAX_REG_FRAME];  // buffer context that is allocated by OMX_AllocateBuffer and OMX_UseBuffer
	vpu_buffer_t vbDPB[MAX_REG_FRAME];		// buffer context that is allocated by OmxAllocateBuffer for DPB
	vpu_buffer_t vbStream[MAX_DEC_BITSTREAM_BUFFER_COUNT];
	int fbWidth;
	int fbHeight;
	int fbStride;
	int fbFormat;
	int rotStride;
	int regFbCount;
	int seqInited;
	Rect rcPrevDisp;
	int ppu;
	int ppuIdx;
	int ppuEnable;
	int frameIdx;
	int dispOutIdx;
	int decodefinish;
    int int_reason;
	int totalNumofErrMbs;
	int needFrameBufCount;
	int instIdx;
	int coreIdx;
	int mapType;
    int keepSeqData;
	int productId;
	int chunkReuseRequired;
    int prevConsumeBytes;
	int curConsumedByte;
	int frameRate;
#ifdef SUPPORT_CROP_BOTTOM
	int scaleNotAlignedHeight;
#endif
        void *deInterlaceBufHandle;
        size_t deInterlaceBufSize;
        unsigned long deInterlacePhyAddr;
        void *deInterlaceVirAddr;
        OMX_BUFFERHEADERTYPE* deInterlaceRefBuf;
        unsigned long deInterlaceFrameNo;
        int deInterlaceDelayNum;
        OMX_BOOL stopdec;
        OMX_U32 saveLen;
} vpu_dec_context_t;

typedef struct dec_config_t
{
	int bitFormat;
	int rotAngle;
	int mirDir;
	int useRot;
	int useDering;
	int outNum;
	int checkeos;
	int mp4DeblkEnable;
	int iframeSearchEnable;
	int dynamicAllocEnable;
	int reorder;
	int mpeg4Class;
	int mapType;
	int tiled2LinearEnable;
	int seqInitMode;
	int dispMixerDirect;

	int cacheOption;
	int cacheBypass;
	int cacheDual;
} dec_config_t;

typedef union VideoParam
{
	OMX_VIDEO_PARAM_H263TYPE h263;
	OMX_VIDEO_PARAM_MPEG4TYPE mpeg4;
	OMX_VIDEO_PARAM_MPEG2TYPE mpeg2;
	OMX_VIDEO_PARAM_AVCTYPE avc;
	OMX_VIDEO_PARAM_RVTYPE rv;
	OMX_VIDEO_PARAM_WMVTYPE wmv;
	OMX_VIDEO_PARAM_MSMPEGTYPE msmpeg;
	OMX_VIDEO_PARAM_VP8TYPE vp8;
} VideoParam;

DERIVEDCLASS(omx_vpudec_component_PortType, omx_base_video_PortType)
#define omx_vpudec_component_PortType_FIELDS omx_base_video_PortType_FIELDS \
	OMX_CONFIG_RECTTYPE omxConfigCrop; \
	OMX_CONFIG_ROTATIONTYPE omxConfigRotate; \
	OMX_CONFIG_MIRRORTYPE omxConfigMirror; \
	OMX_CONFIG_SCALEFACTORTYPE omxConfigScale; \
	OMX_CONFIG_POINTTYPE omxConfigOutputPosition; \
	OMX_U32 nTempBufferCountActual; \
	OMX_BOOL bAllocateBuffer;
ENDCLASS(omx_vpudec_component_PortType)

/** Video Decoder component private structure.
 */
DERIVEDCLASS(omx_vpudec_component_PrivateType, omx_base_filter_PrivateType)
#define omx_vpudec_component_PrivateType_FIELDS omx_base_filter_PrivateType_FIELDS \
	VideoParam codParam; \
	/** @param vpuReady boolean flag that is true when the video coded has been initialized */ \
	OMX_BOOL vpuReady;  \
	/** @param video_coding_type Field that indicate the supported video format of video decoder */ \
	OMX_U32 video_coding_type;   \
	/** @param extradata pointer to extradata*/ \
	OMX_U8* picHeader; \
	OMX_U8* seqHeader; \
	/** @param extradata_size extradata size*/ \
	OMX_S32 seqHeaderSize; \
	OMX_S32 picHeaderSize; \
	vpu_dec_context_t vpu; \
	/** @param useNativeBuffer that indicate ANDROID native buffer use */ \
	OMX_BOOL useNativeBuffer; \
	OMX_BOOL bThumbnailMode; \
	/** @param count the native buffer number  */ \
	int actualFrameBufferNeeded; \
	OMX_BOOL bUseOmxInputBufferAsDecBsBuffer; \
	OMX_BOOL bSeqChangeDetected; \
	OMX_BOOL portSettingChangeRequest; \
    OMX_BOOL bIsOutputEOSReached; \
    OMX_BOOL bIsTimeStampReorder; \
    OMX_U32 portSettingCount;   \
	tsem_t port_setting_change_tsem; \
	omx_bufferheader_queue_item_t *in_bufferheader_queue; \
	omx_timestamp_correction_t omx_ts_correction; \
	omx_display_flag *omx_display_flags; \
	pthread_mutex_t display_flag_mutex; \
	pthread_mutex_t vpu_flush_mutex; \
    /** @param if deinterlace Needed */ \
    OMX_BOOL bIsDeinterlaceNeeded; \
    /** @param thread for de-interlace process */ \
    pthread_t deInterlaceThread; \
    /** @param de-interlace thread function */ \
    void* (*Video_Post_Porcess_Function)(void* param); \
    /** @param buffer Sem that trigger the de-interlace thread */ \
    tsem_t deInterlace_Thread_Sem; \
    /** @param flush complete Sem that indicate whether flush completed in de-interlace thread */ \
    tsem_t port_flush_complete_condition; \
    /** @param bufQue for decoder ouput(de-interlace process input) */ \
    queue_t* bufferQueForDeinterlaceProcess; \
    /** @param mutex for deInterlace buffers*/ \
    pthread_mutex_t deInterlace_buf_mutex; \
    /** @param vpp object*/ \
    void *vpp_obj; \
    OMX_BOOL bSkipBrokenBframeDecode;
ENDCLASS(omx_vpudec_component_PrivateType)


/* Component private entry points declaration */
OMX_ERRORTYPE omx_vpudec_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName);
OMX_ERRORTYPE omx_vpudec_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_vpudec_component_Init(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_vpudec_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_vpudec_component_MessageHandler(OMX_COMPONENTTYPE*, internalRequestMessageType*);
void* omx_vpudec_component_BufferMgmtFunction (
    void* param);

/* SPRD de-interlace thread function*/
void* omx_vpudec_component_deInterlaceFunction(void* param);

void omx_vpudec_component_BufferMgmtCallback(
    OMX_COMPONENTTYPE *openmaxStandComp,
    OMX_BUFFERHEADERTYPE* inputbuffer,
    OMX_BUFFERHEADERTYPE* outputbuffer);
OMX_ERRORTYPE omx_vpudec_component_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure);
OMX_ERRORTYPE omx_vpudec_component_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_IN OMX_PTR pComponentConfigStructure);
OMX_ERRORTYPE omx_vpudec_component_ComponentRoleEnum(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8 *cRole,
    OMX_IN OMX_U32 nIndex);
OMX_ERRORTYPE omx_vpudec_component_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure);
OMX_ERRORTYPE omx_vpudec_component_GetExtensionIndex(
  OMX_HANDLETYPE hComponent,
  OMX_STRING cParameterName,
  OMX_INDEXTYPE* pIndexType);
OMX_ERRORTYPE omx_vpudec_component_GetConfig(
	OMX_IN OMX_HANDLETYPE hComponent,
	OMX_IN OMX_INDEXTYPE nParamIndex,
	OMX_IN OMX_PTR pComponentConfigStructure);
OMX_ERRORTYPE omx_vpudec_component_GetConfig(
	OMX_IN OMX_HANDLETYPE hComponent,
	OMX_IN OMX_INDEXTYPE nParamIndex,
	OMX_IN OMX_PTR pComponentConfigStructure);
OMX_ERRORTYPE omx_vpudec_component_SendBufferFunction(
	omx_base_PortType *openmaxStandPort, 
	OMX_BUFFERHEADERTYPE* pBuffer);
OMX_ERRORTYPE omx_vpudec_component_OutPort_ReturnBufferFunction(
	omx_base_PortType* openmaxStandPort,
	OMX_BUFFERHEADERTYPE* pBuffer);
void* omx_vpudec_component_BufferMgmtFunction (void* param);
OMX_ERRORTYPE omx_videodec_component_AllocateBuffer(
	OMX_IN OMX_HANDLETYPE hComponent,
	OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
	OMX_IN OMX_U32 nPortIndex,
	OMX_IN OMX_PTR pAppPrivate,
	OMX_IN OMX_U32 nSizeBytes);
OMX_ERRORTYPE omx_videodec_component_UseBuffer(
	OMX_HANDLETYPE hComponent,
	OMX_BUFFERHEADERTYPE** ppBufferHdr,
	OMX_U32 nPortIndex,
	OMX_PTR pAppPrivate,
	OMX_U32 nSizeBytes,
	OMX_U8* pBuffer);
OMX_ERRORTYPE omx_videodec_component_FreeBuffer(
	OMX_IN  OMX_HANDLETYPE hComponent,
	OMX_IN  OMX_U32 nPortIndex,
	OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
#endif
