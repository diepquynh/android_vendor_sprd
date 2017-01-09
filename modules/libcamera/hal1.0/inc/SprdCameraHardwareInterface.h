/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ANDROID_HARDWARE_SPRD_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_SPRD_CAMERA_HARDWARE_H

#if(MINICAMERA != 1)
#include <utils/threads.h>
#include <pthread.h>
#include <semaphore.h>
extern "C" {
//    #include <linux/android_pmem.h>
}
#endif
#include <sys/types.h>

#include "cmr_common.h"
#include <utils/threads.h>
#if(MINICAMERA != 1)
#include <utils/RefBase.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <MemoryHeapIon.h>
#include <hardware/camera.h>
#include <hardware/gralloc.h>
#else
#include <MemoryHeapIon.h>
#include <hardware/camera.h>
#endif
#include <camera/CameraParameters.h>
#include "SprdCameraParameters.h"
#include "cmr_oem.h"
#include "cmr_msg.h"

#if(MINICAMERA == 1)
#include "fb_op.h"
#endif
#include "sprd_dma_copy_k.h"
#include <utils/List.h>


namespace android {

#if(MINICAMERA != 1)
typedef void (*preview_callback)(sp<MemoryBase>, void * );
typedef void (*recording_callback)(sp<MemoryBase>, void *);
typedef void (*shutter_callback)(void *);
typedef void (*raw_callback)(sp<MemoryBase> , void *);
typedef void (*jpeg_callback)(sp<MemoryBase>, void *);
typedef void (*autofocus_callback)(bool, void *);
#endif

typedef struct sprd_camera_memory {
	camera_memory_t *camera_memory;
	MemoryHeapIon *ion_heap;
	uintptr_t phys_addr;
	uint32_t phys_size;
	void *handle;
	void *data;
	bool busy_flag;
}sprd_camera_memory_t;

struct ZslBufferQueue {
	camera_frame_type frame;
	sprd_camera_memory_t* heap_array;
	uint32_t valid;
};

typedef struct {
	void *addr_phy;
	void *addr_vir;
	uint32_t valid;
	void *zsl_private;
}hal_mem_info_t;

#define MAX_SUB_RAWHEAP_NUM 10
#define MAX_LOOP_COLOR_COUNT 3
#define MAX_Y_UV_COUNT 2

class SprdCameraHardware : public virtual RefBase {
public:
	SprdCameraHardware(int cameraId);
	virtual                      ~SprdCameraHardware();
	inline int                   getCameraId() const;
	virtual void                 release();
	virtual status_t             startPreview();
	virtual void                 stopPreview();
	virtual bool                 previewEnabled();
	virtual status_t             setPreviewWindow(preview_stream_ops *w);
	virtual status_t             startRecording();
	virtual void                 stopRecording();
	virtual void                 releaseRecordingFrame(const void *opaque);
	virtual bool                 recordingEnabled();
	virtual status_t             takePicture();
	virtual status_t             cancelPicture();
	virtual status_t             setTakePictureSize(uint32_t width, uint32_t height);
	virtual status_t             autoFocus();
	virtual status_t             cancelAutoFocus();
	virtual status_t             setParameters(const SprdCameraParameters& params);
	virtual SprdCameraParameters getParameters();
	virtual void                 setCallbacks(camera_notify_callback notify_cb,
						camera_data_callback data_cb,
						camera_data_timestamp_callback data_cb_timestamp,
						camera_request_memory get_memory,
						void *user);
	virtual void                 enableMsgType(int32_t msgType);
	virtual void                 disableMsgType(int32_t msgType);
	virtual bool                 msgTypeEnabled(int32_t msgType);
	virtual status_t             sendCommand(int32_t cmd, int32_t arg1, int32_t arg2);
	virtual status_t             storeMetaDataInBuffers(bool enable);
	virtual status_t             dump(int fd) const;
	void                            setCaptureRawMode(bool mode);
	void		antiShakeParamSetup();
	int		uv420CopyTrim(struct _dma_copy_cfg_tag dma_copy_cfg);
	int		displayCopy(uintptr_t dst_phy_addr, uintptr_t dst_virtual_addr,
				uintptr_t src_phy_addr, uintptr_t src_virtual_addr, uint32_t src_w, uint32_t src_h);
	int uv420SPTouv420P(char* dst_virtual_addr,char *virtual_addr, uint32_t src_w, uint32_t src_h);

