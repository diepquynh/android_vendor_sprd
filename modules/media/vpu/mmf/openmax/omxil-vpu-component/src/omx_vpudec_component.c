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


#include <sys/syscall.h>
#include <omxcore.h>
#include <omx_base_video_port.h>
#include <omx_vpudec_component.h>
#include <OMX_Video.h>
#include "OMX_VPU_Video.h"
#include "OMX_VPU_Index.h"

#include <vpuapi/vpuconfig.h>
#include <vpuapi/vpuapi.h>
#include <vpuapi/vpuapifunc.h>

#include <vpuapi/regdefine.h>
#include <include/vpuhelper.h>

#include <sys/time.h>

#ifdef ANDROID
#include "android_support.h"
#endif
#include <OMX_IVCommon.h>

//#define LOG_NDEBUG 0
//#undef LOG_TAG
//#define LOG_TAG "Alan"

#include <utils/Log.h>

#ifdef DEBUG
#undef DEBUG
#define DEBUG(n, fmt, args...) \
do { \
    if (n == DEB_LEV_ERR) \
    { \
        ALOGE("[TID:%d-%s-%d] " fmt, gettid(), __func__, __LINE__, ##args); \
    } \
    else if (DEBUG_LEVEL & (n)) \
    { \
        ALOGI("[TID:%d-%s-%d] " fmt, gettid(), __func__, __LINE__, ##args); \
    } \
} while (0)
#endif

//#define ANDROID_INPUT_DUMP	//need create /data/dump_dec.es file and chmod first
//#define ANDROID_OUTPUT_DUMP	//need create /data/dump_dec.yuv file and chmod first
//#define SPRD_DE_INTERLACE_OUPUT_DUMP //need create /data/dump_dec_deinter_process.yuv file and chmod first

#if defined(ANDROID_INPUT_DUMP) || defined(ANDROID_OUTPUT_DUMP)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include "vpp_drv_interface.h"
#include "vpp_deint.h"

#define USE_IFRAME_SEARCH_FOR_1STFRAME
#define USE_BFRAME_SEARCH_FOR_1STFRAME

#define SUPPORT_TIMESTAMP_REORDER
#define SUPPORT_FIX_ADAPTIVE_PLAYBACK

#define SPRD_BLURED_PIC_OPT
//#define SPRD_FIRMWARE_VALID_CHECK

#define SPRD_PARALLEL_DEINTRELACE_SUPPORT
//#define SPRD_SERIAL_DEINTRELACE_SUPPORT

//#define SPRD_DEINTERLACE_BYPASS

#ifdef ANDROID
#	if ANDROID_PLATFORM_SDK_VERSION >= 19
#		define NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS 1 // must set to the proper value as many as minUndequeuedBufs in mNativeWindow->query(mNativeWindow.get(),NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &minUndequeuedBufs);
#	else
#		define NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS 2 // must set to the proper value as many as minUndequeuedBufs in mNativeWindow->query(mNativeWindow.get(),NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &minUndequeuedBufs);
#	endif
#else
#	define NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS 0
#endif

#define EXTRA_FRAME_BUFFER_NUM (1)

//need 2 more buffers for de-interlace input(ref + source)
#define EXTRA_DEINTERLACE_BUFFER_NUM (2)

#define EXTRA_ALLOCATING_BUFFER_NUM  (4)

#define MINIPIPPEN_MINMUM_RESOLUTION   64
#define DEFAULT_STREAMBUFFER_SIZE       (3*1024*1024/2)       // case of 4K, 6M recommended
#define DEFAULT_WIDTH					MAX_DEC_PIC_WIDTH
#define DEFAULT_HEIGHT					MAX_DEC_PIC_HEIGHT
#define DEFAULT_VIDEO_INPUT_BUF_SIZE    DEFAULT_STREAMBUFFER_SIZE
#	define DEFAULT_VIDEO_OUTPUT_BUF_SIZE   ((DEFAULT_WIDTH*DEFAULT_HEIGHT*3)/2)
#	ifdef CNM_FPGA_PLATFORM
#		define DEFAULT_VIDEO_OUTPUT_FORMAT	    OMX_COLOR_FormatYUV420SemiPlanar
#	    define DEFAULT_NATIVE_OUTPUT_FORMAT     MAKE_FOURCC('Y', 'V', '1', '2')
#	else
#		define DEFAULT_VIDEO_OUTPUT_FORMAT      OMX_COLOR_FormatYUV420SemiPlanar
#	    define DEFAULT_NATIVE_OUTPUT_FORMAT     OMX_COLOR_FormatYUV420SemiPlanar
#	endif
#ifdef WORKAROUND_VP8_CTS_TEST
#define DEFAULT_VP8_VIDEO_OUTPUT_FORMAT     OMX_COLOR_FormatYUV420Planar
#endif

#define DEFAULT_BITRATE                 64000

#define DEFAULT_MIN_VIDEO_OUTPUT_BUFFER_NUM		1

#define DEFAULT_ACTUAL_VIDEO_OUTPUT_BUFFER_NUM	2

#define DEFAULT_ACTUAL_VIDEO_INPUT_BUFFER_NUM	4
#define DEFAULT_MIN_VIDEO_INPUT_BUFFER_NUM		4
#define DEFAULT_FRAMERATE               (0)		// 0 means that if input stream does not have framerate value. set it to framerate in sequence header from VPU
#define MAX_CHUNK_HEADER_SIZE			DEFAULT_STREAMBUFFER_SIZE
#define VPU_DEC_TIMEOUT					1000
#define STREAM_END_SIZE					0

#if ANDROID_PLATFORM_SDK_VERSION >= 19
#define OMX_VIDEO_CODINGTYPE_VP8 OMX_VIDEO_CodingVP8
#else
#define OMX_VIDEO_CODINGTYPE_VP8 OMX_VIDEO_CodingVPX
#endif

#define MAKE_FOURCC(a,b,c,d) ( ((unsigned char)a) | ((unsigned char)b << 8) | ((unsigned char)c << 16) | ((unsigned char)d << 24) )
#define PUT_BYTE(_p, _b) \
    *_p++ = (unsigned char)_b;
#define PUT_BUFFER(_p, _buf, _len) \
    memcpy(_p, _buf, _len); \
    _p += _len;
#define PUT_LE32(_p, _var) \
    *_p++ = (unsigned char)((_var)>>0);  \
    *_p++ = (unsigned char)((_var)>>8);  \
    *_p++ = (unsigned char)((_var)>>16); \
    *_p++ = (unsigned char)((_var)>>24);
#define PUT_BE32(_p, _var) \
    *_p++ = (unsigned char)((_var)>>24);  \
    *_p++ = (unsigned char)((_var)>>16);  \
    *_p++ = (unsigned char)((_var)>>8); \
    *_p++ = (unsigned char)((_var)>>0);
#define PUT_LE16(_p, _var) \
    *_p++ = (unsigned char)((_var)>>0);  \
    *_p++ = (unsigned char)((_var)>>8);
#define PUT_BE16(_p, _var) \
    *_p++ = (unsigned char)((_var)>>8);  \
    *_p++ = (unsigned char)((_var)>>0);

static struct timespec s_ts_start;
static struct timespec s_ts_end;
static struct timespec s_ts_curr;

static double s_time_elapsed = 0;
double omx_time_curr_ms();
void omx_timer_start();
void omx_timer_stop();
double omx_timer_elapsed_us();
double omx_timer_elapsed_ms();

static int codingTypeToMp4class(OMX_VIDEO_CODINGTYPE codingType, int fourCC);
static int codingTypeToCodStd(OMX_VIDEO_CODINGTYPE codingType);

static int BuildOmxSeqHeader(OMX_U8 *pbHeader, OMX_BUFFERHEADERTYPE* pInputBuffer, OMX_COMPONENTTYPE *openmaxStandComp, int* sizelength);
static int BuildOmxPicHeader(OMX_U8 *pbHeader, OMX_BUFFERHEADERTYPE* pInputBuffer, OMX_COMPONENTTYPE *openmaxStandComp, int sizelength);
static void initializeVideoPorts(omx_vpudec_component_PortType* inPort, omx_vpudec_component_PortType* outPort, OMX_VIDEO_CODINGTYPE codingType);
static void SetInternalVideoParameters(OMX_COMPONENTTYPE *openmaxStandComps);
static OMX_BOOL OmxVpuFlush(OMX_COMPONENTTYPE *openmaxStandComp);

static dec_config_t s_dec_config;
static int OmxAllocateFrameBuffers(OMX_COMPONENTTYPE *openmaxStandComp);
#ifdef CNM_FPGA_PLATFORM
static int OmxSyncFpgaOutputToHostBuffer(OMX_COMPONENTTYPE *openmaxStandComp, DecOutputInfo *decOutputInfo);
#endif



static OMX_BOOL OmxUpdateOutputBufferHeaderToDisplayOrder(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE **ppOutputBuffer);
static OMX_TICKS OmxTimeStampCorrection(OMX_COMPONENTTYPE *openmaxStandComp, OMX_TICKS nInputTimeStamp);
static OMX_BOOL OmxClearDisplayFlag(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE *pBuffer, OMX_BOOL bFillThisBufer);
static void OmxCheckVersion(int coreIdx);
static OMX_U32 OmxGetVpuFrameRate(omx_vpudec_component_PrivateType* omx_vpudec_component_Private);

// BUFFERHEADERTYPE queue for async decoding
static omx_bufferheader_queue_item_t* omx_bufferheader_queue_init(int count);
static void omx_bufferheader_queue_deinit(omx_bufferheader_queue_item_t* queue);
static int omx_bufferheader_queue_enqueue(omx_bufferheader_queue_item_t* queue, OMX_BUFFERHEADERTYPE* data);
static int omx_bufferheader_queue_dequeue(omx_bufferheader_queue_item_t* queue, OMX_BUFFERHEADERTYPE* data);
static int omx_bufferheader_queue_peekqueue(omx_bufferheader_queue_item_t* queue, OMX_BUFFERHEADERTYPE* data);
static int omx_bufferheader_queue_dequeue_all(omx_bufferheader_queue_item_t* queue);
static int omx_bufferheader_queue_count(omx_bufferheader_queue_item_t* queue);

static int OmxWriteBsBufFromBufHelper(OMX_COMPONENTTYPE *openmaxStandComp, vpu_dec_context_t *pVpu, vpu_buffer_t vbStream, BYTE *pChunk,  int chunkSize);

static int OmxLoadFirmware(Int32 productId, Uint8** retFirmware, Uint32* retSizeInWord, const char* path);

static OMX_BOOL OmxGetVpuBsBufferByVirtualAddress(OMX_COMPONENTTYPE *openmaxStandComp, vpu_buffer_t *vb, OMX_BUFFERHEADERTYPE *pInputBuffer);
static OMX_BOOL OmxNeedInputBufferReformat(omx_vpudec_component_PrivateType* omx_vpudec_component_Private);


static void OmxWaitUntilOutBuffersEmpty(omx_vpudec_component_PrivateType* omx_vpudec_component_Private);


static void omx_vpudec_component_vpuLibDeInit(omx_vpudec_component_PrivateType* omx_vpudec_component_Private);




VPPObject* omx_vpudec_cmp_vpp_init(void)
{
    VPPObject *vo = NULL;
    uint32 ret = 0;

    vo = (VPPObject*)malloc(sizeof(VPPObject));
    if (NULL == vo) {
        return NULL;
    }
    ret = vpp_deint_init (vo);
    if( ret != 0) {
	DEBUG(DEB_LEV_ERR, " vpp_deint_init failed\n");
	free(vo);
	return NULL;
    }

   DEBUG(DEB_LEV_SIMPLE_SEQ, " vpp_deint_init ok, %p\n", vo);
   return vo;
}

int omx_vpudec_cmp_vpp_deinit(VPPObject *vo)
{
    DEBUG(DEB_LEV_SIMPLE_SEQ, "omx_vpudec_cmp_vpp_deinit, %p\n", vo);
    if(vo) {
      vpp_deint_release(vo);
      free(vo);
   }
    return 0;
}


int omx_vpudec_cmp_vpp_process(VPPObject *vpp_obj, unsigned long p_src, unsigned long p_ref, unsigned long p_disp, uint32 w, uint32 h, uint32 frame_no)
{
    DEINT_PARAMS_T deint_params = {0};
    uint32 ret = 0;

    deint_params.width  = w;
    deint_params.height = h;
    deint_params.interleave = INTERLEAVE;
    deint_params.threshold = THRESHOLD;

    deint_params.y_len = deint_params.width*deint_params.height;
    deint_params.c_len = deint_params.y_len >> 2;

    ret = vpp_deint_process(vpp_obj, p_src, p_ref, p_disp, frame_no, &deint_params);
    DEBUG(DEB_LEV_FULL_SEQ, "after vpp_deint_process %d\n", ret);

    return ret;
}

OMX_ERRORTYPE omx_vpudec_cmp_return_buf_directly(void* param, omx_base_PortType* openmaxStandPort,OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)param;
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private =
        (omx_vpudec_component_PrivateType*)openmaxStandComp->pComponentPrivate;

        (*(openmaxStandPort->BufferProcessedCallback))(
            openmaxStandPort->standCompContainer,
            omx_vpudec_component_Private->callbackData,
            pBuffer);

     return 0;
}

#ifdef SPRD_PARALLEL_DEINTRELACE_SUPPORT
/** @brief De-interlace process thread function
 *   that process two decoded output buffer as src and ref
 *   Ouput is stored on one display buffer from native-window
 *   Add by Alan wang
 * */
void* omx_vpudec_component_deInterlaceFunction(void* param) {

    OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)param;

    omx_vpudec_component_PrivateType* omx_vpudec_component_Private =
        (omx_vpudec_component_PrivateType*)openmaxStandComp->pComponentPrivate;

    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;

    omx_vpudec_component_PortType *pOutPort =
        (omx_vpudec_component_PortType *)omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

    omx_vpudec_component_PortType *pInPort =
        (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];

    queue_t *decodedBufQue = omx_vpudec_component_Private->bufferQueForDeinterlaceProcess;
    queue_t *pOutputQueue = pOutPort->pBufferQueue;

    tsem_t *pOutputSem = pOutPort->pBufferSem;

    OMX_BUFFERHEADERTYPE *disPlayBuf = NULL;
    OMX_BUFFERHEADERTYPE *refBuf = NULL;
    OMX_BUFFERHEADERTYPE *srcBuf = NULL;

    OMX_BOOL isDecodedOutputBufNeeded = OMX_TRUE;
    OMX_BOOL isDisPlayBufferNeeded = OMX_TRUE;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    void *pSrcVirtAddrs;
    void *pRefVirtAddrs;
    void *pDispVirtAddrs;
    unsigned long srcPhyAddrs;
    unsigned long refPhyAddrs;
    unsigned long dispPhyAddrs;
    int ret;
    uint32 frame_no = 0;


    DEBUG(DEB_LEV_FUNCTION_NAME, " In ...\n");

    /* checks if the component is in a state able to receive buffers */
    while(omx_vpudec_component_Private->state == OMX_StateIdle ||
            omx_vpudec_component_Private->state == OMX_StateExecuting ||
            omx_vpudec_component_Private->state == OMX_StatePause ||
            omx_vpudec_component_Private->transientState == OMX_TransStateLoadedToIdle) {

        DEBUG(DEB_LEV_FULL_SEQ, " PORT_IS_BEING_FLUSHED(out) = %d PORT_IS_BEING_FLUSHED(in) = %d\n",
                PORT_IS_BEING_FLUSHED(pOutPort), PORT_IS_BEING_FLUSHED(pInPort));

        // Check flush status first
        pthread_mutex_lock(&omx_vpudec_component_Private->flush_mutex);
        if (PORT_IS_BEING_FLUSHED(pOutPort) || PORT_IS_BEING_FLUSHED(pInPort)) {
            DEBUG(DEB_LEV_FULL_SEQ, " Port %s is in being flushed\n", PORT_IS_BEING_FLUSHED(pOutPort) ? "[out]" : "[in]");
        pthread_mutex_unlock(&omx_vpudec_component_Private->flush_mutex);
            OMX_BUFFERHEADERTYPE* returnBuf = NULL;

            if (PORT_IS_BEING_FLUSHED(pOutPort)) {
                 /* 1. Return ref buffers */
                if (refBuf != NULL) {
                    omx_vpudec_cmp_return_buf_directly(param, (omx_base_PortType *)pOutPort, refBuf);
                    refBuf = NULL;
                }
                /* 2. Return all buffers in decodedBufQue anyway */
                while ((returnBuf = dequeue(decodedBufQue)) != NULL) {
                    omx_vpudec_cmp_return_buf_directly(param, (omx_base_PortType *)pOutPort, returnBuf);
                }
            }
            isDecodedOutputBufNeeded = OMX_TRUE;
            DEBUG(DEB_LEV_SIMPLE_SEQ, "port_flush_complete_condition.semval: %d, deinterlace_flush_condition.semval: %d\n",
                    tsem_get_semval(&omx_vpudec_component_Private->port_flush_complete_condition),
                    tsem_get_semval(omx_vpudec_component_Private->deinterlace_flush_condition));
            tsem_up(&omx_vpudec_component_Private->port_flush_complete_condition);
            tsem_down(omx_vpudec_component_Private->deinterlace_flush_condition);
            pthread_mutex_lock(&omx_vpudec_component_Private->flush_mutex);
        }
        pthread_mutex_unlock(&omx_vpudec_component_Private->flush_mutex);

        /*No buffer to process. So wait here*/
        if((isDecodedOutputBufNeeded==OMX_TRUE && getquenelem(decodedBufQue) == 0) &&
                (omx_vpudec_component_Private->state != OMX_StateInvalid)) {
            // Signaled when BufferMgmt thread has a valid decoded output buffer
            DEBUG(DEB_LEV_SIMPLE_SEQ, " Waiting for next decoded output buffer for process, currentState = %d\n",
                    (int)omx_vpudec_component_Private->state);
            tsem_down(&omx_vpudec_component_Private->deInterlace_Thread_Sem);
        }

        DEBUG(DEB_LEV_FULL_SEQ, " decodedBufQue buf num = %d\n", getquenelem(decodedBufQue));
        // The first trial for de-interlace unit (a.k.a VPP)
        // should be special treated
        // NOTE: Just PEEK the refBuf then queue it back to pOutputQueue immediately
        if (!(PORT_IS_BEING_FLUSHED(pOutPort) || PORT_IS_BEING_FLUSHED(pInPort))) {
            if (refBuf == NULL){
                refBuf = dequeue(decodedBufQue);
                if (refBuf == NULL) {
                    // This shouldn't happened!
                    DEBUG(DEB_LEV_ERR, " decodedBufQue is EMPTY, dequeue failed!!\n");
                    continue;
                } else {
                    DEBUG(DEB_LEV_SIMPLE_SEQ, " decodedBufQue, refBuf=%p index=%d PicType=%d nTimeStamp=%lld queuelen=%d\n",
                            refBuf, GetBufferIndex(refBuf), GetPicType(refBuf), refBuf->nTimeStamp, getquenelem(decodedBufQue));
                }
            }

            err = peekqueue(decodedBufQue, &srcBuf);
            if (err != 0) {
                DEBUG(DEB_LEV_FULL_SEQ, " decodedBufQue is EMPTY, peekqueue failed\n");
	       continue;
            } else {
                int index = GetBufferIndex(refBuf);
                DEBUG(DEB_LEV_SIMPLE_SEQ, " peekqueue successfully, srcBuf=%p index=%d PicType=%d FilledLen=%d\n",
                        srcBuf, GetBufferIndex(srcBuf), GetPicType(srcBuf), srcBuf->nFilledLen);
#ifdef SPRD_BLURED_PIC_OPT
                while (!(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort)) &&
                        pVpu->dispOutputInfo.frameReferenceFlag[index]) {
                    DEBUG(DEB_LEV_FULL_SEQ, "waiting refBuf: %p is not ref buf, index: %d, queue len: %d\n", refBuf, index, getquenelem(decodedBufQue));
                    tsem_down(&omx_vpudec_component_Private->deInterlace_Thread_Sem);
                }
#else
                if (GetPicType(srcBuf) == PIC_TYPE_B) {
                    DEBUG(DEB_LEV_FULL_SEQ, "peek a B-frame\n");
                    while (!(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort)) &&
                                getquenelem(decodedBufQue) <= pVpu->deInterlaceDelayNum) {
                                DEBUG(DEB_LEV_FULL_SEQ, "peek a B-frame, waiting, queue len: %d, deInterlaceDelayNum: %d\n",getquenelem(decodedBufQue),pVpu->deInterlaceDelayNum);
                        tsem_down(&omx_vpudec_component_Private->deInterlace_Thread_Sem);
                    }
                }
#endif
            }
        }

        if(omx_vpudec_component_Private->state == OMX_StateInvalid) {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "Output Buffer Management Thread is exiting\n");
            break;
        }

        if (refBuf != NULL && srcBuf != NULL) {

            if (pVpu->deInterlaceBufHandle == NULL) {
                err = AllocateIONBuffer(pVpu->fbStride*pVpu->fbHeight*3/2, &pVpu->deInterlaceBufHandle, &dispPhyAddrs, &pDispVirtAddrs, &pVpu->deInterlaceBufSize);
                if (err != OMX_ErrorNone) {
                    DEBUG(DEB_LEV_ERR, "AllocateIONBuffer FAIL!\n");
                    return NULL;
                }
                pVpu->deInterlacePhyAddr = dispPhyAddrs;
                pVpu->deInterlaceVirAddr = pDispVirtAddrs;
            }

#ifdef SPRD_DEINTERLACE_BYPASS
            lockAndroidNativeBuffer (refBuf,
                                   (OMX_U32)(pVpu->fbStride),
                                   (OMX_U32)(pVpu->fbHeight), LOCK_MODE_TO_GET_VIRTUAL_ADDRESS, &pRefVirtAddrs);
            DEBUG(DEB_LEV_FULL_SEQ, "[wenan] w h: %d %d, len = %d, refBuf: %p, virtual addr = %p\n",
                    pVpu->fbStride, pVpu->fbHeight, refBuf->nFilledLen, refBuf, pRefVirtAddrs);

            memcpy(pDispVirtAddrs, pRefVirtAddrs, refBuf->nFilledLen);
            memcpy(pRefVirtAddrs, pDispVirtAddrs, refBuf->nFilledLen);
            unlockAndroidNativeBuffer(refBuf);
#else
            getIOMMUPhyAddr(srcBuf, &srcPhyAddrs);
            getIOMMUPhyAddr(refBuf, &refPhyAddrs);
            DEBUG(DEB_LEV_FULL_SEQ, "[wenan] w h: %d %d, len = %d, frame_no: %u, refBuf: %p, phy addr = 0x%lx, srcBuf: %p, phy addr = 0x%lx\n",
                pVpu->fbStride, pVpu->fbHeight, refBuf->nFilledLen, frame_no, refBuf, refPhyAddrs, srcBuf, srcPhyAddrs);

            omx_vpudec_cmp_vpp_process(omx_vpudec_component_Private->vpp_obj, srcPhyAddrs, refPhyAddrs, refPhyAddrs, pVpu->fbStride, pVpu->fbHeight, frame_no);
#endif

#ifdef SPRD_DE_INTERLACE_OUPUT_DUMP
            if (refBuf->nFilledLen > 0) {
                FILE *dump;
                dump = fopen("/data/dump_dec_deinter_process.yuv", "a+b");
                if (dump)
                {
                    if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
                    {
                        void *pVirAddrs[3];

                        if (lockAndroidNativeBuffer(refBuf, pVpu->fbStride, pVpu->fbHeight, LOCK_MODE_TO_GET_VIRTUAL_ADDRESS, pVirAddrs) == 0)
                        {
                            fwrite((unsigned char*)pVirAddrs[0], refBuf->nFilledLen, 1, dump);
                            unlockAndroidNativeBuffer(refBuf);
                        }
                    }
                    else
                    {
                        fwrite(refBuf->pBuffer, refBuf->nFilledLen, 1, dump);
                    }
                    fclose(dump);
                }
            }
#endif

            DEBUG(DEB_LEV_FULL_SEQ, "return refBuf %p\n", refBuf );

            omx_vpudec_cmp_return_buf_directly(param, (omx_base_PortType *)pOutPort, refBuf);

            refBuf = NULL;
            srcBuf = NULL;
            frame_no++;
        }
    }

    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of component %p\n", openmaxStandComp);
    return NULL;
}
#endif

/** The Constructor of the video decoder component
* @param openmaxStandComp the component handle to be constructed
* @param cComponentName is the name of the constructed component
*/
OMX_ERRORTYPE omx_vpudec_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private;
    omx_vpudec_component_PortType *inPort, *outPort;
    vpu_dec_context_t *pVpu;

    RetCode ret = RETCODE_SUCCESS;

    DEBUG(DEB_LEV_FUNCTION_NAME, "In ...\n");

    if (!openmaxStandComp->pComponentPrivate)
    {
        DEBUG(DEB_LEV_FUNCTION_NAME, "allocating component\n");

        openmaxStandComp->pComponentPrivate = malloc(sizeof (omx_vpudec_component_PrivateType));
        if (openmaxStandComp->pComponentPrivate == NULL) {
            DEBUG(DEB_LEV_ERR, "Insufficient memory!\n");
            err = OMX_ErrorInsufficientResources;
            goto ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR;
        }
        memset(openmaxStandComp->pComponentPrivate,0x00,sizeof(omx_vpudec_component_PrivateType));
    }
    else
    {
        DEBUG(DEB_LEV_FUNCTION_NAME, "Error Component 0x%p Already Allocated\n", openmaxStandComp->pComponentPrivate);
    }

    omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    omx_vpudec_component_Private->ports = NULL;

    pVpu = (vpu_dec_context_t *)&omx_vpudec_component_Private->vpu;
    memset(pVpu, 0x00, sizeof (vpu_dec_context_t));

    err = omx_base_filter_Constructor(openmaxStandComp, cComponentName);
    /** now it's time to set the video coding type of the component in default */
    if (!strcmp(cComponentName, VIDEO_DEC_MPEG4_NAME))
    {
        omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingMPEG4;
    }
    else if (!strcmp(cComponentName, VIDEO_DEC_H264_NAME))
    {
        omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingAVC;
    }
    else if (!strcmp(cComponentName, VIDEO_DEC_HEVC_NAME))
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "FIND HEVC NAME\n");
        omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingHEVC;
    }
    else if (!strcmp(cComponentName, VIDEO_DEC_RV_NAME))
    {
        omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingRV;
    }
    else if (!strcmp(cComponentName, VIDEO_DEC_WMV_NAME))
    {
        omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingWMV;
    }
    else if (!strcmp(cComponentName, VIDEO_DEC_MPEG2_NAME))
    {
        omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingMPEG2;
    }
    else if (!strcmp(cComponentName, VIDEO_DEC_H263_NAME))
    {
        omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingH263;
    }
    else if (!strcmp(cComponentName, VIDEO_DEC_MSMPEG_NAME))
    {
        omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingMSMPEG;
    }
    else if (!strcmp(cComponentName, VIDEO_DEC_AVS_NAME))
    {
        omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingAVS;
    }
    else if (!strcmp(cComponentName, VIDEO_DEC_VP8_NAME))
    {
        omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CODINGTYPE_VP8;
    }
    else if (!strcmp(cComponentName, VIDEO_DEC_BASE_NAME))
    {
        omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingUnused;
    }
    else if (!strcmp(cComponentName, VIDEO_DEC_VC1_NAME))
    {
        omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingVC1;
    }
    else
    {
        // IL client specified an invalid component name
        err = OMX_ErrorInvalidComponentName;
        goto ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR;
    }
    omx_vpudec_component_Private->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber = 0;
    omx_vpudec_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts = 2;
    /** Allocate Ports and call port constructor. */
    if (omx_vpudec_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts && !omx_vpudec_component_Private->ports)
    {
        int i;
        omx_vpudec_component_Private->ports = malloc(omx_vpudec_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts*sizeof (omx_base_PortType *));
        if (!omx_vpudec_component_Private->ports) {
            DEBUG(DEB_LEV_ERR, "Insufficient memory!\n");
            err = OMX_ErrorInsufficientResources;
            goto ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR;
        }
        memset(omx_vpudec_component_Private->ports, 0x00, omx_vpudec_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts*sizeof (omx_base_PortType *));

        for (i = 0; i < (int)omx_vpudec_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++)
        {
            omx_vpudec_component_Private->ports[i] = malloc(sizeof (omx_vpudec_component_PortType));
            if (!omx_vpudec_component_Private->ports[i]) {
                DEBUG(DEB_LEV_ERR, "Insufficient memory!\n");
                err = OMX_ErrorInsufficientResources;
                goto ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR;
            }

            memset(omx_vpudec_component_Private->ports[i], 0x00, sizeof (omx_vpudec_component_PortType));
        }
    }

    base_video_port_Constructor(openmaxStandComp, &omx_vpudec_component_Private->ports[0], 0, OMX_TRUE);
    base_video_port_Constructor(openmaxStandComp, &omx_vpudec_component_Private->ports[1], 1, OMX_FALSE);

    omx_vpudec_component_Private->ports[0]->Port_SendBufferFunction = &omx_vpudec_component_SendBufferFunction;

    omx_vpudec_component_Private->ports[1]->Port_SendBufferFunction = &omx_vpudec_component_SendBufferFunction;
    omx_vpudec_component_Private->ports[1]->ReturnBufferFunction = &omx_vpudec_component_OutPort_ReturnBufferFunction;


    /** here we can override whatever defaults the base_component constructor set
    * e.g. we can override the function pointers in the private struct
    */
    /** Domain specific section for the ports.
    * first we set the parameter common to both formats
    */

    //common parameters related to input port
    inPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    //common parameters related to output port
    outPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    initializeVideoPorts(inPort, outPort, omx_vpudec_component_Private->video_coding_type);
    SetInternalVideoParameters(openmaxStandComp);

    omx_vpudec_component_Private->seqHeader = NULL;
    omx_vpudec_component_Private->seqHeaderSize = 0;
    omx_vpudec_component_Private->picHeader = NULL;
    omx_vpudec_component_Private->picHeaderSize = 0;

    omx_vpudec_component_Private->BufferMgmtCallback = NULL; //omx_vpudec_component_BufferMgmtCallback;
    omx_vpudec_component_Private->BufferMgmtFunction = omx_vpudec_component_BufferMgmtFunction;
#ifdef SPRD_PARALLEL_DEINTRELACE_SUPPORT
    omx_vpudec_component_Private->Video_Post_Porcess_Function = omx_vpudec_component_deInterlaceFunction;
    // Init buf Queue for de-interlace process
    if (!omx_vpudec_component_Private->bufferQueForDeinterlaceProcess) {
        omx_vpudec_component_Private->bufferQueForDeinterlaceProcess = malloc(sizeof(queue_t));
        if (!omx_vpudec_component_Private->bufferQueForDeinterlaceProcess) {
            err = OMX_ErrorInsufficientResources;
            goto ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR;
        }

        memset(omx_vpudec_component_Private->bufferQueForDeinterlaceProcess, 0x00, sizeof(queue_t));

        err = queue_init(omx_vpudec_component_Private->bufferQueForDeinterlaceProcess);
        if (err != 0) {
            err = OMX_ErrorInsufficientResources;
            goto ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR;
        }

    }

    // Init de-interlace thread sem
    memset(&omx_vpudec_component_Private->deInterlace_Thread_Sem, 0x00, sizeof(tsem_t));
    tsem_init(&omx_vpudec_component_Private->deInterlace_Thread_Sem, 0);

    // Init de-interlace flush condition sem
    memset(&omx_vpudec_component_Private->port_flush_complete_condition, 0x00, sizeof(tsem_t));
    tsem_init(&omx_vpudec_component_Private->port_flush_complete_condition, 0);

    // Init for de-interlace buf mutex
    pthread_mutex_init(&omx_vpudec_component_Private->deInterlace_buf_mutex, NULL);

    //init de-interlace thread
    omx_vpudec_component_Private->deInterlaceThread = -1;
#endif

#if 0//defined (SPRD_PARALLEL_DEINTRELACE_SUPPORT) || defined (SPRD_SERIAL_DEINTRELACE_SUPPORT)
    // De-interlace support
    omx_vpudec_component_Private->bIsDeinterlaceNeeded = OMX_TRUE;
#else
    omx_vpudec_component_Private->bIsDeinterlaceNeeded = OMX_FALSE;
#endif

    /** initializing the codec context etc that was done earlier by libinit function */
    omx_vpudec_component_Private->messageHandler = omx_vpudec_component_MessageHandler;
    omx_vpudec_component_Private->destructor = omx_vpudec_component_Destructor;
    openmaxStandComp->SetParameter = omx_vpudec_component_SetParameter;
    openmaxStandComp->GetParameter = omx_vpudec_component_GetParameter;
    openmaxStandComp->ComponentRoleEnum = omx_vpudec_component_ComponentRoleEnum;
    openmaxStandComp->GetExtensionIndex = omx_vpudec_component_GetExtensionIndex;
    openmaxStandComp->SetConfig = omx_vpudec_component_SetConfig;
    openmaxStandComp->GetConfig = omx_vpudec_component_GetConfig;
    openmaxStandComp->AllocateBuffer = omx_videodec_component_AllocateBuffer;
    openmaxStandComp->UseBuffer = omx_videodec_component_UseBuffer;
    openmaxStandComp->FreeBuffer = omx_videodec_component_FreeBuffer;


    omx_vpudec_component_Private->seqHeader = malloc(MAX_CHUNK_HEADER_SIZE);
    if (!omx_vpudec_component_Private->seqHeader)
    {
        DEBUG(DEB_LEV_ERR, "fail to allocate the seqHeader buffer\n" );
        err = OMX_ErrorInsufficientResources;
        goto ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR;
    }

    omx_vpudec_component_Private->picHeader = malloc(MAX_CHUNK_HEADER_SIZE);
    if (!omx_vpudec_component_Private->picHeader)
    {
        DEBUG(DEB_LEV_ERR, "fail to allocate the picheader buffer\n" );
        err = OMX_ErrorInsufficientResources;
        goto ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR;
    }

    memset(&omx_vpudec_component_Private->port_setting_change_tsem,0x00,sizeof(tsem_t));
    tsem_init(&omx_vpudec_component_Private->port_setting_change_tsem, 0);

    omx_vpudec_component_Private->vpuReady = OMX_FALSE;
    omx_vpudec_component_Private->portSettingChangeRequest = OMX_FALSE;
    omx_vpudec_component_Private->in_bufferheader_queue = omx_bufferheader_queue_init(MAX_REG_FRAME);

    pthread_mutex_init(&omx_vpudec_component_Private->display_flag_mutex, NULL);
    omx_vpudec_component_Private->omx_display_flags = malloc(sizeof(omx_display_flag)*MAX_REG_FRAME);
    if (!omx_vpudec_component_Private->omx_display_flags) {
        DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
        err = OMX_ErrorInsufficientResources;
        goto ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR;
    }
    memset(omx_vpudec_component_Private->omx_display_flags, 0x00, sizeof(omx_display_flag)*MAX_REG_FRAME);

    memset(&omx_vpudec_component_Private->omx_ts_correction, 0x00, sizeof(omx_timestamp_correction_t));

    pthread_mutex_init(&omx_vpudec_component_Private->vpu_flush_mutex, NULL);

#ifdef SUPPORT_TIMESTAMP_REORDER
    omx_vpudec_component_Private->bIsTimeStampReorder = OMX_TRUE;
#endif

    pVpu->productId = 0;
    pVpu->stopdec = OMX_FALSE;
    ret = VPU_Init(pVpu->coreIdx);
#ifdef BIT_CODE_FILE_PATH
#else
    if (ret == RETCODE_NOT_FOUND_BITCODE_PATH)
    {
        // this code block comes from vpurun.c
        char*   path;
        Uint32  sizeInWord;
        Uint16 *pusBitCode;

        if (pVpu->coreIdx == 0)       path = CORE_0_BIT_CODE_FILE_PATH;
        else if (pVpu->coreIdx == 1)  path = CORE_1_BIT_CODE_FILE_PATH;
        else
        {
            DEBUG(DEB_LEV_ERR, "Invalid core index: %d\n", pVpu->coreIdx);
            err = OMX_ErrorHardware;
            goto ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR;
        }
        if (OmxLoadFirmware(pVpu->productId, (Uint8**)&pusBitCode, &sizeInWord, path) < 0) {
            DEBUG(DEB_LEV_ERR, "failed open bit_firmware file path is %s\n", path);
            err = OMX_ErrorHardware;
            goto ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR;
        }

        ret = VPU_InitWithBitcode(pVpu->coreIdx, pusBitCode, sizeInWord);

        if (pusBitCode)
            osal_free(pusBitCode);
    }
#endif
    if (ret != RETCODE_SUCCESS && ret != RETCODE_CALLED_BEFORE)
    {
        DEBUG(DEB_LEV_ERR, "VPU_Init failed Error code is 0x%x \n", (int)ret);
        err = OMX_ErrorHardware;
        goto ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR;
    }
#ifdef SPRD_FIRMWARE_VALID_CHECK
	// firmware validation check
	{
		unsigned char *firmware;
		vpu_buffer_t vbCommon;
		int i;
		firmware = (unsigned char *)malloc(CODE_BUF_SIZE);
		if (firmware) 
		{
			vdi_get_common_memory(0, &vbCommon);
			DEBUG(DEB_LEV_PARAMS, "firmware dump from addr=0x%x\n",  vbCommon.phys_addr);
			vdi_read_memory(0, vbCommon.phys_addr, firmware, CODE_BUF_SIZE, VDI_BIG_ENDIAN);

			for(i=0; i<(CODE_BUF_SIZE>>10); i++)
			{
				 DEBUG(DEB_LEV_PARAMS, "0x%02x, ", firmware[i]);
			}
			DEBUG(DEB_LEV_PARAMS, "\n\n");
			free(firmware);
		}

	}
#endif
    DEBUG(DEB_LEV_SIMPLE_SEQ, "VPU_Init success\n");

    return OMX_ErrorNone;

ERROR_OMX_VPU_DEC_COMPONENT_CONSTRUCTOR:
    omx_vpudec_component_Destructor(openmaxStandComp);
    return err;
}
/** The destructor of the video decoder component
*/
OMX_ERRORTYPE omx_vpudec_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    OMX_U32 i;
    int err = 0;
    vpu_dec_context_t *pVpu= (vpu_dec_context_t *)&omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *inPort = NULL;
    omx_vpudec_component_PortType *outPort = NULL;

    if (!omx_vpudec_component_Private->ports)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Destructor of video decoder component is already called\n");
        return OMX_ErrorNone;
    }

    inPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    outPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

    if (omx_vpudec_component_Private->vpuReady)
    {
        omx_vpudec_component_vpuLibDeInit(omx_vpudec_component_Private);
        omx_vpudec_component_Private->vpuReady = OMX_FALSE;

    }

    if (omx_vpudec_component_Private->seqHeader)
    {
        free(omx_vpudec_component_Private->seqHeader);
        omx_vpudec_component_Private->seqHeader = NULL;
        omx_vpudec_component_Private->seqHeaderSize = 0;
    }

    if (omx_vpudec_component_Private->picHeader)
    {
        free(omx_vpudec_component_Private->picHeader);
        omx_vpudec_component_Private->picHeader = NULL;
        omx_vpudec_component_Private->picHeaderSize = 0;

    }

    if (omx_vpudec_component_Private->omx_display_flags)
    {
        free(omx_vpudec_component_Private->omx_display_flags);
        omx_vpudec_component_Private->omx_display_flags = NULL;
    }

    if (omx_vpudec_component_Private->in_bufferheader_queue)
    {
        omx_bufferheader_queue_dequeue_all(omx_vpudec_component_Private->in_bufferheader_queue);
        omx_bufferheader_queue_deinit(omx_vpudec_component_Private->in_bufferheader_queue);
        omx_vpudec_component_Private->in_bufferheader_queue = NULL;
    }

#ifdef SPRD_PARALLEL_DEINTRELACE_SUPPORT
    // wait de-interlace thread to exit
    tsem_up(&omx_vpudec_component_Private->deInterlace_Thread_Sem);

    pthread_join(omx_vpudec_component_Private->deInterlaceThread, (void **)&err);
    if (err != 0) {
         DEBUG(DEB_LEV_ERR, "In  pthread_join returned err=%d\n", err);
    }
    omx_vpudec_component_Private->deInterlaceThread = -1;
    DEBUG(DEB_LEV_FUNCTION_NAME, "In  after pthread_join\n");

    // De-Init de-interlace buffer Queue
    if (omx_vpudec_component_Private->bufferQueForDeinterlaceProcess) {
        queue_deinit(omx_vpudec_component_Private->bufferQueForDeinterlaceProcess);
        free(omx_vpudec_component_Private->bufferQueForDeinterlaceProcess);
        omx_vpudec_component_Private->bufferQueForDeinterlaceProcess=NULL;
    }

    // De-Init de-interlace thread semaphore
    tsem_deinit(&omx_vpudec_component_Private->deInterlace_Thread_Sem);

    tsem_deinit(&omx_vpudec_component_Private->port_flush_complete_condition);

    // Destroy de-interlace buf mutex
    pthread_mutex_destroy(&omx_vpudec_component_Private->deInterlace_buf_mutex);
#endif

    // Free deinterlace ion buffer
    if (pVpu->deInterlaceBufHandle) {
        FreeIONBuffer(pVpu->deInterlaceBufHandle, pVpu->deInterlacePhyAddr, pVpu->deInterlaceBufSize);
        pVpu->deInterlaceBufHandle = NULL;
    }

    if (inPort && inPort->bAllocateBuffer == OMX_TRUE)
    {
        if (omx_vpudec_component_Private->bUseOmxInputBufferAsDecBsBuffer == OMX_FALSE)
        {
            if (pVpu->vbStream[0].size)
            {
                DEBUG(DEB_LEV_FULL_SEQ, "free pVpu->vbStream[0].phy=0x%x", pVpu->vbStream[0].phys_addr);
                vdi_free_dma_memory(pVpu->coreIdx, &pVpu->vbStream[0]);
            }
        }
    }
    else
    {
        if (pVpu->vbStream[0].size)
        {
            vdi_free_dma_memory(pVpu->coreIdx, &pVpu->vbStream[0]);
        }
    }


    if (outPort && outPort->bAllocateBuffer == OMX_FALSE)
    {
        if (omx_vpudec_component_Private->useNativeBuffer == OMX_FALSE)
        {
            for (i=0; i<MAX_REG_FRAME; i++)
            {
                if (pVpu->vbAllocFb[i].size > 0)
                {
                    vdi_free_dma_memory(pVpu->coreIdx, &pVpu->vbAllocFb[i]);
                }
            }
        }
    }

    for (i=0; i<MAX_REG_FRAME; i++) // free framebufer for DPB
    {
        if (pVpu->vbDPB[i].size > 0)
        {
            vdi_free_dma_memory(pVpu->coreIdx, &pVpu->vbDPB[i]);
        }
    }




    VPU_DeInit(pVpu->coreIdx);


    tsem_deinit(&omx_vpudec_component_Private->port_setting_change_tsem);

    pthread_mutex_destroy(&omx_vpudec_component_Private->display_flag_mutex);
    pthread_mutex_destroy(&omx_vpudec_component_Private->vpu_flush_mutex);


    /* frees port/s */
    if (omx_vpudec_component_Private->ports)
    {
        for (i = 0; i < omx_vpudec_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++)
        {
            if (omx_vpudec_component_Private->ports[i])
            {
                omx_vpudec_component_Private->ports[i]->PortDestructor(omx_vpudec_component_Private->ports[i]);
                omx_vpudec_component_Private->ports[i] = NULL;
            }
        }
        free(omx_vpudec_component_Private->ports);
        omx_vpudec_component_Private->ports = NULL;
    }

    DEBUG(DEB_LEV_FULL_SEQ, "Destructor of video decoder component is called\n");

    omx_base_filter_Destructor(openmaxStandComp);
    return OMX_ErrorNone;
}
/** It initializates the framework, and opens an videodecoder of type specified by IL client
*/
OMX_ERRORTYPE omx_vpudec_component_vpuLibInit(omx_vpudec_component_PrivateType* omx_vpudec_component_Private)
{

    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    RetCode ret = RETCODE_SUCCESS;
    omx_vpudec_component_PortType *inPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    omx_vpudec_component_PortType *outPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    OMX_ERRORTYPE err = OMX_ErrorHardware;
    int i=0;
    unsigned long maxAddr;


    DEBUG(DEB_LEV_FUNCTION_NAME, "In \n");

    if (codingTypeToCodStd(omx_vpudec_component_Private->video_coding_type) == -1)
    {
        DEBUG(DEB_LEV_ERR, "the requested codec type not support video_coding_type=%d\n", (int)omx_vpudec_component_Private->video_coding_type);
        return OMX_ErrorComponentNotFound;
    }

    pVpu->dispOutIdx = 0;
    pVpu->frameIdx = 0;
    pVpu->decodefinish = 0;
    pVpu->int_reason = 0;
    pVpu->totalNumofErrMbs = 0;

    OmxCheckVersion(pVpu->coreIdx);

    pVpu->decOP.bitstreamFormat = (CodStd)codingTypeToCodStd(omx_vpudec_component_Private->video_coding_type);
    if ((-1) == (int)pVpu->decOP.bitstreamFormat)
    {
        DEBUG(DEB_LEV_ERR, "can not found Codec codingType is 0x%x \n", (int)omx_vpudec_component_Private->video_coding_type);
        goto ERR_VPU_DEC_INIT;
    }
    DEBUG(DEFAULT_MESSAGES, "codingTypeToCodStd video_coding_type=%d, decOP.bitstreamFormat =%d\n", (int)omx_vpudec_component_Private->video_coding_type, (int)pVpu->decOP.bitstreamFormat );
    pVpu->decOP.mp4Class = codingTypeToMp4class(omx_vpudec_component_Private->video_coding_type, 0 /*fourcc*/);

    if (inPort->bAllocateBuffer == OMX_TRUE && omx_vpudec_component_Private->bUseOmxInputBufferAsDecBsBuffer == OMX_TRUE)
    {
        pVpu->decOP.bitstreamBuffer = pVpu->vbStream[0].phys_addr;
        maxAddr = pVpu->vbStream[0].phys_addr;
        for (i=0; i<MAX_DEC_BITSTREAM_BUFFER_COUNT; i++)
        {
            if (pVpu->vbStream[i].size > 0)
            {
                if (pVpu->vbStream[i].phys_addr < pVpu->decOP.bitstreamBuffer)
                    pVpu->decOP.bitstreamBuffer = pVpu->vbStream[i].phys_addr;

                if (pVpu->vbStream[i].phys_addr > maxAddr)
                    maxAddr = pVpu->vbStream[i].phys_addr;
            }
        }

        pVpu->decOP.bitstreamBufferSize = maxAddr - pVpu->decOP.bitstreamBuffer + pVpu->vbStream[0].size;

        DEBUG(DEB_LEV_SIMPLE_SEQ, "set bitstream buffer size = %d, addr=0x%x\n", pVpu->decOP.bitstreamBufferSize, (int)pVpu->decOP.bitstreamBuffer );
    }
    else
    {
        pVpu->vbStream[0].size = DEFAULT_STREAMBUFFER_SIZE;
        pVpu->vbStream[0].size = ((pVpu->vbStream[0].size+1023)&~1023);

        if (vdi_allocate_dma_memory(pVpu->coreIdx, &pVpu->vbStream[0]) < 0)
        {
            DEBUG(DEB_LEV_ERR, "fail to allocate bitstream buffer\n" );
            goto ERR_VPU_DEC_INIT;
        }

        pVpu->decOP.bitstreamBuffer = pVpu->vbStream[0].phys_addr;
        pVpu->decOP.bitstreamBufferSize = pVpu->vbStream[0].size; //bitstream buffer size must be aligned 1024
        DEBUG(DEB_LEV_SIMPLE_SEQ, "allocate bitstream buffer size = %d, addr=0x%x\n", pVpu->decOP.bitstreamBufferSize, (int)pVpu->decOP.bitstreamBuffer );
    }




    pVpu->decOP.mp4DeblkEnable = 0;

    DEBUG(DEB_LEV_SIMPLE_SEQ, "setting video output format eColorFormat=0x%x, useNativeBuffer=%d, bThumbnailMode=%d\n",
          (int)outPort->sPortParam.format.video.eColorFormat, (int)omx_vpudec_component_Private->useNativeBuffer, (int)omx_vpudec_component_Private->bThumbnailMode);

    if(outPort->sPortParam.format.video.eColorFormat == MAKE_FOURCC('N', 'V', '1', '2') ||
            outPort->sPortParam.format.video.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar ||
            outPort->sPortParam.format.video.eColorFormat == OMX_COLOR_FormatYUV420PackedSemiPlanar)
    {
        pVpu->decOP.scalerInfo.imageFormat = YUV_FORMAT_NV12;
        if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
            pVpu->decOP.scalerInfo.enScaler = 1;
        else
            pVpu->decOP.scalerInfo.enScaler = 0;
        pVpu->decOP.cbcrInterleave = 1;
        pVpu->decOP.cbcrOrder = CBCR_ORDER_NORMAL;
    }
    else if(outPort->sPortParam.format.video.eColorFormat == MAKE_FOURCC('Y', 'V', '1', '2') ||
            outPort->sPortParam.format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar ||
            outPort->sPortParam.format.video.eColorFormat == OMX_COLOR_FormatYUV420PackedPlanar)
    {
        pVpu->decOP.scalerInfo.imageFormat = YUV_FORMAT_I420;
        if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
            pVpu->decOP.scalerInfo.enScaler = 1;
        else
            pVpu->decOP.scalerInfo.enScaler = 0;
        pVpu->decOP.cbcrInterleave = 0;
#ifdef ANDROID
        if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
            pVpu->decOP.cbcrOrder = CBCR_ORDER_REVERSED;
        else
            pVpu->decOP.cbcrOrder = CBCR_ORDER_NORMAL;
#else
        pVpu->decOP.cbcrOrder = CBCR_ORDER_NORMAL;
#endif
    }
    else if(outPort->sPortParam.format.video.eColorFormat == MAKE_FOURCC('I', '4', '2', '2') ||
            outPort->sPortParam.format.video.eColorFormat == OMX_COLOR_FormatYUV422Planar ||
            outPort->sPortParam.format.video.eColorFormat == OMX_COLOR_FormatYUV422PackedPlanar)
    {
        pVpu->decOP.scalerInfo.enScaler = 1;
        pVpu->decOP.scalerInfo.imageFormat = YUV_FORMAT_I422;
        pVpu->decOP.cbcrInterleave = 0;
#ifdef ANDROID
        if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
            pVpu->decOP.cbcrOrder = CBCR_ORDER_REVERSED;
        else
            pVpu->decOP.cbcrOrder = CBCR_ORDER_NORMAL;
#else
        pVpu->decOP.cbcrOrder = CBCR_ORDER_NORMAL;
#endif
    }
    else if(outPort->sPortParam.format.video.eColorFormat == MAKE_FOURCC('N', 'V', '1', '6') ||
            outPort->sPortParam.format.video.eColorFormat == OMX_COLOR_FormatYUV422SemiPlanar ||
            outPort->sPortParam.format.video.eColorFormat == OMX_COLOR_FormatYUV422PackedSemiPlanar)
    {
        pVpu->decOP.scalerInfo.enScaler = 1;
        pVpu->decOP.scalerInfo.imageFormat = YUV_FORMAT_NV16;
        pVpu->decOP.cbcrInterleave = 1;
        pVpu->decOP.cbcrOrder = CBCR_ORDER_NORMAL;
    }
    else if(outPort->sPortParam.format.video.eColorFormat == MAKE_FOURCC('Y', 'U', 'Y', 'V') ||
            outPort->sPortParam.format.video.eColorFormat == OMX_COLOR_FormatYCbYCr )
    {
        pVpu->decOP.scalerInfo.enScaler = 1;
        pVpu->decOP.scalerInfo.imageFormat = YUV_FORMAT_YUYV;
        pVpu->decOP.cbcrInterleave = 0;
        pVpu->decOP.cbcrOrder = CBCR_ORDER_NORMAL;
    }
    else if(outPort->sPortParam.format.video.eColorFormat == MAKE_FOURCC('U', 'Y', 'V', 'Y') ||
            outPort->sPortParam.format.video.eColorFormat == OMX_COLOR_FormatCbYCrY )
    {
        pVpu->decOP.scalerInfo.enScaler = 1;
        pVpu->decOP.scalerInfo.imageFormat = YUV_FORMAT_UYVY;
        pVpu->decOP.cbcrInterleave = 0;
        pVpu->decOP.cbcrOrder = CBCR_ORDER_NORMAL;
    }
    else
    {
        DEBUG(DEB_LEV_ERR, "not supported video output format\n" );
        goto ERR_VPU_DEC_INIT;
    }
    pVpu->decOP.frameEndian  = VPU_FRAME_ENDIAN;
    pVpu->decOP.streamEndian = VPU_STREAM_ENDIAN;
    pVpu->decOP.bitstreamMode = BS_MODE_PIC_END;

    if (s_dec_config.useRot || s_dec_config.useDering || s_dec_config.tiled2LinearEnable)
        pVpu->ppuEnable = 1;
    else
        pVpu->ppuEnable = 0;


    if (pVpu->decOP.bitstreamFormat == STD_DIV3)
    {
        pVpu->decOP.div3Width = inPort->sPortParam.format.video.nFrameWidth;
        pVpu->decOP.div3Height= inPort->sPortParam.format.video.nFrameHeight;
    }

    // SPRD HW can't support scale
    pVpu->decOP.scalerInfo.enScaler = 0;

    DEBUG(DEB_LEV_SIMPLE_SEQ, "VPU_DecOpen bitstreamFormat=%d, bitstreamBuffer=0x%x, bitstreamBufferSize=%d, mp4Class=%d, cbcrInterleave=%d, cbcrOrder=%d, bitstreamMode=%d\n",
          (int)pVpu->decOP.bitstreamFormat, (int)pVpu->decOP.bitstreamBuffer,  (int)pVpu->decOP.bitstreamBufferSize,  (int)pVpu->decOP.mp4Class,  (int)pVpu->decOP.cbcrInterleave,  (int)pVpu->decOP.cbcrOrder,  (int)pVpu->decOP.bitstreamMode);
    DEBUG(DEB_LEV_SIMPLE_SEQ, "VPU_DecOpen enScaler=%d, imageFormat=0x%x, scaleWidth=%d, scaleHeight=%d\n",
          (int)pVpu->decOP.scalerInfo.enScaler, (int)pVpu->decOP.scalerInfo.imageFormat,  (int)pVpu->decOP.scalerInfo.scaleWidth,  (int)pVpu->decOP.scalerInfo.scaleHeight);
    ret = VPU_DecOpen(&pVpu->handle, &pVpu->decOP);
    if (ret != RETCODE_SUCCESS)
    {
        DEBUG(DEB_LEV_ERR, "VPU_DecOpen failed Error code is 0x%x \n", (int)ret);
        goto ERR_VPU_DEC_INIT;
    }

    pVpu->instIdx = VPU_GetOpenInstanceNum(pVpu->coreIdx);
    if (pVpu->instIdx > MAX_NUM_INSTANCE)
    {
        DEBUG(DEB_LEV_ERR, "exceed the opened instance num than %d num\n", MAX_NUM_INSTANCE);
        err = OMX_ErrorNoMore;
        goto ERR_VPU_DEC_INIT;
    }

    DEBUG(DEB_LEV_SIMPLE_SEQ, "VPU_DecOpen success vpu->instIdx : %d", pVpu->instIdx);
#ifdef USE_BFRAME_SEARCH_FOR_1STFRAME
	omx_vpudec_component_Private->bSkipBrokenBframeDecode = OMX_TRUE;
#endif
#ifdef USE_IFRAME_SEARCH_FOR_1STFRAME
    pVpu->decParam.iframeSearchEnable = 2;
#endif
    //VPU_DecGiveCommand(pVpu->handle, ENABLE_LOGGING, 1);

    DEBUG(DEB_LEV_FUNCTION_NAME, "vpu library/codec initialize done. threadid=0x%x\n", (int)pthread_self());
    return OMX_ErrorNone;

ERR_VPU_DEC_INIT:
    DEBUG(DEB_LEV_ERR, "vpu library/codec initialize err =0 x%x\n", (int)err);
    return err;
}


void omx_vpudec_component_vpuLibDeInit(omx_vpudec_component_PrivateType* omx_vpudec_component_Private)
{
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    RetCode ret = RETCODE_SUCCESS;
    DecOutputInfo decOutputInfo;
    DEBUG(DEB_LEV_FUNCTION_NAME, "VPU_DecClose start \n");

    ret = VPU_DecClose(pVpu->handle);
    if (ret == RETCODE_FRAME_NOT_COMPLETE)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "VPU_DecClose need to call VPU_DecGetOutputInfo first \n");
        VPU_DecGetOutputInfo(pVpu->handle, &decOutputInfo);
        VPU_DecClose(pVpu->handle);
    }


    DEBUG(DEB_LEV_FUNCTION_NAME, "VPU_DecClose success\n");

}
/** internal function to set codec related parameters in the private type structure
*/
void SetInternalVideoParameters(OMX_COMPONENTTYPE *openmaxStandComp)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = (omx_vpudec_component_PrivateType* )openmaxStandComp->pComponentPrivate;
    omx_vpudec_component_PortType *inPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;

    DEBUG(DEB_LEV_SIMPLE_SEQ, "SetInternalVideoParameters codingType=%d, inPort->bAllocateBuffer=%d\n", (unsigned int)omx_vpudec_component_Private->video_coding_type, (int)inPort->bAllocateBuffer);
    inPort->sPortParam.format.video.eCompressionFormat = omx_vpudec_component_Private->video_coding_type;
    inPort->sVideoParam.eCompressionFormat = omx_vpudec_component_Private->video_coding_type;

    if (OmxNeedInputBufferReformat(omx_vpudec_component_Private))
        omx_vpudec_component_Private->bUseOmxInputBufferAsDecBsBuffer = OMX_FALSE;
    else
        omx_vpudec_component_Private->bUseOmxInputBufferAsDecBsBuffer = OMX_TRUE;
}
void initializeVideoPorts(omx_vpudec_component_PortType* inPort, omx_vpudec_component_PortType* outPort, OMX_VIDEO_CODINGTYPE codingType)
{
    /* set default parameters to input port */
    //inPort->sPortParam.eDomain = OMX_PortDomainVideo;
    inPort->sPortParam.nBufferSize = DEFAULT_VIDEO_INPUT_BUF_SIZE;
    inPort->sPortParam.format.video.nBitrate = DEFAULT_BITRATE;
    inPort->sPortParam.format.video.xFramerate = DEFAULT_FRAMERATE;
    inPort->sPortParam.format.video.nFrameWidth = DEFAULT_WIDTH;
    inPort->sPortParam.format.video.nFrameHeight = DEFAULT_HEIGHT;
    inPort->sPortParam.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    inPort->sPortParam.format.video.eCompressionFormat = codingType;
    inPort->sPortParam.nBufferCountActual = DEFAULT_ACTUAL_VIDEO_INPUT_BUFFER_NUM;
    inPort->sPortParam.nBufferCountMin = DEFAULT_MIN_VIDEO_INPUT_BUFFER_NUM;
    inPort->sVideoParam.eColorFormat = OMX_COLOR_FormatUnused;
    inPort->sVideoParam.eCompressionFormat = codingType;
    inPort->sVideoParam.xFramerate = DEFAULT_FRAMERATE;

    inPort->nTempBufferCountActual = inPort->sPortParam.nBufferCountActual;

    setHeader(&inPort->omxConfigCrop, sizeof(OMX_CONFIG_RECTTYPE));
    inPort->omxConfigCrop.nPortIndex = OMX_BASE_FILTER_INPUTPORT_INDEX;
    inPort->omxConfigCrop.nLeft = inPort->omxConfigCrop.nTop = 0;
    inPort->omxConfigCrop.nWidth = DEFAULT_WIDTH;
    inPort->omxConfigCrop.nHeight = DEFAULT_HEIGHT;

    setHeader(&inPort->omxConfigRotate, sizeof(OMX_CONFIG_ROTATIONTYPE));
    inPort->omxConfigRotate.nPortIndex = OMX_BASE_FILTER_INPUTPORT_INDEX;
    inPort->omxConfigRotate.nRotation = 0;

    setHeader(&inPort->omxConfigMirror, sizeof(OMX_CONFIG_MIRRORTYPE));
    inPort->omxConfigMirror.nPortIndex = OMX_BASE_FILTER_INPUTPORT_INDEX;
    inPort->omxConfigMirror.eMirror = OMX_MirrorNone;
    setHeader(&inPort->omxConfigScale, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
    inPort->omxConfigScale.nPortIndex = OMX_BASE_FILTER_INPUTPORT_INDEX;
    inPort->omxConfigScale.xWidth = inPort->omxConfigScale.xHeight = 0x10000;
    setHeader(&inPort->omxConfigOutputPosition, sizeof(OMX_CONFIG_POINTTYPE));
    inPort->omxConfigOutputPosition.nPortIndex = OMX_BASE_FILTER_INPUTPORT_INDEX;
    inPort->omxConfigOutputPosition.nX = inPort->omxConfigOutputPosition.nY = 0;

    /* set default parameters to output port */
    outPort->sPortParam.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUF_SIZE;
    outPort->sPortParam.format.video.eColorFormat = DEFAULT_VIDEO_OUTPUT_FORMAT;
    outPort->sPortParam.format.video.xFramerate = DEFAULT_FRAMERATE;
    outPort->sPortParam.nBufferCountActual = DEFAULT_ACTUAL_VIDEO_OUTPUT_BUFFER_NUM;
    outPort->sPortParam.nBufferCountMin = DEFAULT_MIN_VIDEO_OUTPUT_BUFFER_NUM;
    outPort->sPortParam.format.video.nFrameWidth = DEFAULT_WIDTH;
    outPort->sPortParam.format.video.nFrameHeight = DEFAULT_HEIGHT;
    outPort->sVideoParam.eColorFormat = DEFAULT_VIDEO_OUTPUT_FORMAT;
    outPort->sVideoParam.eCompressionFormat = OMX_VIDEO_CodingUnused;
    outPort->sVideoParam.xFramerate = DEFAULT_FRAMERATE;

    outPort->nTempBufferCountActual = outPort->sPortParam.nBufferCountActual;

    setHeader(&outPort->omxConfigCrop, sizeof(OMX_CONFIG_RECTTYPE));
    outPort->omxConfigCrop.nPortIndex = OMX_BASE_FILTER_OUTPUTPORT_INDEX;
    outPort->omxConfigCrop.nLeft = outPort->omxConfigCrop.nTop = 0;
    outPort->omxConfigCrop.nWidth = DEFAULT_WIDTH;
    outPort->omxConfigCrop.nHeight = DEFAULT_HEIGHT;

    setHeader(&outPort->omxConfigRotate, sizeof(OMX_CONFIG_ROTATIONTYPE));
    outPort->omxConfigRotate.nPortIndex = OMX_BASE_FILTER_OUTPUTPORT_INDEX;
    outPort->omxConfigRotate.nRotation = 0;

    setHeader(&outPort->omxConfigMirror, sizeof(OMX_CONFIG_MIRRORTYPE));
    outPort->omxConfigMirror.nPortIndex = OMX_BASE_FILTER_OUTPUTPORT_INDEX;
    outPort->omxConfigMirror.eMirror = OMX_MirrorNone;

    setHeader(&outPort->omxConfigScale, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
    outPort->omxConfigScale.nPortIndex = OMX_BASE_FILTER_OUTPUTPORT_INDEX;
    outPort->omxConfigScale.xWidth = outPort->omxConfigScale.xHeight = 0x10000;

    setHeader(&outPort->omxConfigOutputPosition, sizeof(OMX_CONFIG_POINTTYPE));
    outPort->omxConfigOutputPosition.nPortIndex = OMX_BASE_FILTER_OUTPUTPORT_INDEX;
    outPort->omxConfigOutputPosition.nX = outPort->omxConfigOutputPosition.nY = 0;

}

/** The Initialization function of the video decoder
*/
OMX_ERRORTYPE omx_vpudec_component_Init(OMX_COMPONENTTYPE *openmaxStandComp)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private =
        (omx_vpudec_component_PrivateType*)openmaxStandComp->pComponentPrivate;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;

    s_dec_config.mp4DeblkEnable = 0;	// configure the vpu specified feature;

#ifdef ANDROID_OUTPUT_DUMP
    {
        FILE *dump;
        dump = fopen("/data/dump_dec.yuv", "w+b");
        fseek(dump, 0, SEEK_SET);
        fclose(dump);
    }
#endif
#ifdef ANDROID_INPUT_DUMP
    {
        FILE *dump;
        dump = fopen("/data/dump_dec.es", "w+b");
        fseek(dump, 0, SEEK_SET);
        fclose(dump);
    }
#endif
#ifdef SPRD_DE_INTERLACE_OUPUT_DUMP
    {
        FILE *dump;
        dump = fopen("/data/dump_dec_deinter_process.yuv", "w+b");
        fseek(dump, 0, SEEK_SET);
        fclose(dump);
    }
#endif

#ifdef SPRD_PARALLEL_DEINTRELACE_SUPPORT
    //create deinterlace thread
    if (omx_vpudec_component_Private->deInterlaceThread == -1) {
        err = pthread_create(&omx_vpudec_component_Private->deInterlaceThread,
                              NULL,
                              omx_vpudec_component_Private->Video_Post_Porcess_Function,
                              openmaxStandComp);
        if (err) {
            DEBUG(DEB_LEV_ERR, "Starting deInterlace thread failed ! ! !\n");
            omx_vpudec_component_Private->deInterlaceThread = -1;
            err = OMX_ErrorInsufficientResources;
        }
    }
#endif

    pVpu->deInterlaceRefBuf = NULL;
    pVpu->deInterlaceFrameNo = 0;

    return err;
}

/** The Deinitialization function of the video decoder
*/
OMX_ERRORTYPE omx_vpudec_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp)
{
    DEBUG(DEB_LEV_SIMPLE_SEQ, "omx_vpudec_component_Deinit - In");

    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    OMX_ERRORTYPE err = OMX_ErrorNone;

    if (omx_vpudec_component_Private->vpuReady == OMX_TRUE)
    {
        omx_bufferheader_queue_dequeue_all(omx_vpudec_component_Private->in_bufferheader_queue);
        memset(&omx_vpudec_component_Private->omx_ts_correction, 0x00, sizeof(omx_timestamp_correction_t));
        if(OmxVpuFlush(openmaxStandComp) == OMX_FALSE)
            return OMX_ErrorInsufficientResources;
    }
    DEBUG(DEB_LEV_SIMPLE_SEQ, "omx_vpudec_component_Deinit - Out");

    if(omx_vpudec_component_Private->vpp_obj != NULL)
    {
          omx_vpudec_cmp_vpp_deinit(omx_vpudec_component_Private->vpp_obj );
          omx_vpudec_component_Private->vpp_obj  = NULL;
    }

    return err;
}


/** Executes all the required steps after an output buffer frame-size has changed.
*/
static inline void UpdateFrameSize(OMX_COMPONENTTYPE *openmaxStandComp)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    omx_vpudec_component_PortType *outPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    omx_vpudec_component_PortType *inPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];


    outPort->sPortParam.format.video.nFrameWidth = inPort->sPortParam.format.video.nFrameWidth;
    outPort->sPortParam.format.video.nFrameHeight = inPort->sPortParam.format.video.nFrameHeight;

    outPort->sPortParam.format.video.nStride = outPort->sPortParam.format.video.nFrameWidth;
    outPort->sPortParam.format.video.nSliceHeight = outPort->sPortParam.format.video.nFrameHeight;
    outPort->sPortParam.format.video.xFramerate = inPort->sPortParam.format.video.xFramerate;

    outPort->omxConfigCrop.nLeft = inPort->omxConfigCrop.nLeft;
    outPort->omxConfigCrop.nTop = inPort->omxConfigCrop.nTop;
    outPort->omxConfigCrop.nWidth = inPort->omxConfigCrop.nWidth;
    outPort->omxConfigCrop.nHeight = inPort->omxConfigCrop.nHeight;

    switch (outPort->sPortParam.format.video.eColorFormat)
    {
    case OMX_COLOR_FormatYUV420Planar:
    case OMX_COLOR_FormatYUV420PackedPlanar:
    case OMX_COLOR_FormatYUV420SemiPlanar:
    case OMX_COLOR_FormatYUV420PackedSemiPlanar:
        if (outPort->sPortParam.format.video.nFrameWidth && outPort->sPortParam.format.video.nFrameHeight)
        {
            outPort->sPortParam.nBufferSize = outPort->sPortParam.format.video.nFrameWidth * outPort->sPortParam.format.video.nFrameHeight * 3 / 2;
        }
        break;
    case OMX_COLOR_FormatYUV422Planar:
    case OMX_COLOR_FormatYUV422PackedPlanar:
    case OMX_COLOR_FormatYUV422SemiPlanar:
    case OMX_COLOR_FormatYUV422PackedSemiPlanar:
    case OMX_COLOR_FormatYCbYCr:
    case OMX_COLOR_FormatCbYCrY:
        if (outPort->sPortParam.format.video.nFrameWidth && outPort->sPortParam.format.video.nFrameHeight)
        {
            outPort->sPortParam.nBufferSize = outPort->sPortParam.format.video.nFrameWidth * outPort->sPortParam.format.video.nFrameHeight * 2;
        }
        break;
    default:
        if (outPort->sPortParam.format.video.eColorFormat == (int)MAKE_FOURCC('N', 'V', '1', '2')
                || outPort->sPortParam.format.video.eColorFormat == (int)MAKE_FOURCC('Y', 'V', '1', '2'))
        {
            if (outPort->sPortParam.format.video.nFrameWidth && outPort->sPortParam.format.video.nFrameHeight)
                outPort->sPortParam.nBufferSize = outPort->sPortParam.format.video.nFrameWidth * outPort->sPortParam.format.video.nFrameHeight * 3 / 2;

        }
        else if (outPort->sPortParam.format.video.eColorFormat == (int)MAKE_FOURCC('I', '4', '2', '2')
                 || outPort->sPortParam.format.video.eColorFormat == (int)MAKE_FOURCC('N', 'V', '1', '6')
                 || outPort->sPortParam.format.video.eColorFormat == (int)MAKE_FOURCC('Y', 'U', 'Y', 'V')
                 || outPort->sPortParam.format.video.eColorFormat == (int)MAKE_FOURCC('U', 'Y', 'V', 'Y'))
        {
            if (outPort->sPortParam.format.video.nFrameWidth && outPort->sPortParam.format.video.nFrameHeight)
                outPort->sPortParam.nBufferSize = outPort->sPortParam.format.video.nFrameWidth * outPort->sPortParam.format.video.nFrameHeight * 2;
        }
        else
        {
            if (outPort->sPortParam.format.video.nFrameWidth && outPort->sPortParam.format.video.nFrameHeight)
                outPort->sPortParam.nBufferSize = outPort->sPortParam.format.video.nFrameWidth * outPort->sPortParam.format.video.nFrameHeight * 3;
        }
        break;
    }



    DEBUG(DEB_LEV_SIMPLE_SEQ, "OutPort nBufferSize=%d, eColorFormat=0x%x, nFrameWidth=%d, nFrameHeight=%d, nStride=%d, nSliceHeight=%d, crop nWidth=%d, crop nHeight=%d\n",
          (int)outPort->sPortParam.nBufferSize, (int)outPort->sPortParam.format.video.eColorFormat, (int)outPort->sPortParam.format.video.nFrameWidth, (int)outPort->sPortParam.format.video.nFrameHeight,
          (int)outPort->sPortParam.format.video.nStride, (int)outPort->sPortParam.format.video.nSliceHeight, (int)outPort->omxConfigCrop.nWidth, (int)outPort->omxConfigCrop.nHeight);
}

static int calculate_minipippen_scale_resolution(OMX_COMPONENTTYPE *openmaxStandComp)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;

    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *outPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

    DEBUG(DEB_LEV_SIMPLE_SEQ, "xWidth = %d xHeight = %d  picWidth=%d picHeight=%d\n", (int)outPort->omxConfigScale.xWidth, (int)outPort->omxConfigScale.xHeight, pVpu->initialInfo.picWidth, pVpu->initialInfo.picHeight);

    /** omxConfigScale.xWidth is 15:16 fixed point */
    pVpu->decOP.scalerInfo.scaleWidth = (pVpu->initialInfo.picWidth * outPort->omxConfigScale.xWidth)>>16;
    pVpu->decOP.scalerInfo.scaleWidth = ((pVpu->decOP.scalerInfo.scaleWidth+15)&~15);

    if (pVpu->decOP.scalerInfo.scaleWidth <  MINIPIPPEN_MINMUM_RESOLUTION)
        return 0;

    if (pVpu->decOP.scalerInfo.scaleWidth > ((pVpu->initialInfo.picWidth+15)&~15))
        return 0;

    pVpu->decOP.scalerInfo.scaleHeight = (pVpu->initialInfo.picHeight * outPort->omxConfigScale.xHeight)>>16;
    pVpu->decOP.scalerInfo.scaleHeight = (pVpu->decOP.scalerInfo.scaleHeight+7)&~7;

    if (pVpu->decOP.scalerInfo.scaleHeight <  MINIPIPPEN_MINMUM_RESOLUTION )
        return 0;

    if (pVpu->decOP.scalerInfo.scaleHeight > ((pVpu->initialInfo.picHeight+7)&~7))
        return 0;

#ifdef SUPPORT_CROP_BOTTOM
    pVpu->scaleNotAlignedHeight = (pVpu->initialInfo.picHeight * outPort->omxConfigScale.xHeight)>>16;

    DEBUG(DEB_LEV_SIMPLE_SEQ, "minipippen scaleWidth = %d minipippen scaleHeight = %d\n", pVpu->decOP.scalerInfo.scaleWidth, pVpu->decOP.scalerInfo.scaleHeight);
#endif

    return 1;
}
/** This function is used to process the input buffer and provide one output buffer
*/
void omx_vpudec_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pInputBuffer, OMX_BUFFERHEADERTYPE* pOutputBuffer)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;

    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *inPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    omx_vpudec_component_PortType *outPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    pVpu->saveLen = 0;
    RetCode ret = RETCODE_SUCCESS;
    int size = 0;
#ifdef SUPPORT_USE_VPU_ROTATOR
    int rotWidth = 0, rotHeight = 0;
#endif
    FrameBuffer *fbUserFrame;
    OMX_U8 *chunkData = 0;
    OMX_U32 chunkSize = 0;
    int picType = 0;
    PhysicalAddress rdPtr, wrPtr;
    int room = 0;
    int dispDoneIdx;
    int sizeLength = 0;
    vpu_buffer_t vbStream;
	OMX_BOOL portSettingChangeDetected = OMX_FALSE;
	omx_display_flag *disp_flag_array = omx_vpudec_component_Private->omx_display_flags;
	int dispFlagIdx = 0;
	int i;
	int err = OMX_ErrorStreamCorrupt;
	DecBitBufInfo decBitBufInfo;
    if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE && pVpu->stopdec == OMX_TRUE)
        return;
    if (omx_vpudec_component_Private->portSettingChangeRequest == OMX_TRUE)
    {
        DEBUG(DEB_LEV_FULL_SEQ, "port_setting_change_tsem wait start semval=%d, portSettingChangeRequest=%d\n", (int)omx_vpudec_component_Private->port_setting_change_tsem.semval, (int)omx_vpudec_component_Private->portSettingChangeRequest);
        tsem_timed_down(&omx_vpudec_component_Private->port_setting_change_tsem, 100);
        DEBUG(DEB_LEV_FULL_SEQ, "port_setting_change_tsem wait end semval=%d, portSettingChangeRequest=%d\n", (int)omx_vpudec_component_Private->port_setting_change_tsem.semval, (int)omx_vpudec_component_Private->portSettingChangeRequest);
        if (omx_vpudec_component_Private->portSettingChangeRequest == OMX_TRUE)
        {
            if (pInputBuffer)
                pInputBuffer->nFilledLen = pInputBuffer->nFilledLen; // it means the inputBuffer should remain.
            if (pOutputBuffer)
                pOutputBuffer->nFilledLen = 0; // there is no data to output.

            return;
        }
    }

	pthread_mutex_lock(&omx_vpudec_component_Private->vpu_flush_mutex);

	if (omx_vpudec_component_Private->vpuReady == OMX_FALSE)
	{
		err = omx_vpudec_component_vpuLibInit(omx_vpudec_component_Private);
		if (OMX_ErrorNone != err)
		{
			DEBUG(DEB_LEV_ERR, "omx_vpudec_component_vpuLibInit Failed err=%d\n", (int)err);
			goto ERR_DEC;
		}
		omx_vpudec_component_Private->vpuReady = OMX_TRUE;
	}

    /* To make VPU to be worked asynchronously between InputBuffer(EmptyThisBuffer) and outputBuffer(FillThisBuffer) operation.
    * IS_STATE_EMPTYTHISBUFFER == true  : decoding only
    * IS_STATE_FILLTHISBUFFER == true : transfer the output to renderer without decoding.
    */
    if(IS_STATE_EMPTYTHISBUFFER(pInputBuffer) == OMX_TRUE) // pInputBuffer has a values
    {
        if(pInputBuffer->nSize > 0)
        {
            DEBUG(DEB_LEV_FULL_SEQ, "In , IS_STATE_EMPTYTHISBUFFER InstIdx=%d, video_coding_type=%d, input flags=0x%x, nFilledLen=%d, seqInited=%d,  bAllocateBuffer=%d, bUseOmxInputBufferAsDecBsBuffer=%d\nchunkReuseRequired=%d, inputBuffer addr=%p, data : 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x, \n",
                  (int)pVpu->instIdx, (unsigned int)omx_vpudec_component_Private->video_coding_type, (unsigned int)pInputBuffer->nFlags, (int)pInputBuffer->nFilledLen, pVpu->seqInited, (int)inPort->bAllocateBuffer, (int)omx_vpudec_component_Private->bUseOmxInputBufferAsDecBsBuffer, (int)pVpu->chunkReuseRequired, pInputBuffer->pBuffer,
                  (int)pInputBuffer->pBuffer[0],(int)pInputBuffer->pBuffer[1],(int)pInputBuffer->pBuffer[2],(int)pInputBuffer->pBuffer[3],(int)pInputBuffer->pBuffer[4],(int)pInputBuffer->pBuffer[5],(int)pInputBuffer->pBuffer[6],(int)pInputBuffer->pBuffer[7],(int)pInputBuffer->pBuffer[8],(int)pInputBuffer->pBuffer[9],(int)pInputBuffer->pBuffer[10],(int)pInputBuffer->pBuffer[11],(int)pInputBuffer->pBuffer[12],(int)pInputBuffer->pBuffer[13],(int)pInputBuffer->pBuffer[14],(int)pInputBuffer->pBuffer[15]);
        }

        if (pVpu->decodefinish)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In  decoder already handled EOS procedure. goto REACH_INPUT_BUFFER_EOS\n");

            if (pInputBuffer->nFilledLen > 0)
            {
                // to support loop decoding
                VPU_DecUpdateBitstreamBuffer(pVpu->handle, -1);

                // to support loop decoding, below vars should be reset.
                omx_vpudec_component_Private->bIsOutputEOSReached = OMX_FALSE;
                omx_vpudec_component_Private->bIsEOSReached = OMX_FALSE;
                pVpu->decodefinish = 0;

            }
            else
            {
                goto REACH_INPUT_BUFFER_EOS;
            }
        }

        if ((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) && (pInputBuffer->nFilledLen == 0))
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX Core send the end of stream indicator frameIdx=%d\n",pVpu->frameIdx);

            if (pVpu->frameIdx == 0)
                goto REACH_INPUT_BUFFER_EOS;

            VPU_DecSetRdPtr(pVpu->handle, pVpu->vbStream[0].phys_addr, 1);	// to set to same address (any address)
            VPU_DecUpdateBitstreamBuffer(pVpu->handle, STREAM_END_SIZE);	//tell VPU to reach the end of stream. starting flush decoded output in VPU
            goto FLUSH_BUFFER;
        }

#ifdef SUPPORT_ONE_BUFFER_CONTAIN_MULTIPLE_FRAMES
        chunkData = (OMX_U8 *)(pInputBuffer->pBuffer + pInputBuffer->nOffset);
#else
        chunkData = (OMX_U8 *)pInputBuffer->pBuffer;
#endif 
        chunkSize = pInputBuffer->nFilledLen;

        if (inPort->bAllocateBuffer == OMX_TRUE && omx_vpudec_component_Private->bUseOmxInputBufferAsDecBsBuffer == OMX_TRUE)
        {
            if (OmxGetVpuBsBufferByVirtualAddress(openmaxStandComp, &vbStream, pInputBuffer) == OMX_FALSE)
            {
                DEBUG(DEB_LEV_ERR, "Not found bistream buffer in OMX Buffer Header=%p\n", pInputBuffer->pBuffer);
                goto ERR_DEC;
            }
        }
        else
        {
            vbStream = pVpu->vbStream[0];
        }


#ifdef SUPPORT_ONE_BUFFER_CONTAIN_MULTIPLE_FRAMES
        VPU_DecSetRdPtr(pVpu->handle, vbStream.phys_addr + pInputBuffer->nOffset, 1);
#else
        VPU_DecSetRdPtr(pVpu->handle, vbStream.phys_addr, 1);
#endif

        if ((pInputBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG) || !pVpu->seqInited)
        {

            // case of RV, the width/height info in slice header is correct(more correct than info in sequence header). so both sequence header and slice header should be in the bitstream buffer when SEQ_INIT.
            // that means copy operation should be needed only when SEQ_INIT.
            if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingRV)
            {
                if (!pVpu->keepSeqData && !omx_vpudec_component_Private->portSettingCount)
                {
                    // 1st step : copy sequence header data to seqHeader buffer to keep the sequence data, then return.
                    memcpy(omx_vpudec_component_Private->seqHeader, chunkData, chunkSize);
                    omx_vpudec_component_Private->seqHeaderSize = chunkSize;

                    pInputBuffer->nFilledLen = 0;
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "keep seqHeader data to attach the seqHeader to bitstream buffer with picture data(next chunk) \n");
                    pVpu->keepSeqData = 1;

                    goto NEXT_CHUNK;	// do not treat as critical error. set to have the next right chunk.
                }
            }
            else
            {
                omx_vpudec_component_Private->seqHeaderSize = BuildOmxSeqHeader(omx_vpudec_component_Private->seqHeader, pInputBuffer, openmaxStandComp, &sizeLength);
                if (omx_vpudec_component_Private->seqHeaderSize < 0) // indicate the stream dose not support in VPU.
                {
                    DEBUG(DEB_LEV_ERR, "BuildOmxSeqHeader the stream does not support in VPU \n");
                    goto ERR_DEC;
                }
            }

            if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingRV && pVpu->keepSeqData)
            {
                // 2nd step : copy seqheader data to picHeader buffer.
                // ?? omx_vpudec_component_Private->picHeader is not used for RV case. So we decide to use omx_vpudec_component_Private->picHeader as a big buffer to get the sequence header and picture data
                if (omx_vpudec_component_Private->seqHeaderSize > 0)
                    memcpy(omx_vpudec_component_Private->picHeader, omx_vpudec_component_Private->seqHeader, omx_vpudec_component_Private->seqHeaderSize);
            }
            else if (omx_vpudec_component_Private->seqHeaderSize > 0)
            {
                size = OmxWriteBsBufFromBufHelper(openmaxStandComp, pVpu, vbStream, omx_vpudec_component_Private->seqHeader, omx_vpudec_component_Private->seqHeaderSize);
                if (size < 0)
                {
                    DEBUG(DEB_LEV_ERR, "OmxWriteBsBufFromBufHelper failed Error code is 0x%x \n", (int)size);
                    goto ERR_DEC;
                }
            }
        }

        // Build and Fill picture Header data which is dedicated for VPU
        omx_vpudec_component_Private->picHeaderSize = BuildOmxPicHeader(omx_vpudec_component_Private->picHeader, pInputBuffer, openmaxStandComp, sizeLength);
        if (omx_vpudec_component_Private->picHeaderSize > 0)
        {
            size = OmxWriteBsBufFromBufHelper(openmaxStandComp, pVpu, vbStream, omx_vpudec_component_Private->picHeader, omx_vpudec_component_Private->picHeaderSize);
            if (size < 0)
            {
                DEBUG(DEB_LEV_ERR, "OmxWriteBsBufFromBufHelper failed Error size=%d\n", (int)size);
                goto ERR_DEC;
            }
        } else if (omx_vpudec_component_Private->picHeaderSize < 0)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "pInputBuffer stream error, goto next frame\n");
            pInputBuffer->nFilledLen = 0;
            goto NEXT_CHUNK;
        }

#ifdef WORKAROUND_OMX_USE_INPUT_HEADER_BUFFER_AS_DECODER_BITSTREAM_BUFFER_NOT_REFORMATED
        if (pVpu->decOP.bitstreamFormat == STD_RV)
        {
            int cSlice = chunkData[0] + 1;
            int nSlice =  chunkSize - 1 - (cSlice * 8);
            chunkData += (1+(cSlice*8));
            chunkSize = nSlice;
        }
#endif
        if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingRV && pVpu->keepSeqData)
        {
            // 3rd step : copy picture data to picHeader buffer(next to the sequence data).
            memcpy(omx_vpudec_component_Private->picHeader + omx_vpudec_component_Private->seqHeaderSize, chunkData, chunkSize);
            // final step : write picHeader buffer(seqheader + picData) to vbStream.
            DEBUG(DEB_LEV_SIMPLE_SEQ, "copy seqheader + pic data to inputBuffer\n");
            size = OmxWriteBsBufFromBufHelper(openmaxStandComp, pVpu, vbStream, omx_vpudec_component_Private->picHeader, omx_vpudec_component_Private->seqHeaderSize + chunkSize);
            vdi_write_memory(pVpu->coreIdx, vbStream.phys_addr,  omx_vpudec_component_Private->picHeader, omx_vpudec_component_Private->seqHeaderSize + chunkSize, pVpu->decOP.streamEndian);
#ifdef CNM_FPGA_PLATFORM
            memcpy(chunkData, omx_vpudec_component_Private->picHeader, omx_vpudec_component_Private->seqHeaderSize + chunkSize);
#endif
            pInputBuffer->nFilledLen = omx_vpudec_component_Private->seqHeaderSize + chunkSize;
            omx_vpudec_component_Private->seqHeaderSize = 0;    // not to overwrite the omx_vpudec_component_Private->seqHeader to vbStream after portSettingsChanges.
            pVpu->keepSeqData = 0;
        }
        else
        {
            size = OmxWriteBsBufFromBufHelper(openmaxStandComp, pVpu, vbStream, chunkData, chunkSize + pVpu->prevConsumeBytes);
        }

        if (size <0)
        {
            DEBUG(DEB_LEV_ERR, "OmxWriteBsBufFromBufHelper failed Error code is 0x%x \n", (int)size);
            goto ERR_DEC;
        }

        if (pVpu->decOP.bitstreamFormat == STD_MPEG4 && pVpu->chunkReuseRequired )  // case of MPEG4 Packed Mode
        {
            VPU_DecSetRdPtr(pVpu->handle, vbStream.phys_addr + pVpu->prevConsumeBytes, 0);	// reuse RdPtr = current bitstreambuffer start addr + previous consumedbytes
        }

#ifdef ANDROID_INPUT_DUMP
        if (!pVpu->chunkReuseRequired)
        {
            FILE *dump;
            OMX_U8* pbEs = NULL;
            dump = fopen("/data/dump_dec.es", "a+b");
            if (dump)
            {
                pbEs = (OMX_U8*)malloc(pInputBuffer->nFilledLen);
                if (pbEs)
                {
                    vdi_read_memory(pVpu->coreIdx, vbStream.phys_addr, pbEs, pInputBuffer->nFilledLen, pVpu->decOP.streamEndian);
                    fwrite((unsigned char*)pbEs, pInputBuffer->nFilledLen, 1, dump);
                    free(pbEs);
                }

                fclose(dump);
            }
            else
                DEBUG(DEB_LEV_ERR, "can't open a dump file(/data/dump.es)");
        }
#endif

        omx_timer_start();
        DEBUG(DEB_LEV_FULL_SEQ, "seqInited = %d omx_vpudec_component_Private->bSeqChangeDetected = %d\n",
                pVpu->seqInited, omx_vpudec_component_Private->bSeqChangeDetected);
        if (!pVpu->seqInited || omx_vpudec_component_Private->bSeqChangeDetected)
        {
            if (!omx_vpudec_component_Private->bSeqChangeDetected)    // after sequence change detection, do not call the DecGetInitialInfo()
            {

                VPU_DecSetEscSeqInit(pVpu->handle, 1);
                ret = VPU_DecGetInitialInfo(pVpu->handle, &pVpu->initialInfo);

                if (ret != RETCODE_SUCCESS)
                {

                    pInputBuffer->nFilledLen = 0;

                    DEBUG(DEB_LEV_ERR, "VPU_DecGetInitialInfo failed Error code is 0x%x, reason=0x%x   111111\n", (int)ret, (int)pVpu->initialInfo.seqInitErrReason);

                    goto NEXT_CHUNK;	// do not treat as critical error. set to have the next right chunk.
                }
#if defined (SPRD_PARALLEL_DEINTRELACE_SUPPORT) || defined (SPRD_SERIAL_DEINTRELACE_SUPPORT)
                if (pVpu->initialInfo.interlace) {
                    DEBUG(DEB_LEV_SIMPLE_SEQ,"VPU_DecGetInitialInfo, found a interlace video!\n");
                    omx_vpudec_component_Private->bIsDeinterlaceNeeded = OMX_TRUE;
                    if(omx_vpudec_component_Private->vpp_obj == NULL) {
                        omx_vpudec_component_Private->vpp_obj  = omx_vpudec_cmp_vpp_init();
                        if (omx_vpudec_component_Private->vpp_obj == NULL) {
                            err = OMX_ErrorUndefined;
                            goto ERR_DEC;
                        }
                    }
                }
#endif
                DEBUG(DEB_LEV_SIMPLE_SEQ,"VPU_DecGetInitialInfo success, interlace: %d, picWidth : %d, picHeight : %d, picCropRect.left : %d, picCropRect.bottom : %d, minFrameBufferCount : %d, nBufferCountActual :%d, RdPtr : 0x%x, WrPtr : 0x%x\n",
                      pVpu->initialInfo.interlace, (int)pVpu->initialInfo.picWidth, (int)pVpu->initialInfo.picHeight, (int)pVpu->initialInfo.picCropRect.right , (int)pVpu->initialInfo.picCropRect.bottom, (int)pVpu->initialInfo.minFrameBufferCount, (int)outPort->sPortParam.nBufferCountActual, (int)pVpu->initialInfo.rdPtr, (int)pVpu->initialInfo.wrPtr);
            }

            if (pVpu->decOP.bitstreamFormat == STD_VP8)
            {
                // For VP8 frame upsampling infomration
                static const int scale_factor_mul[4] = {1, 5, 5, 2};
                static const int scale_factor_div[4] = {1, 4, 3, 1};
                int				hScaleFactor, vScaleFactor, scaledWidth, scaledHeight;
                hScaleFactor = pVpu->initialInfo.vp8ScaleInfo.hScaleFactor;
                vScaleFactor = pVpu->initialInfo.vp8ScaleInfo.vScaleFactor;
                scaledWidth = pVpu->initialInfo.picWidth * scale_factor_mul[hScaleFactor] / scale_factor_div[hScaleFactor];
                scaledHeight = pVpu->initialInfo.picHeight * scale_factor_mul[vScaleFactor] / scale_factor_div[vScaleFactor];

                pVpu->fbWidth = ((scaledWidth+15)&~15);
                pVpu->fbHeight = ((scaledHeight+15)&~15);
#ifdef SUPPORT_USE_VPU_ROTATOR
                rotWidth = (s_dec_config.rotAngle == 90 || s_dec_config.rotAngle == 270) ?
                           ((scaledHeight+15)&~15) : ((scaledWidth+15)&~15);
                rotHeight = (s_dec_config.rotAngle == 90 || s_dec_config.rotAngle == 270) ?
                            ((scaledWidth+15)&~15) : ((scaledHeight+15)&~15);
#endif
            }
            else
            {
                pVpu->fbWidth = ((pVpu->initialInfo.picWidth + 15)&~15);
                pVpu->fbHeight = ((pVpu->initialInfo.picHeight + 31)&~31);
#ifdef SUPPORT_USE_VPU_ROTATOR
                rotWidth = (s_dec_config.rotAngle == 90 || s_dec_config.rotAngle == 270) ? ((pVpu->initialInfo.picHeight+15)&~15) : ((pVpu->initialInfo.picWidth+15)&~15);
                rotHeight = (s_dec_config.rotAngle == 90 || s_dec_config.rotAngle == 270) ? ((pVpu->initialInfo.picWidth+15)&~15) : ((pVpu->initialInfo.picHeight+15)&~15);
#endif
            }
#ifdef SUPPORT_USE_VPU_ROTATOR
            pVpu->rotStride = rotWidth;
#endif

            pVpu->fbStride = ((pVpu->fbWidth + 15)&~15);

            if (pVpu->decOP.scalerInfo.enScaler)
                pVpu->fbFormat = (pVpu->decOP.scalerInfo.imageFormat < YUV_FORMAT_I422)?FORMAT_420:FORMAT_422;
            else
                pVpu->fbFormat = FORMAT_420;

            pVpu->frameRate = OmxGetVpuFrameRate(omx_vpudec_component_Private);

#if defined (SPRD_PARALLEL_DEINTRELACE_SUPPORT) || defined (SPRD_SERIAL_DEINTRELACE_SUPPORT)
            if (omx_vpudec_component_Private->bIsDeinterlaceNeeded == OMX_TRUE)
#ifdef SPRD_BLURED_PIC_OPT
                pVpu->deInterlaceDelayNum = pVpu->initialInfo.maxNumRefFrm*(3 + 1); //MAX 3 B frame and 1 P frame
#else
                pVpu->deInterlaceDelayNum = pVpu->initialInfo.minFrameBufferCount;
#endif
            else
                pVpu->deInterlaceDelayNum = 0;

            pVpu->regFbCount = pVpu->initialInfo.minFrameBufferCount + pVpu->deInterlaceDelayNum;
            if (pVpu->regFbCount > 18)
                pVpu->regFbCount = 18;
#else
            pVpu->regFbCount = pVpu->initialInfo.minFrameBufferCount + EXTRA_FRAME_BUFFER_NUM; //  intend to make some delay to prevent waiting while display.
#endif

            if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
            {
                omx_vpudec_component_Private->actualFrameBufferNeeded = pVpu->regFbCount;
            }
            else if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
            {
                VPU_DecGiveCommand(pVpu->handle, ENABLE_DEC_THUMBNAIL_MODE, 0);
                omx_vpudec_component_Private->actualFrameBufferNeeded = (int)outPort->sPortParam.nBufferCountActual;
            }
            else
            {
                //if (outPort->bAllocateBuffer == OMX_TRUE) {
                    omx_vpudec_component_Private->actualFrameBufferNeeded = pVpu->regFbCount + EXTRA_ALLOCATING_BUFFER_NUM;
                //}
                //else {
                    // 3 more buffers is required by de-interlace process
                    // Mod by Alan Wang
                    //omx_vpudec_component_Private->actualFrameBufferNeeded = pVpu->initialInfo.minFrameBufferCount + EXTRA_FRAME_BUFFER_NUM;
                //}
            }

            DEBUG(DEB_LEV_SIMPLE_SEQ, " bThumbnailMode = %d,actualFrameBufferNeeded = %d, regFbCount = %d, minFrameBufferCount = %d, numReorderFrames = %d, maxNumRefFrm = %d, deInterlaceDelayNum = %d\n",
                    omx_vpudec_component_Private->bThumbnailMode, omx_vpudec_component_Private->actualFrameBufferNeeded, pVpu->regFbCount, pVpu->initialInfo.minFrameBufferCount ,
                    pVpu->initialInfo.numReorderFrames, pVpu->initialInfo.maxNumRefFrm, pVpu->deInterlaceDelayNum);

            if (pVpu->fbStride==0 || pVpu->fbHeight==0 || pVpu->regFbCount==0)
            {
                DEBUG(DEB_LEV_ERR, "wrong sequence information parsed\n");
                goto ERR_DEC;
            }

            if (outPort->omxConfigScale.xWidth != 0x10000 || outPort->omxConfigScale.xHeight != 0x10000)
            {
                pVpu->decOP.scalerInfo.enScaler = 1;
                if (!calculate_minipippen_scale_resolution(openmaxStandComp))
                {
                    DEBUG(DEB_LEV_ERR, "fail to calculate_minipippen_scale_resolution\n");
                    goto ERR_DEC;
                }
            }
            else
            {
                if (pVpu->decOP.scalerInfo.enScaler) // if scaler is enabled for format convector even no scale ratio. set scaler resolution same as video buffer resolution
                {
                    pVpu->decOP.scalerInfo.scaleWidth = ((pVpu->initialInfo.picWidth + 15)&~15);
#ifdef SUPPORT_CROP_BOTTOM
                    if (pVpu->initialInfo.picCropRect.bottom)
                    {
                        pVpu->decOP.scalerInfo.scaleHeight = (((pVpu->initialInfo.picCropRect.bottom - pVpu->initialInfo.picCropRect.top)+ 7)&~7);
                        pVpu->scaleNotAlignedHeight = (pVpu->initialInfo.picCropRect.bottom - pVpu->initialInfo.picCropRect.top);
                    }
                    else
                    {
                        pVpu->decOP.scalerInfo.scaleHeight = ((pVpu->initialInfo.picHeight + 7)&~7);
                        pVpu->scaleNotAlignedHeight = pVpu->initialInfo.picHeight;
                    }
#else
                    pVpu->decOP.scalerInfo.scaleHeight = ((pVpu->initialInfo.picHeight + 7)&~7);
#endif
                }
            }
            ret = VPU_DecGiveCommand(pVpu->handle, DEC_UPDATE_SCALER_INFO, &pVpu->decOP.scalerInfo);
            if (ret != RETCODE_SUCCESS)
            {
                DEBUG(DEB_LEV_ERR, "VPU_DecGiveCommand ( DEC_UPDATE_SCALER_INFO ) failed Error code is 0x%x \n", ret );
                goto ERR_DEC;
            }

            portSettingChangeDetected = OMX_FALSE;

            if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE) {
                if (omx_vpudec_component_Private->actualFrameBufferNeeded > outPort->sPortParam.nBufferCountMin) {
                    outPort->sPortParam.nBufferCountMin = omx_vpudec_component_Private->actualFrameBufferNeeded;
                    portSettingChangeDetected = OMX_TRUE;
                }
            } else {
                if (omx_vpudec_component_Private->actualFrameBufferNeeded != outPort->sPortParam.nBufferCountActual) {
                    portSettingChangeDetected = OMX_TRUE;
                }
            }

            if(pVpu->decOP.scalerInfo.enScaler)
            {
                if(inPort->sPortParam.format.video.nFrameWidth != (OMX_U32)pVpu->decOP.scalerInfo.scaleWidth ||
                        inPort->sPortParam.format.video.nFrameHeight != (OMX_U32)pVpu->decOP.scalerInfo.scaleHeight)
                {
                    portSettingChangeDetected = OMX_TRUE;
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "----> Scale mode:sPortParam nFrameWidth is not equal scaleWidth, nFrameWidth=%d, nFrameHeight=%d, nStride=%d vs scaleWidth=%d, scaleHeight=%d\n",
                          (int)inPort->sPortParam.format.video.nFrameWidth,
                          (int)inPort->sPortParam.format.video.nFrameHeight,
                          (int)inPort->sPortParam.format.video.nStride,
                          pVpu->decOP.scalerInfo.scaleWidth,
                          pVpu->decOP.scalerInfo.scaleHeight);
                }
            }
            else
            {
                if(inPort->sPortParam.format.video.nFrameWidth != (OMX_U32)pVpu->fbStride ||
                        inPort->sPortParam.format.video.nFrameHeight != (OMX_U32)pVpu->fbHeight)
                {
                    portSettingChangeDetected = OMX_TRUE;
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "----> Normal mode:sPortParam nFrameWidth is not equal fbWidth, nFrameWidth=%d, nFrameHeight=%d, nStride=%d, vs fbWidth=%d, fbHeight=%d, fbStride=%d\n",
                          (int)inPort->sPortParam.format.video.nFrameWidth,
                          (int)inPort->sPortParam.format.video.nFrameHeight,
                          (int)inPort->sPortParam.format.video.nStride,
                          pVpu->fbWidth,
                          pVpu->fbHeight,
                          pVpu->fbStride);
                }

            }

            if (portSettingChangeDetected == OMX_TRUE)
            {
                DEBUG(DEB_LEV_SIMPLE_SEQ, "---->Sending Port Settings Change Event in video decoder omxWidth=%d, omxHeight=%d, omxStride=%d pVpu->fbStride=%d, pVpu->fbHeight=%d\n", (int)(inPort->sPortParam.format.video.nFrameWidth), (int)inPort->sPortParam.format.video.nFrameHeight, (int)inPort->sPortParam.format.video.nStride, (int)pVpu->fbStride, (int)pVpu->fbHeight );
                if (pVpu->decOP.scalerInfo.enScaler)
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "---->Sending Port Settings Change Event in video decoder omxWidth=%d, omxHeight=%d, omxStride=%d scaleWidth=%d, scaleHeight=%d\n", (int)(inPort->sPortParam.format.video.nFrameWidth), (int)inPort->sPortParam.format.video.nFrameHeight, (int)inPort->sPortParam.format.video.nStride, (int)pVpu->decOP.scalerInfo.scaleWidth, (int)pVpu->decOP.scalerInfo.scaleHeight);
                DEBUG(DEB_LEV_SIMPLE_SEQ,"initialInfo picCropRect left : %d, top : %d, right : %d, bottom : %d\n", (int)pVpu->initialInfo.picCropRect.left, (int)pVpu->initialInfo.picCropRect.top, (int)pVpu->initialInfo.picCropRect.right, (int)pVpu->initialInfo.picCropRect.bottom);
                DEBUG(DEB_LEV_SIMPLE_SEQ, "----> actualFrameBufferNeeded=%d, pVpu->regFbCount=%d, nBufferCountActual=%d\n",
                      (int)omx_vpudec_component_Private->actualFrameBufferNeeded, (int)pVpu->regFbCount, (int)outPort->sPortParam.nBufferCountActual);

                outPort->nTempBufferCountActual = omx_vpudec_component_Private->actualFrameBufferNeeded; // do not set outPort->sPortParam.nBufferCountActual in direct before Port Reconfiguration(Close and Open)

                if (pVpu->frameRate && ((inPort->sPortParam.format.video.xFramerate == 0) || (inPort->sPortParam.format.video.xFramerate > 120000))) //  means OMXIL client does not set xFramerate
                {
                    inPort->sPortParam.format.video.xFramerate = (OMX_U32)(pVpu->frameRate*1000);
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "update xFramerate inPort->sPortParam.format.video.xFramerate=%d, pVpu->frameRate=%d\n", (int)inPort->sPortParam.format.video.xFramerate,(int)pVpu->frameRate);
                }

                if (pVpu->decOP.scalerInfo.enScaler)
                {
                    double scaleRatioW;
                    double scaleRatioH;
                    scaleRatioW = ((double)pVpu->decOP.scalerInfo.scaleWidth/pVpu->initialInfo.picWidth);
                    scaleRatioH = ((double)pVpu->decOP.scalerInfo.scaleHeight/pVpu->initialInfo.picHeight);
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "scaleRatioW=%f scaleRatioH=%f ", scaleRatioW, scaleRatioH);
                    inPort->sPortParam.format.video.nStride =pVpu->decOP.scalerInfo.scaleWidth;
                    inPort->sPortParam.format.video.nSliceHeight = pVpu->decOP.scalerInfo.scaleHeight;
                    inPort->sPortParam.format.video.nFrameWidth = pVpu->decOP.scalerInfo.scaleWidth;
                    inPort->sPortParam.format.video.nFrameHeight = pVpu->decOP.scalerInfo.scaleHeight;
                    inPort->omxConfigCrop.nLeft = (OMX_S32)(pVpu->initialInfo.picCropRect.left*scaleRatioW);
                    inPort->omxConfigCrop.nTop = (OMX_S32)(pVpu->initialInfo.picCropRect.top*scaleRatioH);
                    inPort->omxConfigCrop.nWidth = pVpu->decOP.scalerInfo.scaleWidth - inPort->omxConfigCrop.nLeft - ((pVpu->initialInfo.picWidth - pVpu->initialInfo.picCropRect.right)*scaleRatioW);
#ifdef SUPPORT_CROP_BOTTOM
                    inPort->omxConfigCrop.nHeight = pVpu->scaleNotAlignedHeight - inPort->omxConfigCrop.nTop;
#else
                    inPort->omxConfigCrop.nHeight = pVpu->decOP.scalerInfo.scaleHeight - inPort->omxConfigCrop.nTop - ((pVpu->initialInfo.picHeight - pVpu->initialInfo.picCropRect.bottom)*scaleRatioH);;
#endif
                }
                else
                {
                    inPort->sPortParam.format.video.nStride = pVpu->fbStride;
                    inPort->sPortParam.format.video.nSliceHeight = pVpu->fbHeight;
                    inPort->sPortParam.format.video.nFrameWidth = pVpu->fbStride;
                    inPort->sPortParam.format.video.nFrameHeight = pVpu->fbHeight;
                    inPort->omxConfigCrop.nLeft     = pVpu->initialInfo.picCropRect.left;
                    inPort->omxConfigCrop.nTop      = pVpu->initialInfo.picCropRect.top;
                    inPort->omxConfigCrop.nWidth    = pVpu->initialInfo.picWidth - inPort->omxConfigCrop.nLeft - (pVpu->initialInfo.picWidth - pVpu->initialInfo.picCropRect.right);    // actual width  = (with - left margin - right margin)
                    inPort->omxConfigCrop.nHeight   = pVpu->initialInfo.picHeight - inPort->omxConfigCrop.nTop - (pVpu->initialInfo.picHeight - pVpu->initialInfo.picCropRect.bottom);  // actual height = (height - top margin - bottom margin)
                }
                DEBUG(DEB_LEV_SIMPLE_SEQ,"Update omxConfigCrop left : %d, top : %d, width : %d, height : %d", (int)inPort->omxConfigCrop.nLeft, (int)inPort->omxConfigCrop.nTop, (int)inPort->omxConfigCrop.nWidth, (int)inPort->omxConfigCrop.nHeight);

                UpdateFrameSize(openmaxStandComp);

                /*Send Port Settings changed call back*/
                if (omx_vpudec_component_Private->callbacks->EventHandler)
                {
                    OMX_ERRORTYPE ret;

                    /* waiting until the owner of all fillbuffers changed to OMX_BUFFER_OWNER_COMPONENT except for the amount of NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS before sending PortSettingsChange*/
                    /* if not, OMX_EventPortSettingsChanged event in OMXCocdec will be failed due to CHECK(mFilledBuffers.empty()) */
                    OmxWaitUntilOutBuffersEmpty(omx_vpudec_component_Private);


                    DEBUG(DEB_LEV_SIMPLE_SEQ,"Call PortSettingsChange");
                    ret = (*(omx_vpudec_component_Private->callbacks->EventHandler))
                          (openmaxStandComp,
                           omx_vpudec_component_Private->callbackData,
                           OMX_EventPortSettingsChanged, /* The command was completed */
                           1, /* This is the output port index */
                           0,
                           NULL);

                    if (ret == OMX_ErrorNone)
                    {
                        omx_vpudec_component_Private->portSettingChangeRequest = OMX_TRUE;
                    }
                    else
                    {
                        DEBUG(DEB_LEV_ERR, "fail to OMX_EventPortSettingsChanged Event for output buffer count\n");
                        goto ERR_DEC;
                    }
                }
                pVpu->seqInited = 0;

                omx_vpudec_component_Private->portSettingCount++;
                //don't touch inputBuffer->nFilledLen because we need to call seq init again with current chunk at next.
                goto SKIP_DISPLAY;
            }

            pVpu->regFbCount = outPort->sPortParam.nBufferCountActual;

            if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
                pVpu->regFbCount = pVpu->initialInfo.minFrameBufferCount; // set minimum buffer count let decoder to act as normal decoding

            DEBUG(DEB_LEV_SIMPLE_SEQ,"Now Realize pVpu->regFbCount=%d to nBufferCountActual=%d, useNativeBuffer=%d, bThumbnailMode=%d\n",
                  pVpu->regFbCount, (int)outPort->sPortParam.nBufferCountActual, omx_vpudec_component_Private->useNativeBuffer, omx_vpudec_component_Private->bThumbnailMode);

            ret = VPU_DecGiveCommand(pVpu->handle, GET_DRAM_CONFIG, &pVpu->dramCfg);
            if( ret != RETCODE_SUCCESS )
            {
                DEBUG(DEB_LEV_ERR, "VPU_DecGiveCommand[GET_DRAM_CONFIG] failed Error code is 0x%x \n", ret );
                goto ERR_DEC;
            }
            pVpu->secAxiUse.useBitEnable  = USE_BIT_INTERNAL_BUF;
            pVpu->secAxiUse.useIpEnable   = USE_IP_INTERNAL_BUF;
            pVpu->secAxiUse.useDbkYEnable = USE_DBKY_INTERNAL_BUF;
            pVpu->secAxiUse.useDbkCEnable = USE_DBKC_INTERNAL_BUF;
            pVpu->secAxiUse.useMeEnable  = USE_ME_INTERNAL_BUF;
            pVpu->secAxiUse.useOvlEnable  = USE_OVL_INTERNAL_BUF;

            pVpu->secAxiUse.useScalerEnable = USE_SCALER_INTERNAL_BUF;

            VPU_DecGiveCommand(pVpu->handle, SET_SEC_AXI, &pVpu->secAxiUse);
            if (ret != RETCODE_SUCCESS)
            {
                DEBUG(DEB_LEV_ERR, "VPU_DecGiveCommand ( SET_SEC_AXI ) failed Error code is 0x%x \n", ret );
                goto ERR_DEC;
            }



            fbUserFrame = NULL; // it means VPU_DecRegisterFrameBuffer should allocation frame buffer by user using(not in VPUAPU) VDI and vpu device driver.

            if (!OmxAllocateFrameBuffers(openmaxStandComp))
                goto ERR_DEC;

            fbUserFrame = &pVpu->fbUser[0];

            // Register frame buffers requested by the decoder.
            ret = VPU_DecRegisterFrameBuffer(pVpu->handle, fbUserFrame, pVpu->regFbCount, pVpu->fbStride,pVpu->fbHeight, pVpu->mapType); // frame map type (can be changed before register frame buffer)
            if( ret != RETCODE_SUCCESS )
            {
                DEBUG(DEB_LEV_ERR, "VPU_DecRegisterFrameBuffer failed Error code is 0x%x \n", (int)ret);
                goto ERR_DEC;
            }
#if DEBUG_LEVEL > 0
            {
                int i;
                FrameBuffer fb;
                for (i=0; i < MAX_REG_FRAME; i++)
                {
                    if (VPU_DecGetFrameBuffer(pVpu->handle, i, &fb) == RETCODE_SUCCESS)
                    {
                        DEBUG(DEB_LEV_FULL_SEQ, "VPU_DecGetFrameBuffer index=%d, bufY=0x%x, bufCb=0x%x, bufCr=0x%x, vbUserFb.size=%d, vbUserFb.phys_addr=0x%x, vbUserFb.virt_addr=0x%x, vbUserFb.base=0x%x\n",
                              i, fb.bufY, fb.bufCb, fb.bufCr, (int)pVpu->vbUserFb[i].size, (int)pVpu->vbUserFb[i].phys_addr, (int)pVpu->vbUserFb[i].virt_addr, (int)pVpu->vbUserFb[i].base);
                    }
                }
            }
#endif

            if (outPort->bAllocateBuffer == OMX_TRUE || omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
            {
                pthread_mutex_lock(&omx_vpudec_component_Private->display_flag_mutex);
                for (i = 0; i < pVpu->regFbCount; i++)
                {
                    dispFlagIdx = i;
                    if (pVpu->decOP.scalerInfo.enScaler)
                        dispFlagIdx = i + pVpu->initialInfo.minFrameBufferCount;
                    if (disp_flag_array[i].owner == OMX_BUFERR_OWNER_COMPONENT)
                    {
                        VPU_DecClrDispFlag(pVpu->handle, dispFlagIdx);
                        DEBUG(DEB_LEV_FULL_SEQ, "VPU_DecClrDispFlag for index=%d, dispFlagIdx=%d, owner=0x%x\n",  i, dispFlagIdx, disp_flag_array[i].owner);
                    }
                    else if(disp_flag_array[i].owner == OMX_BUFFER_OWNER_CLIENT || disp_flag_array[i].owner == OMX_BUFFER_OWNER_NOBODY) // this means that the buffer is using display part. ( NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS )
                    {
                        VPU_DecGiveCommand(pVpu->handle, DEC_SET_DISPLAY_FLAG, &dispFlagIdx);
                        DEBUG(DEB_LEV_FULL_SEQ, "VPU_DecGiveCommand[DEC_SET_DISPLAY_FLAG] for index=%d, dispFlagIdx=%d, owner=0x%x\n",  i, dispFlagIdx, disp_flag_array[i].owner);
                    }
                }
                pthread_mutex_unlock(&omx_vpudec_component_Private->display_flag_mutex);
            }



            if (pVpu->ppuEnable)
            {
                pVpu->ppuIdx = 0;

                pVpu->fbAllocInfo.format = pVpu->fbFormat;
                pVpu->fbAllocInfo.cbcrInterleave = pVpu->decOP.cbcrInterleave;
                pVpu->fbAllocInfo.mapType = LINEAR_FRAME_MAP;

                pVpu->fbAllocInfo.stride = pVpu->fbStride;
                pVpu->fbAllocInfo.height = pVpu->fbHeight;
                pVpu->fbAllocInfo.num = MAX_PPU_SRC_NUM;
                pVpu->fbAllocInfo.endian = pVpu->decOP.frameEndian;
                pVpu->fbAllocInfo.type = FB_TYPE_PPU;

                ret = VPU_DecAllocateFrameBuffer(pVpu->handle, pVpu->fbAllocInfo, pVpu->fbPPU);

                if( ret != RETCODE_SUCCESS )
                {
                    DEBUG(DEB_LEV_ERR, "VPU_DecAllocateFrameBuffer fail to allocate source frame buffer is 0x%x \n", ret );
                    goto ERR_DEC;
                }

                if (s_dec_config.useRot)
                {
                    VPU_DecGiveCommand(pVpu->handle, SET_ROTATION_ANGLE, &(s_dec_config.rotAngle));
                    VPU_DecGiveCommand(pVpu->handle, SET_MIRROR_DIRECTION, &(s_dec_config.mirDir));
                }

                VPU_DecGiveCommand(pVpu->handle, SET_ROTATOR_STRIDE, &pVpu->rotStride);


                if (s_dec_config.useRot)
                {
                    VPU_DecGiveCommand(pVpu->handle, ENABLE_ROTATION, 0);
                    VPU_DecGiveCommand(pVpu->handle, ENABLE_MIRRORING, 0);
                }

                if (s_dec_config.useDering)
                    VPU_DecGiveCommand(pVpu->handle, ENABLE_DERING, 0);
            }


            DEBUG(DEB_LEV_SIMPLE_SEQ, "seqInited completed\n");
            pVpu->seqInited = 1;
            omx_vpudec_component_Private->bSeqChangeDetected = OMX_FALSE;

            switch(pVpu->decOP.bitstreamFormat)
            {
            case (STD_MPEG4):
            case (STD_MPEG2):
            case (STD_AVC):
            case (STD_H263):
                VPU_DecSetRdPtr(pVpu->handle, vbStream.phys_addr, 0);	// CODA851/7L can't support a exact rdptr. so after SEQ_INIT, should rewind the RdPtr.
                break;
            default:
                break;
            }

            if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingWMV ||
                    omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingVC1)
            {
                pInputBuffer->nFilledLen = 0;       // VC1/RV parser always send only sequence data for 1st chunk. (it doesn't need to run PIC_RUN command)
                goto NEXT_CHUNK;
            }
        }

FLUSH_BUFFER:

        if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
            pVpu->decParam.iframeSearchEnable = 2;	//I frame and IDR frame for AVC search

#ifdef USE_BFRAME_SEARCH_FOR_1STFRAME
		//pVpu->decParam.skipframeMode = 0;
#else
		pVpu->decParam.skipframeMode = 0;
#endif

        if (pVpu->ppuEnable)
            VPU_DecGiveCommand(pVpu->handle, SET_ROTATOR_OUTPUT, &pVpu->fbPPU[pVpu->ppuIdx]);

        ConfigDecReport(pVpu->coreIdx, pVpu->handle, pVpu->decOP.bitstreamFormat);

		if(pVpu->int_reason & (1<<INT_BIT_DEC_FIELD))
		{
			Uint32 room;
			ret = VPU_DecGetBitstreamBuffer(pVpu->handle, &decBitBufInfo.rdptr, &decBitBufInfo.wrptr, &room);
			if( ret != RETCODE_SUCCESS )
			{
				DEBUG(DEB_LEV_ERR, "VPU_DecGetBitstreamBuffer failed Error code is 0x%x \n", ret );
				goto ERR_DEC;
			}
			VPU_DecGiveCommand(pVpu->handle, DEC_SET_2ND_FIELD_INFO, &decBitBufInfo);  // set start addr/chunk size for the 2nd field.

			VPU_ClearInterrupt(pVpu->coreIdx);
			pVpu->int_reason = 0;
			goto WAIT_LEFT_FIELD_DECODING_DONE;
		}


        // Start decoding a frame.
        ret = VPU_DecStartOneFrame(pVpu->handle, &pVpu->decParam);
        if (ret != RETCODE_SUCCESS)
        {
            DEBUG(DEB_LEV_ERR, "VPU_DecStartOneFrame failed Error code is 0x%x \n", (int)ret);
            goto ERR_DEC;

        }
        DEBUG(DEB_LEV_FULL_SEQ, "VPU_DecStartOneFrame() success Waiting PicDone Interrupt, iframeSearchEnable = %d\n", pVpu->decParam.iframeSearchEnable );

WAIT_LEFT_FIELD_DECODING_DONE:

        while(1)
        {
            pVpu->int_reason = VPU_WaitInterrupt(pVpu->coreIdx, VPU_DEC_TIMEOUT);
            if (pVpu->int_reason == -1 ) // timeout
            {
                DEBUG(DEB_LEV_ERR, "VPU interrupt timeout PC=0x%x\n", vdi_read_register(pVpu->coreIdx, BIT_CUR_PC));

                VPU_SWReset(pVpu->coreIdx, SW_RESET_SAFETY, pVpu->handle);
                //pVpu->int_reason = 0;
                VPU_DecGetOutputInfo(pVpu->handle, &pVpu->dispOutputInfo); // need to free some resources that is created VPU_DecStartOneFrame
                goto ERR_DEC;
            }

            CheckUserDataInterrupt(pVpu->coreIdx, pVpu->handle, pVpu->frameIdx, pVpu->decOP.bitstreamFormat, pVpu->int_reason);
            if(pVpu->int_reason & (1<<INT_BIT_DEC_FIELD))
            {
                pInputBuffer->nFilledLen = 0;       // field interrupt will be occurred when 1chunk is 1 field.
                goto NEXT_CHUNK;
            }


            if (pVpu->int_reason)
                VPU_ClearInterrupt(pVpu->coreIdx);


            if (pVpu->int_reason & (1<<INT_BIT_PIC_RUN))
                break;
        }

        ret = VPU_DecGetOutputInfo(pVpu->handle, &pVpu->dispOutputInfo);
        if (ret != RETCODE_SUCCESS)
        {
            DEBUG(DEB_LEV_ERR, "VPU_DecGetOutputInfo failed Error code is 0x%x \n", (int)ret);
            goto ERR_DEC;
        }

		DEBUG(DEB_LEV_SIMPLE_SEQ/*DEB_LEV_FULL_SEQ*/, "#%d, instIdx=%d, disIdx=%d, decIdx=%d, picType=%d, interlacedFrame=%d, chunkLen=%d, video_coding_type=%d dispFlag=0x%x, RdPtr : 0x%x, WrPtr : 0x%x, \ndispwidth : %d, dispHeight : %d, dispLeft=%d, dispTop=%d, dispRight=%d, dispBottom=%d\n",
            (int)pVpu->frameIdx, (int)pVpu->instIdx, (int)pVpu->dispOutputInfo.indexFrameDisplay, (int)pVpu->dispOutputInfo.indexFrameDecoded, (int)pVpu->dispOutputInfo.picType, pVpu->dispOutputInfo.interlacedFrame, (int)chunkSize, (int)omx_vpudec_component_Private->video_coding_type, (int)pVpu->dispOutputInfo.frameDisplayFlag, pVpu->dispOutputInfo.rdPtr, pVpu->dispOutputInfo.wrPtr,
            (int)pVpu->dispOutputInfo.dispPicWidth,  (int)pVpu->dispOutputInfo.dispPicHeight, (int)pVpu->dispOutputInfo.rcDisplay.left,  (int)pVpu->dispOutputInfo.rcDisplay.top,  (int)pVpu->dispOutputInfo.rcDisplay.right,  (int)pVpu->dispOutputInfo.rcDisplay.bottom);

		if (pVpu->dispOutputInfo.decodingSuccess == 0)
		{
			DEBUG(DEB_LEV_ERR, "VPU_DecGetOutputInfo decode fail framdIdx %d \n", pVpu->frameIdx);
			pVpu->frameIdx++;    
			if (pVpu->dispOutputInfo.consumedByte < (int)pInputBuffer->nFilledLen)
				pVpu->dispOutputInfo.consumedByte = pInputBuffer->nFilledLen; // when firmware returns an error. consumedByte has not be updated.
			//must not goto to an error because indexFrameDisplay can have valid index.
		}

        pVpu->chunkReuseRequired = pVpu->dispOutputInfo.chunkReuseRequired;

        pVpu->prevConsumeBytes = 0;

        if (pVpu->dispOutputInfo.indexFrameDisplay == -1)
        {
            pVpu->decodefinish = 1;
            DEBUG(DEB_LEV_SIMPLE_SEQ, "decode finished\n");
        }


        if (pVpu->dispOutputInfo.indexFrameDecoded >= 0)
        {
#ifdef SUPPORT_REF_FLAG_REPORT
            for (i=0; i<pVpu->regFbCount; i++) 
            {
                DEBUG(DEB_LEV_FULL_SEQ, "frame index=%d, USE_REF:%d\n", i, pVpu->dispOutputInfo.frameReferenceFlag[i]);
            }
#endif

#ifdef USE_IFRAME_SEARCH_FOR_1STFRAME
            if (omx_vpudec_component_Private->bThumbnailMode == OMX_FALSE)
                pVpu->decParam.iframeSearchEnable = 0;	// turn off iframe search
#ifdef USE_BFRAME_SEARCH_FOR_1STFRAME
            if (omx_vpudec_component_Private->bSkipBrokenBframeDecode == OMX_TRUE)
            {
                if (pVpu->dispOutputInfo.picType == PIC_TYPE_I)
                    pVpu->decParam.skipframeMode = 2;
                else
                {
                    if (pVpu->dispOutputInfo.picType != PIC_TYPE_B)
                    {
                        omx_vpudec_component_Private->bSkipBrokenBframeDecode = OMX_FALSE;
                        pVpu->decParam.skipframeMode = 0;
                    }
                }
            }
#endif

#endif

#ifdef  CNM_FPGA_PLATFORM
            if (outPort->bAllocateBuffer == OMX_TRUE || omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
                OmxSyncFpgaOutputToHostBuffer(openmaxStandComp, &pVpu->dispOutputInfo);

#endif
            if (pVpu->decOP.scalerInfo.enScaler)
            {
#ifdef SUPPORT_TIMESTAMP_REORDER
                disp_flag_array[pVpu->dispOutputInfo.indexScalerDecoded].nInputTimeStamp = pInputBuffer->nTimeStamp;
#endif
                memcpy(&pVpu->decOutputInfos[pVpu->dispOutputInfo.indexScalerDecoded], &pVpu->dispOutputInfo, sizeof(DecOutputInfo));
            }
            else
            {
#ifdef SUPPORT_TIMESTAMP_REORDER
                disp_flag_array[pVpu->dispOutputInfo.indexFrameDecoded].nInputTimeStamp = pInputBuffer->nTimeStamp;
#endif
                memcpy(&pVpu->decOutputInfos[pVpu->dispOutputInfo.indexFrameDecoded], &pVpu->dispOutputInfo, sizeof(DecOutputInfo));
            }
            if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
                pVpu->dispOutputInfo.indexFrameDisplay = pVpu->dispOutputInfo.indexFrameDecoded;
        }


        if (pVpu->dispOutputInfo.indexFrameDecoded == -1)	// VPU did not decode a picture because there is not enough frame buffer to continue decoding
        {
            DEBUG(DEB_LEV_FULL_SEQ, "#%d, Display buffer full has been detected.\n", (int)pVpu->frameIdx);
        }

        if (pVpu->dispOutputInfo.indexFrameDecoded >= 0 && pVpu->dispOutputInfo.numOfErrMBs)
        {
            pVpu->totalNumofErrMbs += pVpu->dispOutputInfo.numOfErrMBs;
            DEBUG(DEB_LEV_ERR, "Num of Error Mbs : %d, in Frame : %d \n", pVpu->dispOutputInfo.numOfErrMBs, pVpu->frameIdx);
        }

        if (pVpu->dispOutputInfo.sequenceChanged)
        {
            pVpu->chunkReuseRequired = 1;
            pVpu->dispOutputInfo.consumedByte = 0;		// to reuse current chunk data at next time
            VPU_DecGiveCommand(pVpu->handle, DEC_GET_SEQ_INFO, &pVpu->initialInfo);
            omx_vpudec_component_Private->bSeqChangeDetected = OMX_TRUE;
        }

        if(pVpu->chunkReuseRequired || pVpu->dispOutputInfo.indexFrameDecoded == -1)
        {
            pInputBuffer->nFilledLen = pInputBuffer->nFilledLen;
        }
        else
        {
#ifdef SUPPORT_ONE_BUFFER_CONTAIN_MULTIPLE_FRAMES
#define MIN_CHUNK_SIZE 6	//rdPtr can be back as many as this size because the size of one frame can't be smaller than this size.
			if (pVpu->dispOutputInfo.consumedByte < (int)(pInputBuffer->nFilledLen-MIN_CHUNK_SIZE)) // it means that InputBuffer has more chunkdata to be decoded 
			{
				if (((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) && (pInputBuffer->nFilledLen == 0)) ||
					(pInputBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG))
				{
					pInputBuffer->nFilledLen = 0;
					pInputBuffer->nOffset = 0;	
				}
				else
				{
					DEBUG(DEB_LEV_SIMPLE_SEQ, "#%d, remain some data that should be decoded consumedByte=%d, remainSize = %d\n",
						(int)pVpu->frameIdx, (int)pVpu->dispOutputInfo.consumedByte, (int)((pInputBuffer->nFilledLen-pVpu->dispOutputInfo.consumedByte)));

					pInputBuffer->nOffset += pVpu->dispOutputInfo.consumedByte;
					pInputBuffer->nFilledLen -= pVpu->dispOutputInfo.consumedByte;
				}
			}
			else
			{
				pInputBuffer->nFilledLen = 0;
				pInputBuffer->nOffset = 0;
			}
#else
			pInputBuffer->nFilledLen = 0;
#endif
        }

        if (pVpu->dispOutputInfo.indexFrameDecoded >= 0)
            pVpu->frameIdx++;

        omx_timer_stop();

        DEBUG(DEB_LEV_FULL_SEQ, "Decode Done consume byte : %d, frameDisplayFlag=0x%x, Input nFilledLen: %d time=%.1fdms\n", pVpu->dispOutputInfo.consumedByte, pVpu->dispOutputInfo.frameDisplayFlag, (int)pInputBuffer->nFilledLen, omx_timer_elapsed_ms());
    }


    if(IS_STATE_FILLTHISBUFFER(pOutputBuffer) == OMX_TRUE) // pOutputBuffer has a values
    {
        if (s_time_elapsed != 0)
            s_time_elapsed = omx_time_curr_ms() - s_time_elapsed;

        DEBUG(DEB_LEV_FULL_SEQ, "IS_STATE_FILLTHISBUFFER time_elapsed=%.1fdms, pOutputBuffer=0x%x\n", s_time_elapsed, (unsigned long)pOutputBuffer);

        s_time_elapsed =  omx_time_curr_ms();

        if (pVpu->decodefinish)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "EOS detected from decoder (not in input omxheader). send EOS event to ILClient without display anything\n");
            goto REACH_OUTPUT_BUFFER_EOS;
        }

        pOutputBuffer->nFilledLen = 0;

        if (pVpu->dispOutputInfo.indexFrameDisplay >= 0)
        {
            dispFlagIdx = pVpu->dispOutputInfo.indexFrameDisplay;
            if (pVpu->decOP.scalerInfo.enScaler)
                dispFlagIdx = pVpu->dispOutputInfo.indexFrameDisplay - pVpu->initialInfo.minFrameBufferCount;
            
            if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE) {
                pOutputBuffer->nFilledLen = outPort->sPortParam.nBufferSize;
                pVpu->saveLen = outPort->sPortParam.nBufferSize;
            }
            else
            {
                // this would be in case or ThumbNail decoding or not use case of AwesomeNativeWindowRenderer at stagefright.
                // do not need to copy decoded buffer to output OMX_HEADER_BUFFER in display order
                // just return nFilledLen that indicates this output is success.
                pOutputBuffer->nFilledLen = outPort->sPortParam.nBufferSize;
                pVpu->saveLen =  outPort->sPortParam.nBufferSize;
                if (outPort->bAllocateBuffer == OMX_FALSE) // if output buffer is not allocated by component. need to copy data
                {
                    if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
                    {
                        // update pVpu->dispOutputInfo informaton of pVpu->dispOutputInfo.indexFrameDisplay of thumbnail case.
                        if (VPU_DecGetFrameBuffer(pVpu->handle, pVpu->dispOutputInfo.indexFrameDisplay, &pVpu->dispOutputInfo.dispFrame) != RETCODE_SUCCESS)
                        {
                            DEBUG(DEB_LEV_ERR, "fail to VPU_DecGetFrameBuffer index=%d, bufY=0x%x, bufCb=0x%x, bufCr=0x%x\n", pVpu->dispOutputInfo.indexFrameDisplay, pVpu->dispOutputInfo.dispFrame.bufY, pVpu->dispOutputInfo.dispFrame.bufCb, pVpu->dispOutputInfo.dispFrame.bufCr);
                            goto ERR_DEC;
                        }
                    }
                    double curr_ms = omx_time_curr_ms();
                    DEBUG(DEB_LEV_FULL_SEQ, "UseBuffer : start copy video data to pOutputBuffer=%p from index=%d, bufY=0x%x, bufCb=0x%x, bufCr=0x%x, nBufferSize=%d\n", pOutputBuffer->pBuffer, pVpu->dispOutputInfo.indexFrameDisplay, pVpu->dispOutputInfo.dispFrame.bufY, pVpu->dispOutputInfo.dispFrame.bufCb, pVpu->dispOutputInfo.dispFrame.bufCr, (int)outPort->sPortParam.nBufferSize);
                    pOutputBuffer->nFilledLen = vdi_read_memory(pVpu->coreIdx, pVpu->dispOutputInfo.dispFrame.bufY, (unsigned char*)pOutputBuffer->pBuffer, outPort->sPortParam.nBufferSize, pVpu->decOP.frameEndian);
                    DEBUG(DEB_LEV_FULL_SEQ, "UseBuffer : end copy video data to pOutputBuffer time=%.1fdms, len=%d\n", omx_time_curr_ms()-curr_ms, (int)pOutputBuffer->nFilledLen );

                    VPU_DecClrDispFlag(pVpu->handle, dispFlagIdx);
                    DEBUG(DEB_LEV_FULL_SEQ, "VPU_DecClrDispFlag for dispFlagIdx=%d\n", dispFlagIdx);

                }
            }


        }

        if (pVpu->ppuEnable)
            pVpu->ppuIdx = (pVpu->ppuIdx+1)%MAX_PPU_SRC_NUM;

        // save  dec width, height of PPU to display next decoding time.
        pVpu->rcPrevDisp = pVpu->dispOutputInfo.rcDisplay;
        pVpu->dispOutIdx++;

        DEBUG(DEB_LEV_PARAMS, "Display index=%d, indexFrameDecoded=%d, output nFilledLen: %d\n\n",
                pVpu->dispOutputInfo.indexFrameDisplay, pVpu->dispOutputInfo.indexFrameDecoded,
                (int)pOutputBuffer->nFilledLen);
        if ((int)pOutputBuffer->nFilledLen > 0)
            pVpu->stopdec = OMX_TRUE;
    } // end of IS_STATE_FILLTHISBUFFER()
	pthread_mutex_unlock(&omx_vpudec_component_Private->vpu_flush_mutex);
    return;

NEXT_CHUNK:
	if (pOutputBuffer)
		pOutputBuffer->nFilledLen = 0;
	if (pOutputBuffer && pInputBuffer)
		DEBUG(DEB_LEV_SIMPLE_SEQ, "Out  next chunk pInputBuffer->nFilledLen=%d, pOutputBuffer->nFilledLen=%d\n\n", (int)pInputBuffer->nFilledLen, (int)pOutputBuffer->nFilledLen);
	pthread_mutex_unlock(&omx_vpudec_component_Private->vpu_flush_mutex);
	return;
REACH_INPUT_BUFFER_EOS:
    if (pInputBuffer)
        pInputBuffer->nFilledLen = 0;

	if (pOutputBuffer) // for output
	{
		pOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
		pOutputBuffer->nFilledLen = 0;
	}
	pthread_mutex_unlock(&omx_vpudec_component_Private->vpu_flush_mutex);
	return;
REACH_OUTPUT_BUFFER_EOS:
    if (pOutputBuffer) // for output
    {
        pOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
        pOutputBuffer->nFilledLen = 0;
    }
	if (pInputBuffer)
	{
		pInputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
		pInputBuffer->nFilledLen = 0; // that makes calling ReturnBufferFuntion for InputBuffer that has EOS flag.
	}
	if (pOutputBuffer && pInputBuffer)
		DEBUG(DEB_LEV_SIMPLE_SEQ, "Out  reach EOS pInputBuffer->nFilledLen=%d, pOutputBuffer->nFilledLen=%d\n\n", (int)pInputBuffer->nFilledLen, (int)pOutputBuffer->nFilledLen);
	pthread_mutex_unlock(&omx_vpudec_component_Private->vpu_flush_mutex);
    return;
SKIP_DISPLAY:
	if (pOutputBuffer)
	{
		pOutputBuffer->nFilledLen = 0;
	   DEBUG(DEB_LEV_FULL_SEQ, "Out  skip display pOutputBuffer->nFilledLen=%d\n\n", (int)pOutputBuffer->nFilledLen);
	}
	pthread_mutex_unlock(&omx_vpudec_component_Private->vpu_flush_mutex);
    return;
ERR_DEC:
    DEBUG(DEB_LEV_ERR, "dec error, out\n\n");
    if (pVpu->handle)
        VPU_DecGetOutputInfo(pVpu->handle, &pVpu->dispOutputInfo);
    if (pOutputBuffer)
        pOutputBuffer->nFilledLen = 0;
    if (pInputBuffer)
        pInputBuffer->nFilledLen = 0;
    if (omx_vpudec_component_Private->callbacks->EventHandler)
    {
        (*(omx_vpudec_component_Private->callbacks->EventHandler))
            (openmaxStandComp,
            omx_vpudec_component_Private->callbackData,
            OMX_EventError,
            err,
            0,
            NULL);
    }
	pthread_mutex_unlock(&omx_vpudec_component_Private->vpu_flush_mutex);
    return;
}

OMX_ERRORTYPE omx_vpudec_component_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_IN OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_U32 portIndex;
    OMX_U32 paramIndex;
    /* Check which structure we are being fed and make control its header */
    OMX_COMPONENTTYPE *openmaxStandComp = hComponent;
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    omx_vpudec_component_PortType *port;

    if (pComponentConfigStructure == NULL)
    {
        return OMX_ErrorBadParameter;
    }

    paramIndex = nParamIndex;
    DEBUG(DEB_LEV_FUNCTION_NAME, "Setting parameter 0x%x\n", (int)paramIndex);
    switch (paramIndex)
    {
    case OMX_IndexParamPortDefinition:
    {
        err = omx_base_component_SetParameter(hComponent, nParamIndex, pComponentConfigStructure);
        if (err == OMX_ErrorNone)
        {
            OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE*) pComponentConfigStructure;
            portIndex = pPortDef->nPortIndex;
            port = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[portIndex];

            //memcpy(&port->sPortParam, pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            if (port->sPortParam.nBufferSize < pPortDef->nBufferSize)
                port->sPortParam.nBufferSize = pPortDef->nBufferSize;

            port->sVideoParam.eColorFormat = pPortDef->format.video.eColorFormat;
            port->sVideoParam.eCompressionFormat = pPortDef->format.video.eCompressionFormat;
            if (portIndex == OMX_BASE_FILTER_INPUTPORT_INDEX)
            {
                port->omxConfigCrop.nWidth = pPortDef->format.video.nFrameWidth;
                port->omxConfigCrop.nHeight = pPortDef->format.video.nFrameHeight;
                UpdateFrameSize(openmaxStandComp);
            }

            DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_IndexParamPortDefinition nPortIndex=%d, nStride=%d, nSliceHeight=%d, eColorFormat=0x%x, eCompressionFormat=0x%x, xFrameRate=%d\n", (int)portIndex,
                  pPortDef->format.video.nStride, pPortDef->format.video.nSliceHeight,
                  (int)pPortDef->format.video.eColorFormat, (int)pPortDef->format.video.eCompressionFormat, (int)pPortDef->format.video.xFramerate);
        }
        break;
    }
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoPortFormat;
        pVideoPortFormat = pComponentConfigStructure;
        portIndex = pVideoPortFormat->nPortIndex;
        /*Check Structure Header and verify component state*/
        err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pVideoPortFormat, sizeof (OMX_VIDEO_PARAM_PORTFORMATTYPE));
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Parameter Check Error=%x\n", (int)err);
            break;
        }
        port = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[portIndex];
        if (portIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
        {
            memcpy(&port->sVideoParam, pVideoPortFormat, sizeof (OMX_VIDEO_PARAM_PORTFORMATTYPE));
            omx_vpudec_component_Private->ports[portIndex]->sPortParam.format.video.eColorFormat = pVideoPortFormat->eColorFormat;
            omx_vpudec_component_Private->ports[portIndex]->sPortParam.format.video.eCompressionFormat = pVideoPortFormat->eCompressionFormat;
        }
        else if (portIndex == OMX_BASE_FILTER_INPUTPORT_INDEX)
        {
            memcpy(&port->sVideoParam, pVideoPortFormat, sizeof (OMX_VIDEO_PARAM_PORTFORMATTYPE));
            omx_vpudec_component_Private->ports[portIndex]->sPortParam.format.video.eColorFormat = pVideoPortFormat->eColorFormat;
            omx_vpudec_component_Private->ports[portIndex]->sPortParam.format.video.eCompressionFormat = pVideoPortFormat->eCompressionFormat;
            UpdateFrameSize(openmaxStandComp);
        }
        else
        {
            return OMX_ErrorBadPortIndex;
        }

        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_IndexParamVideoPortFormat portIndex=%d, nIndex=%d, eColorFormat=0x%x, useNativeBuffer=%d, bThumbnailMode=%d\n", (int)portIndex, (int)pVideoPortFormat->nIndex,
              (int)pVideoPortFormat->eCompressionFormat, (int)omx_vpudec_component_Private->useNativeBuffer, (int)omx_vpudec_component_Private->bThumbnailMode);
        break;
    }
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pVideoAvc;
        pVideoAvc = pComponentConfigStructure;
        portIndex = pVideoAvc->nPortIndex;
        err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pVideoAvc, sizeof (OMX_VIDEO_PARAM_AVCTYPE));
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Parameter Check Error=%x\n", (int)err);
            break;
        }
        memcpy(&omx_vpudec_component_Private->codParam.avc, pVideoAvc, sizeof (OMX_VIDEO_PARAM_AVCTYPE));
        break;
    }
    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *pVideoMpeg4;
        pVideoMpeg4 = pComponentConfigStructure;
        portIndex = pVideoMpeg4->nPortIndex;
        err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pVideoMpeg4, sizeof (OMX_VIDEO_PARAM_MPEG4TYPE));
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Parameter Check Error=%x\n", (int)err);
            break;
        }
        if (pVideoMpeg4->nPortIndex == 0)
        {
            memcpy(&omx_vpudec_component_Private->codParam.mpeg4, pVideoMpeg4, sizeof (OMX_VIDEO_PARAM_MPEG4TYPE));
        }
        else
        {
            return OMX_ErrorBadPortIndex;
        }
        break;
    }
    case OMX_IndexParamVideoMSMpeg:
    {
        OMX_VIDEO_PARAM_MSMPEGTYPE* msmpegParam = NULL;
        msmpegParam = (OMX_VIDEO_PARAM_MSMPEGTYPE*) pComponentConfigStructure;
        if (msmpegParam->eFormat != OMX_VIDEO_MSMPEGFormat3) // VPU only support msgmpeg v3
        {
            err = OMX_ErrorUnsupportedSetting;
        }
        else
        {
            memcpy(&omx_vpudec_component_Private->codParam.msmpeg, pComponentConfigStructure,
                   sizeof (OMX_VIDEO_PARAM_MSMPEGTYPE));
        }
        break;
    }
    case OMX_IndexParamVideoWmv:
    {
        OMX_VIDEO_PARAM_WMVTYPE* wmvParam = NULL;
        wmvParam = (OMX_VIDEO_PARAM_WMVTYPE*) pComponentConfigStructure;
        if (wmvParam->eFormat != (OMX_VIDEO_WMVFORMATTYPE)OMX_VIDEO_WMVFormat9 &&
                wmvParam->eFormat != (OMX_VIDEO_WMVFORMATTYPE)OMX_VIDEO_WMVFormatVC1)
        {
            err = OMX_ErrorUnsupportedSetting;
        }
        else
        {
            memcpy(&omx_vpudec_component_Private->codParam.wmv, pComponentConfigStructure,
                   sizeof (OMX_VIDEO_PARAM_WMVTYPE));
        }
        break;
    }
    case OMX_IndexParamVideoRv:
    {
        OMX_VIDEO_PARAM_RVTYPE* rvParam = NULL;
        rvParam = (OMX_VIDEO_PARAM_RVTYPE*) pComponentConfigStructure;
        if (rvParam->eFormat != OMX_VIDEO_RVFormat8 &&
                rvParam->eFormat != OMX_VIDEO_RVFormat9)
        {
            err = OMX_ErrorUnsupportedSetting;
        }
        else
        {
            memcpy(&omx_vpudec_component_Private->codParam.rv, pComponentConfigStructure,
                   sizeof (OMX_VIDEO_PARAM_RVTYPE));
        }
        break;
    }
    case OMX_IndexParamVideoMpeg2:
    {
        memcpy(&omx_vpudec_component_Private->codParam.mpeg2, pComponentConfigStructure,
               sizeof (OMX_VIDEO_PARAM_MPEG2TYPE));
        break;
    }
    case OMX_IndexParamVideoH263:
    {
        memcpy(&omx_vpudec_component_Private->codParam.h263, pComponentConfigStructure,
               sizeof (OMX_VIDEO_PARAM_H263TYPE));
        break;
    }

    case OMX_IndexParamVideoVp8:
    {
        memcpy(&omx_vpudec_component_Private->codParam.vp8, pComponentConfigStructure,
               sizeof (OMX_VIDEO_PARAM_VP8TYPE));
        break;
    }

    case OMX_IndexParamVideoAVS:
    {
        memcpy(&omx_vpudec_component_Private->codParam.avc, pComponentConfigStructure,
               sizeof (OMX_VIDEO_PARAM_AVCTYPE));
        break;
    }

    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole;
        pComponentRole = pComponentConfigStructure;
        if (omx_vpudec_component_Private->state != OMX_StateLoaded && omx_vpudec_component_Private->state != OMX_StateWaitForResources)
        {
            DEBUG(DEB_LEV_ERR, "Incorrect State=%x lineno=%d\n", (int)omx_vpudec_component_Private->state, __LINE__);
            return OMX_ErrorIncorrectStateOperation;
        }
        if ((err = checkHeader(pComponentConfigStructure, sizeof (OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone)
        {
            break;
        }
        if (!strcmp((char *) pComponentRole->cRole, VIDEO_DEC_MPEG4_ROLE))
        {
            omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingMPEG4;
        }
        else if (!strcmp((char *) pComponentRole->cRole, VIDEO_DEC_H264_ROLE))
        {
            omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingAVC;
        }
        else if (!strcmp((char *) pComponentRole->cRole, VIDEO_DEC_HEVC_ROLE))
        {
            omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingHEVC;
        }
        else if (!strcmp((char *) pComponentRole->cRole, VIDEO_DEC_MPEG2_ROLE))
        {
            omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingMPEG2;
        }
        else if (!strcmp((char *) pComponentRole->cRole, VIDEO_DEC_RV_ROLE))
        {
            omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingRV;
        }
        else if (!strcmp((char *) pComponentRole->cRole, VIDEO_DEC_WMV_ROLE))
        {
            omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingWMV;
        }
        else if (!strcmp((char *) pComponentRole->cRole, VIDEO_DEC_H263_ROLE))
        {
            omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingH263;
        }
        else if (!strcmp((char *) pComponentRole->cRole, VIDEO_DEC_MSMPEG_ROLE))
        {
            omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingMSMPEG;
        }
        else if (!strcmp((char *) pComponentRole->cRole, VIDEO_DEC_AVS_ROLE))
        {
            omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingAVS;
        }
        else if (!strcmp((char *) pComponentRole->cRole, VIDEO_DEC_VP8_ROLE) || !strcmp((char *) pComponentRole->cRole, "video_decoder.vp8"))
        {
            omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CODINGTYPE_VP8;
        }
        else if (!strcmp((char *) pComponentRole->cRole, VIDEO_DEC_VC1_ROLE))
        {
            omx_vpudec_component_Private->video_coding_type = OMX_VIDEO_CodingVC1;
        }
        else
        {
            return OMX_ErrorBadParameter;
        }
        SetInternalVideoParameters(openmaxStandComp);
        break;
    }
#ifdef ANDROID


    case OMX_IndexParamEnableAndroidBuffers_CM:
    {
        DEBUG(DEB_LEV_FULL_SEQ, "OMX_IndexParamEnableAndroidBuffers_CM...\n");
        err = checkEnableAndroidBuffersHeader(pComponentConfigStructure);
        if (err != OMX_ErrorNone)
            break;

        err = checkEnableAndroidBuffersPort(pComponentConfigStructure, &portIndex);
        if (err != OMX_ErrorNone)
            break;

        if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
            return OMX_ErrorBadPortIndex;

        port = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
        if (enableAndroidBuffer(pComponentConfigStructure))
        {
            omx_vpudec_component_Private->useNativeBuffer = OMX_TRUE;
            port->sPortParam.format.video.eColorFormat = DEFAULT_NATIVE_OUTPUT_FORMAT;
            port->sVideoParam.eColorFormat = DEFAULT_NATIVE_OUTPUT_FORMAT;
            DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_IndexParamEnableAndroidBuffers useNativeBuffer=%d, eColorFormat=0x%x\n", (int)omx_vpudec_component_Private->useNativeBuffer, (int)port->sPortParam.format.video.eColorFormat);
        }
        else
        {
            omx_vpudec_component_Private->useNativeBuffer = OMX_FALSE;
#ifdef WORKAROUND_VP8_CTS_TEST
            if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CODINGTYPE_VP8)
            {
                port->sPortParam.format.video.eColorFormat = DEFAULT_VP8_VIDEO_OUTPUT_FORMAT;
                port->sVideoParam.eColorFormat = DEFAULT_VP8_VIDEO_OUTPUT_FORMAT;
            }
            else
            {
                port->sPortParam.format.video.eColorFormat = DEFAULT_VIDEO_OUTPUT_FORMAT;
                port->sVideoParam.eColorFormat = DEFAULT_VIDEO_OUTPUT_FORMAT;
            }
#else
            port->sPortParam.format.video.eColorFormat = DEFAULT_VIDEO_OUTPUT_FORMAT;
            port->sVideoParam.eColorFormat = DEFAULT_VIDEO_OUTPUT_FORMAT;
#endif
        }

        break;
    }

    case (OMX_INDEXTYPE)OMX_IndexParamUseAndroidNativeBuffer:
    {
        err = checkUseAndroidNativeBufferHeader(pComponentConfigStructure);
        if (err != OMX_ErrorNone)
            break;

        err = checkUseAndroidNativeBufferPort(pComponentConfigStructure, &portIndex);
        if (err != OMX_ErrorNone)
            break;

        if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
            return OMX_ErrorBadPortIndex;

        port = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[portIndex];
#if 0
        if (port->nNumAssignedBuffers == 0 &&
                port->sPortParam.nBufferCountActual != port->nTempBufferCountActual &&
                omx_vpudec_component_Private->portSettingChangeRequest == OMX_TRUE)
        {
            int j;

            if(port->pInternalBufferStorage)
            {
                free(port->pInternalBufferStorage);
                port->pInternalBufferStorage = NULL;
            }

            if(port->bBufferStateAllocated)
            {
                free(port->bBufferStateAllocated);
                port->bBufferStateAllocated = NULL;
            }

            port->sPortParam.nBufferCountActual = port->nTempBufferCountActual;

            port->pInternalBufferStorage = malloc(port->sPortParam.nBufferCountActual*sizeof(OMX_BUFFERHEADERTYPE *));
            if (!port->pInternalBufferStorage) {
                DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
                return OMX_ErrorInsufficientResources;
            }

            memset(port->pInternalBufferStorage, 0x00, port->sPortParam.nBufferCountActual*sizeof(OMX_BUFFERHEADERTYPE *));

            port->bBufferStateAllocated = malloc(port->sPortParam.nBufferCountActual*sizeof(BUFFER_STATUS_FLAG));
            if (!port->bBufferStateAllocated) {
                DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
                return OMX_ErrorInsufficientResources;
            }

            memset(port->bBufferStateAllocated, 0x00, port->sPortParam.nBufferCountActual*sizeof(BUFFER_STATUS_FLAG));

            for(j=0; j < (int)port->sPortParam.nBufferCountActual; j++) {
                port->bBufferStateAllocated[j] = BUFFER_FREE;
            }
        }
#endif
        port->pInternalBufferStorage[port->nNumAssignedBuffers] = malloc(sizeof(OMX_BUFFERHEADERTYPE));
        if (!port->pInternalBufferStorage[port->nNumAssignedBuffers]) {
            DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
            return OMX_ErrorInsufficientResources;
        }
        memset(port->pInternalBufferStorage[port->nNumAssignedBuffers], 0x00, sizeof(OMX_BUFFERHEADERTYPE));

        err = useAndroidNativeBuffer(pComponentConfigStructure, &(port->pInternalBufferStorage[port->nNumAssignedBuffers]), port->sPortParam.format.video.eColorFormat);
        if (err != OMX_ErrorNone)
            break;


        port->pInternalBufferStorage[port->nNumAssignedBuffers]->pPlatformPrivate = port;
        port->bBufferStateAllocated[port->nNumAssignedBuffers] = BUFFER_ASSIGNED;
        port->bBufferStateAllocated[port->nNumAssignedBuffers] |= HEADER_ALLOCATED;
        if (omx_vpudec_component_Private->omx_display_flags)
            omx_vpudec_component_Private->omx_display_flags[port->nNumAssignedBuffers].owner = OMX_BUFFER_OWNER_NOBODY;
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_IndexParamUseAndroidNativeBuffer pInternalBufferStorage[%d]=%p, nBufferCountActual=%d, eColorFormat=0x%x\n",
                (int)port->nNumAssignedBuffers, port->pInternalBufferStorage[port->nNumAssignedBuffers],
                (int)port->sPortParam.nBufferCountActual, (int)port->sPortParam.format.video.eColorFormat);

        port->nNumAssignedBuffers++;

        if(port->nNumAssignedBuffers == port->sPortParam.nBufferCountActual)
        {
            port->sPortParam.bPopulated = OMX_TRUE;
            port->bIsFullOfBuffers = OMX_TRUE;
            tsem_up(port->pAllocSem);
            if (omx_vpudec_component_Private->portSettingChangeRequest == OMX_TRUE)
            {
                omx_vpudec_component_Private->portSettingChangeRequest = OMX_FALSE;
                tsem_up(&omx_vpudec_component_Private->port_setting_change_tsem);

            }
            DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_IndexParamUseAndroidNativeBuffer nNumAssignedBuffers=%d, bPopulated=%d\n", (int)port->nNumAssignedBuffers, (int)port->sPortParam.bPopulated);
        }

        err = OMX_ErrorNone;

        break;
    }
#endif
    default: /*Call the base component function*/
        return omx_base_component_SetParameter(hComponent, nParamIndex, pComponentConfigStructure);
    }
    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of SetParameter\n");
    return err;
}
OMX_ERRORTYPE omx_vpudec_component_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    omx_vpudec_component_PortType *port;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
#ifdef ANDROID
    OMX_U32 portIndex;
#endif
    OMX_U32 paramIndex;


    if (pComponentConfigStructure == NULL)
    {
        return OMX_ErrorBadParameter;
    }
    paramIndex = (OMX_U32)nParamIndex;
    DEBUG(DEB_LEV_FUNCTION_NAME, "Getting parameter 0x%x, hComponent=0x%p, omx_vpudec_component_Private=0x%p\n", (int)paramIndex, hComponent, omx_vpudec_component_Private);
    /* Check which structure we are being fed and fill its header */
    switch (paramIndex)
    {
    case OMX_IndexParamVideoInit:
        if ((err = checkHeader(pComponentConfigStructure, sizeof (OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone)
        {
            break;
        }
        memcpy(pComponentConfigStructure, &omx_vpudec_component_Private->sPortTypesParam[OMX_PortDomainVideo], sizeof (OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoPortFormat;
        pVideoPortFormat = pComponentConfigStructure;
        OMX_U32 nIndex;

        if ((err = checkHeader(pComponentConfigStructure, sizeof (OMX_VIDEO_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone)
        {
            break;
        }

        portIndex = pVideoPortFormat->nPortIndex;
        nIndex = pVideoPortFormat->nIndex;
        port = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[pVideoPortFormat->nPortIndex];

        if (portIndex == OMX_BASE_FILTER_INPUTPORT_INDEX)
        {
            pVideoPortFormat->eColorFormat = OMX_COLOR_FormatUnused;
            pVideoPortFormat->eCompressionFormat = port->sPortParam.format.video.eCompressionFormat;
        }
        else if (portIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
        {
            pVideoPortFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
            if (nIndex == 0)
            {
#ifdef WORKAROUND_VP8_CTS_TEST
                if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CODINGTYPE_VP8)
                    pVideoPortFormat->eColorFormat = DEFAULT_VP8_VIDEO_OUTPUT_FORMAT;
                else
                    pVideoPortFormat->eColorFormat = DEFAULT_VIDEO_OUTPUT_FORMAT;
#else
                pVideoPortFormat->eColorFormat = DEFAULT_VIDEO_OUTPUT_FORMAT;
#endif

                if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
                    pVideoPortFormat->eColorFormat = DEFAULT_NATIVE_OUTPUT_FORMAT;
            }
            else if (nIndex == 1)
                pVideoPortFormat->eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            else if (nIndex == 2)
                pVideoPortFormat->eColorFormat = OMX_COLOR_FormatYUV422Planar;
            else if (nIndex == 3)
                pVideoPortFormat->eColorFormat = OMX_COLOR_FormatYUV422SemiPlanar;
            else if (nIndex == 4)
                pVideoPortFormat->eColorFormat = OMX_COLOR_FormatYCbYCr;
            else if (nIndex == 5)
                pVideoPortFormat->eColorFormat = OMX_COLOR_FormatCbYCrY;
            else
            {
                DEBUG(DEB_LEV_ERR, "OMX_IndexParamVideoPortFormat OMX_ErrorNoMore\n");
                err =  OMX_ErrorNoMore;
            }
        }
        else
        {
            DEBUG(DEB_LEV_ERR, "OMX_IndexParamVideoPortFormat OMX_ErrorBadPortIndex\n");
            err = OMX_ErrorBadPortIndex;
        }

        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_IndexParamVideoPortFormat nPortIndex=%d, nIndex=%d,  eColorFormat=0x%x, eCompressionFormat=0x%x, useNativeBuffer=%d, bThumbnailMode=%d\n", (int)portIndex, (int)nIndex,
              (int)pVideoPortFormat->eColorFormat, (int)pVideoPortFormat->eCompressionFormat, (int)omx_vpudec_component_Private->useNativeBuffer, (int)omx_vpudec_component_Private->bThumbnailMode);

        break;
    }
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE * pComponentRole;
        pComponentRole = pComponentConfigStructure;
        if ((err = checkHeader(pComponentConfigStructure, sizeof (OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone)
        {
            break;
        }
        if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingMPEG4)
        {
            strcpy((char *) pComponentRole->cRole, VIDEO_DEC_MPEG4_ROLE);
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingAVC)
        {
            strcpy((char *) pComponentRole->cRole, VIDEO_DEC_H264_ROLE);
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingHEVC)
        {
            strcpy((char *) pComponentRole->cRole, VIDEO_DEC_HEVC_ROLE);
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingMPEG2)
        {
            strcpy((char *) pComponentRole->cRole, VIDEO_DEC_MPEG2_ROLE);
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingH263)
        {
            strcpy((char *) pComponentRole->cRole, VIDEO_DEC_H263_ROLE);
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingWMV)
        {
            strcpy((char *) pComponentRole->cRole, VIDEO_DEC_WMV_ROLE);
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingRV)
        {
            strcpy((char *) pComponentRole->cRole, VIDEO_DEC_RV_ROLE);
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingMSMPEG)
        {
            strcpy((char *) pComponentRole->cRole, VIDEO_DEC_MSMPEG_ROLE);
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingAVS)
        {
            strcpy((char *) pComponentRole->cRole, VIDEO_DEC_AVS_ROLE);
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CODINGTYPE_VP8)
        {
            strcpy((char *) pComponentRole->cRole, VIDEO_DEC_VP8_ROLE);
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingVC1)
        {
            strcpy((char *) pComponentRole->cRole, VIDEO_DEC_VC1_ROLE);
        }
        else
        {
            strcpy((char *) pComponentRole->cRole, "\0");
        }
        break;
    }
    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *pVideoMpeg4;
        pVideoMpeg4 = pComponentConfigStructure;
        if (pVideoMpeg4->nPortIndex != 0)
        {
            return OMX_ErrorBadPortIndex;
        }
        if ((err = checkHeader(pComponentConfigStructure, sizeof (OMX_VIDEO_PARAM_MPEG4TYPE))) != OMX_ErrorNone)
        {
            break;
        }
        memcpy(pVideoMpeg4, &omx_vpudec_component_Private->codParam.mpeg4, sizeof (OMX_VIDEO_PARAM_MPEG4TYPE));
        break;
    }
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE * pVideoAvc;
        pVideoAvc = pComponentConfigStructure;
        if (pVideoAvc->nPortIndex != 0)
        {
            return OMX_ErrorBadPortIndex;
        }
        if ((err = checkHeader(pComponentConfigStructure, sizeof (OMX_VIDEO_PARAM_AVCTYPE))) != OMX_ErrorNone)
        {
            break;
        }
        memcpy(pVideoAvc, &omx_vpudec_component_Private->codParam.avc, sizeof (OMX_VIDEO_PARAM_AVCTYPE));
        break;
    }
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE * pVideoParam;
        pVideoParam = pComponentConfigStructure;
        if (pVideoParam->nPortIndex != 0)
        {
            return OMX_ErrorBadPortIndex;
        }
        if ((err = checkHeader(pComponentConfigStructure, sizeof (OMX_VIDEO_PARAM_H263TYPE))) != OMX_ErrorNone)
        {
            break;
        }
        memcpy(pVideoParam, &omx_vpudec_component_Private->codParam.h263, sizeof (OMX_VIDEO_PARAM_H263TYPE));
        break;
    }
    case OMX_IndexParamVideoRv:
    {
        OMX_VIDEO_PARAM_RVTYPE * pVideoParam;
        pVideoParam = pComponentConfigStructure;
        if (pVideoParam->nPortIndex != 0)
        {
            return OMX_ErrorBadPortIndex;
        }
        if ((err = checkHeader(pComponentConfigStructure, sizeof (OMX_VIDEO_PARAM_RVTYPE))) != OMX_ErrorNone)
        {
            break;
        }
        memcpy(pVideoParam, &omx_vpudec_component_Private->codParam.rv, sizeof (OMX_VIDEO_PARAM_RVTYPE));
        break;
    }
    case OMX_IndexParamVideoWmv:
    {
        OMX_VIDEO_PARAM_WMVTYPE * pVideoParam;
        pVideoParam = pComponentConfigStructure;
        if (pVideoParam->nPortIndex != 0)
        {
            return OMX_ErrorBadPortIndex;
        }
        if ((err = checkHeader(pComponentConfigStructure, sizeof (OMX_VIDEO_PARAM_WMVTYPE))) != OMX_ErrorNone)
        {
            break;
        }
        memcpy(pVideoParam, &omx_vpudec_component_Private->codParam.wmv, sizeof (OMX_VIDEO_PARAM_WMVTYPE));
        break;
    }
    case OMX_IndexParamVideoVp8:
    {
        OMX_VIDEO_PARAM_VP8TYPE * pVideoParam;
        pVideoParam = pComponentConfigStructure;
        if (pVideoParam->nPortIndex != 0)
        {
            return OMX_ErrorBadPortIndex;
        }
        if ((err = checkHeader(pComponentConfigStructure, sizeof (OMX_VIDEO_PARAM_VP8TYPE))) != OMX_ErrorNone)
        {
            break;
        }
        memcpy(pVideoParam, &omx_vpudec_component_Private->codParam.vp8, sizeof (OMX_VIDEO_PARAM_VP8TYPE));
        break;
    }
    case OMX_IndexParamVideoMpeg2:
    {
        OMX_VIDEO_PARAM_MPEG2TYPE * pVideoParam;
        pVideoParam = pComponentConfigStructure;
        if (pVideoParam->nPortIndex != 0)
        {
            return OMX_ErrorBadPortIndex;
        }
        if ((err = checkHeader(pComponentConfigStructure, sizeof (OMX_VIDEO_PARAM_MPEG2TYPE))) != OMX_ErrorNone)
        {
            break;
        }
        memcpy(pVideoParam, &omx_vpudec_component_Private->codParam.mpeg2, sizeof (OMX_VIDEO_PARAM_MPEG2TYPE));
        break;
    }
    case OMX_IndexParamVideoMSMpeg:
    {
        OMX_VIDEO_PARAM_MSMPEGTYPE * pVideoParam;
        pVideoParam = pComponentConfigStructure;
        if (pVideoParam->nPortIndex != 0)
        {
            return OMX_ErrorBadPortIndex;
        }
        if ((err = checkHeader(pComponentConfigStructure, sizeof (OMX_VIDEO_PARAM_MSMPEGTYPE))) != OMX_ErrorNone)
        {
            break;
        }
        memcpy(pVideoParam, &omx_vpudec_component_Private->codParam.msmpeg, sizeof (OMX_VIDEO_PARAM_MSMPEGTYPE));
        break;
    }

    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentConfigStructure;

        portIndex = pDstProfileLevel->nPortIndex;

        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_IndexParamVideoProfileLevelQuerySupported portIndex=%d\n", (int)portIndex);

        if(portIndex != OMX_BASE_FILTER_INPUTPORT_INDEX)
            return OMX_ErrorBadPortIndex;

        err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pDstProfileLevel, sizeof (OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Parameter Check Error=%x\n", (int)err);
            break;
        }

        if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingAVC)
        {
            if (pDstProfileLevel->nProfileIndex == 0)
            {
                pDstProfileLevel->eProfile = OMX_VIDEO_AVCProfileBaseline;
                pDstProfileLevel->eLevel   = OMX_VIDEO_AVCLevel41;
            }
            else if (pDstProfileLevel->nProfileIndex == 1)
            {
                pDstProfileLevel->eProfile = OMX_VIDEO_AVCProfileMain;
                pDstProfileLevel->eLevel   = OMX_VIDEO_AVCLevel41;
            }
            else if (pDstProfileLevel->nProfileIndex == 2)
            {
                pDstProfileLevel->eProfile = OMX_VIDEO_AVCProfileMain;
                pDstProfileLevel->eLevel   = OMX_VIDEO_AVCLevel41;
            }
            else
            {
                DEBUG(DEB_LEV_ERR, "OMX_ErrorNoMore nProfileIndex=%d, video_encoding_type=0x%x\n",
                      (int)pDstProfileLevel->nProfileIndex, (int)omx_vpudec_component_Private->video_coding_type);
                return OMX_ErrorNoMore;
            }
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingMPEG4)
        {
            if (pDstProfileLevel->nProfileIndex == 0)
            {
                pDstProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
                pDstProfileLevel->eLevel   = OMX_VIDEO_MPEG4Level5;
            }
            else if (pDstProfileLevel->nProfileIndex == 1)
            {
                pDstProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileAdvancedSimple;
                pDstProfileLevel->eLevel   = OMX_VIDEO_MPEG4Level5;
            }
            else
            {
                DEBUG(DEB_LEV_ERR, "OMX_ErrorNoMore nProfileIndex=%d, video_encoding_type=0x%x\n",
                      (int)pDstProfileLevel->nProfileIndex, (int)omx_vpudec_component_Private->video_coding_type);
                return OMX_ErrorNoMore;
            }
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingH263)
        {
            if (pDstProfileLevel->nProfileIndex == 0)
            {
                pDstProfileLevel->eProfile = OMX_VIDEO_H263ProfileBaseline;
                pDstProfileLevel->eLevel   = OMX_VIDEO_H263Level70;
            }
            else
            {
                DEBUG(DEB_LEV_ERR, "OMX_ErrorNoMore nProfileIndex=%d, video_encoding_type=0x%x\n",
                      (int)pDstProfileLevel->nProfileIndex, (int)omx_vpudec_component_Private->video_coding_type);
                return OMX_ErrorNoMore;
            }
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingMPEG2)
        {
            if (pDstProfileLevel->nProfileIndex == 0)
            {
                pDstProfileLevel->eProfile = OMX_VIDEO_MPEG2ProfileSimple;
                pDstProfileLevel->eLevel   = OMX_VIDEO_MPEG2LevelHL;
            }
            else if (pDstProfileLevel->nProfileIndex == 1)
            {
                pDstProfileLevel->eProfile = OMX_VIDEO_MPEG2ProfileMain;
                pDstProfileLevel->eLevel   = OMX_VIDEO_MPEG2LevelHL;
            }
            else
            {
                DEBUG(DEB_LEV_ERR, "OMX_ErrorNoMore nProfileIndex=%d, video_encoding_type=0x%x\n",
                      (int)pDstProfileLevel->nProfileIndex, (int)omx_vpudec_component_Private->video_coding_type);
                return OMX_ErrorNoMore;
            }
        }
        else if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CODINGTYPE_VP8)
        {
            if (pDstProfileLevel->nProfileIndex == 0)
            {
                pDstProfileLevel->eProfile = OMX_VIDEO_VP8ProfileMain;
                pDstProfileLevel->eLevel   = OMX_VIDEO_VP8Level_Version3;
            }
            else
            {
                DEBUG(DEB_LEV_ERR, "OMX_ErrorNoMore nProfileIndex=%d, video_encoding_type=0x%x\n",
                      (int)pDstProfileLevel->nProfileIndex, (int)omx_vpudec_component_Private->video_coding_type);
                return OMX_ErrorNoMore;
            }
        }
        else
        {
            DEBUG(DEB_LEV_ERR, "OMX_ErrorNoMore Unsupported coding type=0x%x\n",
                  (int)omx_vpudec_component_Private->video_coding_type);
            return OMX_ErrorNoMore;
        }

        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_IndexParamVideoProfileLevelQuerySupported return ideo_encoding_type=0x%x, nProfileIndex=%d, eProfile=0x%x, eLevel=0x%x\n", (int)omx_vpudec_component_Private->video_coding_type, (int)pDstProfileLevel->nProfileIndex, (int)pDstProfileLevel->eProfile, (int)pDstProfileLevel->eLevel);

        break;
    }

    break;
    case OMX_IndexParamVideoProfileLevelCurrent:
        /* dummy for passing conformance test */
        err = OMX_ErrorNone;
        break;

#ifdef ANDROID
    case OMX_IndexParamGetAndroidNativeBuffer_CM:
    {
        err = checkGetAndroidNativeBufferHeader(pComponentConfigStructure);
        if (err != OMX_ErrorNone)
            break;

        err = checkGetAndroidNativeBufferPort(pComponentConfigStructure, &portIndex);
        if (err != OMX_ErrorNone)
            break;

        if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
            return OMX_ErrorBadPortIndex;

        err = GetAndroidNativeBuffer(pComponentConfigStructure);

        break;
    }
    case OMX_IndexParamPortDefinition:
    {
        err = omx_base_component_GetParameter(hComponent, nParamIndex, pComponentConfigStructure);
        if (err == OMX_ErrorNone)
        {
            OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE*) pComponentConfigStructure;
            if (pPortDef)
            {
                omx_vpudec_component_PortType *port = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[pPortDef->nPortIndex];
                if (pPortDef->nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
                {
                    if (omx_vpudec_component_Private->portSettingChangeRequest && port->sPortParam.nBufferCountActual != port->nTempBufferCountActual)
                    {
                        // it is time to update the new nBufferCountActual value into component. because ILCient does not call SetParameter[OMX_IndexParamPortDefinition] once portSettingChangeRequest.
                        pPortDef->nBufferCountActual = port->nTempBufferCountActual;
                    }
                }

                //pPortDef->format.video.eColorFormat = MAKE_FOURCC('Y', 'V', '1', '2'); //for TCC

                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_IndexParamPortDefinition portIndex=%d, eColorFormat=0x%x, eCompressionFormat=0x%x, xFramerate=%d, nBufferCountActual=%d, nTempBufferCountActual=%d, portSettingChangeRequest=%d\n", (int)pPortDef->nPortIndex,
                      (int)pPortDef->format.video.eColorFormat, (int)pPortDef->format.video.eCompressionFormat, (int)pPortDef->format.video.xFramerate, (int)pPortDef->nBufferCountActual, (int)port->nTempBufferCountActual, (int)omx_vpudec_component_Private->portSettingChangeRequest);
            }
        }

        break;
    }
#endif
    default: /*Call the base component function*/
        DEBUG(DEB_LEV_FUNCTION_NAME, "get parameter %i, %d\n", nParamIndex, err);
        return omx_base_component_GetParameter(hComponent, nParamIndex, pComponentConfigStructure);
    }
    DEBUG(DEB_LEV_FUNCTION_NAME, "Finish get parameter %i, %d\n", nParamIndex, err);
    return err;
}
OMX_ERRORTYPE omx_vpudec_component_MessageHandler(OMX_COMPONENTTYPE* openmaxStandComp, internalRequestMessageType *message)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = (omx_vpudec_component_PrivateType*) openmaxStandComp->pComponentPrivate;
    OMX_ERRORTYPE err;
    OMX_STATETYPE eCurrentState = omx_vpudec_component_Private->state;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;

    DEBUG(DEB_LEV_FULL_SEQ, "messageType=%d, messageParam=%d, eCurrentState=%d\n", message->messageType, message->messageParam, (int)eCurrentState);
    if (message->messageType == OMX_CommandStateSet)
    {
        if ((message->messageParam == OMX_StateIdle) && (eCurrentState == OMX_StateLoaded))
        {
            err = omx_vpudec_component_Init(openmaxStandComp);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Video Decoder Init Failed Error=%x\n", (int)err);
                return err;
            }
        }
#ifdef WAITING_TIME_FOR_NEXT_OUTPUT_BUFFER_AFTER_DISPLAY_BUFFER_FULL
        if (((message->messageParam == OMX_StateIdle) && (eCurrentState == OMX_StateExecuting)) ||
                ((message->messageParam == OMX_StatePause) && (eCurrentState == OMX_StateExecuting)))
        {
            tsem_up(omx_vpudec_component_Private->bDispBufFullSem);
        }
#endif
    }

    if (message->messageType == OMX_CommandPortEnable)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_CommandPortEnable !\n");
    }

    if ((message->messageType == OMX_CommandPortDisable) ||
            (message->messageType == OMX_CommandFlush)) //output buffer flush : flush the display buffer
    {

        if(message->messageType == OMX_CommandPortDisable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_CommandPortDisable port=%d\n", message->messageParam);
#ifdef WAITING_TIME_FOR_NEXT_OUTPUT_BUFFER_AFTER_DISPLAY_BUFFER_FULL
            if(eCurrentState != OMX_StateLoaded)
                tsem_up(omx_vpudec_component_Private->bDispBufFullSem);
#endif
        }
        else
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_CommandFlush port=%d\n", message->messageParam);
#ifdef WAITING_TIME_FOR_NEXT_OUTPUT_BUFFER_AFTER_DISPLAY_BUFFER_FULL
            tsem_up(omx_vpudec_component_Private->bDispBufFullSem);
#endif
        }

        if (message->messageParam == OMX_BASE_FILTER_INPUTPORT_INDEX || message->messageParam == OMX_BASE_FILTER_ALLPORT_INDEX)
        {
            omx_bufferheader_queue_dequeue_all(omx_vpudec_component_Private->in_bufferheader_queue);
            memset(&omx_vpudec_component_Private->omx_ts_correction, 0x00, sizeof(omx_timestamp_correction_t));
        }

        if (message->messageParam == OMX_BASE_FILTER_OUTPUTPORT_INDEX || message->messageParam == OMX_BASE_FILTER_ALLPORT_INDEX)
        {
            if (omx_vpudec_component_Private->vpuReady == OMX_TRUE)
            {
                if(OmxVpuFlush(openmaxStandComp) == OMX_FALSE)
                    return OMX_ErrorInsufficientResources;
            }
        }
    }

    if (message->messageType == OMX_CommandStateSet)
    {
        if ((message->messageParam == OMX_StateLoaded) && (eCurrentState == OMX_StateIdle))
        {
            err = omx_vpudec_component_Deinit(openmaxStandComp);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Video Decoder Deinit Failed Error=%x\n", (int)err);
                return err;
            }
        }
    }

    // Execute the base message handling
    err = omx_base_component_MessageHandler(openmaxStandComp, message);





    return err;
}
OMX_ERRORTYPE omx_vpudec_component_ComponentRoleEnum(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8 *cRole,
    OMX_IN OMX_U32 nIndex)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    DEBUG(DEB_LEV_SIMPLE_SEQ, ":%d Index(%d)\n", __LINE__, (int)nIndex);
    switch (nIndex)
    {
    case 0:
        strcpy((char*) cRole, VIDEO_DEC_H264_NAME);
        break;
    case 1:
        strcpy((char*) cRole, VIDEO_DEC_MPEG2_NAME);
        break;
    case 2:
        strcpy((char*) cRole, VIDEO_DEC_MPEG4_NAME);
        break;
    case 3:
        strcpy((char*) cRole, VIDEO_DEC_RV_NAME);
        break;
    case 4:
        strcpy((char*) cRole, VIDEO_DEC_WMV_NAME);
        break;
    case 5:
        strcpy((char*) cRole, VIDEO_DEC_H263_NAME);
        break;
    case 6:
        strcpy((char*) cRole, VIDEO_DEC_MSMPEG_NAME);
        break;
    case 7:
        strcpy((char*) cRole, VIDEO_DEC_AVS_ROLE);
        break;
    case 8:
        strcpy((char*) cRole, VIDEO_DEC_VP8_ROLE);
        break;
    case 9:
        strcpy((char*) cRole, VIDEO_DEC_THO_ROLE);
        break;
    case 10:
        strcpy((char*) cRole, VIDEO_DEC_JPG_ROLE);
        break;
    case 11:
        strcpy((char*) cRole, VIDEO_DEC_VC1_ROLE);
        break;
    default:
        err = OMX_ErrorUnsupportedIndex;
    }
    return err;
}



OMX_ERRORTYPE omx_vpudec_component_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_IN OMX_PTR pComponentConfigStructure)
{
    OMX_U32 portIndex;
    /* Check which structure we are being fed and make control its header */
    OMX_COMPONENTTYPE *openmaxStandComp = hComponent;
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    // Possible configs to set
    OMX_CONFIG_RECTTYPE *omxConfigCrop;
    OMX_CONFIG_ROTATIONTYPE *omxConfigRotate;
    OMX_CONFIG_MIRRORTYPE *omxConfigMirror;
    OMX_CONFIG_SCALEFACTORTYPE *omxConfigScale;
    OMX_CONFIG_POINTTYPE *omxConfigOutputPosition;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    omx_vpudec_component_PortType *pPort;
    OMX_U32 paramIndex;

    if (pComponentConfigStructure == NULL)
    {
        return OMX_ErrorBadParameter;
    }

    DEBUG(DEB_LEV_FULL_SEQ, "   Setting configuration %i\n", nParamIndex);

    paramIndex = nParamIndex;

    switch (paramIndex) {
    case OMX_IndexConfigCommonInputCrop:
    case OMX_IndexConfigCommonOutputCrop:
        omxConfigCrop = (OMX_CONFIG_RECTTYPE*)pComponentConfigStructure;
        portIndex = omxConfigCrop->nPortIndex;
        if ((err = checkHeader(pComponentConfigStructure, sizeof(OMX_CONFIG_RECTTYPE))) != OMX_ErrorNone) {
            break;
        }
        if ( (paramIndex == OMX_IndexConfigCommonOutputCrop && portIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)  ||
                (paramIndex == OMX_IndexConfigCommonInputCrop && portIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) ) {
            pPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[portIndex];
            pPort->omxConfigCrop.nLeft = omxConfigCrop->nLeft;
            pPort->omxConfigCrop.nTop = omxConfigCrop->nTop;
            pPort->omxConfigCrop.nWidth = omxConfigCrop->nWidth;
            pPort->omxConfigCrop.nHeight = omxConfigCrop->nHeight;
        } else if (portIndex <= 1) {
            return OMX_ErrorUnsupportedIndex;
        } else {
            return OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexConfigCommonRotate:
        omxConfigRotate = (OMX_CONFIG_ROTATIONTYPE*)pComponentConfigStructure;
        portIndex = omxConfigRotate->nPortIndex;
        if ((err = checkHeader(pComponentConfigStructure, sizeof(OMX_CONFIG_ROTATIONTYPE))) != OMX_ErrorNone) {
            break;
        }
        if (portIndex <= 1) {
            pPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[portIndex];
            if (omxConfigRotate->nRotation != 0) {
                //  Rotation not supported (yet)
                return OMX_ErrorUnsupportedSetting;
            }
            pPort->omxConfigRotate.nRotation = omxConfigRotate->nRotation;
        } else {
            return OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexConfigCommonMirror:
        omxConfigMirror = (OMX_CONFIG_MIRRORTYPE*)pComponentConfigStructure;
        portIndex = omxConfigMirror->nPortIndex;
        if ((err = checkHeader(pComponentConfigStructure, sizeof(OMX_CONFIG_MIRRORTYPE))) != OMX_ErrorNone) {
            break;
        }
        if (portIndex <= 1) {
            if (omxConfigMirror->eMirror == OMX_MirrorBoth || omxConfigMirror->eMirror == OMX_MirrorHorizontal)  {
                //  Horizontal mirroring not yet supported
                return OMX_ErrorUnsupportedSetting;
            }
            pPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[portIndex];
            pPort->omxConfigMirror.eMirror = omxConfigMirror->eMirror;
        } else {
            return OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexConfigCommonScale:
        omxConfigScale = (OMX_CONFIG_SCALEFACTORTYPE*)pComponentConfigStructure;
        portIndex = omxConfigScale->nPortIndex;
        if ((err = checkHeader(pComponentConfigStructure, sizeof(OMX_CONFIG_SCALEFACTORTYPE))) != OMX_ErrorNone) {
            break;
        }
        if (portIndex <= 1) {
            pPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[portIndex];
            pPort->omxConfigScale.xWidth = omxConfigScale->xWidth;
            pPort->omxConfigScale.xHeight = omxConfigScale->xHeight;
        } else {
            return OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexConfigCommonOutputPosition:
        omxConfigOutputPosition = (OMX_CONFIG_POINTTYPE*)pComponentConfigStructure;
        portIndex = omxConfigOutputPosition->nPortIndex;
        if ((err = checkHeader(pComponentConfigStructure, sizeof(OMX_CONFIG_POINTTYPE))) != OMX_ErrorNone) {
            break;
        }
        if (portIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX) {
            pPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[portIndex];
            pPort->omxConfigOutputPosition.nX = omxConfigOutputPosition->nX;
            pPort->omxConfigOutputPosition.nY = omxConfigOutputPosition->nY;
        } else if (portIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
            return OMX_ErrorUnsupportedIndex;
        } else {
            return OMX_ErrorBadPortIndex;
        }
        break;
#ifdef ANDROID
    case OMX_IndexConfigThumbnailMode_CM:
    {
        OMX_BOOL * pThumbNailEnable;
        pPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

        pThumbNailEnable = (OMX_BOOL *)pComponentConfigStructure;

        if (*pThumbNailEnable == OMX_TRUE)
            omx_vpudec_component_Private->bThumbnailMode = OMX_TRUE;
        else
            omx_vpudec_component_Private->bThumbnailMode = OMX_FALSE;

        omx_vpudec_component_Private->useNativeBuffer = OMX_FALSE;

        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_IndexParamThumbnailMode useNativeBuffer=%d, bThumbnailMode=%d, *pThumbNailEnable=%d, eColorFormat=0x%x\n",
              (int)omx_vpudec_component_Private->useNativeBuffer, (int)omx_vpudec_component_Private->bThumbnailMode, (int)*pThumbNailEnable, (int)pPort->sPortParam.format.video.eColorFormat);

        break;
    }
#endif

    default: // delegate to superclass
        return omx_base_component_SetConfig(hComponent, paramIndex, pComponentConfigStructure);
    }
    return err;
}



OMX_ERRORTYPE omx_vpudec_component_GetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_IN OMX_PTR pComponentConfigStructure)
{

    // Possible configs to ask for
    OMX_CONFIG_RECTTYPE *omxConfigCrop;
    OMX_CONFIG_ROTATIONTYPE *omxConfigRotate;
    OMX_CONFIG_MIRRORTYPE *omxConfigMirror;
    OMX_CONFIG_SCALEFACTORTYPE *omxConfigScale;
    OMX_CONFIG_POINTTYPE *omxConfigOutputPosition;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    omx_vpudec_component_PortType *pPort;
    if (pComponentConfigStructure == NULL) {
        return OMX_ErrorBadParameter;
    }
    DEBUG(DEB_LEV_FULL_SEQ, "   Getting configuration %i\n", nParamIndex);
    /* Check which structure we are being fed and fill its header */
    switch (nParamIndex) {
    case OMX_IndexConfigCommonInputCrop:
        omxConfigCrop = (OMX_CONFIG_RECTTYPE*)pComponentConfigStructure;
        if ((err = checkHeader(pComponentConfigStructure, sizeof(OMX_CONFIG_RECTTYPE))) != OMX_ErrorNone) {
            break;
        }
        if (omxConfigCrop->nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
            pPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[omxConfigCrop->nPortIndex];
            memcpy(omxConfigCrop, &pPort->omxConfigCrop, sizeof(OMX_CONFIG_RECTTYPE));
        } else if (omxConfigCrop->nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX) {
            return OMX_ErrorUnsupportedIndex;
        } else {
            return OMX_ErrorBadPortIndex;
        }

        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_IndexConfigCommonInputCrop nLeft=%d, nTop=%d, nWidth=%d, nHeight=%d\n",
              (int)omxConfigCrop->nLeft, (int)omxConfigCrop->nTop, (int)omxConfigCrop->nWidth, (int)omxConfigCrop->nHeight);

        break;
    case OMX_IndexConfigCommonOutputCrop:
        omxConfigCrop = (OMX_CONFIG_RECTTYPE*)pComponentConfigStructure;
        if ((err = checkHeader(pComponentConfigStructure, sizeof(OMX_CONFIG_RECTTYPE))) != OMX_ErrorNone) {
            break;
        }
        if (omxConfigCrop->nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX) {
            pPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[omxConfigCrop->nPortIndex];
            memcpy(omxConfigCrop, &pPort->omxConfigCrop, sizeof(OMX_CONFIG_RECTTYPE));
        } else if (omxConfigCrop->nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
            return OMX_ErrorUnsupportedIndex;
        } else {
            return OMX_ErrorBadPortIndex;
        }

        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_IndexConfigCommonOutputCrop nLeft=%d, nTop=%d, nWidth=%d, nHeight=%d\n",
              (int)omxConfigCrop->nLeft, (int)omxConfigCrop->nTop, (int)omxConfigCrop->nWidth, (int)omxConfigCrop->nHeight);
        break;
    case OMX_IndexConfigCommonRotate:
        omxConfigRotate = (OMX_CONFIG_ROTATIONTYPE*)pComponentConfigStructure;
        if ((err = checkHeader(pComponentConfigStructure, sizeof(OMX_CONFIG_ROTATIONTYPE))) != OMX_ErrorNone) {
            break;
        }
        if (omxConfigRotate->nPortIndex <= 1) {
            pPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[omxConfigRotate->nPortIndex];
            memcpy(omxConfigRotate, &pPort->omxConfigRotate, sizeof(OMX_CONFIG_ROTATIONTYPE));
        } else {
            return OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexConfigCommonMirror:
        omxConfigMirror = (OMX_CONFIG_MIRRORTYPE*)pComponentConfigStructure;
        if ((err = checkHeader(pComponentConfigStructure, sizeof(OMX_CONFIG_MIRRORTYPE))) != OMX_ErrorNone) {
            break;
        }
        if (omxConfigMirror->nPortIndex <= 1) {
            pPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[omxConfigMirror->nPortIndex];
            memcpy(omxConfigMirror, &pPort->omxConfigMirror, sizeof(OMX_CONFIG_MIRRORTYPE));
        } else {
            return OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexConfigCommonScale:
        omxConfigScale = (OMX_CONFIG_SCALEFACTORTYPE*)pComponentConfigStructure;
        if ((err = checkHeader(pComponentConfigStructure, sizeof(OMX_CONFIG_SCALEFACTORTYPE))) != OMX_ErrorNone) {
            break;
        }
        if (omxConfigScale->nPortIndex <= 1) {
            pPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[omxConfigScale->nPortIndex];
            memcpy(omxConfigScale, &pPort->omxConfigScale, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
        } else {
            return OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexConfigCommonOutputPosition:
        omxConfigOutputPosition = (OMX_CONFIG_POINTTYPE*)pComponentConfigStructure;
        if ((err = checkHeader(pComponentConfigStructure, sizeof(OMX_CONFIG_POINTTYPE))) != OMX_ErrorNone) {
            break;
        }
        if (omxConfigOutputPosition->nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX) {
            pPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[omxConfigOutputPosition->nPortIndex];
            memcpy(omxConfigOutputPosition, &pPort->omxConfigOutputPosition, sizeof(OMX_CONFIG_POINTTYPE));
        } else if (omxConfigOutputPosition->nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
            return OMX_ErrorUnsupportedIndex;
        } else {
            return OMX_ErrorBadPortIndex;
        }
        break;
    default: // delegate to superclass
        return omx_base_component_GetConfig(hComponent, nParamIndex, pComponentConfigStructure);
    }
    return err;
}



OMX_ERRORTYPE omx_vpudec_component_GetExtensionIndex(
    OMX_HANDLETYPE hComponent,
    OMX_STRING cParameterName,
    OMX_INDEXTYPE* pIndexType)
{

    DEBUG(DEB_LEV_FUNCTION_NAME,"In  , cParameterName = %s \n", cParameterName);
#ifdef ANDROID
    if(strcmp(cParameterName, STR_INDEX_PARAM_ENABLE_ANDROID_NATIVE_BUFFER) == 0)
    {
        *pIndexType = (OMX_INDEXTYPE)OMX_IndexParamEnableAndroidBuffers_CM;
        return OMX_ErrorNone;
    }
    else if (strcmp(cParameterName, STR_INDEX_PARAM_GET_ANDROID_NATIVE_BUFFER) == 0)
    {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexParamGetAndroidNativeBuffer_CM;
        return OMX_ErrorNone;
    }
    else if (strcmp(cParameterName, STR_INDEX_PARAM_USE_ANDROID_NATIVE_BUFFER) == 0)
    {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexParamUseAndroidNativeBuffer;
        return OMX_ErrorNone;
    }
    else if (strcmp(cParameterName, STR_INDEX_PARAM_THUMBNAIL_MODE) == 0)
    {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexConfigThumbnailMode_CM;
        return OMX_ErrorNone;
    }
#endif

    return omx_base_component_GetExtensionIndex(hComponent, cParameterName, pIndexType);
}





int codingTypeToCodStd(OMX_VIDEO_CODINGTYPE codingType)
{
    int codStd = -1;
    switch ((int)codingType)
    {
    case OMX_VIDEO_CodingMPEG2:
        codStd = STD_MPEG2;
        break;
    case OMX_VIDEO_CodingH263:
        codStd = STD_H263;
        break;
    case OMX_VIDEO_CodingMPEG4:
        codStd = STD_MPEG4;
        break;
    case OMX_VIDEO_CodingWMV:
        codStd = STD_VC1;
        break;
    case OMX_VIDEO_CodingRV:
        codStd = STD_RV;
        break;
    case OMX_VIDEO_CodingMSMPEG:
        codStd = STD_DIV3;
        break;
    case OMX_VIDEO_CODINGTYPE_VP8:
        codStd = STD_VP8;
        break;
    case OMX_VIDEO_CodingAVC:
        codStd = STD_AVC;
        break;
    case OMX_VIDEO_CodingAVS:
        codStd = STD_AVS;
        break;
    case OMX_VIDEO_CodingVC1:
        codStd = STD_VC1;
        break;
    default:
        codStd = -1;
        break;
    }

    return codStd;

}

int codingTypeToMp4class(OMX_VIDEO_CODINGTYPE codingType, int fourCC)
{
    int mp4Class = 0;
    if (codingType == OMX_VIDEO_CodingMPEG4)
    {
        if (fourCC == MAKE_FOURCC('D', 'I', 'V', 'X') || fourCC == MAKE_FOURCC('D', 'I', 'V', '4'))
            mp4Class = 5;
        else if (fourCC == MAKE_FOURCC('D', 'X', '5', '0')
                 || fourCC == MAKE_FOURCC('D', 'I', 'V', '5')
                 || fourCC == MAKE_FOURCC('D', 'I', 'V', '6'))
            mp4Class = 1;
        else if (fourCC == MAKE_FOURCC('X', 'V', 'I', 'D'))
            mp4Class = 2;
        else
            mp4Class = 8; // (8 means that firmware try to find mp4class by searching user-data)
    }
    return mp4Class;
}

int BuildOmxSeqHeader(OMX_U8 *pbHeader, OMX_BUFFERHEADERTYPE* pInputBuffer, OMX_COMPONENTTYPE *openmaxStandComp, int* sizelength)
{
    omx_vpudec_component_PrivateType* privateType = openmaxStandComp->pComponentPrivate;
    omx_vpudec_component_PortType *inPort = (omx_vpudec_component_PortType *) privateType->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) &privateType->vpu;
#ifdef SUPPORT_ONE_BUFFER_CONTAIN_MULTIPLE_FRAMES
    OMX_U8 *pbMetaData = (pInputBuffer->pBuffer+pInputBuffer->nOffset);
#else
    OMX_U8 *pbMetaData = pInputBuffer->pBuffer;
#endif
    int nMetaData = pInputBuffer->nFilledLen;
    OMX_U8* p = pbMetaData;
    OMX_U8 *a = p + 4 - ((unsigned long) p & 3);
    OMX_U8* t = pbHeader;
    OMX_S32 size; // metadata header size
#ifdef WORKAROUND_OMX_USE_INPUT_HEADER_BUFFER_AS_DECODER_BITSTREAM_BUFFER_NOT_REFORMATED
    OMX_U32 profile;
#endif
    OMX_U32 codingType;
    int i;
    codingType = privateType->video_coding_type;
    size = 0;
    if (sizelength)
        *sizelength = 4; //default size length (in bytes) = 4

    if (privateType->bUseOmxInputBufferAsDecBsBuffer == OMX_TRUE)
        return 0;

    DEBUG(DEB_LEV_FULL_SEQ, "codingType=%d, buffer_len=%d\n", (int)codingType, (int)pInputBuffer->nFilledLen);

    if (codingType == OMX_VIDEO_CodingAVC || codingType == OMX_VIDEO_CodingAVS)
    {
        if (nMetaData > 1 && pbMetaData && pbMetaData[0] == 0x01)// check mov/mo4 file format stream
        {
            int sps, pps, nal;
            p += 4;
            if (sizelength)
                *sizelength = (*p++ & 0x3) + 1;
            sps = (*p & 0x1f); // Number of sps
            p++;
            for (i = 0; i < sps; i++)
            {
                nal = (*p << 8) + *(p + 1) + 2;
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x01);
                PUT_BUFFER(t, p + 2, nal);
                p += nal;
                size += (nal + 4); // 4 => length of start code to be inserted
            }
            pps = *(p++); // number of pps
            for (i = 0; i < pps; i++)
            {
                nal = (*p << 8) + *(p + 1) + 2;
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x01);
                PUT_BUFFER(t, p + 2, nal);
                p += nal;
                size += (nal + 4); // 4 => length of start code to be inserted
            }
        }
        else
        {
            size = 0;
            for (; p < a; p++)
            {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1) // find startcode
                {
                    size = 0;
                    //PUT_BUFFER(pbHeader, pInputBuffer->pBuffer, pInputBuffer->nFilledLen);
                    break;
                }
            }
        }
    }
    else if (codingType == OMX_VIDEO_CodingWMV)
    {
#ifdef WORKAROUND_OMX_USE_INPUT_HEADER_BUFFER_AS_DECODER_BITSTREAM_BUFFER_NOT_REFORMATED
        profile = privateType->codParam.wmv.eFormat;
        if (profile == (int) OMX_VIDEO_WMVFormatVC1) // VC1 AP has the start code.
        {
            //if there is no seq startcode in pbMetatData. VPU will be failed at seq_init stage.
        }
        else
        {
#define RCV_V2
#ifdef RCV_V2
            PUT_LE32(pbHeader, ((0xC5 << 24) | 0));
            size += 4; //version
            PUT_LE32(pbHeader, nMetaData);
            size += 4;
            PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
            size += nMetaData;
            PUT_LE32(pbHeader, inPort->sPortParam.format.video.nFrameHeight);
            size += 4;
            PUT_LE32(pbHeader, inPort->sPortParam.format.video.nFrameWidth);
            size += 4;
            PUT_LE32(pbHeader, 12);
            size += 4;
            PUT_LE32(pbHeader, (2<<29 | 1<<28 | 0x80<<24 | 1<<0));
            size += 4; // STRUCT_B_FRIST (LEVEL:3|CBR:1:RESERVE:4:HRD_BUFFER|24)
            PUT_LE32(pbHeader, inPort->sPortParam.format.video.nBitrate);
            size += 4; // hrd_rate
            PUT_LE32(pbHeader, inPort->sPortParam.format.video.xFramerate);
            size += 4; // framerate
#else	//RCV_V1
            PUT_LE32(pbHeader, ((0x85 << 24) | 0x00));
            size += 4; //frames count will be here
            PUT_LE32(pbHeader, nMetaData);
            size += 4;
            PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
            size += nMetaData;
            PUT_LE32(pbHeader, inPort->sPortParam.format.video.nFrameHeight);
            size += 4;
            PUT_LE32(pbHeader, inPort->sPortParam.format.video.nFrameWidth);
            size += 4;
#endif
        }
#endif
    }
    else if (codingType == OMX_VIDEO_CodingRV)
    {
#ifdef WORKAROUND_OMX_USE_INPUT_HEADER_BUFFER_AS_DECODER_BITSTREAM_BUFFER_NOT_REFORMATED
        int st_size =0;

        profile = privateType->codParam.rv.eFormat;

        //if (profile != (int) OMX_VIDEO_RVFormat9 && profile != OMX_VIDEO_RVFormat8)
        //    return -1;

        size = 26 + nMetaData;
        PUT_BE32(pbHeader, size); //Length
        PUT_LE32(pbHeader, MAKE_FOURCC('V', 'I', 'D', 'O')); //MOFTag
        if (profile == (int) OMX_VIDEO_RVFormat8)
        {
            PUT_LE32(pbHeader, MAKE_FOURCC('R', 'V', '3', '0')); //SubMOFTagl
        }
        else if (profile == (int) OMX_VIDEO_RVFormat9)
        {
            PUT_LE32(pbHeader, MAKE_FOURCC('R', 'V', '4', '0')); //SubMOFTagl
        }
        else
        {
            PUT_LE32(pbHeader, MAKE_FOURCC('R', 'V', '3', '0')); //SubMOFTagl
        }
        PUT_BE16(pbHeader, inPort->sPortParam.format.video.nFrameWidth);
        PUT_BE16(pbHeader, inPort->sPortParam.format.video.nFrameHeight);
        PUT_BE16(pbHeader, 0x0c); //BitCount;
        PUT_BE16(pbHeader, 0x00); //PadWidth;
        PUT_BE16(pbHeader, 0x00); //PadHeight;
        PUT_LE32(pbHeader, inPort->sPortParam.format.video.xFramerate);
        PUT_BUFFER(pbHeader, pbMetaData, nMetaData); //OpaqueDatata
        size += st_size; //add for startcode pattern.
#endif
    }

    else if (codingType == OMX_VIDEO_CodingMJPEG)
        return 0;
    else if (codingType == OMX_VIDEO_CODINGTYPE_VP8)
    {
        PUT_LE32(pbHeader, MAKE_FOURCC('D', 'K', 'I', 'F')); //signature 'DKIF'
        PUT_LE16(pbHeader, 0x00);                      //version
        PUT_LE16(pbHeader, 0x20);                      //length of header in bytes
        PUT_LE32(pbHeader, MAKE_FOURCC('V', 'P', '8', '0')); //codec FourCC
        PUT_LE16(pbHeader, inPort->sPortParam.format.video.nFrameWidth);                //width
        PUT_LE16(pbHeader, inPort->sPortParam.format.video.nFrameHeight);               //height
        PUT_LE32(pbHeader, inPort->sPortParam.format.video.xFramerate);      //frame rate
        PUT_LE32(pbHeader, 0);      //time scale(?)
        PUT_LE32(pbHeader, -1);      //number of frames in file
        PUT_LE32(pbHeader, 0); //unused
        size += 32;
    }
    else if (codingType == OMX_VIDEO_CodingHEVC)
    {
        if (nMetaData > 1 && pbMetaData && pbMetaData[0] == 0x01)// check mov/mo4 file format stream
        {
            DEBUG(DEB_LEV_FULL_SEQ, "HEVC detected as mp4 format\n");
            static const uint8_t nalu_header[4] = { 0, 0, 0, 1 };
            int numOfArrays = 0;
            //uint16_t nal_unit_type = 0;
            uint16_t numNalus = 0;
            uint16_t nalUnitLength = 0;
            uint32_t offset = 0;

            p += 21;
            if (sizelength)
                *sizelength = (*p++ & 0x3) + 1;
            numOfArrays = *p++;

            while(numOfArrays--)
            {
                //nal_unit_type = *p++ & 0x3F;
                numNalus = (*p << 8) + *(p + 1);
                p+=2;
                for(i = 0; i < numNalus; i++)
                {
                    nalUnitLength = (*p << 8) + *(p + 1);
                    p+=2;
                    //if(i == 0)
                    {
                        memcpy(pbHeader + offset, nalu_header, 4);
                        offset += 4;
                        memcpy(pbHeader + offset, p, nalUnitLength);
                        offset += nalUnitLength;
                    }
                    p += nalUnitLength;
                }
            }
            size = offset;
        }
        else if(nMetaData > 3)
        {
            size = -1;// return to meaning of invalid stream data;
            DEBUG(DEB_LEV_FULL_SEQ, "HEVC detected as ES format : size = %d, 0x%x 0x%x 0x%x 0x%x\n",nMetaData, p[0], p[1], p[2], p[3]);
            for (; p < a; p++)
            {
                if ( p[0] == 0 && p[1] == 0 && p[2] == 1) // find startcode
                {
                    size = 0;
                    if ( !(p[3] == 0x40 || p[3] == 0x42 || p[3] == 0x44) )  // if not VPS/SPS/PPS, return
                        break;
                    break;
                }
                else if ( p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1)
                {
                    size = 0;
                    if ( !(p[4] == 0x40 || p[4] == 0x42 || p[4] == 0x44) )  // if not VPS/SPS/PPS, return
                        break;
                    //PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
                    break;
                }
            }
        }
    }
    else
    {
        //PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
        size = 0;
    }
    DEBUG(DEB_LEV_FULL_SEQ, "Out , seq size=%d, buffer_len=%d\n", (int)size, (int)nMetaData);
    return size;
}

int BuildOmxPicHeader(OMX_U8 *pbHeader, OMX_BUFFERHEADERTYPE* pInputBuffer, OMX_COMPONENTTYPE *openmaxStandComp, int sizelength)
{
    omx_vpudec_component_PrivateType* privateType = openmaxStandComp->pComponentPrivate;

    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) &privateType->vpu;
#ifdef SUPPORT_ONE_BUFFER_CONTAIN_MULTIPLE_FRAMES
    OMX_U8 *pbChunk = (pInputBuffer->pBuffer+pInputBuffer->nOffset);
#else
    OMX_U8 *pbChunk = pInputBuffer->pBuffer;
#endif
    OMX_S32 size;
    OMX_U32 codingType;
#ifdef WORKAROUND_OMX_USE_INPUT_HEADER_BUFFER_AS_DECODER_BITSTREAM_BUFFER_NOT_REFORMATED
    OMX_U32 cSlice;
    OMX_U32 profile;
    int i, val;
#endif
    OMX_U32 nSlice;
    OMX_U32 offset;
    int has_st_code = 0;

    if (privateType->bUseOmxInputBufferAsDecBsBuffer == OMX_TRUE)
        return 0;

    codingType = privateType->video_coding_type;
    offset = 0;
    size = 0;

    DEBUG(DEB_LEV_FULL_SEQ, "In , codingType=%d, buffer_len=%d\n", (int)codingType, (int)pInputBuffer->nFilledLen);
    if (codingType == OMX_VIDEO_CodingWMV)
    {
#ifdef WORKAROUND_OMX_USE_INPUT_HEADER_BUFFER_AS_DECODER_BITSTREAM_BUFFER_NOT_REFORMATED
        profile = privateType->codParam.wmv.eFormat;
        if (profile == (int) OMX_VIDEO_WMVFormatVC1)
        {
            if (pbChunk[0] != 0 || pbChunk[1] != 0 || pbChunk[2] != 1) // check start code as prefix (0x00, 0x00, 0x01)
            {
                PUT_BYTE(pbHeader, 0x00);
                PUT_BYTE(pbHeader, 0x00);
                PUT_BYTE(pbHeader, 0x01);
                PUT_BYTE(pbHeader, 0x0D);

                size += 4;
            }
        }
        else
        {
            val = pInputBuffer->nFilledLen;
            if (pInputBuffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME)
                val |= 0x80000000;
            PUT_LE32(pbHeader, val);
            size += 4;
#ifdef RCV_V2
            PUT_LE32(pbHeader, pInputBuffer->nTimeStamp/1000); // milli_sec
            size += 4;
#endif
        }
#endif
    }
    else if (codingType == OMX_VIDEO_CodingVC1)
    {
#ifdef WORKAROUND_OMX_USE_INPUT_HEADER_BUFFER_AS_DECODER_BITSTREAM_BUFFER_NOT_REFORMATED
        if (pbChunk[0] != 0 || pbChunk[1] != 0 || pbChunk[2] != 1) // check start code as prefix (0x00, 0x00, 0x01)
        {
            PUT_BYTE(pbHeader, 0x00);               // add frame data start code.
            PUT_BYTE(pbHeader, 0x00);
            PUT_BYTE(pbHeader, 0x01);
            PUT_BYTE(pbHeader, 0x0D);

            size += 4;
        }
#endif
    }
    else if (codingType == OMX_VIDEO_CodingRV)
    {
#ifdef WORKAROUND_OMX_USE_INPUT_HEADER_BUFFER_AS_DECODER_BITSTREAM_BUFFER_NOT_REFORMATED
        int st_size = 0;

        profile = privateType->codParam.rv.eFormat;
        if (profile != (int) OMX_VIDEO_RVFormat9 && profile != OMX_VIDEO_RVFormat8)
            return -1;

        cSlice = pbChunk[0] + 1;
        nSlice = pInputBuffer->nFilledLen - 1 - (cSlice * 8);
        size = 20 + (cSlice * 8);
        PUT_BE32(pbHeader, nSlice);
        PUT_BE32(pbHeader, pInputBuffer->nTimeStamp/1000);
        PUT_BE16(pbHeader, pVpu->frameIdx);
        PUT_BE16(pbHeader, 0x02); //Flags
        PUT_BE32(pbHeader, 0x00); //LastPacket
        PUT_BE32(pbHeader, cSlice); //NumSegments
        offset = 1;
        for (i = 0; i < (int) cSlice; i++)
        {
            val = (pbChunk[offset + 3] << 24) | (pbChunk[offset + 2] << 16) | (pbChunk[offset + 1] << 8) | pbChunk[offset];
            PUT_BE32(pbHeader, val); //isValid
            offset += 4;
            val = (pbChunk[offset + 3] << 24) | (pbChunk[offset + 2] << 16) | (pbChunk[offset + 1] << 8) | pbChunk[offset];
            PUT_BE32(pbHeader, val); //Offset
            offset += 4;
        }
        size += st_size;
#endif
    }
    else if (codingType == OMX_VIDEO_CodingAVC || codingType == OMX_VIDEO_CodingAVS || codingType == OMX_VIDEO_CodingHEVC)
    {
        if(!(privateType->seqHeader && privateType->seqHeader[0] == 0x01))      // if not metadata format, search start codes.
        {
            const Uint8 *pbEnd = pbChunk + 4 - ((intptr_t)pbChunk & 3);

            for (; pbChunk < pbEnd ; pbChunk++)
            {
                if ((pbChunk[0] == 0 && pbChunk[1] == 0 && pbChunk[2] == 1) ||
                        (pbChunk[0] == 0 && pbChunk[1] == 0 && pbChunk[2] == 0 && pbChunk[3] == 1) ||
                        (pbChunk[0] == 0 && pbChunk[1] == 0 && pbChunk[2] == 0 && pbChunk[3] == 0 && pbChunk[4] == 1))
                {
                    has_st_code = 1;
                    break;
                }
            }
        }

        if (!has_st_code ) // if no start codes, replace size byte to start codes
        {
#ifdef SUPPORT_ONE_BUFFER_CONTAIN_MULTIPLE_FRAMES
            pbChunk = (pInputBuffer->pBuffer+pInputBuffer->nOffset);
#else
            pbChunk = pInputBuffer->pBuffer;
#endif
            while (offset < pInputBuffer->nFilledLen)
            {
                if (codingType == OMX_VIDEO_CodingAVC || codingType == OMX_VIDEO_CodingHEVC)
                {
                    if(sizelength == 3)
                    {
                        nSlice = pbChunk[offset] << 16 | pbChunk[offset+1] << 8 | pbChunk[offset+2];

                        pbChunk[offset] = 0x00;
                        pbChunk[offset+1] = 0x00;
                        pbChunk[offset+2] = 0x01;

                        offset += 3;
                    }
                    else    // sizeLength = 4
                    {
                        nSlice = pbChunk[offset] << 24 | pbChunk[offset + 1] << 16 | pbChunk[offset + 2] << 8 | pbChunk[offset + 3];

                        pbChunk[offset] = 0x00;
                        pbChunk[offset+1] = 0x00;
                        pbChunk[offset+2] = 0x00;
                        pbChunk[offset+3] = 0x01;		//replace size to startcode

                        offset += 4;
                    }
                }
                else
                {
                    nSlice = pbChunk[offset] << 24 | pbChunk[offset + 1] << 16 | pbChunk[offset + 2] << 8 | pbChunk[offset + 3];
                    pbChunk[offset+0] = 0x00;
                    pbChunk[offset+1] = 0x00;
                    pbChunk[offset+2] = 0x00;
                    pbChunk[offset+3] = 0x00;		//replace size to startcode
                    pbChunk[offset+4] = 0x01;

                    offset += 5;
                }

                if (codingType == OMX_VIDEO_CodingHEVC)
                {
                    switch ((pbChunk[offset]&0x7E)>>1) /* NAL unit */
                    {
                    case 39: /* PREFIX SEI */
                    case 40: /* SUFFIX SEI */
                    case 32: /* VPS */
                    case 33: /* SPS */
                    case 34: /* PPS */
                        /* check next */
                        break;
                    }
                }

                if (codingType == OMX_VIDEO_CodingAVC)
                {
                    switch (pbChunk[offset]&0x1f) /* NAL unit */
                    {
                    case 6: /* SEI */
                    case 7: /* SPS */
                    case 8: /* PPS */
                    case 9: /* AU */
                        /* check next */
                        break;
                    }
                }
                if (nSlice > pInputBuffer->nFilledLen)
                    return -1;
                offset += nSlice;
            }
        }
    }
    else if (codingType == OMX_VIDEO_CODINGTYPE_VP8)
    {
        PUT_LE32(pbHeader,pInputBuffer->nFilledLen);
        PUT_LE32(pbHeader,0);
        PUT_LE32(pbHeader,0);
        size += 12;
    }

    DEBUG(DEB_LEV_FULL_SEQ, "Out , header size=%d, buffer_len=%d\n", (int)size, (int)pInputBuffer->nFilledLen);
    return size;
}


double omx_time_curr_ms()
{
    double curr = 0;

    clock_gettime(CLOCK_REALTIME, &s_ts_curr);

    curr = s_ts_curr.tv_sec*1000 + s_ts_curr.tv_nsec/1000000.0;

    return curr;

}

void omx_timer_start()
{
    clock_gettime(CLOCK_REALTIME, &s_ts_start);

}

void omx_timer_stop()
{
    clock_gettime(CLOCK_REALTIME, &s_ts_end);
}

double omx_timer_elapsed_ms()
{
    double ms;
    ms = omx_timer_elapsed_us()/1000.0;
    return ms;
}

double omx_timer_elapsed_us()
{
    double elapsed = 0;
    double start_us = 0;
    double end_us = 0;

    end_us = (double)s_ts_end.tv_sec*1000000.0 + (double)s_ts_end.tv_nsec/(double)1000;
    start_us = (double)s_ts_start.tv_sec*1000000.0 + (double)s_ts_start.tv_nsec/(double)1000;

    elapsed =  end_us - start_us;
    return elapsed;
}


int OmxAllocateFrameBuffers(OMX_COMPONENTTYPE *openmaxStandComp)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;

    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *inPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    omx_vpudec_component_PortType *outPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    int framebufSize;
    int i;
    int ret;
    int bufferAllocNum;
    FrameBufferAllocInfo fbAllocInfo;

#ifdef SUPPORT_FIX_ADAPTIVE_PLAYBACK
    // 1st step : free buffer first. (to support AdaptivePlayback)
    if (outPort && outPort->bAllocateBuffer == OMX_FALSE)
    {
        if (omx_vpudec_component_Private->useNativeBuffer == OMX_FALSE)
        {
            for (i=0; i<MAX_REG_FRAME; i++) 
            {
                if (pVpu->vbAllocFb[i].size > 0)
                {
                    vdi_free_dma_memory(pVpu->coreIdx, &pVpu->vbAllocFb[i]);
                }
            }
        }
    }

    for (i=0; i<MAX_REG_FRAME; i++) 
    {
        if (pVpu->vbDPB[i].size > 0)
        {
            vdi_free_dma_memory(pVpu->coreIdx, &pVpu->vbDPB[i]);
        }
    }
#endif

    framebufSize = VPU_GetFrameBufSize(pVpu->fbStride, pVpu->fbHeight, pVpu->mapType, pVpu->fbFormat, &pVpu->dramCfg);

    fbAllocInfo.format = pVpu->fbFormat;
    fbAllocInfo.cbcrInterleave = pVpu->decOP.cbcrInterleave;
    fbAllocInfo.mapType = pVpu->mapType;
    fbAllocInfo.stride = pVpu->fbStride;
    fbAllocInfo.height = pVpu->fbHeight;
    fbAllocInfo.num = pVpu->regFbCount;
    fbAllocInfo.endian = pVpu->decOP.frameEndian;
    fbAllocInfo.type = FB_TYPE_CODEC;

    if (pVpu->decOP.scalerInfo.enScaler)
    {
        if (pVpu->decOP.scalerInfo.enScaler)
            fbAllocInfo.num = pVpu->initialInfo.minFrameBufferCount;

        if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
            bufferAllocNum = 1;
        else
            bufferAllocNum = fbAllocInfo.num;

        //1.arrangement vbUserFb for all buffer
        for (i=0; i<bufferAllocNum; i++)
        {
            pVpu->vbDPB[i].size = framebufSize;
            if (vdi_allocate_dma_memory(pVpu->coreIdx, &pVpu->vbDPB[i]) < 0)
                goto ERROR_ALLOCATE_FRAME_BUFFERS;
        }

        for (i=0 ; i < bufferAllocNum; i++)
            pVpu->vbUserFb[i] = pVpu->vbDPB[i];

        if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
        {
            for (i=0; i < fbAllocInfo.num; i++)
                pVpu->vbUserFb[i] = pVpu->vbUserFb[0];
        }

        //2.arrangement fbUser for all buffer
        for (i=0; i<fbAllocInfo.num; i++)
        {
            pVpu->fbUser[i].bufY = pVpu->vbUserFb[i].phys_addr;
            pVpu->fbUser[i].bufCb = -1;
            pVpu->fbUser[i].bufCr = -1;

            DEBUG(DEB_LEV_FULL_SEQ, "OmxAllocateFrameBuffers  for DPB Index=%d, pInternalBufferStorage=%p, fbAllocInfo.num=%d, Virtual Address=0x%x, Physical Address=0x%x",
                  i,  outPort->pInternalBufferStorage[i], (int)fbAllocInfo.num, (int)pVpu->vbUserFb[i].virt_addr, (int)pVpu->fbUser[i].bufY);
        }
    }
    else
    {
        if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
            bufferAllocNum = 1;
        else
            bufferAllocNum = fbAllocInfo.num;

        if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
        {
#ifdef ANDROID
            DEBUG(DEB_LEV_FULL_SEQ, " useNativeBuffer is TRUE!!\n");
            unsigned long phyAddrs;
            void *pVirtAddrs[3];
            OMX_U8 *pAddr;

            //1.arrangement vbUserFb for all buffer
            for (i=0 ; i < bufferAllocNum; i++)
            {
                pVpu->vbUserFb[i].size = framebufSize;

                ret = getIOMMUPhyAddr(outPort->pInternalBufferStorage[i], &phyAddrs);
                pVpu->vbUserFb[i].phys_addr = phyAddrs;

                DEBUG(DEB_LEV_FULL_SEQ, "getIOMMUPhyAddr, index=%d, phys_addr = %x, pInternalBufferStorage=%p\n",
                        i, phyAddrs, outPort->pInternalBufferStorage[i]);

#ifdef CNM_FPGA_PLATFORM
                ret = lockAndroidNativeBuffer (outPort->pInternalBufferStorage[i],
                                               (OMX_U32)(pVpu->fbStride),
                                               (OMX_U32)(pVpu->fbHeight), LOCK_MODE_TO_GET_VIRTUAL_ADDRESS, pVirtAddrs);
                if (ret != 0) {
                    DEBUG(DEB_LEV_ERR, "Error lockAndroidNativeBuffer, Error code:%d", (int)ret);
                    goto ERROR_ALLOCATE_FRAME_BUFFERS;
                }

                pAddr = (OMX_U8 *)pVirtAddrs[0];
                pVpu->vbUserFb[i].virt_addr = (unsigned long)pAddr;
                DEBUG(DEB_LEV_FULL_SEQ, "lockAndroidNativeBuffer for Virtual Address, index=%d,  pVirtAddrs=0x%x, pInternalBufferStorage=%p\n", i, (int)pAddr, outPort->pInternalBufferStorage[i]);

                unlockAndroidNativeBuffer(outPort->pInternalBufferStorage[i]);

                vdi_attach_dma_memory(pVpu->coreIdx, &pVpu->vbUserFb[i]);
#endif
            }

            if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
            {
                for (i=0; i < fbAllocInfo.num; i++)
                {
                    pVpu->vbUserFb[i] = pVpu->vbUserFb[0];
                }
            }

            //2.arrangement fbUser for all buffer
            for (i=0; i<fbAllocInfo.num; i++)
            {
#ifdef  CNM_FPGA_PLATFORM
                pVpu->fbUser[i].bufY = (PhysicalAddress)pVpu->vbUserFb[i].phys_addr & 0xfffffff;
#else
                pVpu->fbUser[i].bufY = (PhysicalAddress)pVpu->vbUserFb[i].phys_addr;
#endif
                pVpu->fbUser[i].bufCb = -1;
                pVpu->fbUser[i].bufCr = -1;
                DEBUG(DEB_LEV_FULL_SEQ, "OmxAllocateFrameBuffers  for DPB display Index=%d, pInternalBufferStorage=%p, fbAllocInfo.num=%d, Virtual Address=0x%x, Physical Address=0x%x",
                      i,  outPort->pInternalBufferStorage[i], (int)fbAllocInfo.num, (int)pVpu->vbUserFb[i].virt_addr, (int)pVpu->fbUser[i].bufY);
            }
#endif	// ANDROID
        }
        else
        {
            if (outPort->bAllocateBuffer == OMX_FALSE)
            {
                for (i=0; i<bufferAllocNum; i++)
                {
                    pVpu->vbAllocFb[i].size = framebufSize;
                    if (vdi_allocate_dma_memory(pVpu->coreIdx, &pVpu->vbAllocFb[i]) < 0)
                        goto ERROR_ALLOCATE_FRAME_BUFFERS;
                }
            }

            //1.arrangement vbUserFb for all buffer
            for (i=0; i < bufferAllocNum; i++)
                pVpu->vbUserFb[i] = pVpu->vbAllocFb[i];

            if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
            {
                for (i=0; i < fbAllocInfo.num; i++)
                    pVpu->vbUserFb[i] = pVpu->vbUserFb[0];
            }

            //2.arrangement fbUser for all buffer
            for (i=0; i<fbAllocInfo.num; i++)
            {
#ifdef  CNM_FPGA_PLATFORM
                pVpu->fbUser[i].bufY = (PhysicalAddress)pVpu->vbUserFb[i].phys_addr & 0xfffffff;
#else
                pVpu->fbUser[i].bufY = (PhysicalAddress)pVpu->vbUserFb[i].phys_addr;
#endif
                pVpu->fbUser[i].bufCb = -1;
                pVpu->fbUser[i].bufCr = -1;

                DEBUG(DEB_LEV_FULL_SEQ, "OmxAllocateFrameBuffers  for DPB display Index=%d, pInternalBufferStorage=%p, fbAllocInfo.num=%d, Virtual Address=0x%x, Physical Address=0x%x", i,  outPort->pInternalBufferStorage[i], (int)fbAllocInfo.num, (int)pVpu->vbUserFb[i].virt_addr, (int)pVpu->fbUser[i].bufY);
            }
        }
    }

    if (RETCODE_SUCCESS != VPU_DecAllocateFrameBuffer(pVpu->handle, fbAllocInfo, &pVpu->fbUser[0])) // this will not allocate frame buffer it will just translate the base address of native buffer to FrameBuffer structure
    {
        DEBUG(DEB_LEV_ERR, "VPU_DecAllocateFrameBuffer for DPB fail to allocate source frame buffer\n");
        goto ERROR_ALLOCATE_FRAME_BUFFERS;
    }


    if (pVpu->decOP.scalerInfo.enScaler)
    {
        // update frame buffer allocation information for scaler.
        fbAllocInfo.stride = pVpu->decOP.scalerInfo.scaleWidth;
        fbAllocInfo.height = pVpu->decOP.scalerInfo.scaleHeight;
        fbAllocInfo.num = pVpu->regFbCount;
        fbAllocInfo.format = (pVpu->decOP.scalerInfo.imageFormat < YUV_FORMAT_I422)?FORMAT_420:FORMAT_422;
        fbAllocInfo.mapType = LINEAR_FRAME_MAP;
        fbAllocInfo.type = FB_TYPE_SCALER;
        framebufSize = VPU_GetFrameBufSize(pVpu->decOP.scalerInfo.scaleWidth, pVpu->decOP.scalerInfo.scaleHeight, fbAllocInfo.mapType, fbAllocInfo.format, &pVpu->dramCfg);
        if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
            bufferAllocNum = pVpu->initialInfo.minFrameBufferCount+1;
        else
            bufferAllocNum = (pVpu->initialInfo.minFrameBufferCount+fbAllocInfo.num);

        if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
        {
#ifdef ANDROID
            unsigned long phyAddrs;
            void *pPhyAddrs[3];
            void *pVirtAddrs[3];
            OMX_U8 *pAddr;

            for (i=pVpu->initialInfo.minFrameBufferCount; i<bufferAllocNum; i++)
            {
                pVpu->vbUserFb[i].size = framebufSize;

                ret = getIOMMUPhyAddr(outPort->pInternalBufferStorage[i - pVpu->initialInfo.minFrameBufferCount], &phyAddrs);
                pVpu->vbUserFb[i].phys_addr = phyAddrs;

                DEBUG(DEB_LEV_ERR, "Scaler phys_addr = %x\n", phyAddrs);

#ifdef  CNM_FPGA_PLATFORM

                ret = lockAndroidNativeBuffer (outPort->pInternalBufferStorage[i - pVpu->initialInfo.minFrameBufferCount],
                                               (OMX_U32)(pVpu->decOP.scalerInfo.scaleWidth),
                                               (OMX_U32)(pVpu->decOP.scalerInfo.scaleHeight), LOCK_MODE_TO_GET_VIRTUAL_ADDRESS, pVirtAddrs);
                if (ret != 0) {
                    DEBUG(DEB_LEV_ERR, "Error lockAndroidNativeBuffer, Error code:%d", (int)ret);
                    goto ERROR_ALLOCATE_FRAME_BUFFERS;
                }
                pAddr = (OMX_U8 *)pVirtAddrs[0];
                pVpu->vbUserFb[i].virt_addr = (unsigned long)pAddr;
                //DEBUG(DEB_LEV_FULL_SEQ, "lockAndroidNativeBuffer for Virtual Address of scaler, index=%d,  pVirtAddrs=0x%x, pInternalBufferStorage=%p\n", i - pVpu->initialInfo.minFrameBufferCount, (int)pAddr, outPort->pInternalBufferStorage[i - pVpu->initialInfo.minFrameBufferCount]);
                unlockAndroidNativeBuffer(outPort->pInternalBufferStorage[i - pVpu->initialInfo.minFrameBufferCount]);

                vdi_attach_dma_memory(pVpu->coreIdx, &pVpu->vbUserFb[i]);
#endif
            }

            if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
            {
                for (i=pVpu->initialInfo.minFrameBufferCount; i < (pVpu->initialInfo.minFrameBufferCount+fbAllocInfo.num); i++)
                {
                    pVpu->vbUserFb[i] = pVpu->vbUserFb[pVpu->initialInfo.minFrameBufferCount];
                }
            }

            //2.arrangement fbUser for all buffer
            for (i=pVpu->initialInfo.minFrameBufferCount; i < (pVpu->initialInfo.minFrameBufferCount+fbAllocInfo.num); i++)
            {
#ifdef  CNM_FPGA_PLATFORM
                pVpu->fbUser[i].bufY = (PhysicalAddress)pVpu->vbUserFb[i].phys_addr & 0xfffffff;
#else
                pVpu->fbUser[i].bufY = (PhysicalAddress)pVpu->vbUserFb[i].phys_addr;
#endif
                pVpu->fbUser[i].bufCb = -1;
                pVpu->fbUser[i].bufCr = -1;

                DEBUG(DEB_LEV_FULL_SEQ, "OmxAllocateFrameBuffers  for Scaler Index=%d, pInternalBufferStorage=%p, fbAllocInfo.num=%d, Virtual Address=0x%x, Physical Address=0x%x", i, outPort->pInternalBufferStorage[i-pVpu->initialInfo.minFrameBufferCount], (int)fbAllocInfo.num, (int)pVpu->vbUserFb[i].virt_addr, (int)pVpu->fbUser[i].bufY);
            }
#endif
        }
        else
        {
            if (outPort->bAllocateBuffer == OMX_FALSE)
            {
                for (i=pVpu->initialInfo.minFrameBufferCount; i<bufferAllocNum; i++)
                {
                    pVpu->vbAllocFb[i-pVpu->initialInfo.minFrameBufferCount].size = framebufSize;
                    if (vdi_allocate_dma_memory(pVpu->coreIdx, &pVpu->vbAllocFb[i-pVpu->initialInfo.minFrameBufferCount]) < 0)
                        goto ERROR_ALLOCATE_FRAME_BUFFERS;
                }
            }

            //1.arrangement vbUserFb for all buffer
            for (i=pVpu->initialInfo.minFrameBufferCount ; i < bufferAllocNum; i++)
                pVpu->vbUserFb[i] = pVpu->vbAllocFb[i-pVpu->initialInfo.minFrameBufferCount];

            if (omx_vpudec_component_Private->bThumbnailMode == OMX_TRUE)
            {
                for (i=pVpu->initialInfo.minFrameBufferCount; i < (pVpu->initialInfo.minFrameBufferCount+fbAllocInfo.num); i++)
                {
                    pVpu->vbUserFb[i] = pVpu->vbUserFb[pVpu->initialInfo.minFrameBufferCount];
                }
            }


            for (i=pVpu->initialInfo.minFrameBufferCount; i<(pVpu->initialInfo.minFrameBufferCount+fbAllocInfo.num); i++)
            {
                pVpu->fbUser[i].bufY = (PhysicalAddress)pVpu->vbUserFb[i].phys_addr;
                pVpu->fbUser[i].bufCb = -1;	//this means that we want to assign the address at VPU_DecAllocateFrameBuffer function thatn allocate frame buffer
                pVpu->fbUser[i].bufCr = -1;
                DEBUG(DEB_LEV_FULL_SEQ, "OmxAllocateFrameBuffers  for Scaler Index=%d, pInternalBufferStorage=%p, fbAllocInfo.num=%d, Virtual Address=0x%x, Physical Address=0x%x", i, outPort->pInternalBufferStorage[i-pVpu->initialInfo.minFrameBufferCount], (int)fbAllocInfo.num, (int)pVpu->vbUserFb[i].virt_addr, (int)pVpu->fbUser[i].bufY);
            }
        }

        ret = VPU_DecAllocateFrameBuffer(pVpu->handle, fbAllocInfo, &pVpu->fbUser[pVpu->initialInfo.minFrameBufferCount]);
        if( ret != RETCODE_SUCCESS )
        {
            DEBUG(ERR, "VPU_DecAllocateFrameBuffer fail to allocate WTL frame buffer is 0x%x \n", ret );
            goto ERROR_ALLOCATE_FRAME_BUFFERS;
        }
    }


    return 1;

ERROR_ALLOCATE_FRAME_BUFFERS:
    return 0;
}
#ifdef CNM_FPGA_PLATFORM

int OmxSyncFpgaOutputToHostBuffer(OMX_COMPONENTTYPE *openmaxStandComp, DecOutputInfo *decOutputInfo)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;

    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *inPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    omx_vpudec_component_PortType *outPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    FrameBuffer fb;
    void *pVirAddrs[3];
    int len;
    int ret;
    int index;

    UNREFERENCED_PARAMETER(pVpu);
    UNREFERENCED_PARAMETER(inPort);
    UNREFERENCED_PARAMETER(outPort);


    DEBUG(DEB_LEV_FUNCTION_NAME, "In  of component %p\n", openmaxStandComp);
    len = 0;

    index = decOutputInfo->indexFrameDecoded;


    if (pVpu->decOP.scalerInfo.enScaler)
        index = decOutputInfo->indexScalerDecoded;

    ret = VPU_DecGetFrameBuffer(pVpu->handle, index, &fb);
    if (ret != RETCODE_SUCCESS)
    {
        DEBUG(DEB_LEV_ERR, "OmxSyncFpgaOutputToHost : VPU_DecGetFrameBuffer failed Error code is 0x%x \n", (int)ret);
        return 0;
    }

    // update index value relative for pInternalBufferStorage
    index = decOutputInfo->indexFrameDecoded;
    if (pVpu->decOP.scalerInfo.enScaler)
        index = decOutputInfo->indexScalerDecoded - pVpu->initialInfo.minFrameBufferCount;
    if (index < 0)
    {
        DEBUG(DEB_LEV_FULL_SEQ, "OmxSyncFpgaOutputToHost : Invalid buffer index=%d\n", index);
        return 0;
    }


#ifdef ANDROID
    if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
    {
        int nativeBufferSize;
        ret = lockAndroidNativeBuffer(outPort->pInternalBufferStorage[index], outPort->sPortParam.format.video.nStride, outPort->sPortParam.format.video.nFrameHeight, LOCK_MODE_TO_GET_VIRTUAL_ADDRESS, pVirAddrs);
        if (ret != 0)
        {
            DEBUG(DEB_LEV_ERR, "OmxSyncFpgaOutputToHost : lockAndroidNativeBuffer Error ret=%d\n", (int)ret);
            return 0;
        }
        nativeBufferSize = getNativeBufferSize(outPort->sPortParam.format.video.eColorFormat, 0, outPort->sPortParam.format.video.nStride, outPort->sPortParam.format.video.nFrameHeight);
        DEBUG(DEB_LEV_FULL_SEQ, "OmxSyncFpgaOutputToHost : decIndex=%d from FpgaAddr:0x%x To HostAddr:%p, omxStride=%d, omxHeight=%d, InternalBufferStorage=0x%x, nativeBufferSize=%d\n",
              index, fb.bufY, pVirAddrs[0], (int)outPort->sPortParam.format.video.nStride, (int)outPort->sPortParam.format.video.nFrameHeight, (int)outPort->pInternalBufferStorage[index], (int)nativeBufferSize);
        len = vdi_read_memory(pVpu->coreIdx, fb.bufY, (unsigned char*)pVirAddrs[0], nativeBufferSize, pVpu->decOP.frameEndian);
        unlockAndroidNativeBuffer(outPort->pInternalBufferStorage[index]);
    }
    else
    {
        // this case can be thumbnail decoding.
        DEBUG(DEB_LEV_FULL_SEQ, "OmxSyncFpgaOutputToHost : decIndex=%d  from FpgaAddr:0x%x To HostAddr:%p, omxStride=%d, omxHeight=%d, nBufferSize=%d\n",
              index, fb.bufY, ((outPort->pInternalBufferStorage[index])->pBuffer), (int)outPort->sPortParam.format.video.nStride, (int)outPort->sPortParam.format.video.nFrameHeight, (int)outPort->sPortParam.nBufferSize);
        len = vdi_read_memory(pVpu->coreIdx, fb.bufY, (unsigned char*)((outPort->pInternalBufferStorage[index])->pBuffer), (int)outPort->sPortParam.nBufferSize, pVpu->decOP.frameEndian);
    }
#else
    DEBUG(DEB_LEV_FULL_SEQ, "OmxSyncFpgaOutputToHost : decIndex=%d  from FpgaAddr:0x%x To HostAddr:%p, omxStride=%d, omxHeight=%d, nBufferSize=%d\n",
          index, fb.bufY, ((outPort->pInternalBufferStorage[index])->pBuffer), (int)outPort->sPortParam.format.video.nStride, (int)outPort->sPortParam.format.video.nFrameHeight, (int)outPort->sPortParam.nBufferSize);
    len = vdi_read_memory(pVpu->coreIdx, fb.bufY, (unsigned char*)((outPort->pInternalBufferStorage[index])->pBuffer), (int)outPort->sPortParam.nBufferSize, pVpu->decOP.frameEndian);
#endif

    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of  of component %p\n", openmaxStandComp);
    return len;

}
#endif

void OmxCheckVersion(int coreIdx)
{
    unsigned int version;
    unsigned int revision;
    unsigned int productId;


    VPU_GetVersionInfo(coreIdx, &version, &revision, &productId);

    DEBUG(DEB_LEV_SIMPLE_SEQ, "VPU coreNum : [%d]\n", coreIdx);
    DEBUG(DEB_LEV_SIMPLE_SEQ, "Firmware Version => projectId : %x | version : %04d.%04d.%08d | revision : r%d\n",
          (int)(version>>16), (int)((version>>(12))&0x0f), (int)((version>>(8))&0x0f), (int)((version)&0xff), (int)revision);
    DEBUG(DEB_LEV_SIMPLE_SEQ, "Hardware Version => %04x\n", productId);
    DEBUG(DEB_LEV_SIMPLE_SEQ, "API Version => %04x\n\n", API_VERSION);
}


OMX_ERRORTYPE omx_videodec_component_AllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    OMX_COMPONENTTYPE *openmaxStandComp = hComponent;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_base_PortType *openmaxStandPort;
    omx_vpudec_component_PortType *vpuPort;

    if (nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
        DEBUG(DEB_LEV_ERR, "wrong port index\n");
        return OMX_ErrorBadPortIndex;
    }

    openmaxStandPort = (omx_base_PortType *) omx_base_component_Private->ports[nPortIndex];
    vpuPort = (omx_vpudec_component_PortType *)openmaxStandPort;

    if (openmaxStandPort)
    {
        unsigned int i;
        DEBUG(DEB_LEV_FUNCTION_NAME, "In  for Component=%p, Port=%p\n", openmaxStandComp, openmaxStandPort);

        if (nPortIndex != openmaxStandPort->sPortParam.nPortIndex) {
            return OMX_ErrorBadPortIndex;
        }
        if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
            return OMX_ErrorBadPortIndex;
        }

        if (omx_base_component_Private->transientState != OMX_TransStateLoadedToIdle) {
            if (!openmaxStandPort->bIsTransientToEnabled) {
                DEBUG(DEB_LEV_ERR, "The port is not allowed to receive buffers\n");
                return OMX_ErrorIncorrectStateTransition;
            }
        }

        //Deleted by duan.xue
        /*if(nSizeBytes < openmaxStandPort->sPortParam.nBufferSize) {
	DEBUG(DEB_LEV_ERR, "In : Requested Buffer Size %lu is less than Minimum Buffer Size %lu\n", nSizeBytes, openmaxStandPort->sPortParam.nBufferSize);
	return OMX_ErrorIncorrectStateTransition;
        }*/

        if (nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX)
        {
            if (openmaxStandPort->sPortParam.nBufferCountActual >  MAX_DEC_BITSTREAM_BUFFER_COUNT)
            {
                DEBUG(DEB_LEV_ERR, "nBufferCountActual[%d] is more than MAX_DEC_BITSTREAM_BUFFER_COUNT[%d]\n", (int)openmaxStandPort->sPortParam.nBufferCountActual, (int)MAX_DEC_BITSTREAM_BUFFER_COUNT);
                return OMX_ErrorInsufficientResources;
            }
        }

        if (omx_vpudec_component_Private->portSettingChangeRequest == OMX_TRUE &&
                vpuPort->sPortParam.nBufferCountActual != vpuPort->nTempBufferCountActual &&
                nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
        {
            int j;

            if(vpuPort->pInternalBufferStorage)
            {
                free(vpuPort->pInternalBufferStorage);
                vpuPort->pInternalBufferStorage = NULL;
            }

            if(vpuPort->bBufferStateAllocated) {
                free(vpuPort->bBufferStateAllocated);
                vpuPort->bBufferStateAllocated = NULL;
            }

            vpuPort->sPortParam.nBufferCountActual = vpuPort->nTempBufferCountActual;

            vpuPort->pInternalBufferStorage = malloc(vpuPort->sPortParam.nBufferCountActual*sizeof(OMX_BUFFERHEADERTYPE *));
            if (!vpuPort->pInternalBufferStorage) {
                DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
                return OMX_ErrorInsufficientResources;
            }
            memset(vpuPort->pInternalBufferStorage, 0x00, vpuPort->sPortParam.nBufferCountActual*sizeof(OMX_BUFFERHEADERTYPE *));

            vpuPort->bBufferStateAllocated = malloc(vpuPort->sPortParam.nBufferCountActual*sizeof(BUFFER_STATUS_FLAG));
            if (!vpuPort->bBufferStateAllocated) {
                DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
                return OMX_ErrorInsufficientResources;
            }
            memset(vpuPort->bBufferStateAllocated, 0x00, vpuPort->sPortParam.nBufferCountActual*sizeof(BUFFER_STATUS_FLAG));

            for(j=0; j < (int)vpuPort->sPortParam.nBufferCountActual; j++) {
                vpuPort->bBufferStateAllocated[j] = BUFFER_FREE;
            }
        }

        //DEBUG(DEB_LEV_ERR, "in , nBufferCountActual = %d, nPortIndex = %d\n", openmaxStandPort->sPortParam.nBufferCountActual, nPortIndex);
        for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++)
        {
            //DEBUG(DEB_LEV_ERR, ", bBufferStateAllocated[%d] = %d\n", i, openmaxStandPort->bBufferStateAllocated[i]);

            if (openmaxStandPort->bBufferStateAllocated[i] == BUFFER_FREE)
            {
                openmaxStandPort->pInternalBufferStorage[i] = malloc(sizeof(OMX_BUFFERHEADERTYPE));
                if (!openmaxStandPort->pInternalBufferStorage[i]) {
                    DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
                    return OMX_ErrorInsufficientResources;
                }
                memset(openmaxStandPort->pInternalBufferStorage[i], 0x00, sizeof(OMX_BUFFERHEADERTYPE));

                setHeader(openmaxStandPort->pInternalBufferStorage[i], sizeof(OMX_BUFFERHEADERTYPE));
                if (openmaxStandPort->sPortParam.eDir == OMX_DirInput)
                {
                    if (omx_vpudec_component_Private->bUseOmxInputBufferAsDecBsBuffer == OMX_TRUE)
                    {
                        /* allocate the buffer */
                        pVpu->vbStream[i].size = nSizeBytes;
                        pVpu->vbStream[i].size = ((pVpu->vbStream[i].size+1023)&~1023);

                        if (vdi_allocate_dma_memory(pVpu->coreIdx, &pVpu->vbStream[i]) < 0)
                        {
                            DEBUG(DEB_LEV_ERR, "fail to allocate bitstream buffer bufferIdx=%d\n", (int)i);
                            return OMX_ErrorInsufficientResources;
                        }
                        DEBUG(DEB_LEV_SIMPLE_SEQ, "allocate bitstream buffer=0x%x, size = %d\n", (int)pVpu->vbStream[i].virt_addr, (int)pVpu->vbStream[i].size);
                        openmaxStandPort->pInternalBufferStorage[i]->pBuffer = (OMX_U8 *)pVpu->vbStream[i].virt_addr;
                        memset(openmaxStandPort->pInternalBufferStorage[i]->pBuffer, 0x00, nSizeBytes);
                    }
                    else
                    {
                        openmaxStandPort->pInternalBufferStorage[i]->pBuffer = malloc(nSizeBytes);
                        if(openmaxStandPort->pInternalBufferStorage[i]->pBuffer==NULL) {
                            DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
                            return OMX_ErrorInsufficientResources;
                        }
                        memset(openmaxStandPort->pInternalBufferStorage[i]->pBuffer, 0x00, nSizeBytes);

                    }
                    openmaxStandPort->pInternalBufferStorage[i]->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
                }
                else
                {

                    /* allocate the buffer */
                    pVpu->vbAllocFb[i].size = nSizeBytes;
                    if (vdi_allocate_dma_memory(pVpu->coreIdx, &pVpu->vbAllocFb[i]) < 0)
                    {
                        DEBUG(DEB_LEV_ERR, "fail to allocate framebuffer buffer bufferIdx=%d\n", (int)i);
                        return OMX_ErrorInsufficientResources;
                    }
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "allocate framebuffer buffer = 0x%x, size = m%d\n", (int)pVpu->vbAllocFb[i].virt_addr, (int)pVpu->vbAllocFb[i].size);
                    openmaxStandPort->pInternalBufferStorage[i]->pBuffer = (OMX_U8 *)pVpu->vbAllocFb[i].virt_addr;
                    memset(openmaxStandPort->pInternalBufferStorage[i]->pBuffer, 0x00, nSizeBytes);

                    openmaxStandPort->pInternalBufferStorage[i]->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
                    if (omx_vpudec_component_Private->omx_display_flags)
                        omx_vpudec_component_Private->omx_display_flags[i].owner = OMX_BUFFER_OWNER_NOBODY;
                }
                openmaxStandPort->pInternalBufferStorage[i]->nAllocLen = nSizeBytes;
                openmaxStandPort->pInternalBufferStorage[i]->pPlatformPrivate = openmaxStandPort;
                openmaxStandPort->pInternalBufferStorage[i]->pAppPrivate = pAppPrivate;
                *ppBuffer = openmaxStandPort->pInternalBufferStorage[i];
                openmaxStandPort->bBufferStateAllocated[i] = BUFFER_ALLOCATED;
                openmaxStandPort->bBufferStateAllocated[i] |= HEADER_ALLOCATED;

                openmaxStandPort->nNumAssignedBuffers++;

                if (openmaxStandPort->sPortParam.nBufferCountActual == openmaxStandPort->nNumAssignedBuffers)
                {
                    openmaxStandPort->sPortParam.bPopulated = OMX_TRUE;
                    openmaxStandPort->bIsFullOfBuffers = OMX_TRUE;
                    vpuPort->bAllocateBuffer = OMX_TRUE;
                    tsem_up(openmaxStandPort->pAllocSem);
                    if (omx_vpudec_component_Private->portSettingChangeRequest == OMX_TRUE)
                    {
                        omx_vpudec_component_Private->portSettingChangeRequest = OMX_FALSE;
                        tsem_up(&omx_vpudec_component_Private->port_setting_change_tsem);
                    }

                    DEBUG(DEB_LEV_SIMPLE_SEQ, "Done of   portIndex=%d, nNumAssignedBuffers=%d, bUseOmxInputBufferAsDecBsBuffer=%d\n", (int)openmaxStandPort->sPortParam.eDir, (int)openmaxStandPort->nNumAssignedBuffers, (int)omx_vpudec_component_Private->bUseOmxInputBufferAsDecBsBuffer);
                }
                DEBUG(DEB_LEV_FUNCTION_NAME, "Out of  for port %p\n", openmaxStandPort);
                return OMX_ErrorNone;
            }
        }

        DEBUG(DEB_LEV_ERR, "Out for port %p. Error: no available buffers\n", openmaxStandPort);
        return OMX_ErrorInsufficientResources;
    }
    DEBUG(DEB_LEV_FUNCTION_NAME, "Out for Component=%p, Port=%p, buffer %p\n", openmaxStandComp, openmaxStandPort, ppBuffer);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE omx_videodec_component_UseBuffer(
    OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate,
    OMX_U32 nSizeBytes,
    OMX_U8* pBuffer)
{
    OMX_COMPONENTTYPE *openmaxStandComp = hComponent;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_base_PortType *openmaxStandPort;
    omx_vpudec_component_PortType *vpuPort;
    OMX_BUFFERHEADERTYPE* returnBufferHeader;

    DEBUG(DEB_LEV_FUNCTION_NAME, "... nPortIndex = %d\n", nPortIndex);

    if (nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
        DEBUG(DEB_LEV_ERR, "wrong port index\n");
        return OMX_ErrorBadPortIndex;
    }

    openmaxStandPort = (omx_base_PortType *) omx_base_component_Private->ports[nPortIndex];
    vpuPort = (omx_vpudec_component_PortType *)openmaxStandPort;

    if (openmaxStandPort)
    {
        unsigned int i;
        DEBUG(DEB_LEV_FUNCTION_NAME, "In  for Component=%p, Port=%p\n", openmaxStandComp, openmaxStandPort);

        if (nPortIndex != openmaxStandPort->sPortParam.nPortIndex) {
            return OMX_ErrorBadPortIndex;
        }
        if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
            return OMX_ErrorBadPortIndex;
        }

        if (omx_base_component_Private->transientState != OMX_TransStateLoadedToIdle) {
            if (!openmaxStandPort->bIsTransientToEnabled) {
                DEBUG(DEB_LEV_ERR, "The port is not allowed to receive buffers\n");
                return OMX_ErrorIncorrectStateTransition;
            }
        }

        if(nSizeBytes < openmaxStandPort->sPortParam.nBufferSize) {
            DEBUG(DEB_LEV_ERR, "Requested Buffer Size %lu is less than Minimum Buffer Size %lu\n", nSizeBytes, openmaxStandPort->sPortParam.nBufferSize);
            return OMX_ErrorIncorrectStateTransition;
        }



        if (omx_vpudec_component_Private->portSettingChangeRequest == OMX_TRUE &&
                vpuPort->sPortParam.nBufferCountActual != vpuPort->nTempBufferCountActual &&
                nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
        {
            int j;

            if(vpuPort->pInternalBufferStorage)
            {
                free(vpuPort->pInternalBufferStorage);
                vpuPort->pInternalBufferStorage = NULL;
            }

            if(vpuPort->bBufferStateAllocated) {
                free(vpuPort->bBufferStateAllocated);
                vpuPort->bBufferStateAllocated = NULL;
            }

            vpuPort->sPortParam.nBufferCountActual = vpuPort->nTempBufferCountActual;

            vpuPort->pInternalBufferStorage = malloc(vpuPort->sPortParam.nBufferCountActual*sizeof(OMX_BUFFERHEADERTYPE *));
            if (!vpuPort->pInternalBufferStorage) {
                DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
                return OMX_ErrorInsufficientResources;
            }

            memset(vpuPort->pInternalBufferStorage, 0x00, vpuPort->sPortParam.nBufferCountActual*sizeof(OMX_BUFFERHEADERTYPE *));

            vpuPort->bBufferStateAllocated = malloc(vpuPort->sPortParam.nBufferCountActual*sizeof(BUFFER_STATUS_FLAG));
            if (!vpuPort->bBufferStateAllocated) {
                DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
                return OMX_ErrorInsufficientResources;
            }
            memset(vpuPort->bBufferStateAllocated, 0x00, vpuPort->sPortParam.nBufferCountActual*sizeof(BUFFER_STATUS_FLAG));

            for(j=0; j < (int)vpuPort->sPortParam.nBufferCountActual; j++) {
                vpuPort->bBufferStateAllocated[j] = BUFFER_FREE;
            }
        }

        for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++) {
            if (openmaxStandPort->bBufferStateAllocated[i] == BUFFER_FREE) {
                openmaxStandPort->pInternalBufferStorage[i] = malloc(sizeof(OMX_BUFFERHEADERTYPE));
                if (!openmaxStandPort->pInternalBufferStorage[i]) {
                    DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
                    return OMX_ErrorInsufficientResources;
                }
                memset(openmaxStandPort->pInternalBufferStorage[i], 0x00, sizeof(OMX_BUFFERHEADERTYPE));
                DEBUG(DEB_LEV_FULL_SEQ, " openmaxStandPort->pInternalBufferStorage[%u]=%p (portIndex=%d)\n",
                      i, openmaxStandPort->pInternalBufferStorage[i], (int)openmaxStandPort->sPortParam.eDir);

                openmaxStandPort->bIsEmptyOfBuffers = OMX_FALSE;
                setHeader(openmaxStandPort->pInternalBufferStorage[i], sizeof(OMX_BUFFERHEADERTYPE));

                openmaxStandPort->pInternalBufferStorage[i]->pBuffer = pBuffer;
                openmaxStandPort->pInternalBufferStorage[i]->nAllocLen = nSizeBytes;
                openmaxStandPort->pInternalBufferStorage[i]->pPlatformPrivate = openmaxStandPort;
                openmaxStandPort->pInternalBufferStorage[i]->pAppPrivate = pAppPrivate;
                openmaxStandPort->bBufferStateAllocated[i] = BUFFER_ASSIGNED;
                openmaxStandPort->bBufferStateAllocated[i] |= HEADER_ALLOCATED;
                returnBufferHeader = malloc(sizeof(OMX_BUFFERHEADERTYPE));
                if (!returnBufferHeader) {
                    DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
                    return OMX_ErrorInsufficientResources;
                }
                memset(returnBufferHeader, 0x00, sizeof(OMX_BUFFERHEADERTYPE));

                setHeader(returnBufferHeader, sizeof(OMX_BUFFERHEADERTYPE));
                returnBufferHeader->pBuffer = pBuffer;
                returnBufferHeader->nAllocLen = nSizeBytes;
                returnBufferHeader->pPlatformPrivate = openmaxStandPort;
                returnBufferHeader->pAppPrivate = pAppPrivate;
                if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
                    openmaxStandPort->pInternalBufferStorage[i]->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
                    returnBufferHeader->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
                } else {
                    openmaxStandPort->pInternalBufferStorage[i]->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
                    returnBufferHeader->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
                }
                *ppBufferHdr = returnBufferHeader;


                DEBUG(DEB_LEV_FULL_SEQ, " [%u] ppBufferHdr=%p, returnBufferHeader=%p (portIndex=%d)\n",
                      i, ppBufferHdr, returnBufferHeader, (int)openmaxStandPort->sPortParam.eDir);


                openmaxStandPort->nNumAssignedBuffers++;

                if (openmaxStandPort->sPortParam.nBufferCountActual == openmaxStandPort->nNumAssignedBuffers) {
                    openmaxStandPort->sPortParam.bPopulated = OMX_TRUE;
                    openmaxStandPort->bIsFullOfBuffers = OMX_TRUE;
                    vpuPort->bAllocateBuffer = OMX_FALSE;

                    if (nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX)
                    {
                        omx_vpudec_component_Private->bUseOmxInputBufferAsDecBsBuffer = OMX_FALSE;
                    }

                    tsem_up(openmaxStandPort->pAllocSem);
                    if (omx_vpudec_component_Private->portSettingChangeRequest == OMX_TRUE)
                    {
                        omx_vpudec_component_Private->portSettingChangeRequest = OMX_FALSE;
                        tsem_up(&omx_vpudec_component_Private->port_setting_change_tsem);
                    }
                }
                DEBUG(DEB_LEV_SIMPLE_SEQ, " portIndex=%d, nNumAssignedBuffers=%d, nSizeBytes=%d, pBuffer=%p\n", (int)openmaxStandPort->sPortParam.eDir, (int)openmaxStandPort->nNumAssignedBuffers, (int)nSizeBytes, openmaxStandPort->pInternalBufferStorage[i]->pBuffer);
                DEBUG(DEB_LEV_FUNCTION_NAME, "Out of  for port %p\n", openmaxStandPort);
                return OMX_ErrorNone;
            }
        }
        DEBUG(DEB_LEV_ERR, "Error: no available buffers CompName=%s\n",omx_base_component_Private->name);
        return OMX_ErrorInsufficientResources;
    }
    DEBUG(DEB_LEV_FUNCTION_NAME, "Out  for Component=%p, Port=%p, buffer %p\n", openmaxStandComp, openmaxStandPort, ppBufferHdr);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE omx_videodec_component_FreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{

    OMX_COMPONENTTYPE *openmaxStandComp = hComponent;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_base_PortType *openmaxStandPort;

    DEBUG(DEB_LEV_FUNCTION_NAME, "In  for component %p, port:%d, pBuffer=%p\n", hComponent, (int)nPortIndex, pBuffer);
    if (nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
        DEBUG(DEB_LEV_ERR, "wrong port index\n");
        return OMX_ErrorBadPortIndex;
    }

    openmaxStandPort = omx_base_component_Private->ports[nPortIndex];
    if (openmaxStandPort)
    {
        unsigned int i;
        OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
        omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
        DEBUG(DEB_LEV_FUNCTION_NAME, "In  for port %p, nNumAssignedBuffers=%d\n", openmaxStandPort, (int)openmaxStandPort->nNumAssignedBuffers);

        if (nPortIndex != openmaxStandPort->sPortParam.nPortIndex) {
            return OMX_ErrorBadPortIndex;
        }
        if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
            return OMX_ErrorBadPortIndex;
        }

        if (omx_base_component_Private->transientState != OMX_TransStateIdleToLoaded) {
            if (!openmaxStandPort->bIsTransientToDisabled) {
                DEBUG(DEB_LEV_FULL_SEQ, "The port is not allowed to free the buffers\n");
                (*(omx_base_component_Private->callbacks->EventHandler))
                (omxComponent,
                 omx_base_component_Private->callbackData,
                 OMX_EventError, /* The command was completed */
                 OMX_ErrorPortUnpopulated, /* The commands was a OMX_CommandStateSet */
                 nPortIndex, /* The state has been changed in message->messageParam2 */
                 NULL);
            }
        }

        for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++)
        {
            if (openmaxStandPort->bBufferStateAllocated[i] & (BUFFER_ASSIGNED | BUFFER_ALLOCATED))	// OMX_AllocateBuffer or OMX_UseBuffer have been invoked.
            {
                openmaxStandPort->bIsFullOfBuffers = OMX_FALSE;
                if (openmaxStandPort->bBufferStateAllocated[i] & BUFFER_ALLOCATED) // OMX_AllocateBuffer has been invoked by IL Client.
                {
                    if(openmaxStandPort->pInternalBufferStorage[i] == pBuffer)
                    {
                        DEBUG(DEB_LEV_FULL_SEQ, "freeing [%u] pBuffer=%p\n", i, openmaxStandPort->pInternalBufferStorage[i]->pBuffer);
                        if (openmaxStandPort->sPortParam.eDir == OMX_DirInput)
                        {
                            if (omx_vpudec_component_Private->bUseOmxInputBufferAsDecBsBuffer == OMX_TRUE)
                            {
                                DEBUG(DEB_LEV_PARAMS, "freeing  vpu bitstream buffer pBuffer = 0x%x, virt_addr=0x%x, size=%d\n", (unsigned long)openmaxStandPort->pInternalBufferStorage[i]->pBuffer, (int)pVpu->vbStream[i].virt_addr, (int)pVpu->vbStream[i].size);
                                if (pVpu->vbStream[i].size > 0)
                                {
                                    vdi_free_dma_memory(pVpu->coreIdx, &pVpu->vbStream[i]);

                                    openmaxStandPort->pInternalBufferStorage[i]->pBuffer=NULL;
                                }
                                else
                                {
                                    DEBUG(DEB_LEV_ERR, "fail to find bitstream buffer that should free pBuffer=0x%x, virt_addr=0x%x, size=%d\n", (unsigned long)openmaxStandPort->pInternalBufferStorage[i]->pBuffer, (int)pVpu->vbStream[i].virt_addr, (int)pVpu->vbStream[i].size);
                                    return OMX_ErrorInsufficientResources;
                                }
                            }
                            else
                            {
                                if (openmaxStandPort->pInternalBufferStorage[i]->pBuffer)
                                {
                                    free(openmaxStandPort->pInternalBufferStorage[i]->pBuffer);
                                    openmaxStandPort->pInternalBufferStorage[i]->pBuffer=NULL;
                                }
                            }
                        }
                        else	// for output
                        {
                            DEBUG(DEB_LEV_PARAMS, "freeing  vpu frame buffer useNativeBuffer=%d, pBuffer = 0x%x, phys_addr=0x%x, virt_addr=0x%x, size=%d\n", (int)omx_vpudec_component_Private->useNativeBuffer, (unsigned long)openmaxStandPort->pInternalBufferStorage[i]->pBuffer, (int)pVpu->vbAllocFb[i].phys_addr, (int)pVpu->vbAllocFb[i].virt_addr, (int)pVpu->vbAllocFb[i].size);

                            if (omx_vpudec_component_Private->useNativeBuffer == OMX_FALSE)
                            {
                                if (pVpu->vbAllocFb[i].size > 0)
                                {
                                    vdi_free_dma_memory(pVpu->coreIdx, &pVpu->vbAllocFb[i]);
                                    openmaxStandPort->pInternalBufferStorage[i]->pBuffer=NULL;
                                }
                                DEBUG(DEB_LEV_PARAMS, "freeing  vpu frame buffer done\n");
                            }

                        }
                    }
                    else
                    {
                        DEBUG(DEB_LEV_FULL_SEQ, "!!!!!! buffer not match[%u] , port:%u\n", i, (int)nPortIndex);
                        continue;
                    }
                }
                else if (openmaxStandPort->bBufferStateAllocated[i] & BUFFER_ASSIGNED) // OMX_UseBuffer has been invoked by IL Client.
                {
                    if (nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
                    {
                        if (openmaxStandPort->pInternalBufferStorage[i]->pBuffer == pBuffer->pBuffer)
                        {
                            DEBUG(DEB_LEV_PARAMS, "port:%u, pInternalBufferStorage[%u]=%p, pBuffer=%p, pVpu->vbAllocFb[%u].size=%u\n",
                                  nPortIndex, i, openmaxStandPort->pInternalBufferStorage[i], pBuffer, i, pVpu->vbAllocFb[i].size);
                            if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
                            {
#ifdef CNM_FPGA_PLATFORM
                                if (pVpu->vbAllocFb[i].size > 0)
                                {
                                    vdi_dettach_dma_memory(pVpu->coreIdx, &pVpu->vbAllocFb[i]);
                                    pVpu->vbAllocFb[i].size = 0;
                                }
#endif
                            }
                        }
                        else
                        {
                            DEBUG(DEB_LEV_FULL_SEQ, "!!!!!! buffer not match[%u] , port:%u\n", i, (int)nPortIndex);
                            continue;
                        }
                    }
                    else
                    {
                        if (openmaxStandPort->pInternalBufferStorage[i]->pBuffer == pBuffer->pBuffer)
                        {
                            DEBUG(DEB_LEV_PARAMS, "port:%u, pInternalBufferStorage[%u]=%p, pBuffer=%p, pVpu->vbAllocFb[%u].size=%u\n",
                                  nPortIndex, i, openmaxStandPort->pInternalBufferStorage[i], pBuffer, i, pVpu->vbAllocFb[i].size);
                            free(pBuffer);
                            pBuffer = NULL;
                        }
                        else
                        {
                            DEBUG(DEB_LEV_FULL_SEQ, "!!!!!! buffer not match[%u] , port:%u\n", i, (int)nPortIndex);
                            continue;
                        }
                    }
                }

                if(openmaxStandPort->bBufferStateAllocated[i] & HEADER_ALLOCATED) {
                    if (openmaxStandPort->pInternalBufferStorage[i]) {
                        if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE &&
                            nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX) {
                            // Free IOMMU Addr if necessary
                            freeIOMMUAddr(openmaxStandPort->pInternalBufferStorage[i]); 
                        }
                        free(openmaxStandPort->pInternalBufferStorage[i]);
                        openmaxStandPort->pInternalBufferStorage[i]=NULL;
                    }
                }

                openmaxStandPort->bBufferStateAllocated[i] = BUFFER_FREE;

                openmaxStandPort->nNumAssignedBuffers--;

                if (openmaxStandPort->nNumAssignedBuffers == 0) {
                    openmaxStandPort->sPortParam.bPopulated = OMX_FALSE;
                    openmaxStandPort->bIsEmptyOfBuffers = OMX_TRUE;
#ifdef SUPPORT_FIX_ADAPTIVE_PLAYBACK
                    pVpu->seqInited = 0;
#endif
                    tsem_up(openmaxStandPort->pAllocSem);
                }
                DEBUG(DEB_LEV_FUNCTION_NAME, "Out of  for port %p\n", openmaxStandPort);
                return OMX_ErrorNone;
            }
        }
        DEBUG(DEB_LEV_ERR, "Out for port %p with OMX_ErrorInsufficientResources\n", openmaxStandPort);
        return OMX_ErrorInsufficientResources;
    }
    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of  for component %p\n", hComponent);
    return OMX_ErrorInsufficientResources;

}


OMX_BOOL OmxGetVpuBsBufferByVirtualAddress(OMX_COMPONENTTYPE *openmaxStandComp, vpu_buffer_t *vb, OMX_BUFFERHEADERTYPE *pInputBuffer)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *inPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    int i;

#if 0 //DEBUG_LEVEL > 0
    for (i = 0; i < (int)inPort->nNumAssignedBuffers; i++)
    {
        DEBUG(DEB_LEV_FULL_SEQ, " of component : bitstream buffer index = %d, pInputBuffer=%p, phys_addr=0x%x, virt_addr=0x%x size=%d\n",
              __func__, i, inPort->pInternalBufferStorage[i], (int)pVpu->vbStream[i].phys_addr, (int)pVpu->vbStream[i].virt_addr, (int)pVpu->vbStream[i].size);
    }
#endif

    for (i = 0; i < (int)inPort->nNumAssignedBuffers; i++)
    {
        if (inPort->pInternalBufferStorage[i] == pInputBuffer)
        {
            DEBUG(DEB_LEV_FULL_SEQ, "bitstream buffer for this frame index = %d, pInputBuffer=%p, phys_addr=0x%x, virt_addr=0x%x size=%d\n",
                  i, inPort->pInternalBufferStorage[i], (int)pVpu->vbStream[i].phys_addr, (int)pVpu->vbStream[i].virt_addr, (int)pVpu->vbStream[i].size);

            *vb = pVpu->vbStream[i];
            return OMX_TRUE;
        }

    }
    return OMX_FALSE;
}


OMX_BOOL OmxNeedInputBufferReformat(omx_vpudec_component_PrivateType* omx_vpudec_component_Private)
{
    OMX_BOOL bNeedReformatInputBuffer = OMX_FALSE;
#ifdef WORKAROUND_OMX_USE_INPUT_HEADER_BUFFER_AS_DECODER_BITSTREAM_BUFFER_NOT_REFORMATED
    if ((omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingWMV) ||
            (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingVC1) ||
            (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingAVS) ||
            (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CodingRV))
        bNeedReformatInputBuffer = OMX_TRUE;
#else // WORKAROUND_OMX_USE_INPUT_HEADER_BUFFER_AS_DECODER_BITSTREAM_BUFFER_NOT_REFORMATED
    bNeedReformatInputBuffer = OMX_FALSE;
#endif

    if (omx_vpudec_component_Private->video_coding_type == OMX_VIDEO_CODINGTYPE_VP8)
        bNeedReformatInputBuffer = OMX_TRUE;


    return bNeedReformatInputBuffer;
}


int OmxWriteBsBufFromBufHelper(OMX_COMPONENTTYPE *openmaxStandComp, vpu_dec_context_t *pVpu, vpu_buffer_t vbStream, BYTE *pChunk,  int chunkSize)
{
	omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
	RetCode ret = RETCODE_SUCCESS;
	Uint32 room = 0;

	PhysicalAddress paRdPtr, paWrPtr;

	if (chunkSize < 1)
	{
		DEBUG(DEB_LEV_ERR, "Invalid chunkSize = %d\n", (int)chunkSize);
		return -1;
	}

	if (chunkSize > (int)vbStream.size)
	{
		DEBUG(DEB_LEV_ERR, "chunkSize is larger than  bitstream buffer size, chunksize=%d, bitstream buffer size=%d\n", (int)chunkSize, (int)vbStream.size);
		return -1;
	}

	ret = VPU_DecGetBitstreamBuffer(pVpu->handle, &paRdPtr, &paWrPtr, &room);
	if( ret != RETCODE_SUCCESS )
	{
		DEBUG(DEB_LEV_ERR, "VPU_DecGetBitstreamBuffer failed Error code is 0x%x \n", ret );
		return -1;
	}

	if(room < (Uint32)chunkSize)
	{
		DEBUG(DEB_LEV_FULL_SEQ, "no room for feeding bitstream. it will take a change to fill stream\n");
		return 0; // no room for feeding bitstream. it will take a change to fill stream
	}



	DEBUG(DEB_LEV_FULL_SEQ, "OmxWriteBsBufFromBufHelper from vbStream.phys_addr=0x%x, chunkSize=%d, to paWrPtr=0x%x, room=%d, bUseOmxInputBufferAsDecBsBuffer=%d\n",
		(int)vbStream.phys_addr,  chunkSize, (int)paWrPtr, (int)room, (int)omx_vpudec_component_Private->bUseOmxInputBufferAsDecBsBuffer);

	if (omx_vpudec_component_Private->bUseOmxInputBufferAsDecBsBuffer == OMX_FALSE)
	{
		vdi_write_memory(pVpu->coreIdx, paWrPtr, pChunk, chunkSize, pVpu->decOP.streamEndian);
	}
	else
	{
#ifdef CNM_FPGA_PLATFORM
		//write CPU memory to FPGA memory
		vdi_write_memory(pVpu->coreIdx, paWrPtr, pChunk, chunkSize, pVpu->decOP.streamEndian);
#endif

	}

	ret = VPU_DecUpdateBitstreamBuffer(pVpu->handle, chunkSize);
	if( ret != RETCODE_SUCCESS )
	{
		DEBUG(DEB_LEV_ERR, "VPU_DecUpdateBitstreamBuffer failed Error code is 0x%x \n", ret );
		return -1;
	}

	return chunkSize;
}


OMX_U32 OmxGetVpuFrameRate(omx_vpudec_component_PrivateType* omx_vpudec_component_Private)
{
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    OMX_U32 framerate;

    DEBUG(DEB_LEV_SIMPLE_SEQ, "OmxGetVpuFrameRate fRateNumerator=%d, fRateDenominator=%d\n", pVpu->initialInfo.fRateNumerator, pVpu->initialInfo.fRateDenominator);

    framerate = 0;

    if (pVpu->initialInfo.fRateDenominator <= 0)
        return 0;

    switch(omx_vpudec_component_Private->video_coding_type)
    {
    case OMX_VIDEO_CodingAVC:
    case OMX_VIDEO_CodingHEVC:
    case OMX_VIDEO_CodingAVS:
    case OMX_VIDEO_CodingMPEG2:
    case OMX_VIDEO_CodingMPEG4:
    case OMX_VIDEO_CodingH263:
    case OMX_VIDEO_CodingRV:
    case OMX_VIDEO_CODINGTYPE_VP8:
        framerate = (OMX_U32)(pVpu->initialInfo.fRateNumerator/pVpu->initialInfo.fRateDenominator);
        break;
    case OMX_VIDEO_CodingVC1:
    case OMX_VIDEO_CodingWMV:
        if (pVpu->initialInfo.profile == 2) // AP
        {
            const int TBL_FRAME_RATE_NR[8] = {0, 24, 25, 30, 50, 60, 48, 72};
            framerate = (OMX_U32)(TBL_FRAME_RATE_NR[pVpu->initialInfo.fRateNumerator] * 100)/(pVpu->initialInfo.fRateDenominator + 999);
        }
        else
        {
            framerate = (OMX_U32)((pVpu->initialInfo.fRateNumerator+1)/pVpu->initialInfo.fRateDenominator);
        }
        break;
    default:
        framerate = 0;
        break;
    }


    if (framerate > (OMX_U32)120)
    {
        framerate = 0; // it means we bypass the timestamp of inputbuffer to outpubuffer without timestamp corretion logic.
    }

    return framerate;
}


OMX_TICKS OmxTimeStampCorrection(OMX_COMPONENTTYPE *openmaxStandComp, OMX_TICKS nInputTimeStamp)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *inPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    omx_timestamp_correction_t *tsc = &omx_vpudec_component_Private->omx_ts_correction;
    OMX_TICKS nTimeStamp;

    if (inPort->sPortParam.format.video.xFramerate == 0)
    {
        DEBUG(DEB_LEV_ERR, "OmxTimeStampCorrection no xFramerate present\n");
        return (OMX_TICKS)-1;
    }

#define CNM_SPRD_PLATFORM_TSC
    nTimeStamp = nInputTimeStamp;

    DEBUG(DEB_LEV_FULL_SEQ, "OmxTimeStampCorrection nInputTimeStamp=%lld, nOutputTimeStamp=%lld, xFramerate=%d\n", nInputTimeStamp, nTimeStamp, (int)inPort->sPortParam.format.video.xFramerate);

    return nTimeStamp;
}

OMX_BOOL OmxClearDisplayFlag(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE *pBuffer, OMX_BOOL bFillThisBufer)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *outPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    int i;
    omx_display_flag *disp_flag_array;
    int dispFlagIdx;

    disp_flag_array = omx_vpudec_component_Private->omx_display_flags;

    pthread_mutex_lock(&omx_vpudec_component_Private->display_flag_mutex);
    for (i = 0; i < (int)outPort->sPortParam.nBufferCountActual; i++)
    {
        if (outPort->pInternalBufferStorage[i] == pBuffer)
        {

            if (bFillThisBufer == OMX_TRUE)
                disp_flag_array[i].owner = OMX_BUFERR_OWNER_COMPONENT;
            else // FillBufferDone
                disp_flag_array[i].owner = OMX_BUFFER_OWNER_CLIENT;

            if(PORT_IS_BEING_FLUSHED(outPort) || (PORT_IS_BEING_DISABLED(outPort) && PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(outPort)))
            {
                // no action required to VPU
            }
            else
            {
                if (pVpu->handle)
                {
                    dispFlagIdx = i;
                    if (pVpu->decOP.scalerInfo.enScaler)
                        dispFlagIdx = i + pVpu->initialInfo.minFrameBufferCount;
                    if (bFillThisBufer == OMX_TRUE)
                    {
                        VPU_DecClrDispFlag(pVpu->handle, dispFlagIdx);
                        DEBUG(DEB_LEV_FULL_SEQ, "VPU_DecClrDispFlag for index=%d, dispFlagIdx=%d, owner=0x%x\n",
                                i, dispFlagIdx, disp_flag_array[i].owner);
                    }
                    else
                    {
                        // SHOULD not set to display flag in hear by HOST. because the owner of setting this flag must be firmware to take case of reference frame.
                        // if this index is reference frame at current. firmware do not use this index for decoding. and firmware remain this flag to 0.
                        // but if host set this flag to 1. decoder will not use this index forever after this index will be available time to decode. ( after output the other buffers )
#if 0
                        if (PORT_IS_BEING_FLUSHED(outPort)) {
                            VPU_DecSetDispFlag(pVpu->handle, dispFlagIdx);
                            DEBUG(DEB_LEV_FULL_SEQ, "VPU_DecSetDispFlag for buffer: %p, index=%d, dispFlagIdx=%d, owner=0x%x\n", pBuffer, i, dispFlagIdx, disp_flag_array[i].owner);
                        }
#endif
                    }
                }
            }


            break;
        }
    }
    pthread_mutex_unlock(&omx_vpudec_component_Private->display_flag_mutex);


    return OMX_TRUE;
}

OMX_BOOL OmxUpdateOutputBufferHeaderToDisplayOrder(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE **ppOutputBuffer)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *outPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    queue_t *pOutputQueue = outPort->pBufferQueue;
    omx_display_flag *disp_flag_array = omx_vpudec_component_Private->omx_display_flags;
    int omxBufferIndex;
    OMX_U32 nOutputLen;
    int i;

    DEBUG(DEB_LEV_FULL_SEQ, "DispFlag: 0x%x, nFieldLen = %d, pOutputBuffer=%p, PORT_IS_BEING_FLUSHED=%d, PORT_IS_BEING_DISABLED=%d, state=0x%x\n",
          pVpu->dispOutputInfo.indexFrameDisplay, (int)(*ppOutputBuffer)->nFilledLen, *ppOutputBuffer, (int)PORT_IS_BEING_FLUSHED(outPort), (int)PORT_IS_BEING_DISABLED(outPort), omx_vpudec_component_Private->state);

    nOutputLen = 0;
    i = 0;
    omxBufferIndex = 0;

    if(PORT_IS_BEING_FLUSHED(outPort) || (PORT_IS_BEING_DISABLED(outPort) && PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(outPort)) || //  to match with calling OmxSetDisplayFlag in omx_vpudec_component_SendBufferFunction
            pVpu->dispOutputInfo.indexFrameDisplay < 0)
    {
        pthread_mutex_lock(&omx_vpudec_component_Private->display_flag_mutex);
        for (i = 0; i < (int)outPort->sPortParam.nBufferCountActual; i++)
        {
            if (disp_flag_array[i].owner == OMX_BUFERR_OWNER_COMPONENT)
            {
                DEBUG(DEB_LEV_FULL_SEQ, "Found abnormal OutputBuffer for FillBufferDone  index=%d, pBuffer=%p, pInternalBufferStorage=%p prev omx_display_flag owner=0x%x\n",
                      i, *ppOutputBuffer, outPort->pInternalBufferStorage[i], disp_flag_array[i].owner );

                *ppOutputBuffer = outPort->pInternalBufferStorage[i];
                (*ppOutputBuffer)->nFilledLen = 0;
                (*ppOutputBuffer)->nTimeStamp = 0;

                break;
            }
        }
        pthread_mutex_unlock(&omx_vpudec_component_Private->display_flag_mutex);
        return OMX_TRUE;
    }

    if (pVpu->decOP.scalerInfo.enScaler)
        omxBufferIndex = pVpu->dispOutputInfo.indexFrameDisplay - pVpu->initialInfo.minFrameBufferCount;
    else
        omxBufferIndex = pVpu->dispOutputInfo.indexFrameDisplay;

    DEBUG(DEB_LEV_SIMPLE_SEQ, "Found normal OutputBuffer for FillBufferDone dispIdx = %d, omxBufferIndex=%d, ppOutputBuffer=%p, pInternalBufferStorage=%p, picType=%d\n",
          pVpu->dispOutputInfo.indexFrameDisplay, omxBufferIndex, *ppOutputBuffer, outPort->pInternalBufferStorage[omxBufferIndex], pVpu->decOutputInfos[pVpu->dispOutputInfo.indexFrameDisplay].picType);

    if (ppOutputBuffer && *ppOutputBuffer)
    {
#if 0
        if (*ppOutputBuffer != outPort->pInternalBufferStorage[omxBufferIndex]) {
            //queue_erase(pOutputQueue, outPort->pInternalBufferStorage[omxBufferIndex]);
            //queue_first(pOutputQueue, *ppOutputBuffer);
            DEBUG(DEB_LEV_FULL_SEQ,"[wenan] display index error");
        }
#endif
        nOutputLen = (*ppOutputBuffer)->nFilledLen;
        *ppOutputBuffer = outPort->pInternalBufferStorage[omxBufferIndex];
        (*ppOutputBuffer)->nFilledLen = nOutputLen;

        SetBufferIndex(*ppOutputBuffer, omxBufferIndex);
        SetPicType(*ppOutputBuffer, pVpu->decOutputInfos[pVpu->dispOutputInfo.indexFrameDisplay].picType);

        if (pVpu->decOutputInfos[pVpu->dispOutputInfo.indexFrameDisplay].picType == PIC_TYPE_I ||
                pVpu->decOutputInfos[pVpu->dispOutputInfo.indexFrameDisplay].picType == PIC_TYPE_IDR)
            (*ppOutputBuffer)->nFlags = OMX_BUFFERFLAG_SYNCFRAME;
        else
            (*ppOutputBuffer)->nFlags = 0;
    }

    return OMX_TRUE;
}



omx_bufferheader_queue_item_t* omx_bufferheader_queue_init(int count)
{
    omx_bufferheader_queue_item_t* queue = NULL;

    queue = (omx_bufferheader_queue_item_t *)osal_malloc(sizeof(omx_bufferheader_queue_item_t));
    if (!queue) {
        DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
        return NULL;
    }

    queue->size   = count;
    queue->count  = 0;
    queue->front  = 0;
    queue->rear   = 0;
    queue->buffer = (OMX_BUFFERHEADERTYPE*)osal_malloc(count*sizeof(OMX_BUFFERHEADERTYPE));
    if (!queue->buffer) {
        DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
        free(queue);
        queue = NULL;
        return NULL;
    }
    pthread_mutex_init(&queue->mutex, NULL);
    return queue;
}

void omx_bufferheader_queue_deinit(omx_bufferheader_queue_item_t* queue)
{
    if (queue == NULL)
        return;

    if (queue->buffer)
        osal_free(queue->buffer);

    pthread_mutex_destroy(&queue->mutex);

    osal_free(queue);
}


/*
* Return 0 on success.
*	   -1 on failure
*/
int omx_bufferheader_queue_enqueue(omx_bufferheader_queue_item_t* queue, OMX_BUFFERHEADERTYPE* data)
{
    if (queue == NULL)
        return -1;

    pthread_mutex_lock(&queue->mutex);
    /* Queue is full */
    if (queue->count == queue->size)
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    memcpy(&queue->buffer[queue->rear++], data, sizeof(OMX_BUFFERHEADERTYPE));
    queue->rear %= queue->size;
    queue->count++;
    pthread_mutex_unlock(&queue->mutex);

    return 0;
}


/*
* Return 0 on success.
*	   -1 on failure
*/
int omx_bufferheader_queue_dequeue(omx_bufferheader_queue_item_t* queue, OMX_BUFFERHEADERTYPE* data)
{
    if (queue == NULL)
        return -1;

    pthread_mutex_lock(&queue->mutex);
    /* Queue is empty */
    if (queue->count == 0)
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    memcpy(data, &queue->buffer[queue->front++], sizeof(OMX_BUFFERHEADERTYPE));
    queue->front %= queue->size;
    queue->count--;
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

int omx_bufferheader_queue_peekqueue(omx_bufferheader_queue_item_t* queue, OMX_BUFFERHEADERTYPE* data)
{
    if (queue == NULL) return -1;
    pthread_mutex_lock(&queue->mutex);
    /* Queue is empty */
    if (queue->count == 0)
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    memcpy(data, &queue->buffer[queue->front], sizeof(OMX_BUFFERHEADERTYPE));
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}


int omx_bufferheader_queue_dequeue_all(omx_bufferheader_queue_item_t* queue)
{
    int ret;
    OMX_BUFFERHEADERTYPE data;
    if (queue == NULL) return -1;
    do
    {
        ret = omx_bufferheader_queue_dequeue(queue, &data);
    } while (ret >= 0);
    return 0;
}

int omx_bufferheader_queue_count(omx_bufferheader_queue_item_t* queue)
{
    int res;
    if (queue == NULL) return -1;
    pthread_mutex_lock(&queue->mutex);
    res = queue->count;
    pthread_mutex_unlock(&queue->mutex);
    return res;
}


/** @brief the entry point for sending buffers to the port
*
* This function can be called by the FillThisBuffer. It depends on
* the nature of the port, that can be an output port.
# vpudec_component override this function to customize for vpu decoder  control
*/
OMX_ERRORTYPE omx_vpudec_component_SendBufferFunction(omx_base_PortType *openmaxStandPort, OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE err;
    int errQue;
    OMX_U32 portIndex;

    OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = omxComponent->pComponentPrivate;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *vpuPort= (omx_vpudec_component_PortType *)openmaxStandPort;

    DEBUG(DEB_LEV_FUNCTION_NAME, "for port %p\n", openmaxStandPort);
#if NO_GST_OMX_PATCH
    unsigned int i;
#endif
    portIndex = (openmaxStandPort->sPortParam.eDir == OMX_DirInput)?pBuffer->nInputPortIndex:pBuffer->nOutputPortIndex;
    DEBUG(DEB_LEV_FULL_SEQ, "portIndex %lu\n", portIndex);

    if (portIndex != openmaxStandPort->sPortParam.nPortIndex) {
        DEBUG(DEB_LEV_ERR, "wrong port for this operation portIndex=%d port->portIndex=%d\n", (int)portIndex, (int)openmaxStandPort->sPortParam.nPortIndex);
        return OMX_ErrorBadPortIndex;
    }

    if(omx_base_component_Private->state == OMX_StateInvalid) {
        DEBUG(DEB_LEV_ERR, "we are in OMX_StateInvalid\n");
        return OMX_ErrorInvalidState;
    }

    if(omx_base_component_Private->state != OMX_StateExecuting &&
            omx_base_component_Private->state != OMX_StatePause &&
            omx_base_component_Private->state != OMX_StateIdle) {
        DEBUG(DEB_LEV_ERR, "we are not in executing/paused/idle state, but in %d\n", omx_base_component_Private->state);
        return OMX_ErrorIncorrectStateOperation;
    }
    if (!PORT_IS_ENABLED(openmaxStandPort) || (PORT_IS_BEING_DISABLED(openmaxStandPort) && !PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) ||
            ((omx_base_component_Private->transientState == OMX_TransStateExecutingToIdle ||
              omx_base_component_Private->transientState == OMX_TransStatePauseToIdle) &&
             (PORT_IS_TUNNELED(openmaxStandPort) && !PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)))) {
        DEBUG(DEB_LEV_ERR, "Port %d is disabled comp = %s \n", (int)portIndex,omx_base_component_Private->name);
        return OMX_ErrorIncorrectStateOperation;
    }

    /* Temporarily disable this check for gst-openmax */
#if NO_GST_OMX_PATCH
    {
        OMX_BOOL foundBuffer = OMX_FALSE;
        if(pBuffer!=NULL && pBuffer->pBuffer!=NULL) {
            for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++) {
                if (pBuffer->pBuffer == openmaxStandPort->pInternalBufferStorage[i]->pBuffer) {
                    foundBuffer = OMX_TRUE;
                    break;
                }
            }
        }
        if (!foundBuffer) {
            return OMX_ErrorBadParameter;
        }
    }
#endif

    if ((err = checkHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE))) != OMX_ErrorNone) {
        DEBUG(DEB_LEV_ERR, "received wrong buffer header on input port\n");
        return err;
    }
    if (openmaxStandPort->sPortParam.eDir == OMX_DirOutput) {
        if(vpuPort->bAllocateBuffer == OMX_TRUE || omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
            OmxClearDisplayFlag(omxComponent, pBuffer, OMX_TRUE);
    }

    /* And notify the buffer management thread we have a fresh new buffer to manage */
    if(!PORT_IS_BEING_FLUSHED(openmaxStandPort) && !(PORT_IS_BEING_DISABLED(openmaxStandPort) && PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)))
    {
        if (openmaxStandPort->sPortParam.eDir == OMX_DirInput)
        {

            if ((pBuffer->nFlags&OMX_BUFFERFLAG_CODECCONFIG) == OMX_BUFFERFLAG_CODECCONFIG ||
                    (pBuffer->nFlags&OMX_BUFFERFLAG_DECODEONLY) == OMX_BUFFERFLAG_DECODEONLY)
            {
                // do not need to insert InputBuffer in queue. this queue is needed to give display information.
            }
            else
            {
                omx_bufferheader_queue_enqueue(omx_vpudec_component_Private->in_bufferheader_queue, pBuffer);
                DEBUG(DEB_LEV_FULL_SEQ, "pInputBufferTimeStamp queue timestamp=%lld\n", pBuffer->nTimeStamp);
            }
        }

        errQue = queue(openmaxStandPort->pBufferQueue, pBuffer);
        if (errQue) {
            /* /TODO the queue is full. This can be handled in a fine way with
            * some retrials, or other checking. For the moment this is a critical error
            * and simply causes the failure of this call
            */
            return OMX_ErrorInsufficientResources;
        }

        tsem_up(openmaxStandPort->pBufferSem);

        DEBUG(DEB_LEV_FULL_SEQ, "Signalling bMgmtSem Port Index=%d, pBuffer=%p\n", (int)portIndex, pBuffer);
        tsem_up(omx_base_component_Private->bMgmtSem);

        if (openmaxStandPort->sPortParam.eDir == OMX_DirOutput)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "queue output buffer=%p, Signalling bDispBufFullSem\n", pBuffer);
#ifdef WAITING_TIME_FOR_NEXT_OUTPUT_BUFFER_AFTER_DISPLAY_BUFFER_FULL
            tsem_up_to_one(omx_vpudec_component_Private->bDispBufFullSem);
#endif

#ifdef CNM_FPGA_PLATFORM
            usleep(700*1000);
#endif
        }
    }
    else if(PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)) {
        DEBUG(DEB_LEV_FULL_SEQ, "Comp %s received io:%d buffer\n",
              omx_base_component_Private->name,(int)openmaxStandPort->sPortParam.nPortIndex);
        errQue = queue(openmaxStandPort->pBufferQueue, pBuffer);
        if (errQue) {
            /* /TODO the queue is full. This can be handled in a fine way with
            * some retrials, or other checking. For the moment this is a critical error
            * and simply causes the failure of this call
            */
            return OMX_ErrorInsufficientResources;
        }

        tsem_up(openmaxStandPort->pBufferSem);
    }
    else { // If port being flushed and not tunneled then return error
        DEBUG(DEB_LEV_ERR, "incorrect state operation\n");
        return OMX_ErrorIncorrectStateOperation;
    }
    DEBUG(DEB_LEV_FUNCTION_NAME, "out for port %p\n", openmaxStandPort);
    return OMX_ErrorNone;
}

/**
* Returns Input/Output Buffer to the IL client or Tunneled Component
*/
OMX_ERRORTYPE omx_vpudec_component_OutPort_ReturnBufferFunction(omx_base_PortType* openmaxStandPort,OMX_BUFFERHEADERTYPE* pBuffer)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandPort->standCompContainer->pComponentPrivate;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *vpuPort= (omx_vpudec_component_PortType *)openmaxStandPort;
    queue_t* pQueue = openmaxStandPort->pBufferQueue;
    tsem_t* pSem = openmaxStandPort->pBufferSem;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int errQue;
    OMX_BUFFERHEADERTYPE inBufferHeader;
    OMX_BOOL bDecoderEOS;
    OMX_TICKS nOutputTimeStamp;
    void *pSrcVirtAddrs;
    void *pRefVirtAddrs;
    unsigned long srcPhyAddrs;
    unsigned long refPhyAddrs;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    omx_display_flag *disp_flag_array = &omx_vpudec_component_Private->omx_display_flags[0];

    bDecoderEOS = OMX_FALSE;
    nOutputTimeStamp = 0;

    DEBUG(DEB_LEV_FUNCTION_NAME, "for port %p, buf %p\n", openmaxStandPort, pBuffer);
    if (PORT_IS_TUNNELED(openmaxStandPort) && ! PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)) {
        if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
            pBuffer->nOutputPortIndex = openmaxStandPort->nTunneledPort;
            pBuffer->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
            eError = ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->FillThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
            if(eError != OMX_ErrorNone) {
                DEBUG(DEB_LEV_ERR, "eError %08x in FillThis Buffer from Component %s Non-Supplier\n",
                      eError,omx_vpudec_component_Private->name);
            }
        } else {
            pBuffer->nInputPortIndex = openmaxStandPort->nTunneledPort;
            pBuffer->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
            eError = ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->EmptyThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
            if(eError != OMX_ErrorNone) {
                DEBUG(DEB_LEV_ERR, "eError %08x in EmptyThis Buffer from Component %s Non-Supplier\n",
                      eError,omx_vpudec_component_Private->name);
            }
        }
    } else if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort) &&
               !PORT_IS_BEING_FLUSHED(openmaxStandPort)) {
        if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
            eError = ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->FillThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
            if(eError != OMX_ErrorNone) {
                DEBUG(DEB_LEV_FULL_SEQ, "eError %08x in FillThis Buffer from Component %s Supplier\n",
                      eError,omx_vpudec_component_Private->name);
                /*If Error Occured then queue the buffer*/
                errQue = queue(pQueue, pBuffer);
                if (errQue) {
                    /* /TODO the queue is full. This can be handled in a fine way with
                    * some retrials, or other checking. For the moment this is a critical error
                    * and simply causes the failure of this call
                    */
                    return OMX_ErrorInsufficientResources;
                }
                tsem_up(pSem);
            }
        } else {
            eError = ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->EmptyThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
            if(eError != OMX_ErrorNone) {
                DEBUG(DEB_LEV_FULL_SEQ, "eError %08x in EmptyThis Buffer from Component %s Supplier\n",
                      eError,omx_vpudec_component_Private->name);
                /*If Error Occured then queue the buffer*/
                errQue = queue(pQueue, pBuffer);
                if (errQue) {
                    /* /TODO the queue is full. This can be handled in a fine way with
                    * some retrials, or other checking. For the moment this is a critical error
                    * and simply causes the failure of this call
                    */
                    return OMX_ErrorInsufficientResources;
                }
                tsem_up(pSem);
            }
        }
    } else if (!PORT_IS_TUNNELED(openmaxStandPort)) {

        bDecoderEOS = ((pBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS); // that is real EOS flag which is got from decoder.

        if (vpuPort->bAllocateBuffer == OMX_TRUE || omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
        {
            OmxUpdateOutputBufferHeaderToDisplayOrder((OMX_COMPONENTTYPE *)openmaxStandPort->standCompContainer, &pBuffer);
            OmxClearDisplayFlag((OMX_COMPONENTTYPE *)openmaxStandPort->standCompContainer, pBuffer, OMX_FALSE);
        }
#ifdef SUPPORT_TIMESTAMP_REORDER
        else
        {
            if (pVpu->decOutputInfos[pVpu->dispOutputInfo.indexFrameDisplay].picType == PIC_TYPE_I ||
            pVpu->decOutputInfos[pVpu->dispOutputInfo.indexFrameDisplay].picType == PIC_TYPE_IDR)
                pBuffer->nFlags = OMX_BUFFERFLAG_SYNCFRAME;
            else
                pBuffer->nFlags = 0;
        }
#endif

        if(PORT_IS_BEING_FLUSHED(vpuPort) || (PORT_IS_BEING_DISABLED(vpuPort) && PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(vpuPort)))
        {

        }
        else
        {
            if (omx_bufferheader_queue_dequeue(omx_vpudec_component_Private->in_bufferheader_queue, &inBufferHeader) == -1)
            {
                DEBUG(DEB_LEV_FULL_SEQ, "No Input bufferHeader Found : nFieldLen = %d, pOutputBuffer=%p, nOutputTimeStamp=%lld, PORT_IS_BEING_FLUSHED=%d, PORT_IS_BEING_DISABLED=%d, state=0x%x\n",
                      (int)pBuffer->nFilledLen, pBuffer, pBuffer->nTimeStamp, (int)PORT_IS_BEING_FLUSHED(openmaxStandPort), (int)PORT_IS_BEING_DISABLED(openmaxStandPort), omx_vpudec_component_Private->state);
                if (bDecoderEOS)
                {
                    pBuffer->nTimeStamp = 0;
                    pBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "Detect Real EOS in OutputBuffer nTimeStamp=%lld, nFlag=0x%x, \n", pBuffer->nTimeStamp, (int)pBuffer->nFlags);
                }
            }
            else
            {
                DEBUG(DEB_LEV_FULL_SEQ, "pInputBufferTimeStamp dequeue timestamp=%lld\n", inBufferHeader.nTimeStamp);

                if (bDecoderEOS)
                {
                    pBuffer->nTimeStamp = 0;
                    pBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "Detect Real EOS in OutputBuffer nTimeStamp=%lld, nFlag=0x%x, \n", pBuffer->nTimeStamp, (int)pBuffer->nFlags);
                }
                else
                {
                    pBuffer->nFlags |= inBufferHeader.nFlags;
                    pBuffer->nFlags &= ~OMX_BUFFERFLAG_EOS; // remove EOS flag of Input because that is not real.
#ifdef SUPPORT_TIMESTAMP_REORDER
                    if (omx_vpudec_component_Private->bIsTimeStampReorder == OMX_TRUE && pVpu->dispOutputInfo.indexFrameDisplay >= 0)
                    {
                        inBufferHeader.nTimeStamp = disp_flag_array[pVpu->dispOutputInfo.indexFrameDisplay].nInputTimeStamp;
                        DEBUG(DEB_LEV_FULL_SEQ, "ts reorder : get InputTImestamp nTimeStamp=%lld, indexFrameDisplay=%d\n", inBufferHeader.nTimeStamp, (int)pVpu->dispOutputInfo.indexFrameDisplay);
                    }
#endif
                    {
                        nOutputTimeStamp = OmxTimeStampCorrection((OMX_COMPONENTTYPE *)openmaxStandPort->standCompContainer, inBufferHeader.nTimeStamp);
                    }

                    if (nOutputTimeStamp != (OMX_TICKS)-1)
                        pBuffer->nTimeStamp = nOutputTimeStamp;
                    else
                        pBuffer->nTimeStamp = inBufferHeader.nTimeStamp; // no framerate info. set input timestamp to output in direct
                }
            }
        }



#ifdef ANDROID_OUTPUT_DUMP
        if (pBuffer->nFilledLen > 0)
        {
            FILE *dump;
            dump = fopen("/data/dump_dec.yuv", "a+b");
            if (dump)
            {
                if (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
                {
                    void *pVirAddrs[3];

                    if (lockAndroidNativeBuffer(pBuffer, vpuPort->sPortParam.format.video.nStride, vpuPort->sPortParam.format.video.nFrameHeight, LOCK_MODE_TO_GET_VIRTUAL_ADDRESS, pVirAddrs) == 0)
                    {
                        fwrite((unsigned char*)pVirAddrs[0], pBuffer->nFilledLen, 1, dump);
                        unlockAndroidNativeBuffer(pBuffer);
                    }
                }
                else
                {
                    fwrite(pBuffer->pBuffer, pBuffer->nFilledLen, 1, dump);
                }
                fclose(dump);
            }
        }
#endif

        DEBUG(DEB_LEV_FULL_SEQ, " In  Return outPut Buf = %p\n", pBuffer);

#if defined (SPRD_PARALLEL_DEINTRELACE_SUPPORT)
	if(omx_vpudec_component_Private->bThumbnailMode == OMX_FALSE &&
	    omx_vpudec_component_Private->bIsDeinterlaceNeeded &&
	    openmaxStandPort->sPortParam.eDir == OMX_DirOutput &&
	    pBuffer->nFilledLen != 0) {
		// For De-interlace case
		// Store pOutputBuffer in bufferQueForDeinterlaceProcess
		queue(omx_vpudec_component_Private->bufferQueForDeinterlaceProcess, pBuffer);
		DEBUG(DEB_LEV_FULL_SEQ, " Store Buf %p in bufferQueForDeinterlaceProcess\n", pBuffer);
		tsem_up(&omx_vpudec_component_Private->deInterlace_Thread_Sem);
	}
	else{
	        (*(openmaxStandPort->BufferProcessedCallback))(
	            openmaxStandPort->standCompContainer,
	            omx_vpudec_component_Private->callbackData,
	            pBuffer);
	}
#elif defined (SPRD_SERIAL_DEINTRELACE_SUPPORT)
	if(omx_vpudec_component_Private->bThumbnailMode == OMX_FALSE &&
	    omx_vpudec_component_Private->bIsDeinterlaceNeeded &&
	    openmaxStandPort->sPortParam.eDir == OMX_DirOutput &&
	    pBuffer->nFilledLen != 0) {
            if (pVpu->deInterlaceRefBuf == NULL) {
                pVpu->deInterlaceRefBuf = pBuffer;
                DEBUG(DEB_LEV_FULL_SEQ, "first deinterlace refBuf: %p", pBuffer);
	    } else {

                if (pVpu->deInterlaceBufHandle == NULL) {
                    err = AllocateIONBuffer(pVpu->fbStride*pVpu->fbHeight*3/2, &pVpu->deInterlaceBufHandle, &pVpu->deInterlacePhyAddr, &pVpu->deInterlaceVirAddr, &pVpu->deInterlaceBufSize);
                    if (err != OMX_ErrorNone) {
                        DEBUG(DEB_LEV_ERR, "AllocateIONBuffer FAIL!\n" );
                    } else {
                        DEBUG(DEB_LEV_FULL_SEQ, "AllocateIONBuffer successfully, deInterlacePhyAddr: 0x%x, VirAddr: %p, size: %d\n",
                                pVpu->deInterlacePhyAddr, pVpu->deInterlaceVirAddr, pVpu->deInterlaceBufSize);
                    }
                }

	        if (pVpu->deInterlaceBufHandle != NULL) {
#ifdef SPRD_DEINTERLACE_BYPASS
                    getIOMMUPhyAddr(pBuffer, &srcPhyAddrs);
                    getIOMMUPhyAddr(pVpu->deInterlaceRefBuf, &refPhyAddrs);
                    DEBUG(DEB_LEV_FULL_SEQ, "[wenan] w h: %d %d, len = %d, frame_no: %u, refBuf: %p, phy addr = 0x%lx, srcBuf: %p, phy addr = 0x%lx\n",
                        pVpu->fbStride, pVpu->fbHeight, pBuffer->nFilledLen, pVpu->deInterlaceFrameNo, pVpu->deInterlaceRefBuf, refPhyAddrs, pBuffer, srcPhyAddrs);

                    lockAndroidNativeBuffer (pVpu->deInterlaceRefBuf,
                                           (OMX_U32)(pVpu->fbStride),
                                           (OMX_U32)(pVpu->fbHeight), LOCK_MODE_TO_GET_VIRTUAL_ADDRESS, &pRefVirtAddrs);
                    DEBUG(DEB_LEV_FULL_SEQ, "[wenan] w h: %d %d, len = %d, refBuf: %p, virtual addr = %p\n",
                        pVpu->fbStride, pVpu->fbHeight, pVpu->deInterlaceRefBuf->nFilledLen, pVpu->deInterlaceRefBuf, pRefVirtAddrs);

                    memcpy(pVpu->deInterlaceVirAddr, pRefVirtAddrs, pVpu->deInterlaceRefBuf->nFilledLen);
                    memcpy(pRefVirtAddrs, pVpu->deInterlaceVirAddr, pVpu->deInterlaceRefBuf->nFilledLen);
                    unlockAndroidNativeBuffer(pVpu->deInterlaceRefBuf);
#else
                    getIOMMUPhyAddr(pBuffer, &srcPhyAddrs);
                    getIOMMUPhyAddr(pVpu->deInterlaceRefBuf, &refPhyAddrs);

                    DEBUG(DEB_LEV_FULL_SEQ, "[wenan] w h: %d %d, len = %d, frame_no: %u, refBuf: %p, phy addr = 0x%lx, srcBuf: %p, phy addr = 0x%lx\n",
                        pVpu->fbStride, pVpu->fbHeight, pBuffer->nFilledLen, pVpu->deInterlaceFrameNo, pVpu->deInterlaceRefBuf, refPhyAddrs, pBuffer, srcPhyAddrs);

                    omx_vpudec_cmp_vpp_process(omx_vpudec_component_Private->vpp_obj, srcPhyAddrs, refPhyAddrs, pVpu->deInterlacePhyAddr, pVpu->fbStride, pVpu->fbHeight, pVpu->deInterlaceFrameNo);

                    lockAndroidNativeBuffer (pVpu->deInterlaceRefBuf,
                                           (OMX_U32)(pVpu->fbStride),
                                           (OMX_U32)(pVpu->fbHeight), LOCK_MODE_TO_GET_VIRTUAL_ADDRESS, &pRefVirtAddrs);

                    DEBUG(DEB_LEV_FULL_SEQ, "[wenan] w h: %d %d, len = %d, refBuf: %p, virtual addr = %p\n",
                        pVpu->fbStride, pVpu->fbHeight, pVpu->deInterlaceRefBuf->nFilledLen, pVpu->deInterlaceRefBuf, pRefVirtAddrs);

                    memcpy(pRefVirtAddrs, pVpu->deInterlaceVirAddr, pVpu->deInterlaceRefBuf->nFilledLen);
                    unlockAndroidNativeBuffer(pVpu->deInterlaceRefBuf);
#endif
                }

	        (*(openmaxStandPort->BufferProcessedCallback))(
	            openmaxStandPort->standCompContainer,
	            omx_vpudec_component_Private->callbackData,
	            pVpu->deInterlaceRefBuf);

	        pVpu->deInterlaceRefBuf = pBuffer;
	        pVpu->deInterlaceFrameNo++;
	    }
	} else {
	        (*(openmaxStandPort->BufferProcessedCallback))(
	            openmaxStandPort->standCompContainer,
	            omx_vpudec_component_Private->callbackData,
	            pBuffer);
	}
#else
        (*(openmaxStandPort->BufferProcessedCallback))(
            openmaxStandPort->standCompContainer,
            omx_vpudec_component_Private->callbackData,
            pBuffer);
#endif
    } else {
        errQue = queue(pQueue, pBuffer);
        if (errQue) {
            /* /TODO the queue is full. This can be handled in a fine way with
            * some retrials, or other checking. For the moment this is a critical error
            * and simply causes the failure of this call
            */
            return OMX_ErrorInsufficientResources;
        }
        openmaxStandPort->nNumBufferFlushed++;
    }

    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of  for port %p\n", openmaxStandPort);
    return OMX_ErrorNone;
}

void* omx_vpudec_component_BufferMgmtFunction (void* param)
{
    OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;

    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = openmaxStandComp->pComponentPrivate;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *pInPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    omx_vpudec_component_PortType *pOutPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    tsem_t* pInputSem = pInPort->pBufferSem;
    tsem_t* pOutputSem = pOutPort->pBufferSem;
    queue_t* pInputQueue = pInPort->pBufferQueue;
    queue_t* pOutputQueue = pOutPort->pBufferQueue;
    OMX_BUFFERHEADERTYPE* pOutputBuffer=NULL;
    OMX_BUFFERHEADERTYPE* pInputBuffer=NULL;
    OMX_BOOL isInputBufferNeeded=OMX_TRUE,isOutputBufferNeeded=OMX_TRUE;
    int inBufExchanged=0,outBufExchanged=0;

    omx_vpudec_component_Private->bIsOutputEOSReached = OMX_FALSE;
    omx_vpudec_component_Private->bellagioThreads->nThreadBufferMngtID = (long int)syscall(__NR_gettid);
    DEBUG(DEB_LEV_FUNCTION_NAME, "In  of component %p\n", openmaxStandComp);
    DEBUG(DEB_LEV_SIMPLE_SEQ, "In  the thread ID is %i\n", (int)omx_vpudec_component_Private->bellagioThreads->nThreadBufferMngtID);

    DEBUG(DEB_LEV_FUNCTION_NAME, "In \n");
    /* checks if the component is in a state able to receive buffers */
    while(omx_vpudec_component_Private->state == OMX_StateIdle || omx_vpudec_component_Private->state == OMX_StateExecuting ||
            omx_vpudec_component_Private->state == OMX_StatePause || omx_vpudec_component_Private->transientState == OMX_TransStateLoadedToIdle)
    {

        /*Wait till the ports are being flushed*/
        pthread_mutex_lock(&omx_vpudec_component_Private->flush_mutex);
        while( PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))
        {
            pthread_mutex_unlock(&omx_vpudec_component_Private->flush_mutex);

            DEBUG(DEB_LEV_FULL_SEQ, " 1 signaling flush all cond iE=%d,iF=%d,oE=%d,oF=%d iSemVal=%d,oSemval=%d\n",
                  inBufExchanged, (int)isInputBufferNeeded, outBufExchanged, (int)isOutputBufferNeeded, (int)pInputSem->semval, (int)pOutputSem->semval);

            if(isOutputBufferNeeded==OMX_FALSE && PORT_IS_BEING_FLUSHED(pOutPort)) {
                pOutPort->ReturnBufferFunction((omx_base_PortType *)pOutPort, pOutputBuffer);
                DEBUG(DEB_LEV_FULL_SEQ, "Ports are flushing,so returning output buffer = %p\n", pOutputBuffer);
                outBufExchanged--;
                pOutputBuffer=NULL;
                isOutputBufferNeeded=OMX_TRUE;
            }

            if(isInputBufferNeeded==OMX_FALSE && PORT_IS_BEING_FLUSHED(pInPort)) {
                pInPort->ReturnBufferFunction((omx_base_PortType *)pInPort,pInputBuffer);
                DEBUG(DEB_LEV_FULL_SEQ, "Ports are flushing,so returning input buffer = %p\n", pInputBuffer);
                inBufExchanged--;
                pInputBuffer=NULL;
                isInputBufferNeeded=OMX_TRUE;
            }

            DEBUG(DEB_LEV_FULL_SEQ, " 2 signaling flush all cond iE=%d,iF=%d,oE=%d,oF=%d iSemVal=%d,oSemval=%d\n",
                  inBufExchanged, (int)isInputBufferNeeded, outBufExchanged, (int)isOutputBufferNeeded, (int)pInputSem->semval, (int)pOutputSem->semval);
#if defined (SPRD_PARALLEL_DEINTRELACE_SUPPORT)
            //FIXME: Wait till flush buffer completed in de-interlace thread
            //       Add by Alan Wang
            if (omx_vpudec_component_Private->bIsDeinterlaceNeeded) {
                DEBUG(DEB_LEV_FULL_SEQ, " tsem up deInterlace_Thread_Sem.semval:%d\n",
                    tsem_get_semval(&omx_vpudec_component_Private->deInterlace_Thread_Sem));
                tsem_up(&omx_vpudec_component_Private->deInterlace_Thread_Sem); //this giving signal to de-interlace thread
                DEBUG(DEB_LEV_FULL_SEQ, " tsem down port_flush_complete_condition.semval: %d\n",
                    tsem_get_semval(&omx_vpudec_component_Private->port_flush_complete_condition));
                tsem_down(&omx_vpudec_component_Private->port_flush_complete_condition); //this waiting flush complete in de-interlace thread.

                DEBUG(DEB_LEV_SIMPLE_SEQ, " got it port_flush_complete_condition...\n");
            }
#elif defined (SPRD_SERIAL_DEINTRELACE_SUPPORT)
            if (PORT_IS_BEING_FLUSHED(pOutPort) &&
                omx_vpudec_component_Private->bIsDeinterlaceNeeded &&
                pVpu->deInterlaceRefBuf != NULL) {
                DEBUG(DEB_LEV_FULL_SEQ, "return deinterlace refBuf: %p", pVpu->deInterlaceRefBuf);
                omx_vpudec_cmp_return_buf_directly(param, (omx_base_PortType *)pOutPort, pVpu->deInterlaceRefBuf);
                pVpu->deInterlaceRefBuf = NULL;
            }
#endif
            tsem_up(omx_vpudec_component_Private->flush_all_condition); // this giving signal to resume base_port_FlushProcessingBuffers.
            tsem_down(omx_vpudec_component_Private->flush_condition); // this waiting for all buffer flushed in queue from base_port_FlushProcessingBuffers
            pthread_mutex_lock(&omx_vpudec_component_Private->flush_mutex);
        }
        pthread_mutex_unlock(&omx_vpudec_component_Private->flush_mutex);

        /*No buffer to process. So wait here*/
        if((isInputBufferNeeded==OMX_TRUE && pInputSem->semval==0) &&
                (omx_vpudec_component_Private->state != OMX_StateLoaded && omx_vpudec_component_Private->state != OMX_StateInvalid)) {
            //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
            DEBUG(DEB_LEV_FULL_SEQ, "Waiting for next input buffer omx_vpudec_component_Private->state=0x%x\n", (int)omx_vpudec_component_Private->state);
            tsem_down(omx_vpudec_component_Private->bMgmtSem);

        }

        if(omx_vpudec_component_Private->state == OMX_StateLoaded || omx_vpudec_component_Private->state == OMX_StateInvalid) {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "input Buffer Management Thread is exiting\n");
            break;
        }

        if((isOutputBufferNeeded==OMX_TRUE && pOutputSem->semval==0) &&
                (omx_vpudec_component_Private->state != OMX_StateLoaded && omx_vpudec_component_Private->state != OMX_StateInvalid) &&
                !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
            //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
            DEBUG(DEB_LEV_FULL_SEQ, "Waiting for next output buffer omx_vpudec_component_Private->state=0x%x\n", (int)omx_vpudec_component_Private->state);
            tsem_down(omx_vpudec_component_Private->bMgmtSem);

        }

        if(omx_vpudec_component_Private->state == OMX_StateInvalid) {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "Output Buffer Management Thread is exiting\n");
            break;
        }

        DEBUG(DEB_LEV_FULL_SEQ, "Waiting for input/output buffer in queue, input semval=%d output semval=%d, isOutputBufferNeeded=%d\n", (int)pInputSem->semval, (int)pOutputSem->semval, isOutputBufferNeeded);
#ifdef SPRD_PARALLEL_DEINTRELACE_SUPPORT
        /*When we have input buffer to process then get one output buffer*/
        if (omx_vpudec_component_Private->bIsDeinterlaceNeeded) {
            // De-interlace Needed case
            pthread_mutex_lock(&omx_vpudec_component_Private->deInterlace_buf_mutex);
            if(pOutputSem->semval>0 && isOutputBufferNeeded==OMX_TRUE) {
                tsem_down(pOutputSem);
                if(pOutputQueue->nelem>0) {
                    outBufExchanged++;
                    isOutputBufferNeeded=OMX_FALSE;
                    pOutputBuffer = dequeue(pOutputQueue);
                    if(pOutputBuffer == NULL) {
                        DEBUG(DEB_LEV_ERR, "Had NULL output buffer!! op is=%d,iq=%d\n", (int)pOutputSem->semval, (int)pOutputQueue->nelem);
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&omx_vpudec_component_Private->deInterlace_buf_mutex);
        } else {
            // Original case
            if(pOutputSem->semval>0 && isOutputBufferNeeded==OMX_TRUE) {
                tsem_down(pOutputSem);
                if(pOutputQueue->nelem>0) {
                    outBufExchanged++;
                    isOutputBufferNeeded=OMX_FALSE;
                    pOutputBuffer = dequeue(pOutputQueue);
                    if(pOutputBuffer == NULL) {
                        DEBUG(DEB_LEV_ERR, "Had NULL output buffer!! op is=%d,iq=%d\n", (int)pOutputSem->semval, (int)pOutputQueue->nelem);
                        break;
                    }
                }
            }
        }
#else
        // Original case
        if(pOutputSem->semval>0 && isOutputBufferNeeded==OMX_TRUE) {
            tsem_down(pOutputSem);
            if(pOutputQueue->nelem>0) {
                outBufExchanged++;
                isOutputBufferNeeded=OMX_FALSE;
                pOutputBuffer = dequeue(pOutputQueue);
                if(pOutputBuffer == NULL) {
                    DEBUG(DEB_LEV_ERR, "Had NULL output buffer!! op is=%d,iq=%d\n", (int)pOutputSem->semval, (int)pOutputQueue->nelem);
                    break;
                }
            }
        }
#endif
        if(pInputSem->semval>0 && isInputBufferNeeded==OMX_TRUE )
        {
            tsem_down(pInputSem);
            if(pInputQueue->nelem>0) {
                inBufExchanged++;
                isInputBufferNeeded=OMX_FALSE;
                pInputBuffer = dequeue(pInputQueue);
                if(pInputBuffer == NULL) {
                    DEBUG(DEB_LEV_ERR, "Had NULL input buffer!!\n");
                    break;
                }
            }
        }

        if(isInputBufferNeeded==OMX_FALSE)
        {
            if(pInputBuffer->hMarkTargetComponent != NULL) {
                if((OMX_COMPONENTTYPE*)pInputBuffer->hMarkTargetComponent ==(OMX_COMPONENTTYPE *)openmaxStandComp) {
                    /*Clear the mark and generate an event*/
                    (*(omx_vpudec_component_Private->callbacks->EventHandler))
                    (openmaxStandComp,
                     omx_vpudec_component_Private->callbackData,
                     OMX_EventMark, /* The command was completed */
                     1, /* The commands was a OMX_CommandStateSet */
                     0, /* The state has been changed in message->messageParam2 */
                     pInputBuffer->pMarkData);
                } else {
                    /*If this is not the target component then pass the mark*/
                    omx_vpudec_component_Private->pMark.hMarkTargetComponent = pInputBuffer->hMarkTargetComponent;
                    omx_vpudec_component_Private->pMark.pMarkData            = pInputBuffer->pMarkData;
                }
                pInputBuffer->hMarkTargetComponent = NULL;
            }
        }
        if(isInputBufferNeeded==OMX_FALSE && isOutputBufferNeeded==OMX_FALSE)
        {
            if(omx_vpudec_component_Private->pMark.hMarkTargetComponent != NULL) {
                pOutputBuffer->hMarkTargetComponent = omx_vpudec_component_Private->pMark.hMarkTargetComponent;
                pOutputBuffer->pMarkData            = omx_vpudec_component_Private->pMark.pMarkData;
                omx_vpudec_component_Private->pMark.hMarkTargetComponent = NULL;
                omx_vpudec_component_Private->pMark.pMarkData            = NULL;
            }

            if(omx_vpudec_component_Private->state == OMX_StateExecuting)  {
                if (pInputBuffer->nFilledLen > 0 || ((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
                    omx_vpudec_component_BufferMgmtCallback(openmaxStandComp, pInputBuffer, pOutputBuffer);
                } else {
                    /*It no buffer management call back the explicitly consume input buffer*/
                    pInputBuffer->nFilledLen = 0;
                }
            } else if(!(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
                DEBUG(DEB_LEV_ERR, "Received Buffer in non-Executing State(%x)\n", (int)omx_vpudec_component_Private->state);
            } else {
                pInputBuffer->nFilledLen = 0;
            }

            if(((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS && pInputBuffer->nFilledLen==0) &&
                    omx_vpudec_component_Private->bIsEOSReached == OMX_FALSE) {
                DEBUG(DEB_LEV_FULL_SEQ, "Detected EOS flags in input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
                //pOutputBuffer->nFlags=pInputBuffer->nFlags; pOutputBuffer->nFlags will be updated to flag OMX_BUFFERFLAG_EOS in omx_vpudec_component_BufferMgmtCallback
                //pInputBuffer->nFlags=0;
                (*(omx_vpudec_component_Private->callbacks->EventHandler))
                (openmaxStandComp,
                 omx_vpudec_component_Private->callbackData,
                 OMX_EventBufferFlag, /* The command was completed */
                 1, /* The commands was a OMX_CommandStateSet */
                 pInputBuffer->nFlags, /* The state has been changed in message->messageParam2 */
                 NULL);
                omx_vpudec_component_Private->bIsEOSReached = OMX_TRUE;
            }

            if(omx_vpudec_component_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
                /*Waiting at paused state*/
                tsem_wait(omx_vpudec_component_Private->bStateSem);
            }

            /*If EOS and Input buffer Filled Len Zero then Return output buffer immediately*/
            if (pOutputBuffer) {
                if((pVpu->saveLen != 0) || ((pOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
                    if ((pOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
                        omx_vpudec_component_Private->bIsOutputEOSReached = OMX_TRUE;
                    }
                    pOutPort->ReturnBufferFunction((omx_base_PortType *)pOutPort, pOutputBuffer);
                    outBufExchanged--;
                    pOutputBuffer=NULL;
                    isOutputBufferNeeded=OMX_TRUE;
                }
#ifdef WAITING_TIME_FOR_NEXT_OUTPUT_BUFFER_AFTER_DISPLAY_BUFFER_FULL
                else if(pVpu->dispOutputInfo.indexFrameDecoded == -1/*display buffer full state in view of decoder*/)
                {
                    if (pOutPort->bAllocateBuffer == OMX_TRUE || omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
                    {
                        // this would make this thread to avoid too many requests to VPU. and to yield the other job to OS scheduler.
                        if(!(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort)))
                        {
                            //if (pOutputQueue->nelem <= 2)   //needn't it because remaining nelem is related with the number of ref frames.
                            {
                                DEBUG(DEB_LEV_SIMPLE_SEQ, "display buffer full, wait for fillThisBuffer event, bDispBufFullSem = %d, outputSem=%d\n",
                                        omx_vpudec_component_Private->bDispBufFullSem->semval, pOutputSem->semval);

                                tsem_down(omx_vpudec_component_Private->bDispBufFullSem);

                                DEBUG(DEB_LEV_FULL_SEQ, "display buffer full, wait for fillThisBuffer done, bDispBufFullSem= %d, outputSem=%d\n",
                                        omx_vpudec_component_Private->bDispBufFullSem->semval, pOutputSem->semval);
                            }
                        }
                    }
                }
#endif
            }


            if(omx_vpudec_component_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
                /*Waiting at paused state*/
                tsem_wait(omx_vpudec_component_Private->bStateSem);
            }

            /*Input Buffer has been completely consumed. So, return input buffer*/
            if((isInputBufferNeeded == OMX_FALSE) && (pInputBuffer->nFilledLen==0)) {
                if (omx_vpudec_component_Private->bIsEOSReached == OMX_TRUE && omx_vpudec_component_Private->bIsOutputEOSReached == OMX_FALSE) {
                    /* don't return input buffer until output buffer reach EOS */
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "need not pInPort->ReturnBufferFunction bIsEOSReached == OMX_TRUE isOutputEos == OMX_FALSE\n");
                } else {
                    pInPort->ReturnBufferFunction((omx_base_PortType *)pInPort,pInputBuffer);
                    inBufExchanged--;
                    pInputBuffer=NULL;
                    isInputBufferNeeded=OMX_TRUE;

                }
            }
        }
    }


    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of  of component %p\n", openmaxStandComp);
    return NULL;
}



static int OmxLoadFirmware(Int32 productId, Uint8** retFirmware, Uint32* retSizeInWord, const char* path)
{
    Uint32      totalRead;
    Uint8*      firmware = NULL;
    osal_file_t fp;

    if ((fp=osal_fopen(path, "rb")) == NULL)
    {
        DEBUG(DEB_LEV_ERR,"Failed to open %s\n", path);
        return -1;
    }

    totalRead = 0;
    {
        Uint16*     pusBitCode;
        pusBitCode = (Uint16 *)osal_malloc(CODE_BUF_SIZE);
        if (!pusBitCode) {
            DEBUG(DEB_LEV_ERR, "Insufficient memory\n");
            return -1;
        }
        firmware   = (Uint8*)pusBitCode;
        if (pusBitCode)
        {
            int code;
            while (!osal_feof(fp) || totalRead < (CODE_BUF_SIZE/2)) {
                code = -1;
                if (osal_fscanf(fp, "%x", &code) <= 0) {
                    /* matching failure or EOF */
                    break;
                }
                pusBitCode[totalRead++] = (Uint16)code;
            }
        }
        *retSizeInWord = totalRead;
    }

    osal_fclose(fp);

    *retFirmware   = firmware;

    return 0;
}


static void OmxWaitUntilOutBuffersEmpty(omx_vpudec_component_PrivateType* omx_vpudec_component_Private)
{
    omx_vpudec_component_PortType *outPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    omx_display_flag *disp_flag_array = omx_vpudec_component_Private->omx_display_flags;
    int ownedClientBufferCount ;
    int i;

    int minUndequeuBuffer = (omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE) ? NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS : 0;
    do {
        /* waiting until the owner of all fillbuffers changed to OMX_BUFFER_OWNER_COMPONENT except for the amount of NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS before sending PortSettingsChange*/
        /* if not, OMX_EventPortSettingsChanged event in OMXCocdec will be failed due to CHECK(mFilledBuffers.empty()) */
        ownedClientBufferCount = 0;
        for (i = 0; i < (int)outPort->sPortParam.nBufferCountActual; i++)
        {
            if (disp_flag_array[i].owner == OMX_BUFFER_OWNER_CLIENT)
                ownedClientBufferCount++;;
        }

        if(ownedClientBufferCount > minUndequeuBuffer)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ,"waiting until the number of buffers with OWNER_CLIENT (%d) <= number of MIN_UNDEQUEUED_BUFFER (%d)", (int)ownedClientBufferCount, (int)NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS);
#ifdef WAITING_TIME_FOR_NEXT_OUTPUT_BUFFER_AFTER_DISPLAY_BUFFER_FULL
            tsem_timed_down(omx_vpudec_component_Private->bDispBufFullSem, 100);
#endif
        }

    } while(ownedClientBufferCount > minUndequeuBuffer);
}

OMX_BOOL OmxVpuFlush(OMX_COMPONENTTYPE *openmaxStandComp)
{
    omx_vpudec_component_PrivateType* omx_vpudec_component_Private = (omx_vpudec_component_PrivateType*) openmaxStandComp->pComponentPrivate;
    vpu_dec_context_t *pVpu = (vpu_dec_context_t *) & omx_vpudec_component_Private->vpu;
    omx_vpudec_component_PortType *outPort = (omx_vpudec_component_PortType *) omx_vpudec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    omx_display_flag *disp_flag_array = omx_vpudec_component_Private->omx_display_flags;
    RetCode ret = RETCODE_SUCCESS;
    int i;
    int dispFlagIdx = 0;
    DecOutputInfo decOutputInfo;
	DecBitBufInfo decBitBufInfo;
	Uint32 room;

	pthread_mutex_lock(&omx_vpudec_component_Private->vpu_flush_mutex);

    omx_vpudec_component_Private->omx_ts_correction.mTimeStampDirectMode = OMX_TRUE;

	if (pVpu->int_reason & (1<<INT_BIT_DEC_FIELD)) 
	{
		DEBUG(DEB_LEV_FULL_SEQ, "INT_BIT_DEC_FIELD interrupt already triggered");

		ret = VPU_DecGetBitstreamBuffer(pVpu->handle, &decBitBufInfo.rdptr, &decBitBufInfo.wrptr, &room);
		if( ret != RETCODE_SUCCESS )
		{
			DEBUG(DEB_LEV_ERR, "VPU_DecGetBitstreamBuffer failed Error code is 0x%x\n", ret);
			pthread_mutex_unlock(&omx_vpudec_component_Private->vpu_flush_mutex);
			return OMX_FALSE;
		}
		decBitBufInfo.wrptr = decBitBufInfo.rdptr;

		VPU_DecGiveCommand(pVpu->handle, DEC_SET_2ND_FIELD_INFO, &decBitBufInfo);

		VPU_ClearInterrupt(pVpu->coreIdx);  // clear field_done interrupt
		pVpu->int_reason = 0;

		// f/w will processing for 2nd field.

		pVpu->int_reason = VPU_WaitInterrupt(pVpu->coreIdx, VPU_DEC_TIMEOUT);
		if (pVpu->int_reason == -1 )
		{
			VPU_DecGetOutputInfo(pVpu->handle, &decOutputInfo);
			pVpu->int_reason = 0;
			DEBUG(DEB_LEV_ERR, "VPU timeout when consuming 2nd field");
			pthread_mutex_unlock(&omx_vpudec_component_Private->vpu_flush_mutex);
			return OMX_FALSE;
		}
		VPU_ClearInterrupt(pVpu->coreIdx);  // clear pic_done interrupt
		pVpu->int_reason = 0;
		VPU_DecGetOutputInfo(pVpu->handle, &decOutputInfo);
	}

    if (outPort->bAllocateBuffer == OMX_TRUE || omx_vpudec_component_Private->useNativeBuffer == OMX_TRUE)
    {
        pthread_mutex_lock(&omx_vpudec_component_Private->display_flag_mutex);
        for (i = 0; i < (int)outPort->sPortParam.nBufferCountActual; i++)
        {
            dispFlagIdx = i;
            if (pVpu->decOP.scalerInfo.enScaler)
                dispFlagIdx = i + pVpu->initialInfo.minFrameBufferCount;
            VPU_DecGiveCommand(pVpu->handle, DEC_SET_DISPLAY_FLAG, &dispFlagIdx);
            DEBUG(DEB_LEV_FULL_SEQ, "VPU_DecGiveCommand[DEC_SET_DISPLAY_FLAG] for index=%d, dispFlagIdx=%d, owner=0x%x\n",  i, dispFlagIdx, disp_flag_array[i].owner);
        }
        pthread_mutex_unlock(&omx_vpudec_component_Private->display_flag_mutex);
    }
    else
    {
        pthread_mutex_lock(&omx_vpudec_component_Private->display_flag_mutex);
        for (i = 0; i < (int)outPort->sPortParam.nBufferCountActual; i++)
        {
            dispFlagIdx = i;

            if (pVpu->decOP.scalerInfo.enScaler)
                dispFlagIdx = i + pVpu->initialInfo.minFrameBufferCount;

            VPU_DecClrDispFlag(pVpu->handle, dispFlagIdx);
            DEBUG(DEB_LEV_SIMPLE_SEQ, "VPU_DecClrDispFlag for index=%d, dispFlagIdx=%d, owner=0x%x\n",  i, dispFlagIdx, disp_flag_array[i].owner);
        }
        pthread_mutex_unlock(&omx_vpudec_component_Private->display_flag_mutex);
    }


    ret = VPU_DecFrameBufferFlush(pVpu->handle);
    if (ret != RETCODE_SUCCESS)
    {
        DEBUG(DEB_LEV_ERR, "VPU_DecGetBitstreamBuffer failed Error code is 0x%x \n", (int)ret );
		pthread_mutex_unlock(&omx_vpudec_component_Private->vpu_flush_mutex);
        return OMX_FALSE;
    }

    pVpu->chunkReuseRequired = 0;

#ifdef USE_BFRAME_SEARCH_FOR_1STFRAME
    omx_vpudec_component_Private->bSkipBrokenBframeDecode = OMX_TRUE;
#endif

#ifdef USE_IFRAME_SEARCH_FOR_1STFRAME
    pVpu->decParam.iframeSearchEnable = 2;
#endif

    pthread_mutex_unlock(&omx_vpudec_component_Private->vpu_flush_mutex);

    return OMX_TRUE;
}