	enum camera_flush_mem_type_e {
		CAMERA_FLUSH_RAW_HEAP,
		CAMERA_FLUSH_RAW_HEAP_ALL,
		CAMERA_FLUSH_PREVIEW_HEAP,
		CAMERA_FLUSH_MAX
	};

	int 				 flush_buffer(camera_flush_mem_type_e  type, int index, void *v_addr, void *p_addr, int size);
	sprd_camera_memory_t* 		 allocCameraMem(int buf_size, uint32_t is_cache);

public:
	static int                   getPropertyAtv();
	static int                   getNumberOfCameras();
	static int                   getCameraInfo(int cameraId, struct camera_info *cameraInfo);
	static const CameraInfo      kCameraInfo[];
	static const CameraInfo      kCameraInfo3[];
	static int                   switch_monitor_thread_init(void *p_data);
	static int                   switch_monitor_thread_deinit(void *p_data);
	static void*                 switch_monitor_thread_proc(void *p_data);
	static int                   ZSLMode_monitor_thread_init(void *p_data);
	static int                   ZSLMode_monitor_thread_deinit(void *p_data);
	static cmr_int               ZSLMode_monitor_thread_proc(struct cmr_msg *message, void *p_data);
	static int                   pre_alloc_cap_mem_thread_init(void *p_data);
	static int                   pre_alloc_cap_mem_thread_deinit(void *p_data);
	static void*                 pre_alloc_cap_mem_thread_proc(void *p_data);
	inline bool                  isCameraInit();
	status_t                     waitSetParamsOK();
	uint32_t                     isPreAllocCapMem();

private:
	inline void                  print_time();

    // This class represents a heap which maintains several contiguous
    // buffers.  The heap may be backed by pmem (when pmem_pool contains
    // the name of a /dev/pmem* file), or by ashmem (when pmem_pool == NULL).
	struct MemPool : public RefBase {
		MemPool(int buffer_size, int num_buffers, int frame_size,
					int frame_offset, const char *name);
		virtual ~MemPool() = 0;
		void     completeInitialization();
		bool     initialized() const {
			if (mHeap != NULL) {
				if(MAP_FAILED != mHeap->base())
					return true;
				else
					return false;
			} else {
				return false;
			}
		}
		virtual status_t dump(int fd, const Vector<String16>& args) const;
		int mBufferSize;
		int mNumBuffers;
		int mFrameSize;
		int mFrameOffset;
		sp<MemoryHeapBase> mHeap;
#if(MINICAMERA != 1)
		sp<MemoryBase> *mBuffers;
#endif
		const char *mName;
	};
	struct OneFrameMem {
		sp<MemoryHeapIon> input_y_pmem_hp;
		uint32_t input_y_pmemory_size;
		int input_y_physical_addr ;
		unsigned char* input_y_virtual_addr;
		uint32_t width;
		uint32_t height;

	};
	enum shake_test_state {
		NOT_SHAKE_TEST,
		SHAKE_TEST,
	};
	struct ShakeTest {
		int diff_yuv_color[MAX_LOOP_COLOR_COUNT][MAX_Y_UV_COUNT];
		uint32_t                       mShakeTestColorCount;
		shake_test_state          mShakeTestState;
	};
	struct AshmemPool : public MemPool {
		AshmemPool(int buffer_size, int num_buffers, int frame_size,
						int frame_offset, const char *name);
	};
	bool allocatePreviewMemFromGraphics(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr);
	void cancelPreviewMemFromGraphics(cmr_u32 sum);
	int Callback_PreviewMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr);
	int Callback_VideoMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr);
	int Callback_ZslMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr);
	int Callback_CaptureMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr);
	int Callback_CapturePathMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr);
	int Callback_OtherMalloc(enum camera_mem_cb_type type, cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr);
	static int Callback_Malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr, cmr_u32 *sum_ptr, cmr_uint *phy_addr,
										cmr_uint *vir_addr, void* private_data);
	static int Callback_Free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
									cmr_uint *vir_addr, cmr_u32 sum, void* private_data);
	int Callback_PreviewFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum);
	int Callback_VideoFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum);
	int Callback_ZslFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum);
	int Callback_CaptureFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum);
	int Callback_CapturePathFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum);
	int Callback_OtherFree(enum camera_mem_cb_type type, cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum);
	int map(sprd_camera_memory_t* camera_memory, hal_mem_info_t *mem_info);
	int unmap(sprd_camera_memory_t* camera_memory, hal_mem_info_t *mem_info);
	void                  freeCameraMem(sprd_camera_memory_t* camera_memory);
	void                  clearCameraMem(sprd_camera_memory_t* camera_memory);
	uint32_t              getPreviewBufferID(buffer_handle_t *buffer_handle);
	uint32_t              getPreviewBufferIDForPhy(cmr_uint phy_addr);
	uint32_t              getVideoBufferIDForPhy(cmr_uint phy_addr);
	uint32_t              getZslBufferIDForPhy(cmr_uint phy_addr);
	uint32_t              releaseZslBuffer(struct camera_frame_type *frame);
	uint32_t              getZslBuffer(hal_mem_info_t *mem_info);
	void                  canclePreviewMem();
	int                     releasePreviewFrame();
	bool                  allocatePreviewMemByGraphics();
	bool                  allocatePreviewMem();
	void                  freePreviewMem();
	bool                  allocateCapMem(uint32_t mem_size);
	bool                  allocateCaptureMem(bool isPreAllocCapMem, uint32_t mem_size);
	void                  freeCaptureMem();
	uintptr_t             getRedisplayMem();
	void                  FreeReDisplayMem();
	status_t              checkSetParameters(const SprdCameraParameters& params);
	static void           camera_cb(camera_cb_type cb,
											const void *client_data,
											camera_func_type func,
											void* parm4);
	void                  sendPreviewFrameToApp(struct camera_frame_type *frame, camera_frame_metadata_t *metadata);
	void                  sendPreviewFrameToVideo(struct camera_frame_type *frame);
	void                  yuvConvertFormat(struct camera_frame_type *frame);
	void                  notifyShutter();
//	void                  receiveJpegPictureFragment(struct camera_jpeg_param *encInfo);
	void                  receiveJpegPosPicture(void);
	void                  receivePostLpmRawPicture(struct camera_frame_type *frame);
	void                  receiveRawPicture(struct camera_frame_type *frame);
	void                  PushAllZslBuffer(void);
	void                  receiveJpegPicture(struct camera_frame_type *frame);
	void                  receivePreviewFrame(struct camera_frame_type *frame);
	void                  receiveZslFrame(struct camera_frame_type *frame);
	void                  processZslFrame(void *p_data);
	void                  receivePreviewFDFrame(struct camera_frame_type *frame);
	void                  receiveCameraExitError(void);
	void                  receiveTakePictureError(void);
	void                  receiveJpegPictureError(void);
	void                  HandleStopCamera(enum camera_cb_type cb, void* parm4);
	void                  HandleStartCamera(enum camera_cb_type cb, void* parm4);
	void                  HandleStartPreview(enum camera_cb_type cb, void* parm4);
	void                  HandleStopPreview(enum camera_cb_type cb, void* parm4);
	void                  HandleTakePicture(enum camera_cb_type cb, void* parm4);
	void                  HandleEncode(enum camera_cb_type cb,  void* parm4);
	void                  HandleFocus(enum camera_cb_type cb, void* parm4);
	void                  HandleCancelPicture(enum camera_cb_type cb, void* parm4);
	bool                  HandleTakePictureInterLock(void);
	bool                  HandleAPPCallBackInterLock(void);
	bool                  HandleFDInterLock(void);
	ZslBufferQueue        popZSLQueue();
	int                   getZSLQueueFrameNum();
	void                  pushZSLQueue(ZslBufferQueue frame);
	void                  releaseZSLQueue();
	int                   getZSLSnapshotFrame(hal_mem_info_t *mem_info);

	enum Sprd_camera_state {
		SPRD_INIT,
		SPRD_IDLE,
		SPRD_ERROR,
		SPRD_PREVIEW_IN_PROGRESS,
		SPRD_FOCUS_IN_PROGRESS,
		SPRD_SET_PARAMS_IN_PROGRESS,
		SPRD_WAITING_RAW,
		SPRD_WAITING_JPEG,

		// internal states
		SPRD_INTERNAL_PREVIEW_STOPPING,
		SPRD_INTERNAL_CAPTURE_STOPPING,
		SPRD_INTERNAL_PREVIEW_REQUESTED,
		SPRD_INTERNAL_RAW_REQUESTED,
		SPRD_INTERNAL_STOPPING,
		SPRD_INTERNAL_CAPTURE_STOPPING_DONE,
	};

	enum state_owner {
		STATE_CAMERA,
		STATE_PREVIEW,
		STATE_CAPTURE,
		STATE_FOCUS,
		STATE_SET_PARAMS,
	};

	typedef struct _camera_state	{
		Sprd_camera_state      camera_state;
		Sprd_camera_state      preview_state;
		Sprd_camera_state      capture_state;
		Sprd_camera_state      focus_state;
		Sprd_camera_state      setParam_state;
	} camera_state;

	const char* getCameraStateStr(Sprd_camera_state s);
	Sprd_camera_state transitionState(Sprd_camera_state from,
						Sprd_camera_state to,
						state_owner owner,
						bool lock = true);
	void                            setCameraState(Sprd_camera_state state,
								state_owner owner = STATE_CAMERA);
	inline Sprd_camera_state        getCameraState();
	inline Sprd_camera_state        getPreviewState();
	inline Sprd_camera_state        getCaptureState();
	inline Sprd_camera_state        getFocusState();
	inline Sprd_camera_state        getSetParamsState();
	inline bool                     isCameraError();
	inline bool                     isCameraIdle();
	inline bool                     isPreviewing();
	inline bool                     isCapturing();
	bool                            WaitForPreviewStart();
	bool                            WaitForPreviewStop();
	bool                            WaitForCaptureStart();
	bool                            WaitForCaptureDone();
	bool                            WaitForCameraStart();
	bool                            WaitForCameraStop();
	bool                            WaitForFocusCancelDone();
	bool                            WaitForCaptureJpegState();
	bool                            isRecordingMode();
	void                            setRecordingMode(bool enable);
	bool                            startCameraIfNecessary();
	void                            getPictureFormat(int *format);
	takepicture_mode                getCaptureMode();
	bool                            getCameraLocation(camera_position_type *pt);
	status_t                        startPreviewInternal(bool isRecordingMode);
	void                            stopPreviewInternal();
	status_t                        cancelPictureInternal();
	virtual status_t                setParametersInternal(const SprdCameraParameters& params);
	bool                            initPreview();
	void                            deinitPreview();
	bool                            initCapture();
	void                            deinitCapture(bool isPreAllocCapMem);
	status_t                        initDefaultParameters();
	status_t                        setCameraParameters();
	status_t                        checkSetParametersEnvironment();
	status_t                        copyParameters(SprdCameraParameters& cur_params,
						const SprdCameraParameters& params);
	status_t                        checkSetParameters(const SprdCameraParameters& params,
							const SprdCameraParameters& oriParams);
	bool                            setCameraDimensions();
	void                            setCameraPreviewMode(bool isRecordMode);
	status_t                        set_ddr_freq(uint32_t mhzVal);
	bool                            displayOneFrame(uint32_t width,
								uint32_t height,
								uintptr_t phy_addr, char *frame_addr,
								uint32_t id);
	bool                            displayOneFrameForCapture(uint32_t width,
		                              uint32_t height,
		                              uintptr_t phy_addr,
		                              char *virtual_addr);
	void                            handleDataCallback(int32_t msg_type,
						uint32_t frame_index, unsigned int index,
						camera_frame_metadata_t *metadata, void *user,
						uint32_t isPrev);
	void                            handleDataCallbackTimestamp(int64_t timestamp,
						int32_t msg_type,
						sprd_camera_memory_t *data, unsigned int index,
						void *user);
	void                            handleFDDataCallback(int32_t msg_type,
						uint32_t frame_index, unsigned int index,
						camera_frame_metadata_t *metadata, void *user,
						uint32_t isPrev);
	void                            cameraBakMemCheckAndFree();
	void                            sync_bak_parameters();
	bool                            iSDisplayCaptureFrame();
	bool                            iSZslMode();
	bool                            checkPreviewStateForCapture();
	bool                            getLcdSize(uint32_t *width, uint32_t *height);
	bool                            switchBufferMode(uint32_t src, uint32_t dst);
	status_t                        checkFlashParameter(SprdCameraParameters& params);
	status_t                        checkEffectParameter(SprdCameraParameters& params);
	void                            setCameraPrivateData(void);
	void                            overwritePreviewFrame(camera_frame_type *frame);
	int                               overwritePreviewFrameMemInit(struct SprdCameraHardware::OneFrameMem *one_frame_mem_ptr);
	void                              shakeTestInit(ShakeTest *tmpShakeTest);
	void                              setShakeTestState(shake_test_state state);
	shake_test_state                  getShakeTestState();
	int                               IommuIsEnabled(void);
	int                               allocOneFrameMem(struct SprdCameraHardware::OneFrameMem *one_frame_mem_ptr);
	int                               relaseOneFrameMem(struct SprdCameraHardware::OneFrameMem *one_frame_mem_ptr);
	void                              cpu_dvfs_disable(uint8_t is_disable);
	void                              cpu_hotplug_disable(uint8_t is_disable);
	void                              prepareForPostProcess(void);
	void                              exitFromPostProcess(void);

	/* These constants reflect the number of buffers that libqcamera requires
	for preview and raw, and need to be updated when libqcamera
	changes.
	*/
	static const int                kPreviewBufferCount    = 8;
	static const int                kPreviewRotBufferCount = 8;
	static const int                kRawBufferCount        = 1;
	static const int                kJpegBufferCount       = 1;
	static const int                kRawFrameHeaderSize    = 0x0;
	static const int                kISPB4awbCount         = 16;
	Mutex                           mLock; // API lock -- all public methods
	Mutex                           mCallbackLock;
	Mutex                           mPreviewCbLock;
	Mutex                           mCaptureCbLock;
	Mutex                           mStateLock;
	Mutex                           mPreviewWindowLock;
	Condition                       mStateWait;
	Mutex                           mParamLock;
	Mutex                           mPreviewLock;
	Condition                       mParamWait;
	Mutex                           mCbPrevDataBusyLock;
	Mutex                           mPrevBakDataLock;
	Mutex                           mCbCapDataBusyLock;
	Mutex                           mCapBakDataLock;
	Mutex                           mPrevBufLock;
	Mutex                           mCapBufLock;
	Mutex                           mGraphBufCntLock;
	Mutex                           mVideoBufLock;
	Mutex                           mZslBufLock;
	Mutex                           mReleaseVideoBufLock;
	uint32_t                        mCapBufIsAvail;
#if(MINICAMERA == 1)
	fb_op  op;
#endif
	uint32_t                        mPreviewHeapSize;
	uint32_t                        mVideoHeapSize;
	uint32_t                        mZslHeapSize;
	uint32_t                        mPreviewHeapNum;
	uint32_t                        mVideoHeapNum;
	uint32_t                        mZslHeapNum;
	uint32_t                        mZslMapNum;
	uint32_t                        mZslChannelStatus;
	uint32_t                        mPreviewDcamAllocBufferCnt;
	sprd_camera_memory_t*           *mPreviewHeapArray;
	sprd_camera_memory_t*           *mVideoHeapArray;
	sprd_camera_memory_t*           *mZslHeapArray;
	sprd_camera_memory_t            mPreviewHeapInfoBak;
	uint32_t                        mPreviewHeapBakUseFlag;
	sprd_camera_memory_t            mRawHeapInfoBak;
	uint32_t                        mRawHeapBakUseFlag;
	uintptr_t                       mPreviewHeapArray_phy[kPreviewBufferCount+kPreviewRotBufferCount+1];
	uintptr_t                       mPreviewHeapArray_vir[kPreviewBufferCount+kPreviewRotBufferCount+1];
	uint32_t                        mPreviewHeapArray_size[kPreviewBufferCount+kPreviewRotBufferCount+1];
	uint32_t                        mVideoHeapArray_phy[kPreviewBufferCount+kPreviewRotBufferCount+1];
	uint32_t                        mVideoHeapArray_vir[kPreviewBufferCount+kPreviewRotBufferCount+1];
	uint32_t                        mVideoHeapArray_size[kPreviewBufferCount+kPreviewRotBufferCount+1];
	uintptr_t                       mZslHeapArray_phy[kPreviewBufferCount+kPreviewRotBufferCount+1];
	uintptr_t                       mZslHeapArray_vir[kPreviewBufferCount+kPreviewRotBufferCount+1];
	uint32_t                        mZslHeapArray_size[kPreviewBufferCount+kPreviewRotBufferCount+1];
#if(MINICAMERA != 1)
	buffer_handle_t                 *mPreviewBufferHandle[kPreviewBufferCount];
	buffer_handle_t                 *mPreviewCancelBufHandle[kPreviewBufferCount];
#endif
	bool                            mCancelBufferEb[kPreviewBufferCount];
	int32_t                         mGraphBufferCount[kPreviewBufferCount];
	/* mCommonHeapReserved for preview, video and zsl reserved buffer*/
	sprd_camera_memory_t*           mCommonHeapReserved;
	sprd_camera_memory_t*           mIspLscHeapReserved;
	sprd_camera_memory_t*           mIspB4awbHeapReserved[kISPB4awbCount];
	sprd_camera_memory_t*           mIspAntiFlickerHeapReserved;
	sprd_camera_memory_t*           mIspRawAemHeapReserved[kISPB4awbCount];
/* [Vendor_5/40] */
	sprd_camera_memory_t*           mIspRawAfmHeapReserved[kISPB4awbCount];


	sprd_camera_memory_t            *mRawHeap;
	uint32_t                        mRawHeapSize;

	sprd_camera_memory_t            *mSubRawHeapArray[MAX_SUB_RAWHEAP_NUM];
	uint32_t                        mSubRawHeapNum;
	uint32_t                        mSubRawHeapSize;

	sprd_camera_memory_t            *mPathRawHeapArray[MAX_SUB_RAWHEAP_NUM];
	uint32_t                        mPathRawHeapNum;
	uint32_t                        mPathRawHeapSize;

	uint32_t                        mFDAddr;
	camera_memory_t                 *mMetadataHeap;
	sprd_camera_memory_t            *mReDisplayHeap;
	//TODO: put the picture dimensions in the CameraParameters object;
	SprdCameraParameters            mParameters;
	SprdCameraParameters            mSetParameters;
	SprdCameraParameters            mSetParametersBak;
	uint32_t                        mPreviewHeight_trimy;
	uint32_t                        mPreviewWidth_trimx;
	int                             mPreviewHeight_backup;
	int                             mPreviewWidth_backup;
	int                             mPreviewHeight;
	int                             mPreviewWidth;
	int                             mVideoHeight;
	int                             mVideoWidth;
	int                             mRawHeight;
	int                             mRawWidth;
	int                             mPreviewFormat;//0:YUV422;1:YUV420;2:RGB;3:YUV420 3 plane,4:YVU420
	int                             mPictureFormat;//0:YUV422;1:YUV420;2:RGB;3:YUV420 3 plane,4:YVU420,5:JPEG
	int                             mPreviewStartFlag;
	uint32_t                        mIsDvPreview;
//#if defined(CONFIG_CAMERA_PREVIEW_YV12)
	uint32_t                        mIsYuv420p;
	void                            *mYV12Buf;
//#endif
	bool                            mRecordingMode;
	bool                            mBakParamFlag;

	nsecs_t                         mRecordingFirstFrameTime;
	uint32_t                        mZoomLevel;
	/* mJpegSize keeps track of the size of the accumulated JPEG.  We clear it
	when we are about to take a picture, so at any time it contains either
	zero, or the size of the last JPEG picture taken.
	*/
	uint32_t                        mJpegSize;
	camera_notify_callback          mNotify_cb;
	camera_data_callback            mData_cb;
	camera_data_timestamp_callback  mData_cb_timestamp;
	camera_request_memory           mGetMemory_cb;
	void                            *mUser;
	preview_stream_ops              *mPreviewWindow;
	static gralloc_module_t const   *mGrallocHal;
        static oem_module_t   *mHalOem;
	int32_t                         mMsgEnabled;
	bool                            mIsStoreMetaData;
	bool                            mIsFreqChanged;
	int32_t                         mCameraId;
	int32_t                         mFlashMode;
	volatile camera_state           mCameraState;
	int                             miSPreviewFirstFrame;
	takepicture_mode                mCaptureMode;
	bool                            mCaptureRawMode;
	bool                            mIsRotCapture;
	bool                            mFlashMask;
	bool                            mReleaseFLag;
	uint32_t                        mTimeCoeff;
	uint32_t                        mPreviewBufferUsage;
	uint32_t                        mOriginalPreviewBufferUsage;
	int                             mSetDDRFreqCount;
	uint32_t                        mSetDDRFreq;
	uint32_t                        mCaptureNum;
	uint32_t                        mCPURaised;
	int                             mVideoShotPushFlag;
	int                             mZslShotPushFlag;

	/*callback thread*/
	pthread_t                       mSwitchMonitorThread;
	uintptr_t                       mSwitchMonitorMsgQueHandle;
	uint32_t                        mSwitchMonitorInited;
	sem_t                           mSwitchMonitorSyncSem;
	uintptr_t                       mZSLModeMonitorMsgQueHandle;
	uint32_t                        mZSLModeMonitorInited;
	cmr_handle                      mCameraHandle;
	uint32_t                        mIsPreAllocCapMem;
	pthread_t                       mPreAllocCapMemThread;
	uint32_t                        mPreAllocCapMemInited;
	uint32_t                        mIsPreAllocCapMemDone;
	sem_t                           mPreAllocCapMemSemDone;
	bool                            mIsPerformanceTestable;
	ShakeTest                       mShakeTest;
	int                             mZSLFrameNum;
	List<ZslBufferQueue>            mZSLQueue;
	bool                            mIsRestartPreview;
	uint32_t                        mIsSupportCallback;
};
	enum {
		SPRD_CMD_START_BURST_TAKE = 1571,
		SPRD_CMD_DELETE_BURST_TAKE = 1573,
		SPRD_CMD_STOP_BURST_TAKE = 1572,
	};
}; // namespace android

#endif //ANDROID_HARDWARE_SPRD_CAMERA_HARDWARE_H


