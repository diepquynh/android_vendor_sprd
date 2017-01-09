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

//#define LOG_NDEBUG 0
#define LOG_TAG "SprdCamera3OEMIf"

#include <camera/Camera.h>
#include "SprdCamera3OEMIf.h"
#include <utils/Log.h>
#include <utils/String16.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/properties.h>
#include "ion_sprd.h"
#include <media/hardware/MetadataBufferType.h>
#include "cmr_common.h"
#ifdef SPRD_PERFORMANCE
#include <androidfw/SprdIlog.h>
#endif
#include <gralloc_priv.h>
#include "SprdCamera3HALHeader.h"
#include "SprdCamera3Channel.h"
#include "SprdCamera3Flash.h"
#include <dlfcn.h>

#ifdef CONFIG_FACE_BEAUTY
#include "ts_makeup_api.h"
#endif

#include <dlfcn.h>

#ifdef CONFIG_FACE_BEAUTY
#include "ts_makeup_api.h"
#endif

#ifdef CONFIG_CAMERA_ISP
extern "C" {
#include "isp_video.h"
}
#endif

using namespace android;


namespace sprdcamera {

/**********************Macro Define**********************/
#define SWITCH_MONITOR_QUEUE_SIZE 50
#define GET_START_TIME do {\
			s_start_timestamp = systemTime();\
		} while(0)
#define GET_END_TIME do {\
			s_end_timestamp = systemTime();\
		} while(0)
#define GET_USE_TIME do {\
			s_use_time = (s_end_timestamp - s_start_timestamp)/1000000;\
		}while(0)
#define SET_PARAM_TIMEOUT    2000000000     /*2000ms*/
#define CAP_TIMEOUT          5000000000     /*5000ms*/
#define PREV_TIMEOUT         5000000000     /*5000ms*/
#define CAP_START_TIMEOUT    5000000000     /* 5000ms*/
#define PREV_STOP_TIMEOUT    3000000000     /* 3000ms*/
#define CANCEL_AF_TIMEOUT    500000000     /*1000ms*/
#define SET_PARAMS_TIMEOUT   250            /*250 means 250*10ms*/
#define ON_OFF_ACT_TIMEOUT   50             /*50 means 50*10ms*/
#define IS_ZOOM_SYNC         0
#define NO_FREQ_REQ          0
#define NO_FREQ_STR          "0"
#if defined(CONFIG_CAMERA_SMALL_PREVSIZE)
#define BASE_FREQ_REQ        192
#define BASE_FREQ_STR        "0"         /*base mode can treated with AUTO*/
#define MEDIUM_FREQ_REQ      200
#define MEDIUM_FREQ_STR      "200000"
#define HIGH_FREQ_REQ        300
#define HIGH_FREQ_STR        "300000"
#else
#define BASE_FREQ_REQ        200
#define BASE_FREQ_STR        "0"         /*base mode can treated with AUTO*/
#define MEDIUM_FREQ_REQ      300
#define MEDIUM_FREQ_STR      "300000"
#define HIGH_FREQ_REQ        500
#define HIGH_FREQ_STR        "500000"
#endif

#define CONFIG_PRE_ALLOC_CAPTURE_MEM /*pre alloc memory for capture*/
#define HAS_CAMERA_HINTS 1
//#define CONFIG_NEED_UNMAP

#ifdef CONFIG_SPRD_PRIVATE_ZSL
/*ZSL Thread*/
#define ZSLMode_MONITOR_QUEUE_SIZE                   50
#define CMR_EVT_ZSL_MON_BASE                         0x800
#define CMR_EVT_ZSL_MON_INIT                         0x801
#define CMR_EVT_ZSL_MON_EXIT                         0x802
#define CMR_EVT_ZSL_MON_PUSH                         0x803
#define CMR_EVT_ZSL_MON_SNP                          0x804
#endif

#define UPDATE_RANGE_FPS_COUNT                          0x04

/**********************Static Members**********************/
static nsecs_t s_start_timestamp = 0;
static nsecs_t s_end_timestamp = 0;
static int s_use_time = 0;
static int s_mem_method = 0;/*0: physical address, 1: iommu  address*/
static nsecs_t cam_init_begin_time = 0;
bool gIsApctCamInitTimeShow = false;
bool gIsApctRead = false;
static cmr_uint fps_range_min_old =0;
static cmr_uint fps_range_max_old =0;

gralloc_module_t const* SprdCamera3OEMIf::mGrallocHal = NULL;
oem_module_t   * SprdCamera3OEMIf::mHalOem = NULL;

static void writeCamInitTimeToApct(char *buf)
{
	int apct_dir_fd = open("/data/apct", O_CREAT, 0777);

	if (apct_dir_fd >= 0) {
		fchmod(apct_dir_fd, 0777);
		close(apct_dir_fd);
	}

	int apct_fd = open("/data/apct/apct_data.log", O_CREAT | O_RDWR | O_TRUNC, 0666);

	if (apct_fd >=0) {
		char buf[100] = {0};
		sprintf(buf, "\n%s", buf);
		write(apct_fd, buf, strlen(buf));
		fchmod(apct_fd, 0666);
		close(apct_fd);
	}
}

static void writeCamInitTimeToProc(float init_time)
{
	char cam_time_buf[256] = {0};
	const char *cam_time_proc = "/proc/benchMark/cam_time";

	sprintf(cam_time_buf, "Camera init time:%.2fs", init_time);

	FILE *f = fopen(cam_time_proc,"r+w");
	if (NULL != f) {
		fseek(f,0,0);
		fwrite(cam_time_buf,strlen(cam_time_buf),1,f);
		fclose(f);
	}
	writeCamInitTimeToApct(cam_time_buf);
}

bool getApctCamInitSupport()
{
	if (gIsApctRead) {
		return gIsApctCamInitTimeShow;
	}
	gIsApctRead = true;

	int ret = 0;
	char str[10] = {'\0'};
	const char *FILE_NAME = "/data/data/com.sprd.APCT/apct/apct_support";

	FILE *f = fopen(FILE_NAME, "r");

	if (NULL != f) {
		fseek(f, 0, 0);
		ret = fread(str, 5, 1, f);
		fclose(f);
		if(ret){
			long apct_config = atol(str);
			gIsApctCamInitTimeShow = (apct_config & 0x8010) == 0x8010 ? true : false;
		}
	}
	return gIsApctCamInitTimeShow;
}

void SprdCamera3OEMIf::shakeTestInit(ShakeTest *tmpShakeTest)
{
	char is_performance_camera_test[100];
	int tmp_diff_yuv_color[MAX_LOOP_COLOR_COUNT][MAX_Y_UV_COUNT]={
		{0x28,0xef},
		{0x51,0x5a},
		{0x90,0x36}
	};
	memcpy(&tmpShakeTest->diff_yuv_color, &tmp_diff_yuv_color, sizeof(tmp_diff_yuv_color));
	tmpShakeTest->mShakeTestColorCount = 0;
	property_get("persist.sys.performance_camera",is_performance_camera_test, "0");
	if((0 == strcmp("1", is_performance_camera_test)) && mIsPerformanceTestable) {
		HAL_LOGD("SHAKE_TEST come in");
		setShakeTestState(SHAKE_TEST);
	} else {
		HAL_LOGV("SHAKE_TEST not come in");
		setShakeTestState(NOT_SHAKE_TEST);
	}

}

SprdCamera3OEMIf::SprdCamera3OEMIf(int cameraId,
							SprdCamera3Setting *setting)
	:
	mPreviewDcamAllocBufferCnt(0),
	mRawHeap(NULL),
	mRawHeapSize(0),
	mSubRawHeapNum(0),
	mParameters(),
	mPreviewHeight_trimy(0),
	mPreviewWidth_trimx(0),
	mPreviewFormat(CAMERA_DATA_FORMAT_YUV422),
	mPictureFormat(1),
	mPreviewStartFlag(0),
	mIsDvPreview(0),
	mRecordingMode(0),
	mIsSetCaptureMode(false),
	mRecordingFirstFrameTime(0),
	mJpegSize(0),
	mUser(0),
	mPreviewWindow(NULL),
	mIsStoreMetaData(false),
	mIsFreqChanged(false),
	mCameraId(cameraId),
	miSPreviewFirstFrame(1),
	mCaptureMode(CAMERA_NORMAL_MODE),
	mCaptureRawMode(0),
#ifdef CONFIG_CAMERA_ROTATION_CAPTURE
	mIsRotCapture(1),
#else
	mIsRotCapture(0),
#endif
	mFlashMask(false),
	mReleaseFLag(false),
	mTimeCoeff(1),
	mPreviewBufferUsage(PREVIEW_BUFFER_USAGE_GRAPHICS),
	mSetDDRFreqCount(0),
	mSetDDRFreq(NO_FREQ_REQ),
	mSetting(setting),
#if defined(CONFIG_PRE_ALLOC_CAPTURE_MEM)
	mIsPreAllocCapMem(1),
#else
	mIsPreAllocCapMem(0),
#endif
	mPreAllocCapMemInited(0),
	mIsPreAllocCapMemDone(0),
	mZSLModeMonitorMsgQueHandle(0),
	mZSLModeMonitorInited(0),
	mIsPerformanceTestable(false),
	mIsAndroidZSL(false),
	BurstCapCnt(0),
	mCapIntent(0),
	mFlashMode(-1),
	mPrvTimerID(NULL),
	mIsAutoFocus(false),
	mIspToolStart(false),
	mPreviewFrameNum(0),
	mRecordFrameNum(0),
	m_pPowerModule(NULL),
	mHDRPowerHint(0),
	mHDRPowerHintFlag(0),
	mIsRecording(false),
	mIsUpdateRangeFps(false),
	mPrvBufferTimestamp(0),
	mUpdateRangeFpsCount(0),
	mPrvMinFps(0),
	mPrvMaxFps(0),
	mSubRawHeapSize(0),
	mPathRawHeapNum(0)
{
	//mIsPerformanceTestable = sprd_isPerformanceTestable();
	HAL_LOGD("openCameraHardware: E cameraId: %d.", cameraId);

#ifdef CONFIG_FACE_BEAUTY
	mSkinWhitenNotDetectFDNum = 0;
	isNeedBeautify = false;
	memset(&mSkinWhitenTsface,0,sizeof(TSRect));
#endif
	shakeTestInit(&mShakeTest);
#if defined(CONFIG_BACK_CAMERA_ROTATION)
	if (0 == cameraId) {
		mPreviewBufferUsage = PREVIEW_BUFFER_USAGE_DCAM;
	}
#endif

#if defined(CONFIG_FRONT_CAMERA_ROTATION)
	if (1 == cameraId) {
		mPreviewBufferUsage = PREVIEW_BUFFER_USAGE_DCAM;
	}
#endif

#ifdef MINICAMERA
        mPreviewBufferUsage = PREVIEW_BUFFER_USAGE_DCAM;
#endif

	mOriginalPreviewBufferUsage = mPreviewBufferUsage;
	s_mem_method = MemoryHeapIon::Mm_iommu_is_enabled();
	memset(mSubRawHeapArray, 0, sizeof(mSubRawHeapArray));
	memset(mZslHeapArray, 0, sizeof(mZslHeapArray));
	memset(mPreviewHeapArray, 0, sizeof(mPreviewHeapArray));
	memset(mVideoHeapArray, 0, sizeof(mVideoHeapArray));
	memset(&mSlowPara, 0, sizeof(slow_motion_para));
	memset(mPathRawHeapArray, 0, sizeof(mPathRawHeapArray));
	setCameraState(SPRD_INIT, STATE_CAMERA);

#ifndef MINICAMERA
	if (!mGrallocHal) {
		int ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&mGrallocHal);
		if (ret)
			HAL_LOGE("ERR(%s):Fail on loading gralloc HAL", __func__);
	}
#endif

      if(!mHalOem) {
              void *handle;
              oem_module_t *omi;

              mHalOem = (oem_module_t *)malloc(sizeof(oem_module_t));

              handle = dlopen("libcamoem.so", RTLD_NOW);

              if (handle == NULL) {
                     char const *err_str = dlerror();
                     ALOGE("dlopen error%s ", err_str?err_str:"unknown");
              }

              /* Get the address of the struct hal_module_info. */
              const char *sym = OEM_MODULE_INFO_SYM_AS_STR;
              omi = (oem_module_t *)dlsym(handle, sym);
              if (omi == NULL) {
                     ALOGE("load: couldn't find symbol %s", sym);
              }

              mHalOem->dso = handle;
              mHalOem->ops = omi->ops;

              ALOGI("loaded libcamoem2v0.so handle=%p", handle);
       }

	mCameraId = cameraId;
	mCbInfoList.clear();
	mSetting->getDefaultParameters(mParameters);

	mPreviewWidth = 0;
	mPreviewHeight = 0;
	mVideoWidth = 0;
	mVideoHeight = 0;
	mCaptureWidth = 0;
	mCaptureHeight = 0;
	mRawWidth = 0;
	mRawHeight = 0;
	mRegularChan = NULL;

	mMetaData = NULL;
	jpeg_gps_location = false;
	mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
	mPreviewHeapBakUseFlag = 0;
	mRawHeapBakUseFlag = 0;
	memset(&mRawHeapInfoBak, 0, sizeof(mRawHeapInfoBak));
	for (int i=0; i<kPreviewBufferCount; i++) {
		mPreviewBufferHandle[i] = NULL;
		mPreviewCancelBufHandle[i] = NULL;
	}
	mTakePictureMode = SNAPSHOT_DEFAULT_MODE;
	mZSLTakePicture = false;

	mZoomInfo.zoom_info.output_ratio = 0.0f;
	mZoomInfo.zoom_info.zoom_ratio = 0.0f;
	mZoomInfo.raw_mode = ZOOM_NORMAL;
	mZoomInfo.mode = ZOOM_INFO;
	mCurZoomInfo.zoom_info.output_ratio = 0.0f;
	mCurZoomInfo.zoom_info.zoom_ratio = 0.0f;
	mCurZoomInfo.mode = ZOOM_INFO;
	mZoomInfo.raw_mode = ZOOM_NORMAL;

	mCameraHandle = NULL;

	memset(mGps_processing_method, 0, sizeof(mGps_processing_method));
	memset(mIspB4awbHeapReserved, 0, sizeof(mIspB4awbHeapReserved));
	memset(mIspRawAemHeapReserved, 0, sizeof(mIspRawAemHeapReserved));
/* [Vendor_11/40] */
	memset(mIspRawAfmHeapReserved, 0, sizeof(mIspRawAfmHeapReserved)); 
	HAL_LOGD("[3A Porting]mem set mIspRawAfmHeapReserved");

	mJpegRotaSet = false;
	mPicCaptureCnt = 0;

	mRegularChan = NULL;
	mPictureChan = NULL;

	mPreviewHeapNum = 0;
	mVideoHeapNum = 0;
	mZslHeapNum = 0;
	mSubRawHeapSize = 0;

#ifdef CONFIG_MEM_OPTIMIZATION
	mCommonHeapReserved = NULL;
#else
	mPreviewHeapReserved = NULL;
	mVideoHeapReserved = NULL;
	mZslHeapReserved = NULL;
#endif
	mIspLscHeapReserved = NULL;
	mIspAntiFlickerHeapReserved = NULL;

#ifdef CONFIG_MULTI_CAP_MEM
	mPathRawHeapNum = 0;
	mPathRawHeapSize = 0;
	memset(mPathRawHeapArray, 0, sizeof(mPathRawHeapArray));
#endif

	mVideoShotFlag = 0;
	mVideoShotNum = 0;
	mVideoShotPushFlag = 0;

	mRestartFlag = false;
	mReDisplayHeap = NULL;

	mZslPreviewMode = false;
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	mSprdZslEnabled = false;
#ifdef CONFIG_NEED_UNMAP
	mZslMaxFrameNum = 2;
	mZslMaxBuffNum = 4;
	mZslMapNum = 2;
#else
	mZslMaxFrameNum = 0;
	mZslMaxBuffNum = 3;
	mZslMapNum = 3;
#endif
	mZslShotPushFlag = 0;
	mZslShotSkipNum = 2;
	mZslChannelStatus = 1;

	mVideoSnapshotSprdZslEnabled = 0;
#endif

#ifdef HAS_CAMERA_HINTS
	if (hw_get_module(POWER_HARDWARE_MODULE_ID, (const hw_module_t **)&m_pPowerModule)) {
		HAL_LOGE("%s: %s module not found", __func__, POWER_HARDWARE_MODULE_ID);
	}

	if (m_pPowerModule) {
		if (m_pPowerModule->powerHint) {
			m_pPowerModule->powerHint(m_pPowerModule, POWER_HINT_VIDEO_ENCODE,
				(void *)"state=1");
		}
	}
#endif

	HAL_LOGD("openCameraHardware: X cameraId: %d.", cameraId);
}

SprdCamera3OEMIf::~SprdCamera3OEMIf()
{
	int i;
	HAL_LOGD("closeCameraHardware: E cameraId: %d.", mCameraId);

	if (!mReleaseFLag) {
		closeCamera();
	}

	// Just in case that exception happen
	if (miSPreviewFirstFrame == 1) {
#ifdef HAS_CAMERA_HINTS
		if (m_pPowerModule) {
			if (m_pPowerModule->powerHint) {
				m_pPowerModule->powerHint(m_pPowerModule, POWER_HINT_VIDEO_ENCODE,
					(void *)"state=0");
			}
		}
#endif
	}

#ifdef HAS_CAMERA_HINTS
	if (1 == mHDRPowerHintFlag) {
		if (m_pPowerModule) {
			if (m_pPowerModule->powerHint) {
				m_pPowerModule->powerHint(m_pPowerModule, POWER_HINT_VIDEO_ENCODE,
					(void *)"state=0");
			}
		}
		mHDRPowerHintFlag = 0;
	}
#endif

	// clean memory in case memory leak
	if(NULL != mReDisplayHeap) {
		freeCameraMem(mReDisplayHeap);
		mReDisplayHeap = NULL;
	}

	for (i = 0; i < mZslHeapNum; i++) {
		if (NULL != mZslHeapArray[i]) {
			freeCameraMem(mZslHeapArray[i]);
			mZslHeapArray[i] = NULL;
		}
	}
	mZslHeapNum = 0;

	if (NULL != mIspLscHeapReserved) {
		freeCameraMem(mIspLscHeapReserved);
		mIspLscHeapReserved = NULL;
	}

	if (NULL != mIspAntiFlickerHeapReserved) {
		freeCameraMem(mIspAntiFlickerHeapReserved);
		mIspAntiFlickerHeapReserved = NULL;
	}


	for (i = 0; i < kISPB4awbCount; i++) {
		if (NULL != mIspB4awbHeapReserved[i]) {
			freeCameraMem(mIspB4awbHeapReserved[i]);
			mIspB4awbHeapReserved[i] = NULL;
		}
	}

	for (i = 0; i < kISPB4awbCount; i++) {
		if (NULL != mIspRawAemHeapReserved[i]) {
			mIspRawAemHeapReserved[i]->ion_heap->free_kaddr();
			freeCameraMem(mIspRawAemHeapReserved[i]);
			mIspRawAemHeapReserved[i] = NULL;
		}
	}
/* [Vendor_12/40] */
	for (i = 0; i < kISPB4awbCount; i++) { 
		if (NULL != mIspRawAfmHeapReserved[i]) {
			mIspRawAfmHeapReserved[i]->ion_heap->free_kaddr();
			freeCameraMem(mIspRawAfmHeapReserved[i]);
			mIspRawAfmHeapReserved[i] = NULL;
		}
	}
#ifdef CONFIG_MULTI_CAP_MEM
	for (i=0 ; i<mPathRawHeapNum ; i++) {
		if (NULL != mPathRawHeapArray[i]) {
			freeCameraMem(mPathRawHeapArray[i]);
		}
		mPathRawHeapArray[i] = NULL;
	}
	mPathRawHeapNum = 0;
	mPathRawHeapSize = 0;
#endif

       if(mHalOem) {
              if(NULL != mHalOem->dso)
                  dlclose(mHalOem->dso);
              free((void *)mHalOem);
              mHalOem = NULL;
       }

	HAL_LOGD("closeCameraHardware: X cameraId: %d.", mCameraId);
	timer_stop();
}

void SprdCamera3OEMIf::closeCamera()
{
	HAL_LOGD("E");
	Mutex::Autolock l(&mLock);

	// Either preview was ongoing, or we are in the middle or taking a
	// picture.  It's the caller's responsibility to make sure the camera
	// is in the idle or init state before destroying this object.
	HAL_LOGD("release:camera state = %s, preview state = %s, capture state = %s",
		getCameraStateStr(getCameraState()), getCameraStateStr(getPreviewState()),
		getCameraStateStr(getCaptureState()));

	if (isCapturing()) {
		cancelPictureInternal();
	}

	if (isPreviewing()) {
		stopPreviewInternal();
	}

#ifdef CONFIG_MEM_OPTIMIZATION
	// Free mCommonHeapReserved for preview, video, and zsl
	if (NULL != mCommonHeapReserved) {
		freeCameraMem(mCommonHeapReserved);
		mCommonHeapReserved = NULL;
	}
#endif

	// Performance optimization:move Callback_CaptureFree to closeCamera function
	Callback_CaptureFree(0, 0, 0);

	while (0 < mSetDDRFreqCount) {
		if (BAD_VALUE == set_ddr_freq(NO_FREQ_REQ)) {
			mSetDDRFreqCount = 0;
			HAL_LOGW("ddr set fail, quit yet!");
			break;
		}
	}

	SprdCamera3Flash::releaseFlash(mCameraId);
	// Performance optimization:move Callback_CaptureFree to closeCamera function
	Callback_CaptureFree(0, 0, 0);

	if (isCameraInit()) {
		// When libqcamera detects an error, it calls camera_cb from the
		// call to camera_stop, which would cause a deadlock if we
		// held the mStateLock.  For this reason, we have an intermediate
		// state SPRD_INTERNAL_STOPPING, which we use to check to see if the
		// camera_cb was called inline.
		setCameraState(SPRD_INTERNAL_STOPPING, STATE_CAMERA);

		HAL_LOGD("stopping camera.");
		if (CMR_CAMERA_SUCCESS != mHalOem->ops->camera_deinit(mCameraHandle)) {
			setCameraState(SPRD_ERROR, STATE_CAMERA);
			mReleaseFLag = true;
			HAL_LOGE("release X: fail to camera_stop().");
			return;
		}

		WaitForCameraStop();
	}

	pre_alloc_cap_mem_thread_deinit((void *)this);
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	ZSLMode_monitor_thread_deinit((void *)this);
#endif
	deinitCapture(0);
	FreeReDisplayMem();

       fps_range_min_old =0;
       fps_range_max_old =0;

	mReleaseFLag = true;
	HAL_LOGI("X");
}

int SprdCamera3OEMIf::getCameraId() const
{
	return mCameraId;
}

void SprdCamera3OEMIf::initialize()
{
	mPreviewFrameNum = 0;
	mRecordFrameNum = 0;
	mZslFrameNum = 0;
	mPictureFrameNum = 0;
	mStartFrameNum = 0;
	mStopFrameNum = 0;

	mDropPreviewFrameNum = 0;
	mDropVideoFrameNum = 0;
	mDropZslFrameNum = 0;
}

int SprdCamera3OEMIf::start(camera_channel_type_t channel_type, uint32_t frame_number)
{
	int ret = NO_ERROR;
	Mutex::Autolock l(&mLock);

#ifdef CONFIG_SPRD_PRIVATE_ZSL
	SPRD_DEF_Tag sprddefInfo;
	mSetting->getSPRDDEFTag(&sprddefInfo);
#endif

	HAL_LOGD("E: channel_type = %d, frame_number = %d", channel_type, frame_number);
	mStartFrameNum = frame_number;
#ifdef FORCE_PASS_FLEXIBLEYUV_IN_DARK
	mCountFromLastSizeChange = 0;
#endif
	switch (channel_type) {
		case CAMERA_CHANNEL_TYPE_REGULAR:
		{
			if (mParaDCDVMode == CAMERA_PREVIEW_FORMAT_DV)
				mRecordingFirstFrameTime = 0;
#ifdef CONFIG_SPRD_PRIVATE_ZSL
			if (mRecordingMode == false && sprddefInfo.sprd_zsl_enabled == 1) {
				mSprdZslEnabled = true;
			} else {
				mSprdZslEnabled = false;
			}
#endif

			ret = startPreviewInternal();
			break;
		}
		case CAMERA_CHANNEL_TYPE_PICTURE:
		{
			if(mTakePictureMode == SNAPSHOT_NO_ZSL_MODE || mTakePictureMode == SNAPSHOT_ONLY_MODE)
				ret = takePicture();
			else if(mTakePictureMode == SNAPSHOT_ZSL_MODE)
				//mZSLTakePicture = true;
				ret = zslTakePicture();
			else if(mTakePictureMode == SNAPSHOT_VIDEO_MODE) {
				//mVideoShotNum = frame_number;
				//mVideoShotFlag = 1;
				ret = VideoTakePicture();
			}
			break;
		}
		default:
			break;
	}

	HAL_LOGD("X");
	return ret;
}

int SprdCamera3OEMIf::stop(camera_channel_type_t channel_type, uint32_t frame_number)
{
	int ret = NO_ERROR;
	int capture_intent = 0;

	HAL_LOGD("channel_type = %d, frame_number = %d", channel_type, frame_number);
	mStopFrameNum = frame_number;

	switch(channel_type)
	{
		case CAMERA_CHANNEL_TYPE_REGULAR:
			stopPreviewInternal();
			break;
		case CAMERA_CHANNEL_TYPE_PICTURE:
			cancelPictureInternal();
			break;
		default:
			break;
	}

	return ret;
}

int SprdCamera3OEMIf::takePicture()
{
	bool result = true, wait_raw = true;
	GET_START_TIME;
	HAL_LOGD("E");
	mFlushLock.lock();
	print_time();
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	uint8_t ae_awb_af_lock;

	if (1 == mHDRPowerHint) {
		ae_awb_af_lock = 1;
		HAL_LOGD("takePicture 3a_lock = %d",ae_awb_af_lock);
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_3A_LOCK_UNLOCK, ae_awb_af_lock);
	}
#endif
#ifdef HAS_CAMERA_HINTS
	if (1 == mHDRPowerHint) {
		if (m_pPowerModule) {
			if (m_pPowerModule->powerHint) {
				m_pPowerModule->powerHint(m_pPowerModule, POWER_HINT_VIDEO_ENCODE,
					(void *)"state=1");
			}
		}
		mHDRPowerHintFlag = 1;
	}
#endif

	{
		//Mutex::Autolock stateLock(&mStateLock);
		if (SPRD_ERROR == mCameraState.capture_state) {
			HAL_LOGE("in error status, deinit capture at first ");
			deinitCapture(mIsPreAllocCapMem);
		} else if (SPRD_IDLE != mCameraState.capture_state) {
			usleep(50*1000);
			if (SPRD_IDLE != mCameraState.capture_state) {
				HAL_LOGE("action alread exist, direct return!");
				return ALREADY_EXISTS;
			}
		}
	}

	setCameraState(SPRD_FLASH_IN_PROGRESS, STATE_CAPTURE);

	if (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE || mTakePictureMode == SNAPSHOT_PREVIEW_MODE || mTakePictureMode == SNAPSHOT_ONLY_MODE) {
		if (((mCaptureMode == CAMERA_ISP_TUNING_MODE) || (mCaptureMode == CAMERA_ISP_SIMULATION_MODE)) && !mIspToolStart) {
			mIspToolStart = true;
			timer_set(this, ISP_TUNING_WAIT_MAX_TIME, timer_hand);
		}
		if(isPreviewStart()) {
			HAL_LOGV("Preview not start! wait preview start");
			WaitForPreviewStart();
			usleep(20*1000);
		}
		if (isPreviewing()) {
			HAL_LOGD("call stopPreviewInternal in takePicture().");
//#ifdef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
			if (CAMERA_ZSL_MODE != mCaptureMode) {
				mHalOem->ops->camera_start_preflash(mCameraHandle);
			}
//#endif
			stopPreviewInternal();
		}
		HAL_LOGD("ok to stopPreviewInternal in takePicture. preview state = %d", getPreviewState());

		if (isPreviewing()) {
			HAL_LOGE("stop preview error!, preview state = %d", getPreviewState());
			return UNKNOWN_ERROR;
		}
		if (!setCameraCaptureDimensions()) {
			deinitCapture(mIsPreAllocCapMem);
			HAL_LOGE("initCapture failed. Not taking picture.");
			return UNKNOWN_ERROR;
		}
	}

	if (isCapturing()) {
		WaitForCaptureDone();
	}

	set_ddr_freq(HIGH_FREQ_REQ);
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM, mPicCaptureCnt);
{
	LENS_Tag lensInfo;
	mSetting->getLENSTag(&lensInfo);
	if(lensInfo.focal_length)
	{
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH,  (int32_t)(lensInfo.focal_length * 1000));
		HAL_LOGD("lensInfo.focal_length = %f",lensInfo.focal_length);
	}
	JPEG_Tag jpgInfo;
	mSetting->getJPEGTag(&jpgInfo);
	{
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY, jpgInfo.quality);
		HAL_LOGD("JPEG quality = %d",jpgInfo.quality);
	}
	{
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_QUALITY, jpgInfo.thumbnail_quality);
		HAL_LOGD("JPEG thumbnail quality = %d",jpgInfo.thumbnail_quality);
	}
	{
		struct img_size jpeg_thumb_size;
		struct img_size capture_size;
		jpeg_thumb_size.width = jpgInfo.thumbnail_size[0];
		jpeg_thumb_size.height = jpgInfo.thumbnail_size[1];
		if(mCaptureWidth != 0 && mCaptureHeight != 0) {
			if(mVideoWidth != 0 && mVideoHeight != 0 && mCaptureMode != CAMERA_ISP_TUNING_MODE &&
				mCaptureMode != CAMERA_ISP_SIMULATION_MODE) {
				capture_size.width = (cmr_u32)mPreviewWidth;//mVideoWidth;
				capture_size.height = (cmr_u32)mPreviewHeight;//mVideoHeight;
			} else {
				capture_size.width = (cmr_u32)mCaptureWidth;
				capture_size.height = (cmr_u32)mCaptureHeight;
			}
		} else {
		capture_size.width = (cmr_u32)mRawWidth;
		capture_size.height = (cmr_u32)mRawHeight;
		}
		if (jpeg_thumb_size.width>capture_size.width  &&jpeg_thumb_size.height > capture_size.height )
		{
			jpeg_thumb_size.width = 0;
			jpeg_thumb_size.height = 0;
		}

		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE, (cmr_uint)&jpeg_thumb_size);
		//camera_set_thumbnail_properties(jpgInfo.thumbnail_size[0], jpgInfo.thumbnail_size[1], jpgInfo.thumbnail_quality);
		HAL_LOGD("JPEG thumbnail size = %d x %d", jpeg_thumb_size.width, jpeg_thumb_size.height);
	}
}

	setCameraPreviewFormat();
	if (CMR_CAMERA_SUCCESS != mHalOem->ops->camera_set_mem_func(mCameraHandle, (void*)Callback_Malloc, (void*)Callback_Free, (void*)this)) {
			setCameraState(SPRD_ERROR);
			HAL_LOGE("start fail to camera_set_mem_func().");
			return UNKNOWN_ERROR;
	}
	setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);
	if (CMR_CAMERA_SUCCESS != mHalOem->ops->camera_take_picture(mCameraHandle, mCaptureMode)) {
		setCameraState(SPRD_ERROR, STATE_CAPTURE);
		HAL_LOGE("fail to camera_take_picture.");
		return UNKNOWN_ERROR;
	}

	if (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE) {
        if ((mCaptureMode == CAMERA_ISP_TUNING_MODE || mCaptureMode == CAMERA_ISP_SIMULATION_MODE) && mIspToolStart) {
            wait_raw = false;
        }
    }
	mFlushLock.unlock();
    if (wait_raw)
        result = WaitForCaptureStart();

    print_time();

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	if (1 == mHDRPowerHint) {
		ae_awb_af_lock = 0;
		HAL_LOGD("takePicture 3a_lock = %d",ae_awb_af_lock);
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_3A_LOCK_UNLOCK, ae_awb_af_lock);
	}
#endif

    HAL_LOGD("X, wait_raw=%d", wait_raw);
	return result ? NO_ERROR : UNKNOWN_ERROR;
}

int SprdCamera3OEMIf::zslTakePicture()
{
	GET_START_TIME;
	HAL_LOGD("E");
	print_time();

	if (SPRD_ERROR == mCameraState.capture_state) {
		HAL_LOGE("in error status, deinit capture at first ");
		deinitCapture(mIsPreAllocCapMem);
	}/* else if (SPRD_IDLE != mCameraState.capture_state) {
		HAL_LOGE("take picture: action alread exist, direct return!");
		return ALREADY_EXISTS;
	}*/

	if (isCapturing()) {
		WaitForCaptureDone();
	}
	if (isPreviewing()) {
		mHalOem->ops->camera_start_preflash(mCameraHandle);
	}

	set_ddr_freq(HIGH_FREQ_REQ);
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM, mPicCaptureCnt);
{
	LENS_Tag lensInfo;
	mSetting->getLENSTag(&lensInfo);
	if(lensInfo.focal_length)
	{
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH,  (int32_t)(lensInfo.focal_length * 1000));
		HAL_LOGD("lensInfo.focal_length = %f",lensInfo.focal_length);
	}
	JPEG_Tag jpgInfo;
	mSetting->getJPEGTag(&jpgInfo);
	{
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY, jpgInfo.quality);
		HAL_LOGV("JPEG quality = %d",jpgInfo.quality);
	}
	{
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_QUALITY, jpgInfo.thumbnail_quality);
		HAL_LOGV("JPEG thumbnail quality = %d",jpgInfo.thumbnail_quality);
	}
	{
		struct img_size jpeg_thumb_size;
		jpeg_thumb_size.width = jpgInfo.thumbnail_size[0];
		jpeg_thumb_size.height = jpgInfo.thumbnail_size[1];
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE, (cmr_uint)&jpeg_thumb_size);
		HAL_LOGD("JPEG thumbnail size = %d x %d", jpgInfo.thumbnail_size[0], jpgInfo.thumbnail_size[1]);
	}
}

	setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);
	if (CMR_CAMERA_SUCCESS != mHalOem->ops->camera_take_picture(mCameraHandle, mCaptureMode)) {
		setCameraState(SPRD_ERROR, STATE_CAPTURE);
		HAL_LOGE("fail to camera_take_picture.");
		return UNKNOWN_ERROR;
	}

#ifdef CONFIG_SPRD_PRIVATE_ZSL
	if(mSprdZslEnabled == true) {
			CMR_MSG_INIT(message);
			uint32_t ret = 0;

		//	HAL_LOGD("start send CMR_EVT_ZSL_MON_SNP");
			mZslShotPushFlag = 1;
			message.msg_type = CMR_EVT_ZSL_MON_SNP;
			message.sync_flag = CMR_MSG_SYNC_NONE;
			message.data = NULL;
		//	HAL_LOGD("msg_type = 0x%x\n", message.msg_type);

			// fot cts, mZslMapNum == 1 means iommu address space is not enough
			if(mZslMapNum > 1) {
				ret = cmr_thread_msg_send((cmr_handle)mZSLModeMonitorMsgQueHandle, &message);
				if (ret) {
					HAL_LOGE("Fail to send one msg!");
					return NO_ERROR;
				}
			}
			HAL_LOGD("mZslShotPushFlag %d",mZslShotPushFlag);
	} else
#endif
	{
		PushZslSnapShotbuff();
	}

	//bool result = WaitForCaptureStart();
	print_time();

	HAL_LOGD("X");
	//return result ? NO_ERROR : UNKNOWN_ERROR;
	return NO_ERROR;
}

int SprdCamera3OEMIf::VideoTakePicture()
{
	GET_START_TIME;
	HAL_LOGD("E");
	print_time();

	if (SPRD_ERROR == mCameraState.capture_state) {
		HAL_LOGE("in error status, deinit capture at first ");
		deinitCapture(mIsPreAllocCapMem);
	}/* else if (SPRD_IDLE != mCameraState.capture_state) {
		HAL_LOGE("take picture: action alread exist, direct return!");
		return ALREADY_EXISTS;
	}*/

	if (isCapturing()) {
		WaitForCaptureDone();
	}

	set_ddr_freq(HIGH_FREQ_REQ);
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM, mPicCaptureCnt);
{
	LENS_Tag lensInfo;
	mSetting->getLENSTag(&lensInfo);
	if(lensInfo.focal_length)
	{
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH,  (int32_t)(lensInfo.focal_length * 1000));
		HAL_LOGD("lensInfo.focal_length = %f",lensInfo.focal_length);
	}
	JPEG_Tag jpgInfo;
	mSetting->getJPEGTag(&jpgInfo);
	{
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY, jpgInfo.quality);
		HAL_LOGV("JPEG quality = %d",jpgInfo.quality);
	}
	{
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_QUALITY, jpgInfo.thumbnail_quality);
		HAL_LOGV("JPEG thumbnail quality = %d",jpgInfo.thumbnail_quality);
	}
	{
		struct img_size jpeg_thumb_size;
		jpeg_thumb_size.width = jpgInfo.thumbnail_size[0];
		jpeg_thumb_size.height = jpgInfo.thumbnail_size[1];
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE, (cmr_uint)&jpeg_thumb_size);
		HAL_LOGD("JPEG thumbnail size = %d x %d", jpgInfo.thumbnail_size[0], jpgInfo.thumbnail_size[1]);
	}
}

	setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);
	if (CMR_CAMERA_SUCCESS != mHalOem->ops->camera_take_picture(mCameraHandle, mCaptureMode)) {
		setCameraState(SPRD_ERROR, STATE_CAPTURE);
		HAL_LOGE("fail to camera_take_picture.");
		return UNKNOWN_ERROR;
	}

	mVideoShotPushFlag = 1;
	mVideoShotWait.signal();

	bool result = WaitForCaptureStart();
	print_time();

	HAL_LOGD("X");
	return result ? NO_ERROR : UNKNOWN_ERROR;
}

int SprdCamera3OEMIf::cancelPicture()
{
	//Mutex::Autolock l(&mLock);

	return cancelPictureInternal();
}

int SprdCamera3OEMIf::setTakePictureSize(uint32_t width, uint32_t height)
{
	mRawWidth = width;
	mRawHeight = height;

	return NO_ERROR;
}
status_t SprdCamera3OEMIf::faceDectect(bool enable)
{
	status_t ret = NO_ERROR;

	if(enable)
	{
		mHalOem->ops->camera_fd_start(mCameraHandle, 1);
	}else{
		mHalOem->ops->camera_fd_start(mCameraHandle, 0);
	}
	return ret;
}
status_t SprdCamera3OEMIf::faceDectect_enable(bool enable)
{
	status_t ret = NO_ERROR;
	if(enable)
	{
		mHalOem->ops->camera_fd_enable(mCameraHandle, 1);
	}else{
		mHalOem->ops->camera_fd_enable(mCameraHandle, 0);
	}
	return ret;
}

status_t SprdCamera3OEMIf::autoFocus(void *user_data)
{
#ifdef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	return NO_ERROR;
#endif
	HAL_LOGD("E");
	Mutex::Autolock l(&mLock);
	CONTROL_Tag controlInfo;

	mMetaData = user_data;

	if(mCameraId == 1)
		return NO_ERROR;

	mSetting->getCONTROLTag(&controlInfo);
	if(isPreviewStart()) {
		HAL_LOGV("Preview not start! wait preview start");
		WaitForPreviewStart();
	}

	if (!isPreviewing()) {
		HAL_LOGE("not previewing");
		controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
		mSetting->setAfCONTROLTag(controlInfo);
		return INVALID_OPERATION;
	}

	if (SPRD_IDLE != getFocusState()) {
		HAL_LOGE("existing, direct return!");
		return NO_ERROR;
	}
	setCameraState(SPRD_FOCUS_IN_PROGRESS, STATE_FOCUS);
	mIsAutoFocus = true;
	/*caf transit to auto focus*/
	if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE || controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO) {
		if( controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE )
			mHalOem->ops->camera_transfer_caf_to_af(mCameraHandle);
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE, CAMERA_FOCUS_MODE_AUTO);
	}
	controlInfo.af_state = ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN;
	mSetting->setAfCONTROLTag(controlInfo);
	if (0 != mHalOem->ops->camera_start_autofocus(mCameraHandle)) {
		HAL_LOGE("auto foucs fail.");
		setCameraState(SPRD_IDLE, STATE_FOCUS);
		controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
		mSetting->setAfCONTROLTag(controlInfo);
	}

	HAL_LOGD("X");
	return NO_ERROR;
}

status_t SprdCamera3OEMIf::cancelAutoFocus()
{
#ifdef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	return NO_ERROR;
#endif
	bool ret = 0;
	HAL_LOGD("E");

	//Mutex::Autolock l(&mLock);

	if(mCameraId == 1)
		return NO_ERROR;

	ret = mHalOem->ops->camera_cancel_autofocus(mCameraHandle);

	WaitForFocusCancelDone();
{
	SprdCamera3MetadataChannel *mChannel = (SprdCamera3MetadataChannel*)mMetaData;
	int64_t timeStamp = 0;
	timeStamp = systemTime();

	CONTROL_Tag controlInfo;
	mSetting->getCONTROLTag(&controlInfo);
	if (!(controlInfo.af_state == ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED
			&& (controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_START || controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_IDLE)))
	controlInfo.af_state = ANDROID_CONTROL_AF_STATE_INACTIVE;
	/*auto focus resume to caf*/
	if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE && mCameraId == 0 && mIsAutoFocus) {
		if( controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE )
			mHalOem->ops->camera_transfer_af_to_caf(mCameraHandle);
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE, CAMERA_FOCUS_MODE_CAF);
	}
	mSetting->setAfCONTROLTag(controlInfo);
}
	mIsAutoFocus = false;
	HAL_LOGD("X");
	return ret;
}

status_t SprdCamera3OEMIf::setAePrecaptureSta(uint8_t state)
{
	status_t ret = 0;
	Mutex::Autolock l(&mLock);
	CONTROL_Tag controlInfo;

	mSetting->getCONTROLTag(&controlInfo);
	controlInfo.ae_state = state;
	mSetting->setAeCONTROLTag(controlInfo);
	HAL_LOGD("ae sta %d", state);
	return ret;
}

void SprdCamera3OEMIf::setCaptureRawMode(bool mode)
{
	struct img_size req_size;
	struct cmr_zoom_param zoom_param;

	mCaptureRawMode = mode;
	HAL_LOGD("ISP_TOOL: setCaptureRawMode: %d, %d", mode, mCaptureRawMode);
	if (1 == mode) {
		HAL_LOGD("ISP_TOOL: setCaptureRawMode, mRawWidth %d, mRawHeight %d", mRawWidth, mRawHeight);
		req_size.width = (cmr_u32)mRawWidth;
		req_size.height = (cmr_u32)mRawHeight;
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE, (cmr_uint)&req_size);

		zoom_param.mode = ZOOM_LEVEL;
		zoom_param.raw_mode = ZOOM_RAW;
		zoom_param.zoom_level = 0;
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ZOOM, (cmr_uint)&zoom_param);

		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ROTATION,  0);
	}
}

void SprdCamera3OEMIf::setIspFlashMode(uint32_t mode)
{
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_FLASH, mode);
}

void SprdCamera3OEMIf::antiShakeParamSetup( )
{
#ifdef CONFIG_CAMERA_ANTI_SHAKE
	mPreviewWidth = mPreviewWidth_backup + ((((mPreviewWidth_backup/ 10) + 15) >> 4) << 4);
	mPreviewHeight = mPreviewHeight_backup + ((((mPreviewHeight_backup/ 10) + 15) >> 4) << 4);
#endif
}

const char* SprdCamera3OEMIf::getCameraStateStr(
	SprdCamera3OEMIf::Sprd_camera_state s)
{
	static const char* states[] = {
#define STATE_STR(x) #x
	STATE_STR(SPRD_INIT),
	STATE_STR(SPRD_IDLE),
	STATE_STR(SPRD_ERROR),
	STATE_STR(SPRD_PREVIEW_IN_PROGRESS),
	STATE_STR(SPRD_FOCUS_IN_PROGRESS),
	STATE_STR(SPRD_SET_PARAMS_IN_PROGRESS),
	STATE_STR(SPRD_WAITING_RAW),
	STATE_STR(SPRD_WAITING_JPEG),
	STATE_STR(SPRD_FLASH_IN_PROGRESS),
	STATE_STR(SPRD_INTERNAL_PREVIEW_STOPPING),
	STATE_STR(SPRD_INTERNAL_CAPTURE_STOPPING),
	STATE_STR(SPRD_INTERNAL_PREVIEW_REQUESTED),
	STATE_STR(SPRD_INTERNAL_RAW_REQUESTED),
	STATE_STR(SPRD_INTERNAL_STOPPING),
#undef STATE_STR
	};
	return states[s];
}

void SprdCamera3OEMIf::print_time()
{
#if PRINT_TIME
	struct timeval time;
	gettimeofday(&time, NULL);
	HAL_LOGD("time: %lld us.", time.tv_sec * 1000000LL + time.tv_usec);
#endif
}

bool SprdCamera3OEMIf::setCameraPreviewDimensions()
{
	uint32_t local_width = 0, local_height = 0;
	uint32_t mem_size = 0;
	struct img_size preview_size, video_size = {0, 0}, capture_size = {0, 0};

	if(mPreviewWidth != 0 && mPreviewHeight != 0) {
		preview_size.width = (cmr_u32)mPreviewWidth;
		preview_size.height = (cmr_u32)mPreviewHeight;
	} else {
		if((mVideoWidth != 0 && mVideoHeight != 0) && (mRawWidth != 0 && mRawHeight != 0)) {
			HAL_LOGE("HAL not support video stream and callback stream simultaneously");
			return false;
		} else {
			preview_size.width = 720;
			preview_size.height = 576;
		}
	}
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PREVIEW_SIZE, (cmr_uint)&preview_size);

	if(mVideoWidth != 0 && mVideoHeight != 0) {
		video_size.width = (cmr_u32)mVideoWidth;
		video_size.height = (cmr_u32)mVideoHeight;
		mCaptureMode = CAMERA_ZSL_MODE;
	} else {
		if((mCaptureMode == CAMERA_ZSL_MODE) && (mRawWidth == 0 && mRawHeight == 0)) {
#ifdef CONFIG_SPRD_PRIVATE_ZSL
			if(mSprdZslEnabled == true) {
				video_size.width = 0;
				video_size.height = 0;
			} else {
				mVideoWidth = (cmr_u32)mPreviewWidth;
				mVideoHeight = (cmr_u32)mPreviewHeight;
				video_size.width = (cmr_u32)mPreviewWidth;
				video_size.height = (cmr_u32)mPreviewHeight;
			}
#else
			mVideoWidth = (cmr_u32)mPreviewWidth;
			mVideoHeight = (cmr_u32)mPreviewHeight;
			video_size.width = (cmr_u32)mPreviewWidth;
			video_size.height = (cmr_u32)mPreviewHeight;
#endif
		} else {
			video_size.width = 0;
			video_size.height = 0;
		}
	}

	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_VIDEO_SIZE, (cmr_uint)&video_size);

	if(mZslPreviewMode == false) {
#ifdef CONFIG_SPRD_PRIVATE_ZSL
		if(mSprdZslEnabled == false) {
#endif
		if(mRawWidth != 0 && mRawHeight != 0) {
			capture_size.width = (cmr_u32)mRawWidth;
			capture_size.height = (cmr_u32)mRawHeight;
			mCaptureMode = CAMERA_ZSL_MODE;
		} else {
			if(mVideoWidth != 0 && mVideoHeight != 0) { //capture size must equal with video size
				if (mVideoWidth <= mCaptureWidth && mVideoHeight <= mCaptureHeight) {
					capture_size.width = (cmr_u32)mVideoWidth;
					capture_size.height = (cmr_u32)mVideoHeight;
				} else {
					capture_size.width = (cmr_u32)mPreviewWidth;
					capture_size.height = (cmr_u32)mPreviewHeight;
				}
			} else {
				if (mCaptureWidth != 0 && mCaptureHeight != 0) {
					if ((mPreviewWidth >= 1920 && mPreviewHeight >= 1088) && (mCaptureWidth > 720 && mCaptureHeight > 480)) {
						capture_size.width = 720;
						capture_size.height = 480;
					}
				}
			}
			}
#ifdef CONFIG_SPRD_PRIVATE_ZSL
		}else {
			if (mCaptureWidth != 0 && mCaptureHeight != 0) {
				capture_size.width = mCaptureWidth;
				capture_size.height = mCaptureHeight;
				mCaptureMode = CAMERA_ZSL_MODE;
			}
		}
#endif
	} else {
		if(mCaptureWidth != 0 && mCaptureHeight != 0) {//API 1.0 ZSL
			/*if(mCaptureWidth > 2592 || mCaptureHeight >1952) {
				capture_size.width = 2592;
				capture_size.height = 1952;
			} else {*/
				capture_size.width = (cmr_u32)mCaptureWidth;
				capture_size.height = (cmr_u32)mCaptureHeight;
			//}
		} else {//API 2.0 ZSL
			capture_size.width = (cmr_u32)mRawWidth;
			capture_size.height = (cmr_u32)mRawHeight;
		}
	}
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE, (cmr_uint)&capture_size);

	HAL_LOGD("preview_size.width %d, preview_size.height %d, video_size.width %d, video_size.height %d, capture_size.width %d, capture_size.height %d", preview_size.width, preview_size.height, video_size.width, video_size.height, capture_size.width, capture_size.height);
	HAL_LOGD("mCaptureMode %d", mCaptureMode);

	return true;
}

bool SprdCamera3OEMIf::setCameraCaptureDimensions()
{
	uint32_t local_width = 0, local_height = 0;
	uint32_t mem_size = 0;
	struct img_size capture_size;

	HAL_LOGD("E: mCaptureWidth=%d, mCaptureHeight=%d, mPreviewWidth=%d, mPreviewHeight=%d, mVideoWidth=%d, mVideoHeight=%d", mCaptureWidth, mCaptureHeight,mPreviewWidth,mPreviewHeight, mVideoWidth, mVideoHeight);
	if(mCaptureMode != CAMERA_ZSL_MODE){
		mVideoWidth = 0;
		mVideoHeight = 0;
	}
	mHalOem->ops->camera_fast_ctrl(mCameraHandle, CAMERA_FAST_MODE_FD, 0);

	if(mCaptureWidth != 0 && mCaptureHeight != 0) {
		if(mVideoWidth != 0 && mVideoHeight != 0 && ((mCaptureMode != CAMERA_ISP_TUNING_MODE) && (mCaptureMode != CAMERA_ISP_SIMULATION_MODE))) {
			capture_size.width = (cmr_u32)mPreviewWidth;//mVideoWidth;
			capture_size.height = (cmr_u32)mPreviewHeight;//mVideoHeight;
		} else {
			capture_size.width = (cmr_u32)mCaptureWidth;
			capture_size.height = (cmr_u32)mCaptureHeight;
		}
	} else {
		capture_size.width = (cmr_u32)mRawWidth;
		capture_size.height = (cmr_u32)mRawHeight;
	}
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE, (cmr_uint)&capture_size);

	HAL_LOGD("X");

	return true;
}

void SprdCamera3OEMIf::setCameraPreviewMode(bool isRecordMode)
{
	struct cmr_range_fps_param fps_param;
	CONTROL_Tag controlInfo;
	mSetting->getCONTROLTag(&controlInfo);

	fps_param.is_recording = isRecordMode;
	if (isRecordMode) {
		fps_param.min_fps = controlInfo.ae_target_fps_range[1];
		fps_param.max_fps = controlInfo.ae_target_fps_range[1];
		fps_param.video_mode = 1;

		//to set recording fps by setprop
		char prop[PROPERTY_VALUE_MAX];
		int val_max = 0;
		int val_min = 0;
		property_get("persist.sys.camera.record.fps", prop, "0");
		if(atoi(prop) != 0) {
			val_min = atoi(prop)%100;
			val_max = atoi(prop)/100;
			fps_param.min_fps = val_min > 5 ? val_min : 5;
			fps_param.max_fps = val_max;
		}

	} else {
		fps_param.min_fps = controlInfo.ae_target_fps_range[0];
		fps_param.max_fps = controlInfo.ae_target_fps_range[1];
		fps_param.video_mode = 0;

		//to set preview fps by setprop
		char prop[PROPERTY_VALUE_MAX];
		int val_max = 0;
		int val_min = 0;
		property_get("persist.sys.camera.preview.fps", prop, "0");
		if(atoi(prop) != 0) {
			val_min = atoi(prop)%100;
			val_max = atoi(prop)/100;
			fps_param.min_fps = val_min > 5 ? val_min : 5;
			fps_param.max_fps = val_max;
		}
	}
	fps_param.old_min_fps=&fps_range_min_old;
	fps_param.old_max_fps=&fps_range_max_old;
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_RANGE_FPS, (cmr_uint)&fps_param);

	HAL_LOGD("min_fps=%d, max_fps=%d, video_mode=%d", fps_param.min_fps, fps_param.max_fps, fps_param.video_mode);
#if 1 //for cts
	mIsUpdateRangeFps = true;
	mUpdateRangeFpsCount++;
//	HAL_LOGD("mUpdateRangeFpsCount=%d", mUpdateRangeFpsCount);
	if (mUpdateRangeFpsCount == UPDATE_RANGE_FPS_COUNT) {
		if ((mPrvMinFps == fps_param.min_fps) && (mPrvMaxFps == fps_param.max_fps))
			mUpdateRangeFpsCount = 0;
	}
	mPrvMinFps = fps_param.min_fps;
	mPrvMaxFps = fps_param.max_fps;
#endif
}

bool SprdCamera3OEMIf::setCameraPreviewFormat()
{
	HAL_LOGD("E: mPreviewFormat=%d, mPictureFormat=%d", mPreviewFormat, mPictureFormat);

	mPictureFormat = mPreviewFormat;
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PREVIEW_FORMAT, (cmr_uint)mPreviewFormat);
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_FORMAT, (cmr_uint)mPictureFormat);

	HAL_LOGD("X");

	return true;
}

void SprdCamera3OEMIf::setCameraState(Sprd_camera_state state, state_owner owner)
{
	Sprd_camera_state   org_state   = SPRD_IDLE;
	volatile Sprd_camera_state      * state_owner = NULL;
	Mutex::Autolock stateLock(&mStateLock);
//	HAL_LOGD("E:state: %s, owner: %d", getCameraStateStr(state), owner);

	if(owner == STATE_CAPTURE) {
		if(mCameraState.capture_state == SPRD_INTERNAL_CAPTURE_STOPPING
			&& state != SPRD_INIT
			&& state != SPRD_ERROR
			&& state != SPRD_IDLE){
			HAL_LOGD("capture_state = SPRD_INTERNAL_CAPTURE_STOPPING , return");
			return;
		}
	}

	switch (owner) {
		case STATE_CAMERA:
			org_state = mCameraState.camera_state;
			state_owner = &(mCameraState.camera_state);
			break;

		case STATE_PREVIEW:
			org_state = mCameraState.preview_state;
			state_owner = &(mCameraState.preview_state);
			break;

		case STATE_CAPTURE:
			org_state = mCameraState.capture_state;
			state_owner = &(mCameraState.capture_state);
			break;

		case STATE_FOCUS:
			org_state = mCameraState.focus_state;
			state_owner = &(mCameraState.focus_state);
			break;

		case STATE_SET_PARAMS:
			org_state = mCameraState.setParam_state;
			state_owner = &(mCameraState.setParam_state);
			break;
		default:
			HAL_LOGE("owner error!");
			break;
	}

	switch (state) {
	/*camera state*/
	case SPRD_INIT:
		mCameraState.camera_state = SPRD_INIT;
		mCameraState.preview_state = SPRD_IDLE;
		mCameraState.capture_state = SPRD_IDLE;
		mCameraState.focus_state = SPRD_IDLE;
		mCameraState.setParam_state = SPRD_IDLE;
		break;

	case SPRD_IDLE:
		*state_owner = SPRD_IDLE;
		break;

	case SPRD_INTERNAL_STOPPING:
		mCameraState.camera_state = state;
		break;

	case SPRD_ERROR:
		*state_owner = SPRD_ERROR;
		break;

	/*preview state*/
	case SPRD_PREVIEW_IN_PROGRESS:
	case SPRD_INTERNAL_PREVIEW_STOPPING:
	case SPRD_INTERNAL_PREVIEW_REQUESTED:
		mCameraState.preview_state = state;
		break;

	/*capture state*/
	case SPRD_FLASH_IN_PROGRESS:
	case SPRD_INTERNAL_RAW_REQUESTED:
	case SPRD_WAITING_RAW:
	case SPRD_WAITING_JPEG:
	case SPRD_INTERNAL_CAPTURE_STOPPING:
		mCameraState.capture_state = state;
		break;

	/*focus state*/
	case SPRD_FOCUS_IN_PROGRESS:
		mCameraState.focus_state = state;
		break;

	/*set_param state*/
	case SPRD_SET_PARAMS_IN_PROGRESS:
		mCameraState.setParam_state = state;
		break;

	default:
		HAL_LOGE("unknown owner");
		break;
	}

	if (org_state != state)
        //mStateWait.signal();              /*if state changed should broadcasting*/
        //use broadcast can make sure all wait thread can recive this message.
        /** SPRD: add { */
        mStateWait.broadcast();
        /** SPRD: add } */

	HAL_LOGD("owner: %d  camera_state = %s, preview_state = %s, capture state = %s, focus_state = %s, setParam_state = %s",
				owner,
				getCameraStateStr(mCameraState.camera_state),
				getCameraStateStr(mCameraState.preview_state),
				getCameraStateStr(mCameraState.capture_state),
				getCameraStateStr(mCameraState.focus_state),
				getCameraStateStr(mCameraState.setParam_state));

}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getCameraState()
{
	HAL_LOGD("%s", getCameraStateStr(mCameraState.camera_state));
	return mCameraState.camera_state;
}

camera_status_t SprdCamera3OEMIf::GetCameraStatus(camera_status_type_t state)
{
	camera_status_t ret = CAMERA_PREVIEW_IN_PROC;
	switch (state) {
		case CAMERA_STATUS_PREVIEW:
			switch (mCameraState.preview_state) {
			case SPRD_IDLE:
				ret = CAMERA_PREVIEW_IDLE;
				break;
			default:
				break;
			}
			break;
		case CAMERA_STATUS_SNAPSHOT:
			ret = CAMERA_SNAPSHOT_IDLE;
			switch (mCameraState.capture_state) {
			case SPRD_INTERNAL_RAW_REQUESTED:
			case SPRD_WAITING_JPEG:
			case SPRD_WAITING_RAW:
				ret = CAMERA_SNAPSHOT_IN_PROC;
				break;
			default:
				break;
			}
			break;
	}
	return ret;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getPreviewState()
{
	HAL_LOGV("%s", getCameraStateStr(mCameraState.preview_state));
	return mCameraState.preview_state;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getCaptureState()
{
	Mutex::Autolock stateLock(&mStateLock);
	HAL_LOGV("%s", getCameraStateStr(mCameraState.capture_state));
	return mCameraState.capture_state;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getFocusState()
{
	HAL_LOGD("%s", getCameraStateStr(mCameraState.focus_state));
	return mCameraState.focus_state;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getSetParamsState()
{
	HAL_LOGD("%s", getCameraStateStr(mCameraState.setParam_state));
	return mCameraState.setParam_state;
}

bool SprdCamera3OEMIf::isCameraInit()
{
	HAL_LOGD("%s", getCameraStateStr(mCameraState.camera_state));
	return (SPRD_IDLE == mCameraState.camera_state);
}

bool SprdCamera3OEMIf::isCameraIdle()
{
	return (SPRD_IDLE == mCameraState.preview_state
			&& SPRD_IDLE == mCameraState.capture_state);
}

bool SprdCamera3OEMIf::isPreviewing()
{
	HAL_LOGD("%s", getCameraStateStr(mCameraState.preview_state));
	return (SPRD_PREVIEW_IN_PROGRESS == mCameraState.preview_state);
}

bool SprdCamera3OEMIf::isPreviewStart()
{
	HAL_LOGV("%s", getCameraStateStr(mCameraState.preview_state));
	return (SPRD_INTERNAL_PREVIEW_REQUESTED == mCameraState.preview_state);
}

bool SprdCamera3OEMIf::isCapturing()
{
	bool ret = false;
	HAL_LOGV("%s", getCameraStateStr(mCameraState.capture_state));
	if (SPRD_FLASH_IN_PROGRESS == mCameraState.capture_state) {
		return false;
	}

	if ((SPRD_INTERNAL_RAW_REQUESTED == mCameraState.capture_state) ||
		(SPRD_WAITING_RAW == mCameraState.capture_state) ||
		(SPRD_WAITING_JPEG == mCameraState.capture_state)) {
		ret = true;
	} else if ((SPRD_INTERNAL_CAPTURE_STOPPING == mCameraState.capture_state) ||
			(SPRD_ERROR == mCameraState.capture_state)) {
			setCameraState(SPRD_IDLE, STATE_CAPTURE);
			HAL_LOGD("%s", getCameraStateStr(mCameraState.capture_state));
	} else if (SPRD_IDLE != mCameraState.capture_state) {
		HAL_LOGE("error: unknown capture status");
		ret = true;
	}
	return ret;
}

bool SprdCamera3OEMIf::checkPreviewStateForCapture()
{
	bool ret = true;
	Sprd_camera_state tmpState = SPRD_IDLE;

	tmpState = getCaptureState();
	if ((SPRD_INTERNAL_CAPTURE_STOPPING == tmpState) ||
		(SPRD_ERROR == tmpState)) {
		HAL_LOGE("incorrect capture status %s", getCameraStateStr(tmpState));
		ret = false;
	} else {
		tmpState = getPreviewState();
		if (iSZslMode()) {
			if (SPRD_PREVIEW_IN_PROGRESS != tmpState) {
				HAL_LOGE("incorrect preview status %d of ZSL capture mode", (uint32_t)tmpState);
				ret = false;
			}
		} else {
			if (SPRD_IDLE != tmpState) {
				HAL_LOGE("incorrect preview status %d of normal capture mode", (uint32_t)tmpState);
				ret = false;
			}
		}
	}
	return ret;
}

bool SprdCamera3OEMIf::WaitForCameraStart()
{
	HAL_LOGD("E");
	Mutex::Autolock stateLock(&mStateLock);

	while(SPRD_IDLE != mCameraState.camera_state
		&& SPRD_ERROR != mCameraState.camera_state) {
		HAL_LOGD("waiting for SPRD_IDLE");
		mStateWait.wait(mStateLock);
		HAL_LOGD("woke up");
	}

	HAL_LOGD("X");
	return SPRD_IDLE == mCameraState.camera_state;
}

bool SprdCamera3OEMIf::WaitForCameraStop()
{
	HAL_LOGD("E");
	Mutex::Autolock stateLock(&mStateLock);

	if (SPRD_INTERNAL_STOPPING == mCameraState.camera_state) {
		while(SPRD_INIT != mCameraState.camera_state
			&& SPRD_ERROR != mCameraState.camera_state) {
			HAL_LOGD("waiting for SPRD_IDLE");
			mStateWait.wait(mStateLock);
			HAL_LOGD("woke up");
		}
	}

	HAL_LOGD("X");

	return SPRD_INIT == mCameraState.camera_state;
}

bool SprdCamera3OEMIf::WaitForPreviewStart()
{
	HAL_LOGD("E");
	Mutex::Autolock stateLock(&mStateLock);

	while (SPRD_PREVIEW_IN_PROGRESS != mCameraState.preview_state
		&& SPRD_ERROR != mCameraState.preview_state) {
		HAL_LOGD("waiting for SPRD_PREVIEW_IN_PROGRESS");
		if (mStateWait.waitRelative(mStateLock, PREV_TIMEOUT)) {
			HAL_LOGE("timeout");
			break;
		}
		HAL_LOGD("woke up");
	}

	HAL_LOGD("X");

	return SPRD_PREVIEW_IN_PROGRESS == mCameraState.preview_state;
}

bool SprdCamera3OEMIf::WaitForPreviewStop()
{
	Mutex::Autolock statelock(&mStateLock);

	while (SPRD_IDLE != mCameraState.preview_state
		&& SPRD_ERROR != mCameraState.preview_state) {
		HAL_LOGD("waiting for SPRD_IDLE");
		if (mStateWait.waitRelative(mStateLock, PREV_STOP_TIMEOUT)) {
			HAL_LOGE("Timeout");
			break;
		}
		HAL_LOGD("woke up");
	}

	return SPRD_IDLE == mCameraState.preview_state;
}

bool SprdCamera3OEMIf::WaitForCaptureStart()
{
	HAL_LOGD("E");
	Mutex::Autolock stateLock(&mStateLock);

	/* It's possible for the YUV callback as well as the JPEG callbacks*/
	/*to be invoked before we even make it here, so we check for all*/
	/*possible result states from takePicture.*/
	while (SPRD_WAITING_RAW != mCameraState.capture_state
		&& SPRD_WAITING_JPEG != mCameraState.capture_state
		&& SPRD_IDLE != mCameraState.capture_state
		&& SPRD_ERROR != mCameraState.capture_state
		&& SPRD_ERROR != mCameraState.camera_state) {
		HAL_LOGD("waiting for SPRD_WAITING_RAW or SPRD_WAITING_JPEG");
		if (mStateWait.waitRelative(mStateLock, CAP_START_TIMEOUT)) {
			HAL_LOGE("timeout");
			break;
		}

		HAL_LOGD("woke up, state is %s",
			getCameraStateStr(mCameraState.capture_state));
	}

	HAL_LOGD("X");
	return (SPRD_WAITING_RAW == mCameraState.capture_state
		|| SPRD_WAITING_JPEG == mCameraState.capture_state
		|| SPRD_IDLE == mCameraState.capture_state);
}

bool SprdCamera3OEMIf::WaitForCaptureDone()
{
	Mutex::Autolock stateLock(&mStateLock);
	while (SPRD_IDLE != mCameraState.capture_state
		&& SPRD_ERROR != mCameraState.capture_state) {

		HAL_LOGD("waiting for SPRD_IDLE");

		/*if (camera_capture_is_idle()) {
			HAL_LOGD("WaitForCaptureDone: for OEM cap is IDLE, set capture state to %s",
				getCameraStateStr(mCameraState.capture_state));
			setCameraState(SPRD_IDLE, STATE_CAPTURE);
		} else {*/
			if (mStateWait.waitRelative(mStateLock, CAP_TIMEOUT)) {
				HAL_LOGE("timeout");
				break;
			}
		//}

		HAL_LOGD("woke up");
	}

	return SPRD_IDLE == mCameraState.capture_state;
}

bool SprdCamera3OEMIf::WaitForBurstCaptureDone()
{
	Mutex::Autolock stateLock(&mStateLock);

	SprdCamera3PicChannel *pic_channel = reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);
	SprdCamera3Stream *callback_stream = NULL;
	SprdCamera3Stream *jpeg_stream = NULL;
	int32_t jpeg_ret = BAD_VALUE;
	int32_t callback_ret = BAD_VALUE;
	cmr_uint jpeg_vir,  callback_vir;

	if(pic_channel == NULL)
		return true;
	HAL_LOGD("pic_channel 0x%lx", pic_channel);
	pic_channel->getStream(CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT, &jpeg_stream);
	pic_channel->getStream(CAMERA_STREAM_TYPE_PICTURE_CALLBACK, &callback_stream);
	HAL_LOGD("jpeg_stream 0x%lx, callback_stream 0x%lx", jpeg_stream, callback_stream);
	if(jpeg_stream == NULL && callback_stream == NULL)
		return true;

	if(jpeg_stream)
		jpeg_ret = jpeg_stream->getQBuffFirstVir(&jpeg_vir);
	if(callback_stream)
		callback_ret = callback_stream->getQBuffFirstVir(&callback_vir);

	HAL_LOGD("jpeg_ret %d, callback_ret %d, NO_ERROR %d", jpeg_ret, callback_ret, NO_ERROR);

	while (NO_ERROR == jpeg_ret || NO_ERROR == callback_ret) {

		HAL_LOGD("waiting for jpeg_ret = %d, callback_ret = %d", jpeg_ret, callback_ret);

		if((mTakePictureMode == SNAPSHOT_PREVIEW_MODE || mTakePictureMode == SNAPSHOT_ONLY_MODE)&& mCameraState.capture_state == SPRD_IDLE){
			return true;
		}

		if (mStateWait.waitRelative(mStateLock, CAP_TIMEOUT)) {
			HAL_LOGE("timeout");
			return false;
		}

		if(jpeg_stream)
			jpeg_ret = jpeg_stream->getQBuffFirstVir(&jpeg_vir);
		if(callback_stream)
			callback_ret = callback_stream->getQBuffFirstVir(&callback_vir);

		HAL_LOGD("woke up:  jpeg_ret = %d, callback_ret = %d", jpeg_ret, callback_ret);
	}

	return true;
}

bool SprdCamera3OEMIf::WaitForCaptureJpegState()
{
	Mutex::Autolock stateLock(&mStateLock);

	while (SPRD_WAITING_JPEG != mCameraState.capture_state
		&& SPRD_IDLE != mCameraState.capture_state
		&& SPRD_ERROR != mCameraState.capture_state
		&& SPRD_INTERNAL_CAPTURE_STOPPING != mCameraState.capture_state) {
		HAL_LOGD("waiting for SPRD_WAITING_JPEG");
		mStateWait.wait(mStateLock);
		HAL_LOGD("woke up, state is %s",
			getCameraStateStr(mCameraState.capture_state));
	}
	return (SPRD_WAITING_JPEG == mCameraState.capture_state);
}

bool SprdCamera3OEMIf::WaitForFocusCancelDone()
{
	Mutex::Autolock stateLock(&mStateLock);
	while (SPRD_IDLE != mCameraState.focus_state
		&& SPRD_ERROR != mCameraState.focus_state) {
		HAL_LOGD("waiting for SPRD_IDLE from %s",
			getCameraStateStr(getFocusState()));
		if (mStateWait.waitRelative(mStateLock, CANCEL_AF_TIMEOUT)) {
			HAL_LOGV("timeout");
		}
		HAL_LOGD("woke up");
	}

	return SPRD_IDLE == mCameraState.focus_state;
}

bool SprdCamera3OEMIf::startCameraIfNecessary()
{
	cmr_uint is_support_zsl = 0;
	cmr_uint max_width = 0;
	cmr_uint max_height = 0;
	struct exif_info exif_info = {0};
	LENS_Tag lensInfo;

	if (!isCameraInit()) {
		HAL_LOGD("wait for camera_init");
		if (CMR_CAMERA_SUCCESS != mHalOem->ops->camera_init_with_mem_func(mCameraId, camera_cb, this, 0, &mCameraHandle, (void*)Callback_Malloc, (void*)Callback_Free)) {
			setCameraState(SPRD_INIT);
			HAL_LOGE("CameraIfNecessary: fail to camera_init().");
			return false;
		} else {
			setCameraState(SPRD_IDLE);
		}
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
		cmr_handle isp_handle = 0;
		mHalOem->ops->camera_get_isp_handle(mCameraHandle, &isp_handle);
		setispserver(isp_handle);
#endif
		mHalOem->ops->camera_get_zsl_capability(mCameraHandle, &is_support_zsl, &max_width, &max_height);

		if (!is_support_zsl) {
			mParameters.setZSLSupport("false");
		}

		/*if (isPreAllocCapMem()) {
			pre_alloc_cap_mem_thread_init((void *)this);
		}*/

		/*get sensor and lens info from oem layer*/
		mHalOem->ops->camera_get_sensor_exif_info(mCameraHandle, &exif_info);
		mSetting->getLENSTag(&lensInfo);
		lensInfo.aperture = exif_info.aperture;
		mSetting->setLENSTag(lensInfo);
		HAL_LOGD("camera_id: %d.apert %f", mCameraId, lensInfo.aperture);
		/*get sensor and lens info from oem layer*/
		set_ddr_freq(BASE_FREQ_REQ);

		HAL_LOGD("initializing param");
	} else {
		HAL_LOGD("camera hardware has been started already");
	}

	return true;
}

sprd_camera_memory_t* SprdCamera3OEMIf::allocCameraMem(int buf_size, int num_bufs, uint32_t is_cache)
{
	unsigned long paddr = 0;
	size_t psize = 0;
	int result = 0;
	size_t mem_size = 0;
	MemoryHeapIon *pHeapIon = NULL;

	HAL_LOGD("buf_size %d, num_bufs %d", buf_size, num_bufs);

	sprd_camera_memory_t *memory = (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
	if (NULL == memory) {
		HAL_LOGE("fatal error! memory pointer is null.");
		goto getpmem_fail;
	}
	memset(memory, 0 , sizeof(sprd_camera_memory_t));
	memory->busy_flag = false;

	mem_size = buf_size * num_bufs ;
	if(mem_size == 0) {
		goto getpmem_fail;
	}

	if (0 == s_mem_method) {
		if (is_cache) {
			pHeapIon = new MemoryHeapIon("/dev/ion", mem_size ,0 , (1<<31) | ION_HEAP_ID_MASK_MM);
		} else {
			pHeapIon = new MemoryHeapIon("/dev/ion", mem_size , MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
		}
	} else {
		if (is_cache) {
			pHeapIon = new MemoryHeapIon("/dev/ion", mem_size ,0 , (1<<31) | ION_HEAP_ID_MASK_SYSTEM);
		} else {
			pHeapIon = new MemoryHeapIon("/dev/ion", mem_size , MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
		}
	}

	if (NULL == pHeapIon) {
		HAL_LOGE("error pHeapIon is null.");
		goto getpmem_fail;
	}
	if (0 > pHeapIon->getHeapID()) {
		HAL_LOGE("error HeapID.");
		goto getpmem_fail;
	}
	if (NULL == pHeapIon->getBase()
		|| MAP_FAILED == pHeapIon->getBase()) {
		HAL_LOGE("error getBase is null.");
		goto getpmem_fail;
	}

	if (0 == s_mem_method) {
		result = pHeapIon->get_phy_addr_from_ion(&paddr, &psize);
	} else {
		result = pHeapIon->get_iova(ION_MM, &paddr, &psize);
	}
	if (result < 0) {
		HAL_LOGE("error get pHeapIon addr - method %d result 0x%x ",s_mem_method, result);
		goto getpmem_fail;
	}

	memory->ion_heap = pHeapIon;
	memory->phys_addr = paddr;
	memory->phys_size = psize;
	memory->data = pHeapIon->getBase();

	if (0 == s_mem_method) {
		HAL_LOGD("X: phys_addr 0x%lx, data: 0x%p, phys_size: 0x%lx heap=%p",
			memory->phys_addr, memory->data, memory->phys_size, pHeapIon);
	} else {
		HAL_LOGD("X: mm_iova: phys_addr 0x%lx, data: 0x%p, phys_size: 0x%lx.heap=%p",
			memory->phys_addr, memory->data,memory->phys_size, pHeapIon);
	}

	return memory;

getpmem_fail:
	if (memory != NULL) {
		free(memory);
		memory = NULL;
	}
	return NULL;
}

void SprdCamera3OEMIf::freeCameraMem(sprd_camera_memory_t* memory)
{
	if (memory) {
		if(memory->ion_heap) {
			if (0 != s_mem_method) {
				HAL_LOGD("free_mm_iova: 0x%x,data: 0x%p, 0x%lx",memory->phys_addr, memory->data,memory->phys_size);
				memory->ion_heap->free_iova(ION_MM, memory->phys_addr, memory->phys_size);
			}
			delete memory->ion_heap;
			memory->ion_heap = NULL;
		}
		free(memory);
	} else {
		HAL_LOGD("null");
	}
}

void SprdCamera3OEMIf::canclePreviewMem()
{
}

int SprdCamera3OEMIf::releasePreviewFrame(int i)
{
	int ret = 0;

	ret = mHalOem->ops->camera_release_frame(mCameraHandle, CAMERA_PREVIEW_DATA, i);
	return ret;
}

bool SprdCamera3OEMIf::initPreview()
{
	HAL_LOGD("E");

	setCameraPreviewMode(mRecordingMode);

	if (!setCameraPreviewDimensions()) {
		HAL_LOGE("setCameraDimensions failed");
		return false;
	}

	setCameraPreviewFormat();

	if (CMR_CAMERA_SUCCESS != mHalOem->ops->camera_set_mem_func(mCameraHandle, (void*)Callback_Malloc, (void*)Callback_Free, (void*)this)) {
		setCameraState(SPRD_ERROR);
		HAL_LOGE("start fail to camera_set_mem_func().");
		return false;
	}

	HAL_LOGD("X");
	return true;
}

void SprdCamera3OEMIf::deinitPreview()
{
	HAL_LOGV("E");

	//freePreviewMem();
	//camera_set_preview_mem(0, 0, 0, 0);

	HAL_LOGV("X");
}

void SprdCamera3OEMIf::deinitCapture(bool isPreAllocCapMem)
{
	HAL_LOGD("E %d", isPreAllocCapMem);

	if (0 == isPreAllocCapMem) {
		Callback_CaptureFree(0, 0, 0);
	} else {
		HAL_LOGD("pre_allocate mode.");
	}

        Callback_OtherFree(CAMERA_SNAPSHOT_ZSL_RESERVED_MEM, 0, 0, 0);

	/*
	if(mCaptureMode != CAMERA_ZSL_MODE)
		Callback_CaptureFree(0, 0, 0);
	*/
	Callback_CapturePathFree(0, 0, 0);
}

int SprdCamera3OEMIf::set_ddr_freq(uint32_t mhzVal)
{
	const char*     freq_in_khz = NO_FREQ_STR;
	uint32_t        tmpSetFreqCount = mSetDDRFreqCount;

	HAL_LOGV("to %d now count %d freq %d E", mhzVal, mSetDDRFreqCount, mSetDDRFreq);
	if (mhzVal == mSetDDRFreq && NO_FREQ_REQ != mhzVal) {
		HAL_LOGW("set_ddr_freq same freq %d need not set", mhzVal);
		return NO_ERROR;
	}

	const char* const set_freq = "/sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_freq";

	FILE* fp = fopen(set_freq, "wb");
	if (NULL == fp) {
		HAL_LOGV("failed to open %s X", set_freq);
		return BAD_VALUE;
	}

	switch (mhzVal) {
		case NO_FREQ_REQ:
			tmpSetFreqCount--;
			break;

		case BASE_FREQ_REQ:
			if (NO_FREQ_REQ == mSetDDRFreq) {
				tmpSetFreqCount++;
			} else {
				HAL_LOGV("clear freq for change!");
				fprintf(fp, "%s", NO_FREQ_STR);
			}
			freq_in_khz = BASE_FREQ_STR;
			break;

		case MEDIUM_FREQ_REQ:
			if (NO_FREQ_REQ == mSetDDRFreq) {
				tmpSetFreqCount++;
			} else {
				HAL_LOGV("clear freq for change!");
				fprintf(fp, "%s", NO_FREQ_STR);
			}
			freq_in_khz = MEDIUM_FREQ_STR;
			break;

		case HIGH_FREQ_REQ:
			if (NO_FREQ_REQ == mSetDDRFreq) {
				tmpSetFreqCount++;
			} else {
				HAL_LOGV("clear freq for change!");
				fprintf(fp, "%s", NO_FREQ_STR);
			}
			freq_in_khz = HIGH_FREQ_STR;
			break;

		default:
			HAL_LOGE("unrecognize set frequency, error!");
			break;
	}

	fclose(fp);
	fp = NULL;

	fp = fopen(set_freq, "wb");
	if (NULL == fp) {
		HAL_LOGE("failed to open %s X", set_freq);
		return BAD_VALUE;
	}

	fprintf(fp, "%s", freq_in_khz);
	mSetDDRFreq = mhzVal;
	mSetDDRFreqCount = tmpSetFreqCount;
	HAL_LOGD("to %skhz now count %d freq %d X", freq_in_khz, mSetDDRFreqCount, mSetDDRFreq);
	fclose(fp);
	return NO_ERROR;
}

int SprdCamera3OEMIf::startPreviewInternal()
{
	bool is_push_zsl = false;
	char value[PROPERTY_VALUE_MAX];
	property_get("persist.sys.camera.raw.mode", value, "jpeg");
	LENS_Tag lensInfo;
	mSetting->getLENSTag(&lensInfo);

	HAL_LOGD("E");

	if (isCapturing()) {
		//WaitForCaptureDone();
		WaitForBurstCaptureDone();
		cancelPictureInternal();
	}

	if(isPreviewStart()) {
		HAL_LOGV("Preview not start! wait preview start");
		WaitForPreviewStart();
	}

	if (isPreviewing()) {
		HAL_LOGD("X: Preview already in progress, mRestartFlag=%d", mRestartFlag);
		if(mRestartFlag == false) {
			return NO_ERROR;
		} else
			stopPreviewInternal();
	}
	mRestartFlag = false;

	if (!initPreview()) {
		HAL_LOGE("initPreview failed.  Not starting preview.");
		deinitPreview();
		return UNKNOWN_ERROR;
	}
	SprdCamera3Flash::reserveFlash(mCameraId);

	set_ddr_freq(BASE_FREQ_REQ);
	if (mCaptureMode == CAMERA_ZSL_MODE) {
		memset(mZSLCapInfo, 0, sizeof(mZSLCapInfo));
		struct img_size jpeg_thumb_size;
		jpeg_thumb_size.width = 320;
		jpeg_thumb_size.height = 240;
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE, (cmr_uint)&jpeg_thumb_size);

		set_ddr_freq(HIGH_FREQ_REQ);
	}

	if(lensInfo.facing) {
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_LENS_FACING, (cmr_uint)(lensInfo.facing));
		HAL_LOGD("lensInfo.facing = %d",lensInfo.facing);
	}

	setCameraState(SPRD_INTERNAL_PREVIEW_REQUESTED, STATE_PREVIEW);
#ifdef CONFIG_CAMERA_ROTATION_CAPTURE
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 1);
	if (!strcmp(value, "raw")) {
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
	}
#else
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
#endif

#ifdef CONFIG_SPRD_PRIVATE_ZSL
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_ZSL_ENABLED, (cmr_uint)mSprdZslEnabled);
#endif
	cmr_int qret = mHalOem->ops->camera_start_preview(mCameraHandle, mCaptureMode);
	if (qret != CMR_CAMERA_SUCCESS) {
		HAL_LOGE("failed: sensor error.");
		setCameraState(SPRD_ERROR, STATE_PREVIEW);
		deinitPreview();
		return UNKNOWN_ERROR;
	}
	if (mIspToolStart) {
		mIspToolStart = false;
	}
	if (PushFirstPreviewbuff()) {
		is_push_zsl = true;
		PushFirstZslbuff();
	}
	if (mCaptureMode == CAMERA_ZSL_MODE) {
		PushFirstVideobuff();
		if (!is_push_zsl)
			PushFirstZslbuff();
	}
	HAL_LOGD("X");

	return NO_ERROR;
}

void SprdCamera3OEMIf::stopPreviewInternal()
{
	nsecs_t start_timestamp = systemTime();
	nsecs_t end_timestamp;
	mUpdateRangeFpsCount = 0;

	HAL_LOGD("E");
	if (isCapturing()) {
		setCameraState(SPRD_INTERNAL_CAPTURE_STOPPING, STATE_CAPTURE);
		if (0 != mHalOem->ops->camera_cancel_takepicture(mCameraHandle)) {
			HAL_LOGE("camera_stop_capture failed!");
			return;
		}
	}

#ifdef CONFIG_CAMERA_GSP_SCALING
	//stop video, we need to notify GSP to close HWC
	if(mHalOem->ops->camera_notify_closing_gsp_hwc(0))
		HAL_LOGE("stopPreviewInternal: stop GSP failed!");
#endif

	if(isPreviewStart()) {
		HAL_LOGV("Preview not start! wait preview start");
		WaitForPreviewStart();
	}

	if (!isPreviewing()) {
		HAL_LOGD("Preview not in progress! stopPreviewInternal X");
		return;
	}

	setCameraState(SPRD_INTERNAL_PREVIEW_STOPPING, STATE_PREVIEW);
	if(CMR_CAMERA_SUCCESS != mHalOem->ops->camera_stop_preview(mCameraHandle)) {
		setCameraState(SPRD_ERROR, STATE_PREVIEW);
		HAL_LOGE("fail to camera_stop_preview()");
	}

	WaitForPreviewStop();

	deinitPreview();
	end_timestamp = systemTime();

	HAL_LOGD("X Time:%lld(ms).",(end_timestamp - start_timestamp)/1000000);
}

takepicture_mode SprdCamera3OEMIf::getCaptureMode()
{
	HAL_LOGD("cap mode %d.\n", mCaptureMode);

	return mCaptureMode;
}

bool SprdCamera3OEMIf::iSDisplayCaptureFrame()
{
	if(CAMERA_ZSL_MODE == mCaptureMode)
		return false;

	SprdCamera3RegularChannel *channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	SprdCamera3Stream *stream = NULL;
	int32_t ret = BAD_VALUE;
	cmr_uint addr_vir = NULL;

	if(channel) {
		channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &stream);
		if(stream) {
			ret = stream->getQBuffFirstVir(&addr_vir);
		}
	}

	return ret == NO_ERROR;
}

bool SprdCamera3OEMIf::iSZslMode()
{
	bool ret = true;

	/*if ((CAMERA_ZSL_MODE != mCaptureMode)
		&& (CAMERA_ZSL_CONTINUE_SHOT_MODE != mCaptureMode)
		&& (CAMERA_ANDROID_ZSL_MODE != mCaptureMode)) {
		ret = false;
	}*/
	if (CAMERA_ZSL_MODE != mCaptureMode)
		ret = false;

	return ret;
}


int SprdCamera3OEMIf::cancelPictureInternal()
{
	bool result = true;
	HAL_LOGD("E, state %s", getCameraStateStr(getCaptureState()));

	switch (getCaptureState()) {
	case SPRD_INTERNAL_RAW_REQUESTED:
	case SPRD_WAITING_RAW:
	case SPRD_WAITING_JPEG:
	case SPRD_FLASH_IN_PROGRESS:
		HAL_LOGD("camera state is %s, stopping picture.", getCameraStateStr(getCaptureState()));
		mFlushLock.lock();
		setCameraState(SPRD_INTERNAL_CAPTURE_STOPPING, STATE_CAPTURE);
		if (0 != mHalOem->ops->camera_cancel_takepicture(mCameraHandle)) {
			HAL_LOGE("camera_stop_capture failed!");
			return UNKNOWN_ERROR;
		}

		result = WaitForCaptureDone();
		if (!iSZslMode()) {
			deinitCapture(mIsPreAllocCapMem);
			//camera_set_capture_trace(0);
		}
		mFlushLock.unlock();
		break;

	default:
		HAL_LOGW("not taking a picture (state %s)", getCameraStateStr(getCaptureState()));
		break;
	}

	set_ddr_freq(BASE_FREQ_REQ);

	HAL_LOGD("X");
	return result ? NO_ERROR : UNKNOWN_ERROR;
}

void SprdCamera3OEMIf::setCameraPrivateData()
{
/*	camera_sensor_exif_info exif_info;
	const char * isFlashSupport = mParameters.get("flash-mode-supported");
	memset(&exif_info, 0, sizeof(camera_sensor_exif_info));

	if (isFlashSupport
		&& 0 == strcmp("true", (char*)isFlashSupport)) {
		exif_info.flash = 1;
	}

	camera_config_exif_info(&exif_info);*/
}

void SprdCamera3OEMIf::getPictureFormat(int * format)
{
	*format = mPictureFormat;
}

#define PARSE_LOCATION(what,type,fmt,desc) do {\
	pt->what = 0;\
	const char *what##_str = mParameters.get("gps-"#what);\
	HAL_LOGI("%s: GPS PARM %s --> [%s]", __func__, "gps-"#what, what##_str);\
	if (what##_str) {\
		type what = 0;\
		if (sscanf(what##_str, fmt, &what) == 1) {\
		pt->what = what;\
		} else {\
			HAL_LOGE("GPS " #what " %s could not"\
				" be parsed as a " #desc,\
				what##_str);\
			result = false;\
		}\
	} else {\
		HAL_LOGW("%s: GPS " #what " not specified: "\
			"defaulting to zero in EXIF header.", __func__);\
		result = false;\
	}\
}while(0)

int SprdCamera3OEMIf::CameraConvertCoordinateToFramework(int32_t *cropRegion)
{
    float    zoomWidth,zoomHeight;
    uint32_t i = 0;
    int ret = 0;
    uint32_t      sensorOrgW = 0, sensorOrgH = 0, fdWid = 0, fdHeight = 0;
    HAL_LOGD("crop %d %d %d %d",
            cropRegion[0], cropRegion[1], cropRegion[2], cropRegion[3]);
    if(mCameraId == 0) {
        sensorOrgW = BACK_SENSOR_ORIG_WIDTH;
        sensorOrgH = BACK_SENSOR_ORIG_HEIGHT;
    } else if(mCameraId == 1) {
        sensorOrgW = FRONT_SENSOR_ORIG_WIDTH;
        sensorOrgH = FRONT_SENSOR_ORIG_HEIGHT;
    }
    fdWid = cropRegion[2] - cropRegion[0];
    fdHeight = cropRegion[3] - cropRegion[1];
    if (fdWid == 0 || fdHeight == 0){
        HAL_LOGE("parameters error.");
        return 1;
    }
    zoomWidth = (float)sensorOrgW / (float)mPreviewWidth;
    zoomHeight = (float)sensorOrgH / (float)mPreviewHeight;
    cropRegion[0] = (cmr_u32)((float)cropRegion[0] * zoomWidth);
    cropRegion[1] = (cmr_u32)((float)cropRegion[1] * zoomHeight);
    cropRegion[2] = (cmr_u32)((float)cropRegion[2] * zoomWidth);
    cropRegion[3] = (cmr_u32)((float)cropRegion[3] * zoomHeight);
    HAL_LOGD("Crop calculated (xs=%d,ys=%d,xe=%d,ye=%d)",
                cropRegion[0], cropRegion[1], cropRegion[2], cropRegion[3]);
    return ret;
}

int SprdCamera3OEMIf::CameraConvertCoordinateFromFramework(int32_t *cropRegion)
{
    float    zoomWidth,zoomHeight;
    uint32_t i = 0;
    int ret = 0;
    uint32_t      sensorOrgW = 0, sensorOrgH = 0, fdWid = 0, fdHeight = 0;
    HAL_LOGD("crop %d %d %d %d",
            cropRegion[0], cropRegion[1], cropRegion[2], cropRegion[3]);
    if(mCameraId == 0) {
        sensorOrgW = BACK_SENSOR_ORIG_WIDTH;
        sensorOrgH = BACK_SENSOR_ORIG_HEIGHT;
    } else if(mCameraId == 1) {
        sensorOrgW = FRONT_SENSOR_ORIG_WIDTH;
        sensorOrgH = FRONT_SENSOR_ORIG_HEIGHT;
    }
    fdWid = cropRegion[2] - cropRegion[0];
    fdHeight = cropRegion[3] - cropRegion[1];
    if (fdWid == 0 || fdHeight == 0){
        HAL_LOGE("parameters error.");
        return 1;
    }
    zoomWidth = (float)mPreviewWidth / (float)sensorOrgW;
    zoomHeight = (float)mPreviewHeight / (float)sensorOrgH;
    cropRegion[0] = (cmr_u32)((float)cropRegion[0] * zoomWidth);
    cropRegion[1] = (cmr_u32)((float)cropRegion[1] * zoomHeight);
    cropRegion[2] = (cmr_u32)((float)cropRegion[2] * zoomWidth);
    cropRegion[3] = (cmr_u32)((float)cropRegion[3] * zoomHeight);
    HAL_LOGD("Crop calculated (xs=%d,ys=%d,xe=%d,ye=%d)",
                cropRegion[0], cropRegion[1], cropRegion[2], cropRegion[3]);
    return ret;
}

bool SprdCamera3OEMIf::getCameraLocation(camera_position_type *pt)
{
	bool result = true;

	PARSE_LOCATION(timestamp, long, "%ld", "long");
	if (0 == pt->timestamp)
		pt->timestamp = time(NULL);

	PARSE_LOCATION(altitude, double, "%lf", "double float");
	PARSE_LOCATION(latitude, double, "%lf", "double float");
	PARSE_LOCATION(longitude, double, "%lf", "double float");

	pt->process_method = mParameters.get("gps-processing-method");
/*
	HAL_LOGV("%s: setting image location result %d,  ALT %lf LAT %lf LON %lf",
			__func__, result, pt->altitude, pt->latitude, pt->longitude);
*/
	return result;
}

int SprdCamera3OEMIf::displayCopy(uintptr_t dst_phy_addr, uintptr_t dst_virtual_addr,
							uintptr_t src_phy_addr, uintptr_t src_virtual_addr, uint32_t src_w, uint32_t src_h)
{
	int ret = 0;

#ifndef MINICAMERA
	if (!mPreviewWindow || !mGrallocHal)
		return -EOWNERDEAD;
#endif

	if (mIsDvPreview) {
		memcpy((void *)dst_virtual_addr, (void *)src_virtual_addr, SIZE_ALIGN(src_w) * SIZE_ALIGN(src_h) * 3/2);
	} else {
		memcpy((void *)dst_virtual_addr, (void *)src_virtual_addr, src_w*src_h*3/2);
	}

	return ret;
}

void SprdCamera3OEMIf::receivePreviewFDFrame(struct camera_frame_type *frame)
{
	if (NULL == frame) {
		HAL_LOGE("invalid frame pointer");
		return;
	}
	Mutex::Autolock l(&mPreviewCbLock);
	FACE_Tag faceInfo;

	ssize_t offset = frame->buf_id;
	//camera_frame_metadata_t metadata;
	//camera_face_t face_info[FACE_DETECT_NUM];
	int32_t k = 0;
	int32_t sx = 0;
	int32_t sy = 0;
	int32_t ex = 0;
	int32_t ey = 0;
	struct img_rect rect = {0};
	mSetting->getFACETag(&faceInfo);
	memset(&faceInfo,0,sizeof(FACE_Tag));
	HAL_LOGD("receive face_num %d.",frame->face_num);
	int32_t number_of_faces = frame->face_num <= FACE_DETECT_NUM ? frame->face_num:FACE_DETECT_NUM;
	faceInfo.face_num = number_of_faces;
	if (0 != number_of_faces) {
		for(k=0 ; k< number_of_faces ; k++) {
			faceInfo.face[k].id = k;
			sx = MIN(MIN(frame->face_info[k].sx, frame->face_info[k].srx),
					MIN(frame->face_info[k].ex, frame->face_info[k].elx));
			sy = MIN(MIN(frame->face_info[k].sy, frame->face_info[k].sry),
					MIN(frame->face_info[k].ey, frame->face_info[k].ely));
			ex = MAX(MAX(frame->face_info[k].sx, frame->face_info[k].srx),
					MAX(frame->face_info[k].ex, frame->face_info[k].elx));
			ey = MAX(MAX(frame->face_info[k].sy, frame->face_info[k].sry),
					MAX(frame->face_info[k].ey, frame->face_info[k].ely));
			HAL_LOGD("face rect:s(%d,%d) sr(%d,%d) e(%d,%d) el(%d,%d)", frame->face_info[k].sx, frame->face_info[k].sy,
				frame->face_info[k].srx, frame->face_info[k].sry, frame->face_info[k].ex, frame->face_info[k].ey,
				frame->face_info[k].elx, frame->face_info[k].ely);
			//faceInfo.face[k].rect[0] = (frame->face_info[k].sx*2000/mPreviewWidth)-1000;
			//faceInfo.face[k].rect[1] = (frame->face_info[k].sy*2000/mPreviewHeight)-1000;
			//faceInfo.face[k].rect[2] = (frame->face_info[k].ex*2000/mPreviewWidth)-1000;
			//faceInfo.face[k].rect[3] = (frame->face_info[k].ey*2000/mPreviewHeight)-1000;
			faceInfo.face[k].rect[0] = sx;
			faceInfo.face[k].rect[1] = sy;
			faceInfo.face[k].rect[2] = ex;
			faceInfo.face[k].rect[3] = ey;
			HAL_LOGD("smile level %d. face:%d  %d  %d  %d \n",frame->face_info[k].smile_level,
				faceInfo.face[k].rect[0],faceInfo.face[k].rect[1],faceInfo.face[k].rect[2],faceInfo.face[k].rect[3]);
			CameraConvertCoordinateToFramework(faceInfo.face[k].rect);
			/*When the half of face at the edge of the screen,the smile level returned by face detection library  can often more than 30.
			In order to repaier this defetion ,so when the face on the screen is too close to the edge of screen, the smile level will be set to 0.
			*/
			if(sx >= 40 && sy >= 40)
				faceInfo.face[k].score = frame->face_info[k].smile_level;
			else
				faceInfo.face[k].score = 0;
			if(faceInfo.face[k].score < 0)
				faceInfo.face[k].score = 0;
		}
	}

	mSetting->setFACETag(faceInfo);


}

SprdCamera3OEMIf::shake_test_state SprdCamera3OEMIf::getShakeTestState()
{
	return mShakeTest.mShakeTestState;
}

void SprdCamera3OEMIf::setShakeTestState(shake_test_state state)
{
	mShakeTest.mShakeTestState = state;
}

int SprdCamera3OEMIf::IommuIsEnabled(void)
{
	return MemoryHeapIon::Mm_iommu_is_enabled();
}

int SprdCamera3OEMIf::allocOneFrameMem(struct SprdCamera3OEMIf::OneFrameMem *one_frame_mem_ptr)
{
	struct SprdCamera3OEMIf::OneFrameMem *tmp_one_frame_mem_ptr = one_frame_mem_ptr;
	int s_mem_method = IommuIsEnabled();
	int ret = 0;

	/* alloc input y buffer */
	HAL_LOGD(" %d  %d  %d\n",s_mem_method,tmp_one_frame_mem_ptr->width,tmp_one_frame_mem_ptr->height);
	if (0 == s_mem_method) {
		tmp_one_frame_mem_ptr->input_y_pmem_hp = new MemoryHeapIon("/dev/ion",
											tmp_one_frame_mem_ptr->width * tmp_one_frame_mem_ptr->height,
											MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
	} else {
		tmp_one_frame_mem_ptr->input_y_pmem_hp = new MemoryHeapIon("/dev/ion",
										tmp_one_frame_mem_ptr->width * tmp_one_frame_mem_ptr->height,
										MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
	}
	if (tmp_one_frame_mem_ptr->input_y_pmem_hp->getHeapID() < 0) {
		HAL_LOGE("failed to alloc input_y pmem buffer.\n");
		return -1;
	}

	if (0 == s_mem_method) {
		ret = tmp_one_frame_mem_ptr->input_y_pmem_hp->get_phy_addr_from_ion(&tmp_one_frame_mem_ptr->input_y_physical_addr,
		&tmp_one_frame_mem_ptr->input_y_pmemory_size);
		if (ret) {
			HAL_LOGE("failed to get_phy_addr_from_ion.\n");
			return -1;
		}
	} else {
		ret = tmp_one_frame_mem_ptr->input_y_pmem_hp->get_iova(ION_MM, &tmp_one_frame_mem_ptr->input_y_physical_addr,
		&tmp_one_frame_mem_ptr->input_y_pmemory_size);
		if (ret) {
			HAL_LOGE("failed to get_mm_iova.\n");
			return -1;
		}
	}
	tmp_one_frame_mem_ptr->input_y_virtual_addr = (unsigned char*)tmp_one_frame_mem_ptr->input_y_pmem_hp->getBase();
	if (!tmp_one_frame_mem_ptr->input_y_physical_addr) {
		HAL_LOGE("failed to alloc input_y pmem buffer:addr is null.\n");
		return -1;
	}

	return 0;

}

int SprdCamera3OEMIf::relaseOneFrameMem(struct SprdCamera3OEMIf::OneFrameMem *one_frame_mem_ptr)
{
	struct SprdCamera3OEMIf::OneFrameMem *tmp_one_frame_mem_ptr = one_frame_mem_ptr;
	int s_mem_method = IommuIsEnabled();

	HAL_LOGD("%d\n",s_mem_method);
	if (tmp_one_frame_mem_ptr->input_y_physical_addr) {
		if (0 == s_mem_method) {
			tmp_one_frame_mem_ptr->input_y_pmem_hp.clear();
		} else {
			tmp_one_frame_mem_ptr->input_y_pmem_hp->free_iova(ION_MM, tmp_one_frame_mem_ptr->input_y_physical_addr,tmp_one_frame_mem_ptr->input_y_pmemory_size);
		}
	}
	return 0;
}

void SprdCamera3OEMIf::cpu_hotplug_disable(uint8_t is_disable)
{
#define DISABLE_CPU_HOTPLUG	"1"
#define ENABLE_CPU_HOTPLUG	"0"
	const char* const hotplug = "/sys/devices/system/cpu/cpuhotplug/cpu_hotplug_disable";
	const char* cmd_str  = DISABLE_CPU_HOTPLUG;
	uint8_t org_flag = 0;

	FILE* fp = fopen(hotplug, "w");

	HAL_LOGD("cpu hotplug disable %d", is_disable);
	if (!fp) {
		HAL_LOGW("Failed to open: cpu_hotplug_disable, %s", hotplug);
		return;
	}

	if(1 == is_disable) {
		cmd_str = DISABLE_CPU_HOTPLUG;
	} else {
		cmd_str = ENABLE_CPU_HOTPLUG;
	}
	fprintf(fp, "%s", cmd_str);
	fclose(fp);

	return;
}

void SprdCamera3OEMIf::cpu_dvfs_disable(uint8_t is_disable)
{
#define DISABLE_CPU_DVFS	"10"
#define ENABLE_CPU_DVFS	    "95"
	const char* const dvfs = "/sys/devices/system/cpu/cpuhotplug/up_threshold";
	const char* cmd_str  = DISABLE_CPU_DVFS;
	uint8_t org_flag = 0;

	FILE* fp = fopen(dvfs, "w");

	HAL_LOGD("is_disable %d", is_disable);

	if (!fp) {
		HAL_LOGW("failed to open: cpu_dvfs_disable, %s", dvfs);
		return;
	}

	if(1 == is_disable) {
		cmd_str = DISABLE_CPU_DVFS;
	} else {
		cmd_str = ENABLE_CPU_DVFS;
	}
	fprintf(fp, "%s", cmd_str);
	fclose(fp);

	return;
}

void  SprdCamera3OEMIf::prepareForPostProcess(void)
{
	int b_need_raise = 0;

#ifdef CONFIG_CAPTURE_DENOISE
	b_need_raise = 1;
#endif

	if (0/* == strcmp("hdr",mSetParameters.get_SceneMode())*/) {
		b_need_raise = 1;
	}

	HAL_LOGD("prepareForPostProcess %d %d", b_need_raise, mCPURaised);

	if (b_need_raise && !mCPURaised) {
		cpu_hotplug_disable(1);
		cpu_dvfs_disable(1);
		mCPURaised = 1;
	}

	return;
}

void  SprdCamera3OEMIf::exitFromPostProcess(void)
{
	HAL_LOGD("mCPURaised = %d", mCPURaised);
	if (mCPURaised) {
		cpu_dvfs_disable(0);
		cpu_hotplug_disable(0);
		mCPURaised = 0;
	}

	return;
}

int SprdCamera3OEMIf::overwritePreviewFrameMemInit(struct SprdCamera3OEMIf::OneFrameMem *one_frame_mem_ptr)
{
	struct SprdCamera3OEMIf::OneFrameMem *tmp_one_frame_mem_ptr = one_frame_mem_ptr;
	uint32_t overwrite_offset = tmp_one_frame_mem_ptr->width*tmp_one_frame_mem_ptr->height *2 /3;
	uint32_t addr_offset = overwrite_offset;
	HAL_LOGD("0 %d %d 0x%x",overwrite_offset,addr_offset,mShakeTest.diff_yuv_color[mShakeTest.mShakeTestColorCount][0]);
	memset(tmp_one_frame_mem_ptr->input_y_virtual_addr, mShakeTest.diff_yuv_color[mShakeTest.mShakeTestColorCount][0], overwrite_offset);
	overwrite_offset = tmp_one_frame_mem_ptr->width*tmp_one_frame_mem_ptr->height /3;
	HAL_LOGD("1 %d %d 0x%x",overwrite_offset,addr_offset,mShakeTest.diff_yuv_color[mShakeTest.mShakeTestColorCount][1]);
	memset((tmp_one_frame_mem_ptr->input_y_virtual_addr + addr_offset), mShakeTest.diff_yuv_color[mShakeTest.mShakeTestColorCount][1], overwrite_offset);
	if ((MAX_LOOP_COLOR_COUNT-1) == mShakeTest.mShakeTestColorCount) {
		mShakeTest.mShakeTestColorCount = 0;
	} else {
		mShakeTest.mShakeTestColorCount++;
	}
	return 0;
}

void SprdCamera3OEMIf::overwritePreviewFrame(struct camera_frame_type *frame)
{
	uint32_t overwrite_offset = 0;

	static struct SprdCamera3OEMIf::OneFrameMem overwrite_preview_frame;
	static struct SprdCamera3OEMIf::OneFrameMem *one_frame_mem_ptr = &overwrite_preview_frame;
	struct SprdCamera3OEMIf::OneFrameMem *tmp_one_frame_mem_ptr = one_frame_mem_ptr;
	tmp_one_frame_mem_ptr->width = frame->width;
	tmp_one_frame_mem_ptr->height = frame->height *3 /2;

	HAL_LOGD("SHAKE_TEST, E");
	allocOneFrameMem(&overwrite_preview_frame);
	overwritePreviewFrameMemInit(&overwrite_preview_frame);
	memcpy((void *)(frame->y_vir_addr + overwrite_offset), (void *)tmp_one_frame_mem_ptr->input_y_virtual_addr, (tmp_one_frame_mem_ptr->width*tmp_one_frame_mem_ptr->height));
	relaseOneFrameMem(&overwrite_preview_frame);
	HAL_LOGD("SHAKE_TEST, X");
}

void SprdCamera3OEMIf::calculateTimestampForSlowmotion(int64_t frm_timestamp)
{
	int64_t diff_timestamp = 0;
	uint8_t tmp_slow_mot = 1;
	SPRD_DEF_Tag sprddefInfo;

	diff_timestamp = frm_timestamp - mSlowPara.last_frm_timestamp;
	mSetting->getSPRDDEFTag(&sprddefInfo);
	HAL_LOGD("diff time=%lld slow=%d", diff_timestamp, sprddefInfo.slowmotion);
	tmp_slow_mot = sprddefInfo.slowmotion;
	if (tmp_slow_mot == 0)
		tmp_slow_mot = 1;

	mSlowPara.rec_timestamp += diff_timestamp * tmp_slow_mot;
	mSlowPara.last_frm_timestamp = frm_timestamp;
}

void SprdCamera3OEMIf::receivePreviewFrame(struct camera_frame_type *frame)
{
	Mutex::Autolock cbLock(&mPreviewCbLock);
	int ret = NO_ERROR;

	if (NULL == frame) {
		HAL_LOGE("invalid frame pointer");
		return;
	}

	HAL_LOGD("E");

	if(SHAKE_TEST == getShakeTestState()) {
		overwritePreviewFrame(frame);
	}

	if (miSPreviewFirstFrame) {
		GET_END_TIME;
		GET_USE_TIME;
		HAL_LOGD("Launch Camera Time:%d(ms).",s_use_time);

		float cam_init_time;
		if (getApctCamInitSupport()) {
			cam_init_time = ((float)(systemTime() - cam_init_begin_time))/1000000000;
			writeCamInitTimeToProc(cam_init_time);
		}
		miSPreviewFirstFrame = 0;

#ifdef HAS_CAMERA_HINTS
		if (m_pPowerModule) {
			if (m_pPowerModule->powerHint) {
				m_pPowerModule->powerHint(m_pPowerModule, POWER_HINT_VIDEO_ENCODE,
					(void *)"state=0");
			}
		}
#endif

	}

#ifdef CONFIG_FACE_BEAUTY
    // init the static parameters table. save the value until the process is restart or the device is restart.
	static int tab_skinWhitenLevel[10]={0,15,25,35,45,55,60,75,80,95};
	static int tab_skinCleanLevel[10]={0,25,45,50,55,60,70,80,85,95};

	SPRD_DEF_Tag sprddefInfo;
	mSetting->getSPRDDEFTag(&sprddefInfo);
//	HAL_LOGD("perfect_skin_level = %d", sprddefInfo.perfect_skin_level);
	int level = sprddefInfo.perfect_skin_level;
	int skinWhitenLevel = 0;
	int skinCleanLevel = 0;
	int level_num = 0;
	// get the property level and parameters value and save in the parameters table.
	// one time only adjust one level's parameters. In order to adjust all the values, six time should be done.
	{
		char str_adb_level[PROPERTY_VALUE_MAX];
		char str_adb_white[PROPERTY_VALUE_MAX];
		char str_adb_clean[PROPERTY_VALUE_MAX];
		int  adb_level_val = 0;
		int  adb_white_val = 0;
		int  adb_clean_val = 0;

		if( ( property_get("persist.sys.camera.beauty.level", str_adb_level, "0")) &&
			( property_get("persist.sys.camera.beauty.white", str_adb_white, "0")) &&
			( property_get("persist.sys.camera.beauty.clean", str_adb_clean, "0"))
		) {
			adb_level_val = atoi(str_adb_level);
			adb_level_val = (adb_level_val < 0) ? 0 : ((adb_level_val > 9) ? 9 : adb_level_val);
			adb_white_val = atoi(str_adb_white);
			// TODO: clip the adb_white_val.
			adb_clean_val = atoi(str_adb_clean);
			// TODO: clip the adb_clean_val.

			// replace the static value.
			tab_skinWhitenLevel[adb_level_val] = adb_white_val;
			tab_skinCleanLevel[adb_level_val] = adb_clean_val;
		}
	}

	// convert the skin_level set by APP to skinWhitenLevel & skinCleanLevel according to the table saved.
	level = (level<0)?0:((level>90)?90:level);
	level_num = level/10;

	skinWhitenLevel = tab_skinWhitenLevel[level_num];
	skinCleanLevel = tab_skinCleanLevel[level_num];

	if (PREVIEW_ZSL_FRAME != frame->type) {

	HAL_LOGD("UCAM skinWhitenLevel is %d, skinCleanLevel is %d", skinWhitenLevel, skinCleanLevel);
	if( skinWhitenLevel > 0 && isPreviewing()){
		faceDectect(1);
		FACE_Tag faceInfo;
		mSetting->getFACETag(&faceInfo);
		if(faceInfo.face_num>0){
			mSkinWhitenNotDetectFDNum = 0;
			isNeedBeautify = true;
			CameraConvertCoordinateFromFramework(faceInfo.face[0].rect);
			mSkinWhitenTsface.left = faceInfo.face[0].rect[0];
			mSkinWhitenTsface.top = faceInfo.face[0].rect[1];
			mSkinWhitenTsface.right = faceInfo.face[0].rect[2];
			mSkinWhitenTsface.bottom = faceInfo.face[0].rect[3];
			HAL_LOGV("UCAM update rect:%d-%d-%d-%d",mSkinWhitenTsface.left,mSkinWhitenTsface.top,mSkinWhitenTsface.right,mSkinWhitenTsface.bottom);
		}
		mSkinWhitenNotDetectFDNum++;
#define SHINWHITED_NOT_DETECTFD_MAXNUM 30
		if(mSkinWhitenNotDetectFDNum>SHINWHITED_NOT_DETECTFD_MAXNUM){
			HAL_LOGD("UCAM:can not find fd in %d frame.reset mSkinWhitenTsface",SHINWHITED_NOT_DETECTFD_MAXNUM);
			mSkinWhitenNotDetectFDNum = 0;
			isNeedBeautify = false;
			memset(&mSkinWhitenTsface,0,sizeof(TSRect));
		}
		TSMakeupData  inMakeupData, outMakeupData;
		unsigned char *yBuf = (unsigned char *)(frame->y_vir_addr);
		unsigned char *uvBuf = (unsigned char *)(frame->y_vir_addr) + frame->width*frame->height ;
		unsigned char * tmpBuf = new unsigned char[frame->width*frame->height* 3 / 2];

		inMakeupData.frameWidth = frame->width;
		inMakeupData.frameHeight = frame->height;
		inMakeupData.yBuf = yBuf;
		inMakeupData.uvBuf = uvBuf;

		outMakeupData.frameWidth = frame->width;
		outMakeupData.frameHeight = frame->height;
		outMakeupData.yBuf = tmpBuf;
		outMakeupData.uvBuf = tmpBuf + frame->width*frame->height ;
		HAL_LOGD("UCAM frameWidth is %d, frameHeight is %d", frame->width, frame->height);

		if (isNeedBeautify && isPreviewing() && frame->width > 0 && frame->height > 0) {
			int mu_retVal = ts_face_beautify(&inMakeupData, &outMakeupData, skinCleanLevel, skinWhitenLevel, &mSkinWhitenTsface,0);
			if(mu_retVal !=  TS_OK) {
				HAL_LOGD("UCAM ts_face_beautify ret is %d.rect:%d-%d-%d-%d", mu_retVal,mSkinWhitenTsface.left,mSkinWhitenTsface.top,mSkinWhitenTsface.right,mSkinWhitenTsface.bottom);
			} else if (isPreviewing()){
				HAL_LOGD("UCAM ts_face_beautify return OK.rect:%d-%d-%d-%d",mSkinWhitenTsface.left,mSkinWhitenTsface.top,mSkinWhitenTsface.right,mSkinWhitenTsface.bottom);
				memcpy((unsigned char *)(frame->y_vir_addr), tmpBuf, frame->width * frame->height * 3 / 2);
			}
		}
		delete tmpBuf;
	       tmpBuf = NULL;
		}
	}
#endif

#ifdef CONFIG_CAMERA_ISP
        if (frame->type == PREVIEW_FRAME) {
                send_img_data(ISP_TOOL_YVU420_2FRAME, mPreviewWidth, mPreviewHeight, (char *)frame->y_vir_addr, frame->width * frame->height * 3 /2);
        }
#endif

#if 0
	int64_t buffer_timestamp = frame->timestamp;
#else //for cts
	int64_t buffer_timestamp = 0;

	if ((!mRecordingMode) && (mUpdateRangeFpsCount >= UPDATE_RANGE_FPS_COUNT)) {
		struct cmr_range_fps_param fps_param;
		CONTROL_Tag controlInfo;
		mSetting->getCONTROLTag(&controlInfo);

		fps_param.min_fps = controlInfo.ae_target_fps_range[0];
		fps_param.max_fps = controlInfo.ae_target_fps_range[1];

		int64_t fps_range_up = 0;
		int64_t fps_range_low = 0;
		int64_t fps_range_offset = 0;
		int64_t timestamp = 0;

		fps_range_offset = 1000000000 / fps_param.max_fps;
		fps_range_low = (1000000000 / fps_param.max_fps) * (1 - 0.015);
		fps_range_up = (1000000000 / fps_param.min_fps) * (1 + 0.015);

		if (mPrvBufferTimestamp == 0) {
			buffer_timestamp = frame->timestamp;
		} else {
			if (mIsUpdateRangeFps) {
				buffer_timestamp = frame->timestamp;
				mIsUpdateRangeFps = false;
			} else {
				timestamp = frame->timestamp - mPrvBufferTimestamp;
				HAL_LOGD("timestamp is %lld", timestamp);
				if ((timestamp > fps_range_low) && (timestamp < fps_range_up)) {
					buffer_timestamp = frame->timestamp;
				} else {
					buffer_timestamp = mPrvBufferTimestamp + fps_range_offset;
					HAL_LOGD("fix buffer_timestamp is %lld", buffer_timestamp);
				}
			}
		}
		mPrvBufferTimestamp = buffer_timestamp;
	} else {
		buffer_timestamp = frame->timestamp;
	}
#endif

	SprdCamera3RegularChannel* channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	cmr_uint pre_addr_vir = NULL, rec_addr_vir = NULL, callback_addr_vir = NULL;
	SprdCamera3Stream *pre_stream = NULL, *rec_stream = NULL, *callback_stream = NULL;
	int32_t pre_dq_num;
	uint32_t frame_num = 0;
	uintptr_t buff_vir = (uintptr_t)(frame->y_vir_addr);
	uint32_t buff_id = frame->buf_id;
	uint16_t img_width = frame->width, img_height = frame->height;
	buffer_handle_t* buff_handle = NULL;
	int32_t buf_deq_num = 0;
	int32_t buf_num = 0;
	SENSOR_Tag sensorInfo;

	mSetting->getSENSORTag(&sensorInfo);
	sensorInfo.timestamp = buffer_timestamp;
	mSetting->setSENSORTag(sensorInfo);

	if(channel) {
		channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &pre_stream);
		channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &rec_stream);
		channel->getStream(CAMERA_STREAM_TYPE_CALLBACK, &callback_stream);

		if(rec_stream) {//recording stream
			//reset timestamp and mIsRecording after recording
			rec_stream->getQBufListNum(&buf_deq_num);
			if (buf_deq_num == 0) {
				mSlowPara.last_frm_timestamp = 0;
				mIsRecording = false;
			}

			ret = rec_stream->getQBufNumForVir(buff_vir, &frame_num);
			if(ret == NO_ERROR) {
				HAL_LOGD("Record, buff_vir = 0x%lx, frame_num = %d, ret = %d, buffer_timestamp = %lld, frame->type = %d rec=%lld",
						buff_vir, frame_num, ret, buffer_timestamp,frame->type, mSlowPara.rec_timestamp);
				if(frame->type == PREVIEW_VIDEO_FRAME && frame_num >= mRecordFrameNum && (frame_num > mPictureFrameNum ||frame_num == 0)) {
					if (mVideoWidth <= mCaptureWidth && mVideoHeight <= mCaptureHeight) {
						if(mVideoShotFlag)
							PushVideoSnapShotbuff(frame_num, CAMERA_STREAM_TYPE_VIDEO);
					}
					if (mSlowPara.last_frm_timestamp == 0) {/*record first frame*/
						mSlowPara.last_frm_timestamp = buffer_timestamp;
						mSlowPara.rec_timestamp = buffer_timestamp;
						mIsRecording = true;
					}
					if (frame_num > mPreviewFrameNum) {/*record first coming*/
						calculateTimestampForSlowmotion(buffer_timestamp);
					}
					channel->channelCbRoutine(frame_num, mSlowPara.rec_timestamp, CAMERA_STREAM_TYPE_VIDEO);
					if(frame_num == (mDropVideoFrameNum+1)) //for IOMMU error
						channel->channelClearInvalidQBuff(mDropVideoFrameNum, buffer_timestamp, CAMERA_STREAM_TYPE_VIDEO);
				}
				else {
					if(frame->type == PREVIEW_VIDEO_CANCELED_FRAME && mVideoShotFlag) {
						if (mVideoWidth <= mCaptureWidth && mVideoHeight <= mCaptureHeight) {
							if(mVideoShotFlag)
								PushVideoSnapShotbuff(frame_num, CAMERA_STREAM_TYPE_VIDEO);
						}
					}
					channel->channelClearInvalidQBuff(frame_num, buffer_timestamp, CAMERA_STREAM_TYPE_VIDEO);
				}

				if(frame_num > mRecordFrameNum)
					mRecordFrameNum = frame_num;
			}
		}

		if(pre_stream) {//preview stream
			ret = pre_stream->getQBufNumForVir(buff_vir, &frame_num);
			if(ret == NO_ERROR) {
				HAL_LOGD("prev buff 0x%lx, num %d, ret %d, time 0x%llx, type = %d rec=%lld", buff_vir, frame_num,
							ret, buffer_timestamp,frame->type, mSlowPara.rec_timestamp);
				if(frame->type == PREVIEW_FRAME && frame_num >= mPreviewFrameNum && (frame_num > mPictureFrameNum ||frame_num == 0)) {
					if (mVideoWidth > mCaptureWidth && mVideoHeight > mCaptureHeight) {
						if(mVideoShotFlag)
							PushVideoSnapShotbuff(frame_num, CAMERA_STREAM_TYPE_PREVIEW);
					}
					if (mIsRecording) {
						if (frame_num > mRecordFrameNum)
							calculateTimestampForSlowmotion(buffer_timestamp);
						channel->channelCbRoutine(frame_num, mSlowPara.rec_timestamp, CAMERA_STREAM_TYPE_PREVIEW);
					} else {
						channel->channelCbRoutine(frame_num, buffer_timestamp, CAMERA_STREAM_TYPE_PREVIEW);
					}
					if(frame_num == (mDropPreviewFrameNum+1)) //for IOMMU error
						channel->channelClearInvalidQBuff(mDropPreviewFrameNum, buffer_timestamp, CAMERA_STREAM_TYPE_PREVIEW);
				} else {
					channel->channelClearInvalidQBuff(frame_num, buffer_timestamp, CAMERA_STREAM_TYPE_PREVIEW);
				}
				if (callback_stream)
					callback_stream->getQBufListNum(&buf_num);
				if (mTakePictureMode == SNAPSHOT_PREVIEW_MODE && buf_num == 0) {
					timer_set(this, 1, timer_hand_take);
				}
				if(frame_num > mPreviewFrameNum)
					mPreviewFrameNum = frame_num;
			} else {
				if(pre_stream)
					pre_stream->getQBufListNum(&buf_num);
				pre_stream = NULL;
			}
		}

		if(callback_stream) {//callback stream
			HAL_LOGD("callback_stream");
			ret = callback_stream->getQBufNumForVir(buff_vir, &frame_num);
			if(ret == NO_ERROR) {
				HAL_LOGD("Callback buff 0x%lx, num %d, ret %d, time 0x%llx, type = %d", buff_vir, frame_num, ret, buffer_timestamp,frame->type);

				if((!pre_stream || (frame->type != PREVIEW_ZSL_CANCELED_FRAME))&& frame_num >= mZslFrameNum && (frame_num > mPictureFrameNum ||frame_num == 0)) {
#ifdef FORCE_PASS_FLEXIBLEYUV_IN_DARK
					mCountFromLastSizeChange ++;
					if (mCountFromLastSizeChange <= 1){
						memset((uintptr_t*)buff_vir,0x07,frame->width * frame->height/100);
						HAL_LOGD(" testAllocationFromCameraFlexibleYuv force add some data = %d", mCountFromLastSizeChange);
					}
#endif
					channel->channelCbRoutine(frame_num, buffer_timestamp, CAMERA_STREAM_TYPE_CALLBACK);
					if(frame_num == (mDropZslFrameNum+1)) //for IOMMU error
						channel->channelClearInvalidQBuff(mDropZslFrameNum, buffer_timestamp, CAMERA_STREAM_TYPE_CALLBACK);
				} else {
					channel->channelClearInvalidQBuff(frame_num, buffer_timestamp, CAMERA_STREAM_TYPE_CALLBACK);
				}
				if (mTakePictureMode == SNAPSHOT_PREVIEW_MODE && buf_num == 0) {
					timer_set(this, 1, timer_hand_take);
				}
				if(frame_num > mZslFrameNum)
					mZslFrameNum = frame_num;
			}
		}
#ifdef CONFIG_SPRD_PRIVATE_ZSL
		if(mSprdZslEnabled) {
			cmr_int need_pause;
			CMR_MSG_INIT(message);
			mHalOem->ops->camera_zsl_snapshot_need_pause(mCameraHandle, &need_pause);
			if (PREVIEW_ZSL_FRAME == frame->type) {
#ifndef CONFIG_NEED_UNMAP
				receiveZslFrame(frame);
#else
				ret = releaseZslBuffer(frame);
				if (ret != 0) {
					HAL_LOGE("releaseZslBuffer failed");
				}

				message.msg_type = CMR_EVT_ZSL_MON_PUSH;
				message.sync_flag = CMR_MSG_SYNC_NONE;
				message.data = NULL;
				HAL_LOGV("msg_type = 0x%x\n", message.msg_type);
				ret = cmr_thread_msg_send((cmr_handle)mZSLModeMonitorMsgQueHandle, &message);
				if(ret){
					HAL_LOGE("Fail to send one msg!");
					return;
				}
#endif
			} else if (PREVIEW_ZSL_CANCELED_FRAME == frame->type) {
				if (!isCapturing() || !need_pause) {
					mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, frame->y_phy_addr, frame->y_vir_addr, frame->zsl_private);
				}
			}

		}
#endif
	}

	HAL_LOGV("X");
}

//display on frame for take picture
uintptr_t SprdCamera3OEMIf::getRedisplayMem(uint32_t width,uint32_t height)
{
	uint32_t buffer_size = mHalOem->ops->camera_get_size_align_page(SIZE_ALIGN(width) * (SIZE_ALIGN(height)) * 3 / 2);

	if (mIsRotCapture) {
		buffer_size <<= 1 ;
	}

	if (mReDisplayHeap) {
		if (buffer_size > mReDisplayHeap->phys_size) {
			HAL_LOGD("Redisplay need re-allocm. 0x%x 0x%lx", buffer_size, mReDisplayHeap->phys_size);
			freeCameraMem(mReDisplayHeap);
			mReDisplayHeap = allocCameraMem(buffer_size, 1, false);
		}
	} else {
		mReDisplayHeap = allocCameraMem(buffer_size, 1, false);
	}

	if (NULL == mReDisplayHeap)
		return 0;

	if (mReDisplayHeap->phys_addr & 0xFF) {
		HAL_LOGE("error mReDisplayHeap is not 256 bytes aligned.");
		return 0;
	}
	HAL_LOGD("addr=0x%p.",mReDisplayHeap->data);

	return mReDisplayHeap->phys_addr;
}

void SprdCamera3OEMIf::FreeReDisplayMem()
{
	HAL_LOGD("free redisplay mem.");
	if(mReDisplayHeap) {
		freeCameraMem(mReDisplayHeap);
		mReDisplayHeap = NULL;
	}
}

bool SprdCamera3OEMIf::displayOneFrameForCapture(uint32_t width, uint32_t height, uintptr_t phy_addr, char *virtual_addr)
{
	HAL_LOGD("E: size = %dx%d, phy_addr = 0x%lx, virtual_addr = 0x%lx", width, height, phy_addr, virtual_addr);

	Mutex::Autolock cbLock(&mPreviewCbLock);
	int64_t timestamp = systemTime();
	SprdCamera3RegularChannel *regular_channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	SprdCamera3PicChannel *pic_channel = reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);
	SprdCamera3Stream *pre_stream = NULL, *pic_stream = NULL;
	int32_t ret = 0;
	cmr_uint addr_vir = NULL, addr_phy = NULL;
	uint32_t frame_num = 0;

	if(regular_channel) {
		regular_channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &pre_stream);
		pic_channel->getStream(CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT, &pic_stream);
		if(pre_stream) {
			if(pic_stream) {
				ret = pic_stream->getQBuffFirstNum(&frame_num);
				if(ret != NO_ERROR)
					ret = pre_stream->getQBuffFirstVir(&addr_vir);
				else
					ret = pre_stream->getQBufAddrForNum(frame_num, &addr_vir, &addr_phy);
			} else {
				ret = pre_stream->getQBuffFirstVir(&addr_vir);
				if(ret == NO_ERROR)
					ret = pre_stream->getQBuffFirstNum(&frame_num);
			}
			HAL_LOGD("pic_addr_vir = 0x%lx, frame_num = %d, ret = %d", addr_vir, frame_num, ret);
			if(ret == NO_ERROR) {
				if(addr_vir != NULL && virtual_addr != NULL && isCapturing())
					memcpy((char *)addr_vir, (char *)virtual_addr, (width * height * 3) / 2);
				regular_channel->channelCbRoutine(frame_num, timestamp, CAMERA_STREAM_TYPE_PREVIEW);
			}
		}
	}

	HAL_LOGD("X");

	return true;
}

bool SprdCamera3OEMIf::iSCallbackCaptureFrame()
{
	SprdCamera3PicChannel *pic_channel = reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);
	SprdCamera3Stream *stream = NULL;
	int32_t ret = BAD_VALUE;
	cmr_uint addr_vir = NULL;

	if(pic_channel) {
		pic_channel->getStream(CAMERA_STREAM_TYPE_PICTURE_CALLBACK, &stream);
		if(stream) {
			ret = stream->getQBuffFirstVir(&addr_vir);
		}
	}

	return ret == NO_ERROR;
}

bool SprdCamera3OEMIf::receiveCallbackPicture(uint32_t width, uint32_t height, uintptr_t phy_addr, char *virtual_addr)
{
	HAL_LOGD("E: size = %dx%d, phy_addr = 0x%lx, virtual_addr = 0x%lx", width, height, phy_addr, virtual_addr);

	Mutex::Autolock cbLock(&mPreviewCbLock);
	int64_t timestamp = systemTime();
	SprdCamera3PicChannel *pic_channel = reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);
	SprdCamera3Stream *stream = NULL;
	int32_t ret = 0;
	cmr_uint addr_vir = NULL;
	uint32_t frame_num = 0;

	if(pic_channel) {
		pic_channel->getStream(CAMERA_STREAM_TYPE_PICTURE_CALLBACK, &stream);
		if(stream) {
			ret = stream->getQBuffFirstVir(&addr_vir);
			stream->getQBuffFirstNum(&frame_num);
			HAL_LOGD("pic_callback_addr_vir = 0x%lx, frame_num = %d", addr_vir, frame_num);
			if(ret == NO_ERROR) {
				if(addr_vir != NULL && virtual_addr != NULL)
					memcpy((char *)addr_vir, (char *)virtual_addr, (width * height * 3) / 2);
				pic_channel->channelCbRoutine(frame_num, timestamp, CAMERA_STREAM_TYPE_PICTURE_CALLBACK);

				if(frame_num > mPictureFrameNum)
					mPictureFrameNum = frame_num;
			}
		}
	}

	HAL_LOGD("X");

	return true;
}

void SprdCamera3OEMIf::yuvNv12ConvertToYv12(struct camera_frame_type *frame, char* tmpbuf)
{
	int width, height;

	width = frame->width;
	height = frame->height;
	if (tmpbuf) {
		char * addr0 = (char *)frame->y_vir_addr + width * height;
		char * addr1 = addr0 + SIZE_ALIGN(width/2) * height/2;
		char * addr2 = (char *)tmpbuf;

		memcpy((void *)tmpbuf, (void *)addr0, width * height/2);
		if (width % 32) {
			int gap = SIZE_ALIGN(width/2) - width/2;
			for (int i = 0; i < width * height/4; i++) {
				*addr0++ = *addr2++;
				*addr1++ = *addr2++;
				if (!((i+1) % (width/2))) {
					addr0 = addr0 + gap;
					addr1 = addr1 + gap;
				}
			}
		} else {
			for (int i = 0; i < width * height/4; i++) {
				*addr0++ = *addr2++;
				*addr1++ = *addr2++;
			}
		}
	}

}

void SprdCamera3OEMIf::receiveRawPicture(struct camera_frame_type *frame)
{
	HAL_LOGD("E, mReDisplayHeap = %p", mReDisplayHeap);
	Mutex::Autolock cbLock(&mCaptureCbLock);
	bool display_flag, callback_flag;

	if (NULL == frame) {
		HAL_LOGE("invalid frame pointer");
		return;
	}

	if (SPRD_INTERNAL_CAPTURE_STOPPING == getCaptureState()) {
		HAL_LOGW("warning: capture state = SPRD_INTERNAL_CAPTURE_STOPPING, return");
		return;
	}

	display_flag = iSDisplayCaptureFrame();
	callback_flag = iSCallbackCaptureFrame();

	if (display_flag) {
		uintptr_t dst_paddr = 0;
		uint32_t dst_width = mPreviewWidth;
		uint32_t dst_height = mPreviewHeight;

		dst_paddr = getRedisplayMem(dst_width,dst_height);
		if (0 == dst_paddr) {
			HAL_LOGE("get review memory failed");
			return;
		}

		if ( 0 != mHalOem->ops->camera_get_redisplay_data(mCameraHandle, dst_paddr, dst_width, dst_height, frame->y_phy_addr,
				frame->uv_phy_addr, frame->width, frame->height)) {
			HAL_LOGE("Fail to camera_get_data_redisplay.");
			FreeReDisplayMem();
			return;
		}

		if(mReDisplayHeap)
			displayOneFrameForCapture(dst_width, dst_height, dst_paddr, (char *)mReDisplayHeap->data);
		FreeReDisplayMem();
	}

	if(callback_flag){
		uintptr_t dst_paddr = 0;
		uint32_t dst_width = mRawWidth;
		uint32_t dst_height = mRawHeight;

		dst_paddr = getRedisplayMem(dst_width,dst_height);
		if (0 == dst_paddr) {
			HAL_LOGE("get review memory failed");
			return;
		}

		if ( 0 != mHalOem->ops->camera_get_redisplay_data(mCameraHandle, dst_paddr, dst_width, dst_height, frame->y_phy_addr,
				frame->uv_phy_addr, frame->width, frame->height)) {
			HAL_LOGE("Fail to camera_get_data_redisplay.");
			FreeReDisplayMem();
			return;
		}

		if(mReDisplayHeap)
			receiveCallbackPicture(dst_width, dst_height, dst_paddr, (char *)mReDisplayHeap->data);
		FreeReDisplayMem();
	}

	HAL_LOGD("X, mReDisplayHeap = %p", mReDisplayHeap);
}

void SprdCamera3OEMIf::receiveJpegPicture(struct camera_frame_type *frame)
{
	//HAL_LOGD("E");
	print_time();
	Mutex::Autolock cbLock(&mCaptureCbLock);
	Mutex::Autolock cbPreviewLock(&mPreviewCbLock);
	struct camera_jpeg_param *encInfo = &frame->jpeg_param;
	int64_t temp = 0, temp1 = 0;;
	//camera_encode_mem_type *enc = (camera_encode_mem_type *)encInfo->outPtr;
	HAL_LOGD("E encInfo->size = %d, enc->buffer = 0x%lx, encInfo->need_free = %d time=%lld",encInfo->size, encInfo->outPtr, encInfo->need_free, frame->timestamp);

	SENSOR_Tag sensorInfo;
	mSetting->getSENSORTag(&sensorInfo);
	if (0 != frame->sensor_info.exposure_time_denominator) {
		sensorInfo.exposure_time = 1000000000ll*frame->sensor_info.exposure_time_numerator/frame->sensor_info.exposure_time_denominator;
	}
	sensorInfo.timestamp = frame->timestamp;
	mSetting->setSENSORTag(sensorInfo);
	camera3_jpeg_blob * jpegBlob = NULL;
	int64_t timestamp = sensorInfo.timestamp;
	cmr_uint pic_addr_vir = NULL;
	SprdCamera3Stream *pic_stream = NULL;
	int ret;
	uint32_t heap_size;
	SprdCamera3PicChannel* picChannel = reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);
	uint32_t frame_num = 0;
	char value[PROPERTY_VALUE_MAX];
	property_get("persist.sys.camera.raw.mode", value, "jpeg");

	if(picChannel && ((mCaptureMode != CAMERA_ISP_TUNING_MODE) || ((mCaptureMode == CAMERA_ISP_TUNING_MODE)&&(!strcmp(value, "raw")))
		|| ((mCaptureMode == CAMERA_ISP_SIMULATION_MODE)&&(!strcmp(value, "sim"))))) {
		picChannel->getStream(CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT, &pic_stream);
		if(pic_stream) {
			ret = pic_stream->getQBuffFirstVir(&pic_addr_vir);
			pic_stream->getQBuffFirstNum(&frame_num);
			HAL_LOGD("pic_addr_vir = 0x%lx, frame_num = %d", pic_addr_vir, frame_num);
			if(ret == NO_ERROR && pic_addr_vir != NULL) {
				void *isp_info_addr;
				int isp_info_size = 0;
				if(encInfo->outPtr != NULL) {
					memcpy((char *)pic_addr_vir, (char *)(encInfo->outPtr), encInfo->size);
					char debugable[PROPERTY_VALUE_MAX];
					property_get("ro.debuggable", debugable, "0");//isp info just need in user_debug version ,close in user version
					if (0 == strcmp(debugable, "1") && !mHalOem->ops->camera_get_isp_info(mCameraHandle, &isp_info_addr, &isp_info_size)) {
						mJpegSize = encInfo->size;
						JPEG_Tag jpgInfo;
						mSetting->getJPEGTag(&jpgInfo);
						uint32_t jpegMaxSize = jpgInfo.max_size;
						if(jpegMaxSize < mJpegSize + isp_info_size){
							isp_info_size =  jpegMaxSize - mJpegSize;
							HAL_LOGD("isp tuning para is too large reduce it, max:%d ,isp_info_size:%d",jpegMaxSize,isp_info_size);
						}
							memcpy(((char *)pic_addr_vir+mJpegSize),(char *)isp_info_addr,isp_info_size);
					}
				}
				pic_stream->getHeapSize(&heap_size);
				jpegBlob = (camera3_jpeg_blob*)((char *)pic_addr_vir + (heap_size - sizeof(camera3_jpeg_blob)));
				jpegBlob->jpeg_size = encInfo->size+isp_info_size;
				jpegBlob->jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;
				picChannel->channelCbRoutine(frame_num, timestamp, CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);

				if(frame_num > mPictureFrameNum)
					mPictureFrameNum = frame_num;
			}
		}

#ifdef CONFIG_SPRD_PRIVATE_ZSL
		if (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE /*|| mTakePictureMode == SNAPSHOT_ZSL_MODE*/ || mTakePictureMode == SNAPSHOT_DEFAULT_MODE) {
#else
		if (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE || mTakePictureMode == SNAPSHOT_ZSL_MODE || mTakePictureMode == SNAPSHOT_DEFAULT_MODE) {
#endif
			SprdCamera3RegularChannel* regularChannel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
			if(regularChannel) {
				HAL_LOGD("clear preview stream");
				regularChannel->channelClearInvalidQBuff(mPictureFrameNum, timestamp, CAMERA_STREAM_TYPE_PREVIEW);
				HAL_LOGD("clear video stream");
				regularChannel->channelClearInvalidQBuff(mPictureFrameNum, timestamp, CAMERA_STREAM_TYPE_VIDEO);
				HAL_LOGD("clear callback stream");
				regularChannel->channelClearInvalidQBuff(mPictureFrameNum, timestamp, CAMERA_STREAM_TYPE_CALLBACK);
			}
		}

		if (encInfo->need_free) {
			if (!iSZslMode()) {
				deinitCapture(mIsPreAllocCapMem);
				set_ddr_freq(BASE_FREQ_REQ);
			}
		}
	}

#ifdef HAS_CAMERA_HINTS
	if (1 == mHDRPowerHint) {
		if (m_pPowerModule) {
			if (m_pPowerModule->powerHint) {
				m_pPowerModule->powerHint(m_pPowerModule, POWER_HINT_VIDEO_ENCODE,
					(void *)"state=0");
			}
		}
		mHDRPowerHintFlag = 0;
	}
#endif

	HAL_LOGD("X");
}

void SprdCamera3OEMIf::receiveJpegPictureError(void)
{
	HAL_LOGD("E");
	print_time();
	Mutex::Autolock cbLock(&mCaptureCbLock);
	if (!checkPreviewStateForCapture()) {
		HAL_LOGE("drop current jpegPictureError msg");
		return;
	}

	int index = 0;

	HAL_LOGD("JPEG callback was cancelled--not delivering image.");

	// NOTE: the JPEG encoder uses the raw image contained in mRawHeap, so we need
	// to keep the heap around until the encoding is complete.

	print_time();
	HAL_LOGD("X");
}

void SprdCamera3OEMIf::receiveCameraExitError(void)
{
	Mutex::Autolock cbPreviewLock(&mPreviewCbLock);
	Mutex::Autolock cbCaptureLock(&mCaptureCbLock);

	if (!checkPreviewStateForCapture()) {
		HAL_LOGE("drop current cameraExit msg");
		return;
	}

	HAL_LOGE("HandleErrorState:don't enable error msg!");
}

void SprdCamera3OEMIf::receiveTakePictureError(void)
{
	Mutex::Autolock cbLock(&mCaptureCbLock);

	if (!checkPreviewStateForCapture()) {
		HAL_LOGE("drop current takePictureError msg");
		return;
	}

	HAL_LOGE("camera cb: invalid state %s for taking a picture!",
		 getCameraStateStr(getCaptureState()));

}

/*transite from 'from' state to 'to' state and signal the waitting thread. if the current*/
/*state is not 'from', transite to SPRD_ERROR state should be called from the callback*/
SprdCamera3OEMIf::Sprd_camera_state
SprdCamera3OEMIf::transitionState(SprdCamera3OEMIf::Sprd_camera_state from,
		SprdCamera3OEMIf::Sprd_camera_state to,
		SprdCamera3OEMIf::state_owner owner, bool lock)
{
	volatile SprdCamera3OEMIf::Sprd_camera_state *which_ptr = NULL;
//	HAL_LOGD("E");

	if (lock) mStateLock.lock();
	HAL_LOGD("owner = %d, lock = %d", owner, lock);

	switch (owner) {
	case STATE_CAMERA:
		which_ptr = &mCameraState.camera_state;
		break;

	case STATE_PREVIEW:
		which_ptr = &mCameraState.preview_state;
		break;

	case STATE_CAPTURE:
		which_ptr = &mCameraState.capture_state;
		break;

	case STATE_FOCUS:
		which_ptr = &mCameraState.focus_state;
		break;

	default:
		HAL_LOGD("changeState: error owner");
		break;
	}

	if (NULL != which_ptr) {
		if (from != *which_ptr) {
			to = SPRD_ERROR;
		}

		HAL_LOGD("changeState: %s --> %s", getCameraStateStr(from),
			getCameraStateStr(to));

		if (*which_ptr != to) {
			*which_ptr = to;
			//mStateWait.signal();
			mStateWait.broadcast();
		}
	}

	if (lock) mStateLock.unlock();
//	HAL_LOGD("X");

	return to;
}

void SprdCamera3OEMIf::HandleStartPreview(enum camera_cb_type cb,
		void* parm4)
{
	HAL_LOGV("in: cb = %d, parm4 = 0x%x, state = %s",
		cb, (cmr_uint)parm4, getCameraStateStr(getPreviewState()));

	switch(cb) {
	case CAMERA_EXIT_CB_PREPARE:
		if (isPreAllocCapMem() && mTakePictureMode != SNAPSHOT_ONLY_MODE) {
			pre_alloc_cap_mem_thread_init((void *)this);
		}

		if(mCaptureMode == CAMERA_NORMAL_MODE) {
			uintptr_t dst_paddr = 0;
			uint32_t dst_width = mPreviewWidth;
			uint32_t dst_height = mPreviewHeight;
			dst_paddr = getRedisplayMem(dst_width, dst_height);
		}
		break;

	case CAMERA_RSP_CB_SUCCESS:
		setCameraState(SPRD_PREVIEW_IN_PROGRESS, STATE_PREVIEW);
		break;

	case CAMERA_EVT_CB_FRAME:
		HAL_LOGV("CAMERA_EVT_CB_FRAME");
		switch (getPreviewState()) {
		case SPRD_PREVIEW_IN_PROGRESS:
			receivePreviewFrame((struct camera_frame_type *)parm4);
			break;

		case SPRD_INTERNAL_PREVIEW_STOPPING:
			HAL_LOGD("camera cb: discarding preview frame "
			"while stopping preview");
			break;

		default:
			HAL_LOGW("invalid state");
			break;
			}
		break;

	case CAMERA_EVT_CB_FD:
		HAL_LOGV("CAMERA_EVT_CB_FD");
		if (isPreviewing()) {
			receivePreviewFDFrame((struct camera_frame_type *)parm4);
		}
		break;

	case CAMERA_EXIT_CB_FAILED:
		HAL_LOGE("SprdCamera3OEMIf::camera_cb: @CAMERA_EXIT_CB_FAILURE(%d) in state %s.",
			parm4, getCameraStateStr(getPreviewState()));
		transitionState(getPreviewState(), SPRD_ERROR, STATE_PREVIEW);
		receiveCameraExitError();
		break;

	case CAMERA_EVT_CB_FLUSH:
		HAL_LOGV("CAMERA_EVT_CB_FLUSH");
		{
			struct camera_frame_type *frame = (struct camera_frame_type *)parm4;
			mPrevBufLock.lock();
			if (isPreviewing()) {
				flush_buffer(CAMERA_FLUSH_PREVIEW_HEAP, frame->buf_id,
					(void*)frame->y_vir_addr,
					(void*)frame->y_phy_addr,
					0);
			}
			mPrevBufLock.unlock();
		}
		break;

	case CAMERA_EVT_CB_RESUME:
//		HAL_LOGD("CAMERA_EVT_CB_RESUME");
#ifdef CONFIG_SPRD_PRIVATE_ZSL
		if (isPreviewing() && iSZslMode()) {
			PushAllZslBuffer();
			mZslChannelStatus = 1;
		}
#endif
		break;

	default:
		transitionState(getPreviewState(), SPRD_ERROR, STATE_PREVIEW);
		HAL_LOGE("unexpected cb %d for CAMERA_FUNC_START_PREVIEW.", cb);
		break;
	}

	HAL_LOGV("out, state = %s", getCameraStateStr(getPreviewState()));
}

void SprdCamera3OEMIf::HandleStopPreview(enum camera_cb_type cb,
										void*  parm4)
{
//	HAL_LOGD("debug cb = %d parm4 = 0x%x", cb, (cmr_uint)parm4);
	Mutex::Autolock cbPreviewLock(&mPreviewCbLock);
	Sprd_camera_state tmpPrevState = SPRD_IDLE;
	tmpPrevState = getPreviewState();
	HAL_LOGD("in: cb = %d, parm4 = 0x%x, state = %s",
			cb, (cmr_uint)parm4, getCameraStateStr(tmpPrevState));

	if ((SPRD_IDLE == tmpPrevState) || (SPRD_INTERNAL_PREVIEW_STOPPING == tmpPrevState)) {
		setCameraState(SPRD_IDLE, STATE_PREVIEW);
	} else {
		HAL_LOGE("HandleEncode: error preview status, %s", getCameraStateStr(tmpPrevState));
		transitionState(tmpPrevState,
			SPRD_ERROR,
			STATE_PREVIEW);
	}

	CONTROL_Tag controlInfo;
	mSetting->getCONTROLTag(&controlInfo);
	controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_INACTIVE;
	controlInfo.awb_state = ANDROID_CONTROL_AWB_STATE_INACTIVE;
	mSetting->setAeCONTROLTag(controlInfo);
	mSetting->setAwbCONTROLTag(controlInfo);
	//LOGI("HandleStopPreview out, state = %s", getCameraStateStr(getPreviewState()));
	HAL_LOGD("out, state = %s", getCameraStateStr(getPreviewState()));
}

void SprdCamera3OEMIf::HandleTakePicture(enum camera_cb_type cb,
											void* parm4)
{
	HAL_LOGD("E: cb = %d, parm4 = 0x%x, state = %s",
			cb, parm4, getCameraStateStr(getCaptureState()));
	bool encode_location = true;
	camera_position_type pt = {0, 0, 0, 0, NULL};

	switch (cb) {
	case CAMERA_EXIT_CB_PREPARE:
		prepareForPostProcess();
		break;
	case CAMERA_EVT_CB_FLUSH:
		HAL_LOGV("CAMERA_EVT_CB_FLUSH");
		mCapBufLock.lock();
		if (mCapBufIsAvail == 1) {
			flush_buffer(CAMERA_FLUSH_RAW_HEAP_ALL, 0,(void*)0,(void*)0,0);
		}
		mCapBufLock.unlock();
		break;
	case CAMERA_RSP_CB_SUCCESS:
	{
		HAL_LOGV("CAMERA_RSP_CB_SUCCESS");

		Sprd_camera_state tmpCapState= SPRD_INIT;
		tmpCapState = getCaptureState();
		if (SPRD_WAITING_RAW == tmpCapState) {
			HAL_LOGD("CAMERA_RSP_CB_SUCCESS has been called before, skip it");
		} else if(tmpCapState != SPRD_INTERNAL_CAPTURE_STOPPING){
			transitionState(SPRD_INTERNAL_RAW_REQUESTED,
				SPRD_WAITING_RAW,
				STATE_CAPTURE);
		}
		break;
	}
	case CAMERA_EVT_CB_CAPTURE_FRAME_DONE:
	{
		HAL_LOGV("CAMERA_EVT_CB_CAPTURE_FRAME_DONE");
		JPEG_Tag jpegInfo;
		mSetting->getJPEGTag(&jpegInfo);
		if(jpeg_gps_location)
		{
			camera_position_type pt = {0, 0, 0, 0, NULL};
			pt.altitude = jpegInfo.gps_coordinates[2];
			pt.latitude = jpegInfo.gps_coordinates[0];
			pt.longitude = jpegInfo.gps_coordinates[1];
			memcpy(mGps_processing_method, jpegInfo.gps_processing_method, sizeof(mGps_processing_method));
			pt.process_method = reinterpret_cast<const char *>(&mGps_processing_method[0]);
			pt.timestamp = jpegInfo.gps_timestamp;
			HAL_LOGV("gps pt.latitude = %f, pt.altitude = %f, pt.longitude = %f, pt.process_method = %s, jpegInfo.gps_timestamp = %lld", pt.latitude, pt.altitude, pt.longitude, pt.process_method, jpegInfo.gps_timestamp);

			/*if (camera_set_position(&pt, NULL, NULL) != CAMERA_SUCCESS) {
				HAL_LOGE("receiveRawPicture: camera_set_position: error");
			}*/
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_POSITION, (cmr_uint)&pt);

			jpeg_gps_location = false;
		}
		break;
	}
	case CAMERA_EVT_CB_SNAPSHOT_JPEG_DONE:
		exitFromPostProcess();
		break;
	case CAMERA_EVT_CB_SNAPSHOT_DONE:
		HAL_LOGV("CAMERA_EVT_CB_SNAPSHOT_DONE");
		if (checkPreviewStateForCapture() && (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE || mTakePictureMode == SNAPSHOT_DEFAULT_MODE)) {
			receiveRawPicture((struct camera_frame_type *)parm4);
		} else {
			HAL_LOGW("drop current rawPicture");
		}
		break;

	case CAMERA_EXIT_CB_DONE:
		HAL_LOGV("CAMERA_EXIT_CB_DONE");

		if (SPRD_WAITING_RAW == getCaptureState())
		{
			transitionState(SPRD_WAITING_RAW,
				SPRD_WAITING_JPEG,
				STATE_CAPTURE);
		}
		break;

	case CAMERA_EXIT_CB_FAILED:
		HAL_LOGE("SprdCamera3OEMIf::camera_cb: @CAMERA_EXIT_CB_FAILURE(%d) in state %s.",
			(cmr_uint)parm4, getCameraStateStr(getCaptureState()));
		transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
		receiveCameraExitError();
		//camera_set_capture_trace(0);
		break;

	case CAMERA_EVT_CB_ZSL_FRM:
	{
		HAL_LOGV("CAMERA_EVT_CB_HAL2_ZSL_NEW_FRM");
		break;
	}

	default:
		HAL_LOGE("unkown cb = %d", cb);
		transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
		receiveTakePictureError();
		//camera_set_capture_trace(0);
		break;
	}

	HAL_LOGD("X, state = %s", getCameraStateStr(getCaptureState()));
}

void SprdCamera3OEMIf::HandleCancelPicture(enum camera_cb_type cb,
												void* parm4)
{
	HAL_LOGD("E: cb = %d, parm4 = 0x%x, state = %s",
			cb, parm4, getCameraStateStr(getCaptureState()));

	if (SPRD_INTERNAL_CAPTURE_STOPPING != getCaptureState()) {
		HAL_LOGD("HandleCancelPicture don't handle");
		return;
	}
	setCameraState(SPRD_IDLE,
			STATE_CAPTURE);

	HAL_LOGD("X, state = %s", getCameraStateStr(getCaptureState()));
}

void SprdCamera3OEMIf::HandleEncode(enum camera_cb_type cb,
										void* parm4)
{
	HAL_LOGD("E: cb = %d, parm4 = 0x%x, state = %s",
			cb, parm4, getCameraStateStr(getCaptureState()));

	switch (cb) {
	case CAMERA_RSP_CB_SUCCESS:
		break;

	case CAMERA_EXIT_CB_DONE:
		HAL_LOGV("CAMERA_EXIT_CB_DONE");

		if (!WaitForCaptureJpegState()) {
			//HAL_LOGE("state error");
			break;
		}

		if ((SPRD_WAITING_JPEG == getCaptureState())) {
			Sprd_camera_state tmpCapState= SPRD_WAITING_JPEG;

			if (checkPreviewStateForCapture()) {
				receiveJpegPicture((struct camera_frame_type  *)parm4);
			} else {
				HAL_LOGE("HandleEncode: drop current jpgPicture");
			}

			tmpCapState = getCaptureState();
			if ((SPRD_WAITING_JPEG == tmpCapState)
				|| (SPRD_INTERNAL_CAPTURE_STOPPING == tmpCapState)) {
				if (((struct camera_frame_type *)parm4)->jpeg_param.need_free) {
					setCameraState(SPRD_IDLE,
						STATE_CAPTURE);
				} else if(tmpCapState != SPRD_INTERNAL_CAPTURE_STOPPING){
					setCameraState(SPRD_INTERNAL_RAW_REQUESTED,
						STATE_CAPTURE);
				}
			} else if (SPRD_IDLE != tmpCapState) {
				HAL_LOGE("HandleEncode: CAMERA_EXIT_CB_DONE error cap status, %s",
					getCameraStateStr(tmpCapState));
				transitionState(tmpCapState,
					SPRD_ERROR,
					STATE_CAPTURE);
			}
		}
		else if( mTakePictureMode != SNAPSHOT_NO_ZSL_MODE) {
		    receiveJpegPicture((struct camera_frame_type  *)parm4);
		    mZSLTakePicture = false;
		}
		break;

	case CAMERA_EXIT_CB_FAILED:
		HAL_LOGD("CAMERA_EXIT_CB_FAILED");
		transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
		receiveCameraExitError();
		break;

	default:
		HAL_LOGD("unkown error = %d", cb);
		transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
		receiveJpegPictureError();
		break;
	}

	HAL_LOGD("X, state = %s", getCameraStateStr(getCaptureState()));
}

void SprdCamera3OEMIf::HandleFocus(enum camera_cb_type cb,
										void* parm4)
{
	SprdCamera3MetadataChannel *channel = (SprdCamera3MetadataChannel*)mMetaData;
	int64_t timeStamp = 0;
	timeStamp = systemTime();

	HAL_LOGD("E: cb = %d, parm4 = 0x%x, state = %s",
				cb, parm4, getCameraStateStr(getPreviewState()));

	setCameraState(SPRD_IDLE, STATE_FOCUS);

	CONTROL_Tag controlInfo;
	mSetting->getCONTROLTag(&controlInfo);

	switch (cb) {
	case CAMERA_RSP_CB_SUCCESS:
		HAL_LOGV("camera cb: autofocus has started.");
		break;

	case CAMERA_EXIT_CB_DONE:
		HAL_LOGV("camera cb: autofocus succeeded.");
	{
		controlInfo.af_state = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
		mSetting->setAfCONTROLTag(controlInfo);

		//channel->channelCbRoutine(0, timeStamp, CAMERA_STREAM_TYPE_DEFAULT);
	}
		break;

	case CAMERA_EXIT_CB_ABORT:
	case CAMERA_EXIT_CB_FAILED:
	{
		controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
		mSetting->setAfCONTROLTag(controlInfo);
		//channel->channelCbRoutine(0, timeStamp, CAMERA_STREAM_TYPE_DEFAULT);
	}
		break;

	case CAMERA_EVT_CB_FOCUS_MOVE:
		HAL_LOGV("camera cb: autofocus focus moving  %d autofocus=%d", parm4, mIsAutoFocus);
		if (!mIsAutoFocus) {
			if (parm4) {
				controlInfo.af_state = ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN;
			} else {
				controlInfo.af_state = ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED;
			}
			mSetting->setAfCONTROLTag(controlInfo);
		}
		break;

	default:
		HAL_LOGE("camera cb: unknown cb %d for CAMERA_FUNC_START_FOCUS!", cb);
	{
		controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
		mSetting->setAfCONTROLTag(controlInfo);

		//channel->channelCbRoutine(0, timeStamp, CAMERA_STREAM_TYPE_DEFAULT);
	}
		break;
	}

	HAL_LOGD("out, state = %s", getCameraStateStr(getFocusState()));
}

void SprdCamera3OEMIf::HandleAutoExposure(enum camera_cb_type cb,
										void* parm4)
{
	CONTROL_Tag controlInfo;
	mSetting->getCONTROLTag(&controlInfo);
	HAL_LOGD("E: cb = %d, parm4 = 0x%x, state = %s",
				cb, parm4, getCameraStateStr(getPreviewState()));

	switch (cb) {
	case CAMERA_EVT_CB_AE_STAB_NOTIFY:
		if(controlInfo.ae_state != ANDROID_CONTROL_AE_STATE_LOCKED) {
			controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_CONVERGED;
			controlInfo.awb_state = ANDROID_CONTROL_AWB_STATE_CONVERGED;
			mSetting->setAeCONTROLTag(controlInfo);
			mSetting->setAwbCONTROLTag(controlInfo);
		}
		HAL_LOGI("CAMERA_EVT_CB_AE_STAB_NOTIFY, ae_state = %d",controlInfo.ae_state);
		break;
	case CAMERA_EVT_CB_AE_LOCK_NOTIFY:
		//controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_LOCKED;
		//mSetting->setAeCONTROLTag(controlInfo);
		HAL_LOGI("CAMERA_EVT_CB_AE_LOCK_NOTIFY, ae_state = %d",controlInfo.ae_state);
		break;
	case CAMERA_EVT_CB_AE_UNLOCK_NOTIFY:
		//controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_CONVERGED;
		//mSetting->setAeCONTROLTag(controlInfo);
		HAL_LOGI("CAMERA_EVT_CB_AE_UNLOCK_NOTIFY, ae_state = %d",controlInfo.ae_state);
		break;
	default:
		break;
	}

	HAL_LOGD("out");
}


void SprdCamera3OEMIf::HandleStartCamera(enum camera_cb_type cb,
												void* parm4)
{
	HAL_LOGD("in: cb = %d, parm4 = 0x%x, state = %s",
			cb, parm4, getCameraStateStr(getCameraState()));

	transitionState(SPRD_INIT, SPRD_IDLE, STATE_CAMERA);

	HAL_LOGD("out, state = %s", getCameraStateStr(getCameraState()));
}

void SprdCamera3OEMIf::HandleStopCamera(enum camera_cb_type cb, void* parm4)
{
	HAL_LOGD("in: cb = %d, parm4 = 0x%x, state = %s",
			cb, parm4, getCameraStateStr(getCameraState()));

	transitionState(SPRD_INTERNAL_STOPPING, SPRD_INIT, STATE_CAMERA);

	HAL_LOGD("out, state = %s", getCameraStateStr(getCameraState()));
}

void SprdCamera3OEMIf::camera_cb(enum camera_cb_type cb,
					const void *client_data,
					enum camera_func_type func,
					void* parm4)
{
	SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)client_data;
//	HAL_LOGD("E");
	HAL_LOGV("E: debug cb = %d func = %d parm4 = 0x%x", cb, func, (cmr_uint)parm4);
	switch(func) {
	case CAMERA_FUNC_START_PREVIEW:
		obj->HandleStartPreview(cb, parm4);
		break;

	case CAMERA_FUNC_STOP_PREVIEW:
		obj->HandleStopPreview(cb, parm4);
		break;

	case CAMERA_FUNC_RELEASE_PICTURE:
		obj->HandleCancelPicture(cb, parm4);
		break;

	case CAMERA_FUNC_TAKE_PICTURE:
		obj->HandleTakePicture(cb, parm4);
		break;

	case CAMERA_FUNC_ENCODE_PICTURE:
		obj->HandleEncode(cb, parm4);
		break;

	case CAMERA_FUNC_START_FOCUS:
		obj->HandleFocus(cb, parm4);
		break;

	case CAMERA_FUNC_AE_STATE_CALLBACK:
		obj->HandleAutoExposure(cb, parm4);
		break;

	case CAMERA_FUNC_START:
		obj->HandleStartCamera(cb, parm4);
		break;

	case CAMERA_FUNC_STOP:
		obj->HandleStopCamera(cb, parm4);
		break;

	default:
		HAL_LOGE("Unknown camera-callback status %d", cb);
		break;
	}
	HAL_LOGV("X");
}

int SprdCamera3OEMIf::flush_buffer(camera_flush_mem_type_e  type, int index, void *v_addr, void *p_addr, int size)
{
	int ret = 0;
	sprd_camera_memory_t *pmem = NULL;
	MemoryHeapIon *pHeapIon = NULL;
	uint32_t i;

	switch(type)
	{
	case CAMERA_FLUSH_RAW_HEAP:
		pmem = mRawHeap;
		break;

	case CAMERA_FLUSH_RAW_HEAP_ALL:
		for ( i=0 ; i<mSubRawHeapNum ; i++) {
			pmem = mSubRawHeapArray[i];
			v_addr = (void*)mSubRawHeapArray[i]->data;
			p_addr = (void*)mSubRawHeapArray[i]->phys_addr;
			size = (int)mSubRawHeapArray[i]->phys_size;

			if (pmem) {
				pHeapIon = pmem->ion_heap;
			}
			if (pHeapIon) {
				HAL_LOGD("index=%d,vaddr=%p, paddr=%p,size=0x%x", i, v_addr, p_addr,size);
				ret = pHeapIon->flush_ion_buffer(v_addr, p_addr, size);
				if (ret) {
					HAL_LOGW("abnormal ret=%d", ret);
					HAL_LOGW("index=%d,vaddr=%p, paddr=%p", i, v_addr, p_addr);
				}
			}

		}
		break;

	case CAMERA_FLUSH_PREVIEW_HEAP:
		break;

	default:
		break;
	}

	if (pmem) {
		pHeapIon = pmem->ion_heap;
	}

	if (pHeapIon) {
		HAL_LOGV("index=%d,vaddr=0x%p, paddr=0x%p,size=0x%zx", index, v_addr, p_addr,size);
		ret = pHeapIon->flush_ion_buffer(v_addr, p_addr, size);
		if (ret) {
			HAL_LOGW("abnormal ret=%d", ret);
			HAL_LOGW("index=%d,vaddr=0x%p, paddr=0x%p", index, v_addr, p_addr);
		}
	}

	return ret;
}

int SprdCamera3OEMIf::openCamera()
{
	int ret = NO_ERROR;

	GET_START_TIME;
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	ZSLMode_monitor_thread_init((void *)this);
#endif
	if (!startCameraIfNecessary()) {
		ret = UNKNOWN_ERROR;
		HAL_LOGE("start failed");
	}

	return ret;
}

int SprdCamera3OEMIf::handleCbData(hal3_trans_info_t &result_info, void *userdata)
{
	cam_stream_type_t type = CAM_STREAM_TYPE_DEFAULT;
	SprdCamera3Channel *channel = (SprdCamera3Channel *)userdata;

	HAL_LOGD("S data=0x%x", channel);
	for (List < hal3_trans_info_t >::iterator i = mCbInfoList.begin();
	     i != mCbInfoList.end(); i++) {
		if (channel == (SprdCamera3Channel *)i->user_data) {
			type = i->stream_type;
			HAL_LOGD("type=%d", type);
			break;
		}
	}
	//channel->channelCbRoutine(result_info.frame.index, result_info.timestamp, NULL, type);
	HAL_LOGD("X");
	return 0;
}

int SprdCamera3OEMIf::setCameraConvertCropRegion(void)
{
	float minOutputRatio;
	float zoomWidth,zoomHeight,zoomRatio;
	int ret = 0, integer;
	uint32_t sensorOrgW = 0, sensorOrgH = 0;
	struct cmr_zoom_param zoomInfo;
	SCALER_Tag scaleInfo;
	CONTROL_Tag ctlInfo;
	uint8_t capIntent = 0;
	cam_dimension_t imgSize = {0};
	struct img_rect cropRegion;
	float table[30] = {1.0, 1.03, 1.05, 1.08, 1.1, 1.13, 1.16, 1.19, 1.22, 1.25, 1.28, 1.31,
			1.34, 1.37, 1.40, 1.42, 1.45, 1.47, 1.50, 1.52, 1.55, 1.58, 1.61, 1.64, 1.67, 1.7,
			1.73, 1.76, 1.79, 1.79};//, 1.84, 1.87, 1.89, 1.92, 1.94, 1.97
#ifdef CONFIG_DISABLE_1080P_RECORDING_ZOOM
	float table_1080p[30] = {
		1.0, 1.01, 1.02, 1.03, 1.04, 1.05, 1.06, 1.07, 1.08, 1.09,
		1.10, 1.11, 1.12, 1.13, 1.14, 1.15, 1.16, 1.17, 1.18, 1.19,
		1.20, 1.21, 1.22, 1.23, 1.24, 1.25, 1.26, 1.27, 1.28, 1.29 
	};
#endif
	//during taking picture ,can`t zoom
	if(GetCameraStatus(CAMERA_STATUS_SNAPSHOT) != CAMERA_SNAPSHOT_IDLE){
		return 0;
	}

	mSetting->getSCALERTag(&scaleInfo);
	mSetting->getCONTROLTag(&ctlInfo);
	capIntent = ctlInfo.capture_intent;
	cropRegion.start_x = scaleInfo.crop_region[0];
	cropRegion.start_y = scaleInfo.crop_region[1];
	cropRegion.width = scaleInfo.crop_region[2];
	cropRegion.height = scaleInfo.crop_region[3];
	HAL_LOGD("crop %d %d %d %d intent=%d", cropRegion.start_x, cropRegion.start_y,
                cropRegion.width ,cropRegion.height, capIntent);
	if(mCameraId == 0) {
		sensorOrgW = BACK_SENSOR_ORIG_WIDTH;
		sensorOrgH = BACK_SENSOR_ORIG_HEIGHT;
	} else if(mCameraId == 1) {
		sensorOrgW = FRONT_SENSOR_ORIG_WIDTH;
		sensorOrgH = FRONT_SENSOR_ORIG_HEIGHT;
	}
	if (cropRegion.width == 0 || cropRegion.height == 0){
		HAL_LOGW("cropRegion.width or cropRegion.height is 0");
		//return 1;
		/*if (capIntent == ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW || capIntent ==
		ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD
			|| capIntent == ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG || capIntent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
			//mSetting->getPreviewSize(&imgSize);
			cropRegion.width = mPreviewWidth;
			cropRegion.height = mPreviewHeight;
		} else {
			//mSetting->getPictureSize(&imgSize);
			cropRegion.width = mCaptureWidth;
			cropRegion.height = mCaptureHeight;
		}*/
		 struct cmr_zoom_param zoom_param;
		 zoom_param.mode = ZOOM_LEVEL;
		 zoom_param.zoom_level = 0;
		 SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ZOOM, (cmr_uint)&zoom_param);
		 return ret;
	}
	zoomWidth = static_cast<float>(cropRegion.width);
	zoomHeight = static_cast<float>(cropRegion.height);

	//get dstRatio and zoomRatio frm framework
	minOutputRatio = zoomWidth / zoomHeight;
	if (minOutputRatio > (static_cast<float>(sensorOrgW) / static_cast<float>(sensorOrgH))) {
		zoomRatio = static_cast<float>(sensorOrgW) / zoomWidth;
	} else {
		zoomRatio = static_cast<float>(sensorOrgH) / zoomHeight;
	}
	integer = static_cast<int>(zoomRatio * 10);
	if (integer > 40 || integer < 10) {/*max zoom is 4, integer is 39;if max zoom is a, integer is 10a - 1*/
		HAL_LOGE("ratio error integer=%d", integer);
		return 1;
	}
	if (integer == 40)/*for cts testImmediateZoom*/
		integer = 39;

	zoomInfo.mode = ZOOM_INFO;
	zoomInfo.zoom_info.zoom_ratio = table[integer - 10];
#ifdef CONFIG_DISABLE_1080P_RECORDING_ZOOM
	if(mVideoWidth*mVideoHeight >= 1920*1080 )
		 zoomInfo.zoom_info.zoom_ratio = table_1080p[integer - 10];
#endif
	if (capIntent == ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW || capIntent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD
                                || capIntent == ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG || capIntent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
        //mSetting->getPreviewSize(&imgSize);
        zoomWidth = mPreviewWidth;
        zoomHeight = mPreviewHeight;
    } else {
        //mSetting->getPictureSize(&imgSize);
        zoomWidth = mCaptureWidth;
        zoomHeight = mCaptureHeight;
    }
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	if(mSprdZslEnabled == 1 && mCaptureWidth>0 && mCaptureHeight>0) {
		zoomWidth = mCaptureWidth;
        	zoomHeight = mCaptureHeight;
	}
#endif
	zoomInfo.zoom_info.output_ratio = zoomWidth/zoomHeight;
	mZoomInfo = zoomInfo;
	SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ZOOM,(cmr_uint)&mZoomInfo);
	HAL_LOGD("c%f trueRat=%f dst=%f zoomWidth=%f zoomHeight=%f output_ratio=%f)", zoomRatio, zoomInfo.zoom_info.zoom_ratio,
		minOutputRatio, zoomWidth, zoomHeight, zoomInfo.zoom_info.output_ratio);

	return ret;

}

int SprdCamera3OEMIf::CameraConvertCropRegion(uint32_t sensorWidth, uint32_t sensorHeight, struct img_rect *cropRegion)
{
	float    minOutputRatio;
	float    zoomWidth,zoomHeight,zoomRatio;
	uint32_t i = 0;
	int ret = 0;
	uint32_t      sensorOrgW = 0, sensorOrgH = 0, endX = 0, endY = 0, startXbak = 0, startYbak = 0;
	cmr_uint SensorRotate = IMG_ANGLE_0;
#define ALIGN_ZOOM_CROP_BITS (~0x03)

	HAL_LOGD("crop %d %d %d %d sens w/h %d %d.",
		cropRegion->start_x, cropRegion->start_y, cropRegion->width, cropRegion->height,
		sensorWidth, sensorHeight);
	if(mCameraId == 0) {
            sensorOrgW = BACK_SENSOR_ORIG_WIDTH;
            sensorOrgH = BACK_SENSOR_ORIG_HEIGHT;
	} else if(mCameraId == 1) {
	    sensorOrgW = FRONT_SENSOR_ORIG_WIDTH;
	    sensorOrgH = FRONT_SENSOR_ORIG_HEIGHT;
	}
	if (sensorWidth == 0 || sensorHeight == 0 || cropRegion->width == 0	|| cropRegion->height == 0){
		HAL_LOGW("cropRegion->height or cropRegion->width is 0");
		return 1;
	}
	SensorRotate = mHalOem->ops->camera_get_preview_rot_angle(mCameraHandle);
	if (sensorWidth == sensorOrgW && sensorHeight == sensorOrgH && SensorRotate == IMG_ANGLE_0) {
	    HAL_LOGE("dont' need to convert.");
	    return 0;
	}
	/*
	zoomWidth = (float)cropRegion->width;
	zoomHeight = (float)cropRegion->height;
	//get dstRatio and zoomRatio frm framework
	minOutputRatio = zoomWidth / zoomHeight;
	if (minOutputRatio > ((float)sensorOrgW / (float)sensorOrgH)) {
	    zoomRatio = (float)sensorOrgW / zoomWidth;
	} else {
	    zoomRatio = (float)sensorOrgH / zoomHeight;
	}
	if(IsRotate) {
	    minOutputRatio = 1 / minOutputRatio;
	}
	if (minOutputRatio > ((float)sensorWidth / (float)sensorHeight)) {
	    zoomWidth = (float)sensorWidth / zoomRatio;
	    zoomHeight = zoomWidth / minOutputRatio;
	} else {
	    zoomHeight = (float)sensorHeight / zoomRatio;
	    zoomWidth = zoomHeight * minOutputRatio;
	}
	cropRegion->start_x = ((uint32_t)(sensorWidth - zoomWidth) >> 1) & ALIGN_ZOOM_CROP_BITS;
	cropRegion->start_y = ((uint32_t)(sensorHeight - zoomHeight) >> 1) & ALIGN_ZOOM_CROP_BITS;
	cropRegion->width = ((uint32_t)zoomWidth) & ALIGN_ZOOM_CROP_BITS;
	cropRegion->height = ((uint32_t)zoomHeight) & ALIGN_ZOOM_CROP_BITS;*/

	zoomWidth = (float)sensorWidth / (float)sensorOrgW;
	zoomHeight = (float)sensorHeight / (float)sensorOrgH;
	endX = cropRegion->start_x + cropRegion->width;
	endY = cropRegion->start_y + cropRegion->height;
	cropRegion->start_x = (cmr_u32)((float)cropRegion->start_x * zoomWidth);
	cropRegion->start_y = (cmr_u32)((float)cropRegion->start_y * zoomHeight);
	endX = (cmr_u32)((float)endX * zoomWidth);
	endY = (cmr_u32)((float)endY * zoomHeight);
	cropRegion->width = endX - cropRegion->start_x;
	cropRegion->height = endY - cropRegion->start_y;

	switch (SensorRotate) {
	case IMG_ANGLE_90:
	    startYbak = cropRegion->start_y;
	    cropRegion->start_y = cropRegion->start_x;
	    cropRegion->start_x = sensorHeight - endY;
	    cropRegion->width = (sensorHeight - startYbak) - cropRegion->start_x;
	    cropRegion->height = endX - cropRegion->start_y;
	    break;
	case IMG_ANGLE_180:
	    startYbak = cropRegion->start_y;
	    startXbak = cropRegion->start_x;
	    cropRegion->start_x= sensorHeight - endX;
	    cropRegion->start_y = sensorWidth- endY;
	    cropRegion->width = (sensorHeight - startXbak) - cropRegion->start_x;
	    cropRegion->height = (sensorWidth - startYbak) - cropRegion->start_y;
	    break;
	case IMG_ANGLE_270:
	    startYbak = cropRegion->start_y;
	    startXbak = cropRegion->start_x;
	    cropRegion->start_x= cropRegion->start_y;
	    cropRegion->start_y = sensorHeight - endX;
	    cropRegion->width = endY - cropRegion->start_x;
	    cropRegion->height = (sensorHeight - startXbak) - cropRegion->start_y;
	    break;
    }
	HAL_LOGD("Crop calculated (x=%d,y=%d,w=%d,h=%d rot=%d)",
	            cropRegion->start_x, cropRegion->start_y, cropRegion->width, cropRegion->height, SensorRotate);
	return ret;
}

int SprdCamera3OEMIf::SetCameraParaTag(cmr_int cameraParaTag)
{
	int ret = 0;
	CONTROL_Tag controlInfo;
	char value[PROPERTY_VALUE_MAX];
	property_get("persist.sys.camera.raw.mode", value, "jpeg");

	HAL_LOGV("set camera para, tag is %ld", cameraParaTag);
	mSetting->getCONTROLTag(&controlInfo);
	switch (cameraParaTag) {
	case ANDROID_CONTROL_SCENE_MODE:
		{
			int8_t drvSceneMode = 0;
			mSetting->androidSceneModeToDrvMode(controlInfo.scene_mode, &drvSceneMode);
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SCENE_MODE, drvSceneMode);
			if (CAMERA_SCENE_MODE_HDR == drvSceneMode) {
				mHDRPowerHint = 1;
			} else {
				mHDRPowerHint = 0;
			}
		}
		break;

	case ANDROID_CONTROL_EFFECT_MODE:
		{
			int8_t effectMode = 0;
			mSetting->androidEffectModeToDrvMode(controlInfo.effect_mode, &effectMode);
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EFFECT, effectMode);
		}
		break;

	case ANDROID_CONTROL_CAPTURE_INTENT:
		mCapIntent = controlInfo.capture_intent;
		break;

	case ANDROID_CONTROL_AWB_MODE:
		{
			int8_t drvAwbMode = 0;
			mSetting->androidAwbModeToDrvAwbMode(controlInfo.awb_mode, &drvAwbMode);
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_WB, drvAwbMode);
		}
		break;

	case ANDROID_SCALER_CROP_REGION:
		//SET_PARM(mHalOem, CAMERA_PARM_ZOOM_RECT, cameraParaValue);
		break;

	case ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION:
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EXPOSURE_COMPENSATION, controlInfo.ae_exposure_compensation);
		break;
	case ANDROID_CONTROL_AF_REGIONS:
		{
			struct img_rect zoom1 = {0,0,0,0};
			struct img_rect zoom = {0,0,0,0};
			struct cmr_focus_param focus_para;
			if (mCameraState.preview_state == SPRD_PREVIEW_IN_PROGRESS) {
				zoom.start_x = controlInfo.af_regions[0];
				zoom.start_y = controlInfo.af_regions[1];
				zoom.width = controlInfo.af_regions[2];
				zoom.height = controlInfo.af_regions[3];
				mHalOem->ops->camera_get_sensor_trim(mCameraHandle, &zoom1);
				if ((0 == zoom.start_x && 0 == zoom.start_y && 0 == zoom.width && 0 == zoom.height) ||
					!CameraConvertCropRegion(zoom1.width, zoom1.height, &zoom)) {
					focus_para.zone[0].start_x = zoom.start_x;
					focus_para.zone[0].start_y = zoom.start_y;
					focus_para.zone[0].width = zoom.width;
					focus_para.zone[0].height = zoom.height;
					focus_para.zone_cnt = 1;
					SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCUS_RECT, (cmr_uint)&focus_para);
				}
			}
		}
		break;
	case ANDROID_SPRD_SENSOR_ORIENTATION:
		{
			SPRD_DEF_Tag sprddefInfo;

			mSetting->getSPRDDEFTag(&sprddefInfo);
			HAL_LOGD("orient=%d", sprddefInfo.sensor_orientation);
			if (1 == sprddefInfo.sensor_orientation) {
				SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ORIENTATION, 1);
			} else {
				SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ORIENTATION, 0);
			}
		}
		break;
	case ANDROID_SPRD_SENSOR_ROTATION:
		{
			SPRD_DEF_Tag sprddefInfo;

			mSetting->getSPRDDEFTag(&sprddefInfo);
			HAL_LOGD("rot=%d", sprddefInfo.sensor_rotation);
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ROTATION, sprddefInfo.sensor_rotation);
		}
		break;
	case ANDROID_SPRD_UCAM_SKIN_LEVEL:
		{
			SPRD_DEF_Tag sprddefInfo;

			mSetting->getSPRDDEFTag(&sprddefInfo);
			HAL_LOGD("perfect_skin_level = %d", sprddefInfo.perfect_skin_level);
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PERFECT_SKIN_LEVEL, sprddefInfo.perfect_skin_level);
		}
		break;
        case ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR:
		{
			SPRD_DEF_Tag sprddefInfo;
			mSetting->getSPRDDEFTag(&sprddefInfo);
			HAL_LOGD("flip_on_level = %d", sprddefInfo.flip_on);
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLIP_ON, sprddefInfo.flip_on);
		}
		break;
	case ANDROID_CONTROL_AF_MODE:
		{
			int8_t AfMode = 0;
			mSetting->androidAfModeToDrvAfMode(controlInfo.af_mode, &AfMode);
			if (mCameraId == 0 && !mIsAutoFocus) {
				if (mRecordingMode && CAMERA_FOCUS_MODE_CAF == AfMode) {/*dv mode but recording not start*/
					AfMode = CAMERA_FOCUS_MODE_CAF_VIDEO;
				}
				SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE, AfMode);
			}
		}
		break;
	case ANDROID_CONTROL_AE_ANTIBANDING_MODE:
		{
			int8_t antibanding_mode = 0;
			mSetting->androidAntibandingModeToDrvAntibandingMode(controlInfo.antibanding_mode, &antibanding_mode);
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ANTIBANDING, antibanding_mode);
		}
		break;

	case ANDROID_SPRD_BRIGHTNESS:
		{
			SPRD_DEF_Tag sprddefInfo;
			mSetting->getSPRDDEFTag(&sprddefInfo);
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_BRIGHTNESS, (uint32_t)sprddefInfo.brightness);
		}
		break;

	case ANDROID_SPRD_CONTRAST:
		{
			SPRD_DEF_Tag sprddefInfo;
			mSetting->getSPRDDEFTag(&sprddefInfo);
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CONTRAST, (uint32_t)sprddefInfo.contrast);
		}
		break;

	case ANDROID_SPRD_SATURATION:
		{
			SPRD_DEF_Tag sprddefInfo;
			mSetting->getSPRDDEFTag(&sprddefInfo);
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SATURATION, (uint32_t)sprddefInfo.saturation);
		}
		break;

	case ANDROID_JPEG_ORIENTATION:
	{
		JPEG_Tag jpegInfo;
		mSetting->getJPEGTag(&jpegInfo);
		mJpegRotaSet = true;
		#ifdef CONFIG_CAMERA_ROTATION_CAPTURE
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 1);
		if (!strcmp(value, "raw")) {
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
		}
		#else
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
		#endif
		SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ENCODE_ROTATION, jpegInfo.orientation);
		HAL_LOGD("JPEG orientation = %d", jpegInfo.orientation);
	}
		break;

	case ANDROID_CONTROL_AE_MODE:
		if (mCameraId == 0 || mCameraId == 1) {
			int8_t drvAeMode;
			mSetting->androidAeModeToDrvAeMode(controlInfo.ae_mode, &drvAeMode);

			if (drvAeMode != CAMERA_FLASH_MODE_TORCH && mFlashMode != CAMERA_FLASH_MODE_TORCH) {
				if (mFlashMode != drvAeMode) {
					mFlashMode = drvAeMode;
					SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLASH, mFlashMode);
				}
			}
		}
		break;

	case ANDROID_CONTROL_AE_LOCK:
		{
			uint8_t ae_lock;
			ae_lock = controlInfo.ae_lock;
			HAL_LOGD("ANDROID_CONTROL_AE_LOCK, ae_lock = %d",ae_lock);
			//SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_AE_LOCK_UNLOCK, ae_lock);
			if(ae_lock == 1) {
				controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_LOCKED;
				mSetting->setAeCONTROLTag(controlInfo);
			} else {
				controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_CONVERGED;
				mSetting->setAeCONTROLTag(controlInfo);
			}
		}
		break;

	case ANDROID_FLASH_MODE:
		if (mCameraId == 0 || mCameraId == 1) {
			int8_t flashMode;
			FLASH_Tag flashInfo;
			mSetting->getFLASHTag(&flashInfo);
			mSetting->androidFlashModeToDrvFlashMode(flashInfo.mode, &flashMode);
			if (CAMERA_FLASH_MODE_TORCH == flashMode || CAMERA_FLASH_MODE_TORCH == mFlashMode) {
				mFlashMode = flashMode;
				SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLASH, mFlashMode);
			}
		}
		break;

	case ANDROID_SPRD_CAPTURE_MODE:
		{
			SPRD_DEF_Tag sprddefInfo;
			mSetting->getSPRDDEFTag(&sprddefInfo);
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM, sprddefInfo.capture_mode);
			if (sprddefInfo.capture_mode > 1)
				BurstCapCnt = sprddefInfo.capture_mode;
		}
		break;

	case ANDROID_LENS_FOCAL_LENGTH:
		LENS_Tag lensInfo;
		mSetting->getLENSTag(&lensInfo);
		if(lensInfo.focal_length)
		{
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH, (cmr_uint)(lensInfo.focal_length * 1000));
		}
		break;

	case ANDROID_JPEG_QUALITY:
		JPEG_Tag jpgInfo;
		mSetting->getJPEGTag(&jpgInfo);
		if(jpgInfo.quality)
		{
			SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY, jpgInfo.quality);
		}
		break;

	case ANDROID_JPEG_THUMBNAIL_QUALITY:
		break;

	case ANDROID_JPEG_THUMBNAIL_SIZE:
		break;

    case ANDROID_CONTROL_AE_REGIONS:
        {
            SPRD_DEF_Tag sprddefInfo;
            struct cmr_ae_param ae_param;
            struct img_rect zoom1 = {0,0,0,0};
            struct img_rect zoom = {0,0,0,0};
            mSetting->getSPRDDEFTag(&sprddefInfo);
            zoom.start_x = sprddefInfo.am_regions[0];
            zoom.start_y = sprddefInfo.am_regions[1];
            zoom.width = sprddefInfo.am_regions[2];
            zoom.height = sprddefInfo.am_regions[3];
            mHalOem->ops->camera_get_sensor_trim(mCameraHandle, &zoom1);
            if (!CameraConvertCropRegion(zoom1.width, zoom1.height, &zoom)) {
                sprddefInfo.am_regions[0] = zoom.start_x;
                sprddefInfo.am_regions[1] = zoom.start_y;
                sprddefInfo.am_regions[2] = zoom.width;
                sprddefInfo.am_regions[3] = zoom.height;
                mSetting->setSPRDDEFTag(sprddefInfo);
            }
            mSetting->androidAmModeToDrvAwbMode(sprddefInfo.am_mode, &ae_param);
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AE_METERING_AREA, (cmr_uint)&ae_param);
        }
        break;

    case ANDROID_SPRD_METERING_MODE:
        {
            SPRD_DEF_Tag sprddefInfo;
            struct cmr_ae_param ae_param;
            mSetting->getSPRDDEFTag(&sprddefInfo);
            memset(&ae_param, 0, sizeof(ae_param));
            ae_param.mode = sprddefInfo.am_mode;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AUTO_EXPOSURE_MODE, (cmr_uint)&ae_param);
        }
        break;

	case ANDROID_SPRD_ISO:
		{
			int8_t iso = 0;
			SPRD_DEF_Tag sprddefInfo;
			mSetting->getSPRDDEFTag(&sprddefInfo);
			if(mRecordingMode == false)
				SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISO, (cmr_uint)sprddefInfo.iso);
		}
		break;

	case ANDROID_CONTROL_AE_TARGET_FPS_RANGE:
		setCameraPreviewMode(mRecordingMode);
		break;
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	case ANDROID_SPRD_ZSL_ENABLED:
		{
			SPRD_DEF_Tag sprddefInfo;
			mSetting->getSPRDDEFTag(&sprddefInfo);
			if(sprddefInfo.sprd_zsl_enabled == 0 && mRecordingMode == false) {
				mSprdZslEnabled = false;
				SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_ZSL_ENABLED, 0);
			}
		}
		break;
#endif

	default:
		ret = BAD_VALUE;
		break;
	}
	return ret;
}

int SprdCamera3OEMIf::SetJpegGpsInfo(bool is_set_gps_location)
{
	if (is_set_gps_location)
		jpeg_gps_location = true;

	return 0;
}

snapshot_mode_type_t SprdCamera3OEMIf::GetTakePictureMode()
{
	return mTakePictureMode;
}

int SprdCamera3OEMIf::setCapturePara(camera_capture_mode_t cap_mode, uint32_t frame_number)
{
	char value[PROPERTY_VALUE_MAX];
	property_get("persist.sys.camera.raw.mode", value, "jpeg");
	HAL_LOGD("cap_mode = %d",cap_mode);
	switch(cap_mode)
	{
		case CAMERA_CAPTURE_MODE_PREVIEW:
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mCaptureMode = CAMERA_NORMAL_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
			mTakePictureMode = SNAPSHOT_DEFAULT_MODE;
			mRecordingMode = false;
			mZslPreviewMode = false;
			break;
		case CAMERA_CAPTURE_MODE_VIDEO:
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mCaptureMode = CAMERA_ZSL_MODE;
			mTakePictureMode = SNAPSHOT_DEFAULT_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
			mRecordingMode = true;
			mZslPreviewMode = false;
			mVideoShotFlag = 0;
			break;
		case CAMERA_CAPTURE_MODE_ZSL_PREVIEW:
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mCaptureMode = CAMERA_ZSL_MODE;
			mTakePictureMode = SNAPSHOT_DEFAULT_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
			mRecordingMode =false;
			mZslPreviewMode = true;
			//setAePrecaptureSta(ANDROID_CONTROL_AE_STATE_CONVERGED);
			break;
		case CAMERA_CAPTURE_MODE_ONLY_SNAPSHOT:
			mTakePictureMode = SNAPSHOT_ONLY_MODE;
			mCaptureMode = CAMERA_NORMAL_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mRecordingMode =false;
			mPicCaptureCnt = 1;
			mZslPreviewMode = false;
			break;
		case CAMERA_CAPTURE_MODE_NON_ZSL_SNAPSHOT:
			mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
			mCaptureMode = CAMERA_NORMAL_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mRecordingMode =false;
			mPicCaptureCnt = 1;
			mZslPreviewMode = false;
			if (!strcmp(value, "raw")) {
				HAL_LOGE("enter isp tuning mode ");
				mCaptureMode = CAMERA_ISP_TUNING_MODE;
			} else if (!strcmp(value, "sim")) {
				HAL_LOGE("enter isp simulation mode ");
				mCaptureMode = CAMERA_ISP_SIMULATION_MODE;
			}
			break;
		case CAMERA_CAPTURE_MODE_CONTINUE_NON_ZSL_SNAPSHOT:
			mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
			mCaptureMode = CAMERA_NORMAL_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mRecordingMode =false;
			mPicCaptureCnt = 100;
			mZslPreviewMode = false;
			if (!strcmp(value, "raw")) {
				HAL_LOGE("enter isp tuning mode ");
				mCaptureMode = CAMERA_ISP_TUNING_MODE;
			} else if (!strcmp(value, "sim")) {
				HAL_LOGE("enter isp simulation mode ");
				mCaptureMode = CAMERA_ISP_SIMULATION_MODE;
			}
			break;
		case CAMERA_CAPTURE_MODE_ZSL_SNAPSHOT:
			mTakePictureMode = SNAPSHOT_ZSL_MODE;
			mCaptureMode = CAMERA_ZSL_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mRecordingMode = false;
			mPicCaptureCnt = 1;
			mZslPreviewMode = false;
			break;
		case CAMERA_CAPTURE_MODE_VIDEO_SNAPSHOT:
			mTakePictureMode = SNAPSHOT_VIDEO_MODE;
			mCaptureMode = CAMERA_ZSL_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mRecordingMode = true;
			mPicCaptureCnt = 1;
			mZslPreviewMode = false;
			mVideoShotNum = frame_number;
			mVideoShotFlag = 1;
			break;
		case CAMERA_CAPTURE_MODE_ISP_TUNING_TOOL:
			mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
			mCaptureMode = CAMERA_ISP_TUNING_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mRecordingMode =false;
			mPicCaptureCnt = 1;
			mZslPreviewMode = false;
			break;
		case CAMERA_CAPTURE_MODE_ISP_SIMULATION_TOOL:
			mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
			mCaptureMode = CAMERA_ISP_SIMULATION_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mRecordingMode =false;
			mPicCaptureCnt = 1;
			mZslPreviewMode = false;
			break;
		case CAMERA_CAPTURE_MODE_PREVIEW_SNAPSHOT:
			mTakePictureMode = SNAPSHOT_PREVIEW_MODE;
			mCaptureMode = CAMERA_ZSL_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mRecordingMode = false;
			mPicCaptureCnt = 1;
			mZslPreviewMode = false;
			break;
#ifdef CONFIG_SPRD_PRIVATE_ZSL
		case CAMERA_CAPTURE_MODE_SPRD_ZSL_SNAPSHOT:
			mTakePictureMode = SNAPSHOT_ZSL_MODE;
			mCaptureMode = CAMERA_ZSL_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mRecordingMode =false;
			mPicCaptureCnt = 1;
			mZslPreviewMode = false;
			break;
		case CAMERA_CAPTURE_MODE_SPRD_ZSL_PREVIEW:
			mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
			mCaptureMode = CAMERA_ZSL_MODE;
			mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
			mTakePictureMode = SNAPSHOT_ZSL_MODE;
			mRecordingMode = false;
			mZslPreviewMode = false;
			break;
#endif
		default:
			break;
	}

	return NO_ERROR;
}

int SprdCamera3OEMIf::timer_stop()
{
	if(mPrvTimerID) {
		timer_delete(mPrvTimerID);
		mPrvTimerID = NULL;
	}

	return NO_ERROR;
}

int SprdCamera3OEMIf::timer_set(void *obj, int32_t delay_ms, timer_handle_func handler)
{
	int status;
	struct itimerspec ts;
	struct sigevent se;
	SprdCamera3OEMIf *dev = reinterpret_cast<SprdCamera3OEMIf *>(obj);

	if(!mPrvTimerID) {
		memset(&se, 0, sizeof(struct sigevent));
		se.sigev_notify = SIGEV_THREAD;
		se.sigev_value.sival_ptr = dev;
		se.sigev_notify_function = handler;
		se.sigev_notify_attributes = NULL;

		status = timer_create(CLOCK_MONOTONIC, &se,&mPrvTimerID);
		if(status != 0) {
			HAL_LOGD("time create fail");
			return status;
		}
		HAL_LOGD("timer id=0x%x ms=%d", mPrvTimerID, delay_ms);
	}

	ts.it_value.tv_sec = delay_ms / 1000;
	ts.it_value.tv_nsec = (delay_ms - (ts.it_value.tv_sec * 1000)) * 1000000;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;

	status = timer_settime(mPrvTimerID, 0, &ts, NULL);

	return status;
}

void SprdCamera3OEMIf::timer_hand(union sigval arg)
{
	SprdCamera3Stream *pre_stream = NULL;
	uint32_t frm_num = 0;
	int ret = NO_ERROR;
	bool ispStart = false;
	SENSOR_Tag sensorInfo;
	SprdCamera3OEMIf *dev = reinterpret_cast<SprdCamera3OEMIf *>(arg.sival_ptr);
	SprdCamera3RegularChannel* channel = reinterpret_cast<SprdCamera3RegularChannel *>(dev->mRegularChan);
	HAL_LOGD("E proc stat=%d timer id=0x%x get lock bef", dev->mIspToolStart, dev->mPrvTimerID);

	timer_delete(dev->mPrvTimerID);
	dev->mPrvTimerID = NULL;
	{
        Mutex::Autolock l(&dev->mLock);
        ispStart = dev->mIspToolStart;
    }
	if (ispStart) {
		channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &pre_stream);
		if(pre_stream) {//preview stream
			ret = pre_stream->getQBuffFirstNum(&frm_num);
			if(ret == NO_ERROR) {
				dev->mSetting->getSENSORTag(&sensorInfo);
				channel->channelClearInvalidQBuff(frm_num, sensorInfo.timestamp, CAMERA_STREAM_TYPE_PREVIEW);
				dev->timer_set(dev, ISP_TUNING_WAIT_MAX_TIME, timer_hand);
			}
		}
	}

	HAL_LOGD("X frm=%d", frm_num);
}

void SprdCamera3OEMIf::timer_hand_take(union sigval arg)
{
	int ret = NO_ERROR;
	SprdCamera3OEMIf *dev = reinterpret_cast<SprdCamera3OEMIf *>(arg.sival_ptr);
	HAL_LOGD("E timer id=0x%x", dev->mPrvTimerID);

	timer_delete(dev->mPrvTimerID);
	dev->mPrvTimerID = NULL;
	dev->mCaptureMode = CAMERA_NORMAL_MODE;
	dev->takePicture();
}

int SprdCamera3OEMIf::Callback_VideoFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum)
{
	cmr_u32 i;
	Mutex::Autolock l(&mPrevBufLock);

	HAL_LOGD("mVideoHeapNum %d sum %d", mVideoHeapNum, sum);

	for (i=0 ; i<mVideoHeapNum ; i++) {
		if (NULL != mVideoHeapArray[i]) {
			freeCameraMem(mVideoHeapArray[i]);
		}
		mVideoHeapArray[i] = NULL;
	}
	mVideoHeapNum = 0;

	return 0;
}

int SprdCamera3OEMIf::Callback_VideoMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr)
{
	sprd_camera_memory_t *memory = NULL;
	cmr_int              i = 0;

	HAL_LOGD("size %d sum %d mVideoHeapNum %d", size, sum, mVideoHeapNum);

	*phy_addr = 0;
	*vir_addr = 0;

	if (mVideoHeapNum >= (kVideoBufferCount+kVideoRotBufferCount+1)) {
		HAL_LOGE("error mPreviewHeapNum %d", mVideoHeapNum);
		return BAD_VALUE;
	}

	if ((mVideoHeapNum+sum) >= (kVideoBufferCount+kVideoRotBufferCount+1)) {
		HAL_LOGE("malloc is too more %d %d", mVideoHeapNum, sum);
		return BAD_VALUE;
	}

	if (sum > kVideoBufferCount) {
		mVideoHeapNum = kVideoBufferCount;
		phy_addr += kVideoBufferCount;
		vir_addr += kVideoBufferCount;
		for (i=kVideoBufferCount ; i<(cmr_int)sum ; i++) {
			memory = allocCameraMem(size, 1, true);

			if (NULL == memory) {
				HAL_LOGE("error memory is null.");
				goto mem_fail;
			}

			mVideoHeapArray[mVideoHeapNum] = memory;
			mVideoHeapNum++;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		}
	} else {
		HAL_LOGD("Do not need malloc, malloced num %d,request num %d, request size 0x%x",
			mVideoHeapNum, sum, size);
		goto mem_fail;
	}

	return 0;

mem_fail:
	Callback_VideoFree(0, 0, 0);
	return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_ZslFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum)
{
	cmr_u32 i;
	Mutex::Autolock l(&mPrevBufLock);
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	Mutex::Autolock zsllock(&mZslBufLock);
#endif
	HAL_LOGD("mZslHeapNum %d sum %d", mZslHeapNum, sum);

	for (i=0 ; i<mZslHeapNum ; i++) {
		if (NULL != mZslHeapArray[i]) {
			freeCameraMem(mZslHeapArray[i]);
		}
		mZslHeapArray[i] = NULL;
	}
	mZslHeapNum = 0;

#ifdef CONFIG_SPRD_PRIVATE_ZSL
	releaseZSLQueue();
#endif
	return 0;
}

int SprdCamera3OEMIf::Callback_ZslMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr)
{
	sprd_camera_memory_t *memory = NULL;
	cmr_int              i = 0;

#ifdef CONFIG_SPRD_PRIVATE_ZSL
	hal_mem_info_t mem_info;
	int ret = 0;
	ZslBufferQueue zsl_buffer_q;
#endif

	HAL_LOGD("size %d sum %d mZslHeapNum %d", size, sum, mZslHeapNum);

	*phy_addr = 0;
	*vir_addr = 0;

	if (mZslHeapNum >= (kZslBufferCount+kZslRotBufferCount+1)) {
		HAL_LOGE("error mPreviewHeapNum %d", mZslHeapNum);
		return BAD_VALUE;
	}

	if ((mZslHeapNum+sum) >= (kZslBufferCount+kZslRotBufferCount+1)) {
		HAL_LOGE("malloc is too more %d %d", mZslHeapNum, sum);
		return BAD_VALUE;
	}

#ifdef CONFIG_SPRD_PRIVATE_ZSL
	if (mSprdZslEnabled == true) {
		releaseZSLQueue();
		sum = mZslMaxBuffNum;
		for (i=0; i<(cmr_int)sum ; i++) {
			memory = allocCameraMem(size, 1, true);
			if (NULL == memory) {
				HAL_LOGE("error memory is null.");
				goto mem_fail;
			}

			if (i >= mZslMaxBuffNum - mZslMapNum) {
				*phy_addr++ = (cmr_uint)memory->phys_addr;
				*vir_addr++ = (cmr_uint)memory->data;
				HAL_LOGD("DCAM %ld, phys_addr 0x%lx", i, (unsigned long)memory->phys_addr);
			} else {
				memset(&mem_info, 0, sizeof(mem_info));
				ret = unmap(memory, &mem_info);
				if (ret) {
					HAL_LOGE("unmap failed");
					return -1;
				}
				HAL_LOGD("unmap DCAM %ld, phys_addr 0x%lx", i, (unsigned long)memory->phys_addr);
			}

			mZslHeapArray[mZslHeapNum] = memory;
			mZslHeapNum++;
			mZslHeapArray_phy[i] = (uintptr_t)memory->phys_addr;
			mZslHeapArray_vir[i] = (uintptr_t)memory->data;

			if (i < mZslMaxBuffNum - mZslMapNum) {
				memset(&zsl_buffer_q, 0, sizeof(zsl_buffer_q));
				zsl_buffer_q.valid = 0;
				zsl_buffer_q.heap_array = mZslHeapArray[mZslHeapNum-1];
				zsl_buffer_q.frame.buf_id = i;
				pushZSLQueue(zsl_buffer_q);
			}
		}
		return 0;
	}
#endif
	if (sum > kZslBufferCount) {
		mZslHeapNum = kZslBufferCount;
		phy_addr += kZslBufferCount;
		vir_addr += kZslBufferCount;
		for (i=kZslBufferCount; i<(cmr_int)sum ; i++) {
			memory = allocCameraMem(size, 1, true);

			if (NULL == memory) {
				HAL_LOGE("error memory is null.");
				goto mem_fail;
			}

			mZslHeapArray[mZslHeapNum] = memory;
			mZslHeapNum++;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		}
	} else {
		HAL_LOGD("Do not need malloc, malloced num %d,request num %d, request size 0x%x",
				mZslHeapNum, sum, size);
		goto mem_fail;
	}

	return 0;

mem_fail:
	Callback_ZslFree(0, 0, 0);
	return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_PreviewFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum)
{
	cmr_u32 i;
	Mutex::Autolock l(&mPrevBufLock);

	HAL_LOGD("mPreviewHeapNum %d sum %d", mPreviewHeapNum, sum);

	for (i=0 ; i<mPreviewHeapNum ; i++) {
		if (NULL != mPreviewHeapArray[i]) {
			freeCameraMem(mPreviewHeapArray[i]);
		}
		mPreviewHeapArray[i] = NULL;
	}
	mPreviewHeapNum = 0;

	return 0;
}

int SprdCamera3OEMIf::Callback_PreviewMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr)
{
	sprd_camera_memory_t *memory = NULL;
	cmr_int              i = 0;

	HAL_LOGD("size %d sum %d mPreviewHeapNum %d", size, sum, mPreviewHeapNum);

	*phy_addr = 0;
	*vir_addr = 0;

	if (mPreviewHeapNum >= (kPreviewBufferCount+kPreviewRotBufferCount+1)) {
		HAL_LOGE("error mPreviewHeapNum %d", mPreviewHeapNum);
		return BAD_VALUE;
	}

	if ((mPreviewHeapNum+sum) >= (kPreviewBufferCount+kPreviewRotBufferCount+1)) {
		HAL_LOGE("malloc is too more %d %d", mPreviewHeapNum, sum);
		return BAD_VALUE;
	}

	if (sum > kPreviewBufferCount) {
		mPreviewHeapNum = kPreviewBufferCount;
		phy_addr += kPreviewBufferCount;
		vir_addr += kPreviewBufferCount;
		for (i=kPreviewBufferCount ; i<(cmr_int)sum ; i++) {
			memory = allocCameraMem(size, 1, true);

			if (NULL == memory) {
				HAL_LOGE("error memory is null.");
				goto mem_fail;
			}

			mPreviewHeapArray[mPreviewHeapNum] = memory;
			mPreviewHeapNum++;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		}
	} else {
		HAL_LOGD("Do not need malloc, malloced num %d,request num %d, request size 0x%x",
				mPreviewHeapNum, sum, size);
		goto mem_fail;
	}

	return 0;

mem_fail:
	Callback_PreviewFree(0, 0, 0);
	return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_CaptureFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum)
{
	cmr_u32 i;
	HAL_LOGD("mSubRawHeapNum %d sum %d", mSubRawHeapNum, sum);
	Mutex::Autolock l(&mCapBufLock);

	for (i=0 ; i<mSubRawHeapNum ; i++) {
		if (NULL != mSubRawHeapArray[i]) {
			freeCameraMem(mSubRawHeapArray[i]);
		}
		mSubRawHeapArray[i] = NULL;
	}
	mSubRawHeapNum = 0;
	mSubRawHeapSize = 0;
	mCapBufIsAvail = 0;

	return 0;
}

int SprdCamera3OEMIf::Callback_CaptureMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr)
{
	sprd_camera_memory_t *memory = NULL;
	cmr_int              i = 0;
	malloc:
	HAL_LOGD("size %d sum %d mSubRawHeapNum %d", size, sum, mSubRawHeapNum);
	mCapBufLock.lock();

	*phy_addr = 0;
	*vir_addr = 0;

	if (mSubRawHeapNum >= MAX_SUB_RAWHEAP_NUM) {
		HAL_LOGE("error mSubRawHeapNum %d", mSubRawHeapNum);
		return BAD_VALUE;
	}

	if ((mSubRawHeapNum+sum) >= MAX_SUB_RAWHEAP_NUM) {
		HAL_LOGE("malloc is too more %d %d", mSubRawHeapNum, sum);
		return BAD_VALUE;
	}

	if (0 == mSubRawHeapNum) {
		for (i=0 ; i<(cmr_int)sum ; i++) {
			memory = allocCameraMem(size, 1, true);

			if (NULL == memory) {
				HAL_LOGE("error memory is null.");
				goto mem_fail;
			}

			mSubRawHeapArray[mSubRawHeapNum] = memory;
			mSubRawHeapNum++;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		}
		mSubRawHeapSize = size;
	} else {
		if ((mSubRawHeapNum >= sum) && (mSubRawHeapSize >= size)) {
			HAL_LOGD("use pre-alloc cap mem");
			for (i=0 ; i<(cmr_int)sum ; i++) {
				*phy_addr++ = (cmr_uint)mSubRawHeapArray[i]->phys_addr;
				*vir_addr++ = (cmr_uint)mSubRawHeapArray[i]->data;
			}
		} else {
			/*HAL_LOGE("failed to malloc memory, malloced num %d, request num %d, request size 0x%x",
				mSubRawHeapNum, sum, size);
			goto mem_fail;*/
			mCapBufLock.unlock();
			Callback_CaptureFree(0, 0, 0);
			goto malloc;
		}
	}

	mCapBufIsAvail = 1;
	mCapBufLock.unlock();
	return 0;

mem_fail:
	mCapBufLock.unlock();
	Callback_CaptureFree(0, 0, 0);
	return -1;
}

int SprdCamera3OEMIf::Callback_CapturePathMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr)
{
	sprd_camera_memory_t *memory = NULL;
	cmr_int              i = 0;

	LOGI("Callback_CapturePathMalloc: size %d sum %d mPathRawHeapNum %d mPathRawHeapSize %d", size, sum, mPathRawHeapNum, mPathRawHeapSize);

	*phy_addr = 0;
	*vir_addr = 0;

	if (mPathRawHeapNum >= MAX_SUB_RAWHEAP_NUM) {
		LOGE("Callback_CapturePathMalloc: error mPathRawHeapNum %d", mPathRawHeapNum);
	return -1;
	}
	if ((mPathRawHeapNum+sum) >= MAX_SUB_RAWHEAP_NUM) {
		LOGE("Callback_CapturePathMalloc: malloc is too more %d %d", mPathRawHeapNum, sum);
	return -1;
	}
	if (0 == mPathRawHeapNum) {
		mPathRawHeapSize = size;
		for (i=0 ; i<(cmr_int)sum ; i++) {
			memory = allocCameraMem(size, 1, true);

			if (NULL == memory) {
				LOGE("Callback_CapturePathMalloc: error memory is null.");
				goto mem_fail;
			}

			mPathRawHeapArray[mPathRawHeapNum] = memory;
			mPathRawHeapNum++;

			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		}
	} else {
		if ((mPathRawHeapNum >= sum) && (mPathRawHeapSize >= size)) {
			LOGI("Callback_CapturePathMalloc :test");
			for (i=0 ; i<(cmr_int)sum ; i++) {
				*phy_addr++ = (cmr_uint)mPathRawHeapArray[i]->phys_addr;
				*vir_addr++ = (cmr_uint)mPathRawHeapArray[i]->data;
			}
		} else {
			LOGE("failed to malloc memory, malloced num %d,request num %d, size 0x%x, request size 0x%x",
				mPathRawHeapNum, sum, mPathRawHeapSize, size);
			goto mem_fail;
		}
	}
	return 0;

mem_fail:
	Callback_CapturePathFree(0, 0, 0);
	return -1;
}

int SprdCamera3OEMIf::Callback_CapturePathFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum)
{
	cmr_u32 i;

	LOGI("Callback_CapturePathFree: mPathRawHeapNum %d sum %d", mPathRawHeapNum, sum);

	for (i=0 ; i<mPathRawHeapNum ; i++) {
		if (NULL != mPathRawHeapArray[i]) {
			freeCameraMem(mPathRawHeapArray[i]);
		}
		mPathRawHeapArray[i] = NULL;
	}
	mPathRawHeapNum = 0;
	mPathRawHeapSize = 0;

	return 0;
}

int SprdCamera3OEMIf::Callback_OtherFree(enum camera_mem_cb_type type, cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum)
{
	cmr_u32 i;
	Mutex::Autolock l(&mPrevBufLock);

	HAL_LOGD("sum %d", sum);

#ifndef CONFIG_MEM_OPTIMIZATION
	if (type == CAMERA_PREVIEW_RESERVED) {
		if (NULL != mPreviewHeapReserved) {
			freeCameraMem(mPreviewHeapReserved);
		}
		mPreviewHeapReserved = NULL;
	}

	if (type == CAMERA_VIDEO_RESERVED) {
		if (NULL != mVideoHeapReserved) {
			freeCameraMem(mVideoHeapReserved);
		}
		mVideoHeapReserved = NULL;
	}

	if (type == CAMERA_SNAPSHOT_ZSL_RESERVED_MEM) {
		if (NULL != mZslHeapReserved) {
			freeCameraMem(mZslHeapReserved);
		}
		mZslHeapReserved = NULL;
	}
#endif

	if (type == CAMERA_ISP_LSC) {
		if (NULL != mIspLscHeapReserved) {
			freeCameraMem(mIspLscHeapReserved);
		}
		mIspLscHeapReserved = NULL;
	}

	if (type == CAMERA_ISP_ANTI_FLICKER) {
		if (NULL != mIspAntiFlickerHeapReserved) {
			freeCameraMem(mIspAntiFlickerHeapReserved);
		}
		mIspAntiFlickerHeapReserved = NULL;
	}

	if (type == CAMERA_ISP_BINGING4AWB) {
		for (i=0 ; i < kISPB4awbCount ; i++) {
			if (NULL != mIspB4awbHeapReserved[i]) {
				freeCameraMem(mIspB4awbHeapReserved[i]);
			}
			mIspB4awbHeapReserved[i] = NULL;
		}
	}

	if (type == CAMERA_ISP_RAWAE) {
		for (i=0 ; i < kISPB4awbCount ; i++) {
			if (NULL != mIspRawAemHeapReserved[i]) {
				mIspRawAemHeapReserved[i]->ion_heap->free_kaddr();
				freeCameraMem(mIspRawAemHeapReserved[i]);
			}
			mIspRawAemHeapReserved[i] = NULL;
		}
	}

/* [Vendor_13/40] */
	if (type == CAMERA_ISP_RAWAF) { 
		for (i=0 ; i < kISPB4awbCount ; i++) {
			if (NULL != mIspRawAfmHeapReserved[i]) {
				mIspRawAfmHeapReserved[i]->ion_heap->free_kaddr();
				freeCameraMem(mIspRawAfmHeapReserved[i]);
			}
			mIspRawAfmHeapReserved[i] = NULL;
		}
	}
#ifdef CONFIG_MULTI_CAP_MEM
	if (type == CAMERA_SNAPSHOT_PATH) {
		HAL_LOGD("Callback_CapturePathFree: mPathRawHeapNum %d sum %d", mPathRawHeapNum, sum);

		for (i=0 ; i<mPathRawHeapNum ; i++) {
			if (NULL != mPathRawHeapArray[i]) {
				freeCameraMem(mPathRawHeapArray[i]);
			}
			mPathRawHeapArray[i] = NULL;
		}
		mPathRawHeapNum = 0;
		mPathRawHeapSize = 0;
	}
#endif
	return 0;
}

int SprdCamera3OEMIf::Callback_OtherMalloc(enum camera_mem_cb_type type, cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr)
{
	sprd_camera_memory_t *memory = NULL;
	cmr_u32              i;

#ifdef CONFIG_MEM_OPTIMIZATION
	cmr_u32 mem_size;
	cmr_u32 mem_sum;
	int buffer_id;
#endif

	HAL_LOGD("size %d sum %d mPreviewHeapNum %d", size, sum, mPreviewHeapNum);

	*phy_addr = 0;
	*vir_addr = 0;
	/*
	if (0 == mPreviewHeapNum) {
		memory = allocCameraMem(size, 1, true);

		if (NULL == memory) {
			HAL_LOGE("error memory is null.");
			goto mem_fail;
		}
		if (type == CAMERA_PREVIEW_RESERVED) {
			mPreviewHeapReserved = memory;
		} else if (type == CAMERA_VIDEO_RESERVED) {
			mVideoHeapReserved = memory;
		} else if(type == CAMERA_SNAPSHOT_ZSL_RESERVED_MEM) {
			mZslHeapReserved = memory;
		}
		*phy_addr++ = (cmr_uint)memory->phys_addr;
		*vir_addr++ = (cmr_uint)memory->data;
	} else {
		HAL_LOGE("failed to malloc memory, malloced num %d,request num %d, request size 0x%x",
				1, sum, size);
		goto mem_fail;
	}*/

#ifdef CONFIG_MEM_OPTIMIZATION
	if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_VIDEO_RESERVED || type == CAMERA_SNAPSHOT_ZSL_RESERVED_MEM) {
		if(NULL == mCommonHeapReserved) {
			buffer_id = camera_pre_capture_get_buffer_id(mCameraId);
			if (camera_get_reserve_buffer_size(mCameraId, buffer_id, &mem_size, &mem_sum) != CMR_CAMERA_SUCCESS) {
				HAL_LOGE("camera_get_reserve_buffer_size failed");
				goto mem_fail;
			}

			memory = allocCameraMem(mem_size, 1, true);
			if (NULL == memory) {
				HAL_LOGE("error memory is null.");
				goto mem_fail;
			}
			mCommonHeapReserved = memory;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		} else {
			HAL_LOGD("malloc Common memory for preview, video, and zsl, malloced type %d,request num %d, request size 0x%x",
				type, sum, size);
			*phy_addr++ = (cmr_uint)mCommonHeapReserved->phys_addr;
			*vir_addr++ = (cmr_uint)mCommonHeapReserved->data;
		}
	}
#else
	if (type == CAMERA_PREVIEW_RESERVED) {
		if(mPreviewHeapReserved == NULL) {
			memory = allocCameraMem(size, 1, true);
			if (NULL == memory) {
				HAL_LOGE("error memory is null.");
				goto mem_fail;
			}
			mPreviewHeapReserved = memory;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		} else {
			HAL_LOGD("malloc Preview memory, malloced type %d,request num %d, request size 0x%x",
				type, sum, size);
			*phy_addr++ = (cmr_uint)mPreviewHeapReserved->phys_addr;
			*vir_addr++ = (cmr_uint)mPreviewHeapReserved->data;
		}
	} else if (type == CAMERA_VIDEO_RESERVED) {
		if(mVideoHeapReserved == NULL) {
			memory = allocCameraMem(size, 1, true);
			if (NULL == memory) {
				HAL_LOGE("error memory is null.");
				goto mem_fail;
			}
			mVideoHeapReserved = memory;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		} else {
			HAL_LOGD("malloc Video memory, malloced type %d,request num %d, request size 0x%x",
				type, sum, size);
			*phy_addr++ = (cmr_uint)mVideoHeapReserved->phys_addr;
			*vir_addr++ = (cmr_uint)mVideoHeapReserved->data;
		}
	} else if(type == CAMERA_SNAPSHOT_ZSL_RESERVED_MEM) {
		if(mZslHeapReserved == NULL) {
			memory = allocCameraMem(size, 1, true);
			if (NULL == memory) {
				HAL_LOGE("error memory is null.");
				goto mem_fail;
			}
			mZslHeapReserved = memory;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		} else {
			HAL_LOGD("malloc ZSL memory, malloced type %d,request num %d, request size 0x%x",
				type, sum, size);
			*phy_addr++ = (cmr_uint)mZslHeapReserved->phys_addr;
			*vir_addr++ = (cmr_uint)mZslHeapReserved->data;
		}
	}
#endif
	else if (type == CAMERA_ISP_LSC) {
		if(mIspLscHeapReserved == NULL) {
			memory = allocCameraMem(size, 1, false);
			if (NULL == memory) {
				HAL_LOGE("error memory is null,malloced type %d",type);
				goto mem_fail;
			}
			mIspLscHeapReserved = memory;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		} else {
			HAL_LOGD("malloc isp lsc memory, malloced type %d,request num %d, request size 0x%x",
				type, sum, size);
			*phy_addr++ = (cmr_uint)mIspLscHeapReserved->phys_addr;
			*vir_addr++ = (cmr_uint)mIspLscHeapReserved->data;
		}
	} else if (type == CAMERA_ISP_BINGING4AWB) {
		cmr_u64* phy_addr_64 = (cmr_u64*)phy_addr;
		cmr_u64* vir_addr_64 = (cmr_u64*)vir_addr;
		for (i = 0; i < sum; i++) {
				memory = allocCameraMem(size, 1, false);
				if(NULL == memory) {
					HAL_LOGE("error memory is null,malloced type %d",type);
					goto mem_fail;
				}
			mIspB4awbHeapReserved[i] = memory;
			*phy_addr_64++ = (cmr_u64)memory->phys_addr;
			*vir_addr_64++ = (cmr_u64)memory->data;
			}
	} else if (type == CAMERA_ISP_ANTI_FLICKER) {
		if(mIspAntiFlickerHeapReserved == NULL) {
			memory = allocCameraMem(size, 1, false);
			if (NULL == memory) {
				HAL_LOGE("error memory is null,malloced type %d",type);
				goto mem_fail;
			}
			mIspAntiFlickerHeapReserved = memory;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		} else {
			HAL_LOGD("malloc isp anti-flicker memory, malloced type %d,request num %d, request size 0x%x",
				type, sum, size);
			*phy_addr++ = (cmr_uint)mIspAntiFlickerHeapReserved->phys_addr;
			*vir_addr++ = (cmr_uint)mIspAntiFlickerHeapReserved->data;
		}
	} else if (type == CAMERA_ISP_RAWAE) {
		cmr_u64 kaddr = 0;
		cmr_u64* phy_addr_64 = (cmr_u64*)phy_addr;
		cmr_u64* vir_addr_64 = (cmr_u64*)vir_addr;
		size_t ksize = 0;
		for (i = 0; i < sum; i++) {
				memory = allocCameraMem(size, 1, false);
				if(NULL == memory) {
					HAL_LOGE("error memory is null,malloced type %d",type);
					goto mem_fail;
				}
			mIspRawAemHeapReserved[i] = memory;
			memory->ion_heap->get_kaddr(&kaddr, &ksize);
			*phy_addr_64++ = kaddr;
			*vir_addr_64++ = (cmr_u64)(memory->data);
		}
/* [Vendor_14/40] */
	} else if (type == CAMERA_ISP_RAWAF) { 
		cmr_u64 kaddr = 0;
		cmr_u64* phy_addr_64 = (cmr_u64*)phy_addr;
		cmr_u64* vir_addr_64 = (cmr_u64*)vir_addr;
		size_t ksize = 0;
		for (i = 0; i < sum; i++) {
				memory = allocCameraMem(size, 1, false);
				if(NULL == memory) {
					HAL_LOGE("error memory is null,malloced type %d",type);
					goto mem_fail;
				}
			mIspRawAfmHeapReserved[i] = memory;
			memory->ion_heap->get_kaddr(&kaddr, &ksize);
			*phy_addr_64++ = kaddr;
			*vir_addr_64++ = (cmr_u64)(memory->data);
		}
	}
#ifdef CONFIG_MULTI_CAP_MEM
	else if (type == CAMERA_SNAPSHOT_PATH) {
		HAL_LOGD(" size %d sum %d mPathRawHeapNum %d mPathRawHeapSize %d", size, sum, mPathRawHeapNum, mPathRawHeapSize);

		if (mPathRawHeapNum >= kPathBufferCount) {
			HAL_LOGE(" error mPathRawHeapNum %d", mPathRawHeapNum);
			goto mem_fail;
		}
		if ((mPathRawHeapNum+sum) >= kPathBufferCount) {
			HAL_LOGE(" malloc is too more %d %d", mPathRawHeapNum, sum);
			goto mem_fail;
		}
		if (0 == mPathRawHeapNum) {
			mPathRawHeapSize = size;
			for (i=0 ; i<(cmr_int)sum ; i++) {
				memory = allocCameraMem(size, 1, false);

				if (NULL == memory) {
					HAL_LOGE(" error memory is null.");
					goto mem_fail;
				}

				mPathRawHeapArray[mPathRawHeapNum] = memory;
				mPathRawHeapNum++;

				*phy_addr++ = (cmr_uint)memory->phys_addr;
				*vir_addr++ = (cmr_uint)memory->data;
			}
		} else {
			if ((mPathRawHeapNum >= sum) && (mPathRawHeapSize >= size)) {
				HAL_LOGD("test");
				for (i=0 ; i<(cmr_int)sum ; i++) {
					*phy_addr++ = (cmr_uint)mPathRawHeapArray[i]->phys_addr;
					*vir_addr++ = (cmr_uint)mPathRawHeapArray[i]->data;
				}
			} else {
				HAL_LOGE("failed to malloc memory, malloced num %d,request num %d, size 0x%x, request size 0x%x",
					mPathRawHeapNum, sum, mPathRawHeapSize, size);
				goto mem_fail;
			}
		}
	}
#endif

	return 0;

mem_fail:
	Callback_OtherFree(type, 0, 0, 0);
	return BAD_VALUE;
}


int SprdCamera3OEMIf::Callback_Free(enum camera_mem_cb_type type, cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum, void* private_data)
{
	SprdCamera3OEMIf* camera = (SprdCamera3OEMIf*)private_data;
	int                 ret = 0;
	HAL_LOGV("E");

	if (!private_data || !phy_addr || !vir_addr) {
		HAL_LOGE("error param 0x%lx 0x%lx 0x%lx", (cmr_uint)phy_addr, (cmr_uint)vir_addr, (cmr_uint)private_data);
		return BAD_VALUE;
	}

	if (type >= CAMERA_MEM_CB_TYPE_MAX) {
		HAL_LOGE("mem type error %ld", (cmr_uint)type);
		return BAD_VALUE;
	}

	if (CAMERA_PREVIEW == type) {
		ret = camera->Callback_PreviewFree(phy_addr, vir_addr, sum);
	} else if (CAMERA_SNAPSHOT == type) {
		// Performance optimization:move Callback_CaptureFree to closeCamera function
		//ret = camera->Callback_CaptureFree(phy_addr, vir_addr, sum);
	} else if (CAMERA_VIDEO == type) {
		ret = camera->Callback_VideoFree(phy_addr, vir_addr, sum);
	} else if(CAMERA_SNAPSHOT_ZSL == type) {
		ret = camera->Callback_ZslFree(phy_addr, vir_addr, sum);
	} else if (CAMERA_SNAPSHOT_PATH == type) {
		ret = camera->Callback_CapturePathFree(phy_addr, vir_addr, sum);
	} else if (CAMERA_PREVIEW_RESERVED == type || CAMERA_VIDEO_RESERVED == type ||
		CAMERA_SNAPSHOT_ZSL_RESERVED_MEM == type || CAMERA_ISP_LSC == type ||
		CAMERA_ISP_BINGING4AWB == type || CAMERA_ISP_ANTI_FLICKER == type ||
/* [Vendor_15/40] */
		CAMERA_ISP_RAWAE == type || CAMERA_SNAPSHOT_PATH == type || CAMERA_ISP_RAWAF == type) { 
		ret = camera->Callback_OtherFree(type, phy_addr, vir_addr, sum);
		HAL_LOGD("[3A Porting]hal3 Callback_Free. type= %ld\n", (cmr_uint)type);
	}

	HAL_LOGV("X");
	return ret;
}

int SprdCamera3OEMIf::Callback_Malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr, cmr_u32 *sum_ptr, cmr_uint *phy_addr,
												cmr_uint *vir_addr, void* private_data)
{
	SprdCamera3OEMIf *camera = (SprdCamera3OEMIf *)private_data;
	int                 ret = 0, i = 0;
	uint32_t            size, sum;
	SprdCamera3RegularChannel *channel = NULL;
	HAL_LOGV("E");

	if (!private_data || !phy_addr || !vir_addr || !size_ptr || !sum_ptr
		|| (0 == *size_ptr) || (0 == *sum_ptr)) {
		HAL_LOGE("error param 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx", (cmr_uint)phy_addr,
			(cmr_uint)vir_addr, (cmr_uint)private_data, (cmr_uint)size_ptr, (cmr_uint)sum_ptr);
		return BAD_VALUE;
	}

	size = *size_ptr;
	sum = *sum_ptr;

	if (type >= CAMERA_MEM_CB_TYPE_MAX) {
		HAL_LOGE("mem type error %ld", (cmr_uint)type);
		return BAD_VALUE;
	}

	if (CAMERA_PREVIEW == type) {
		ret = camera->Callback_PreviewMalloc(size, sum, phy_addr, vir_addr);
	} else if (CAMERA_SNAPSHOT == type){
		ret = camera->Callback_CaptureMalloc(size, sum, phy_addr, vir_addr);
	} else if (CAMERA_VIDEO == type) {
		ret = camera->Callback_VideoMalloc(size, sum, phy_addr, vir_addr);
	} else if(CAMERA_SNAPSHOT_ZSL == type) {
		ret = camera->Callback_ZslMalloc(size, sum, phy_addr, vir_addr);
	} else if (CAMERA_SNAPSHOT_PATH == type) {
		ret = camera->Callback_CapturePathMalloc(size, sum, phy_addr, vir_addr);
	} else if (CAMERA_PREVIEW_RESERVED == type || CAMERA_VIDEO_RESERVED == type ||
		CAMERA_SNAPSHOT_ZSL_RESERVED_MEM == type || CAMERA_ISP_LSC == type ||
		CAMERA_ISP_BINGING4AWB == type || CAMERA_ISP_ANTI_FLICKER == type ||
/* [Vendor_16/40] */
		CAMERA_ISP_RAWAE == type || CAMERA_SNAPSHOT_PATH == type || CAMERA_ISP_RAWAF == type) { 
		ret = camera->Callback_OtherMalloc(type, size, sum, phy_addr, vir_addr);
		HAL_LOGD("[3A Porting]hal3 Callback_Malloc. type= %ld\n", (cmr_uint)type);
	}

	HAL_LOGV("X");
	return ret;
}

int SprdCamera3OEMIf::SetChannelHandle(void *regular_chan, void *picture_chan)
{
	mRegularChan = regular_chan;
	mPictureChan = picture_chan;
	return NO_ERROR;
}

int SprdCamera3OEMIf::SetDimensionPreview(cam_dimension_t preview_size)
{
	if((mPreviewWidth != preview_size.width) || (mPreviewHeight != preview_size.height)) {
		mPreviewWidth = preview_size.width;
		mPreviewHeight = preview_size.height;
		mRestartFlag = true;
	}

	return NO_ERROR;
}

int SprdCamera3OEMIf::SetDimensionVideo(cam_dimension_t video_size)
{
	if((mVideoWidth != video_size.width) || (mVideoHeight != video_size.height)) {
		mVideoWidth = video_size.width;
		mVideoHeight = video_size.height;
		mRestartFlag = true;
	}

	return NO_ERROR;
}

int SprdCamera3OEMIf::SetDimensionRaw(cam_dimension_t raw_size)
{
	if((mRawWidth != raw_size.width) || (mRawHeight != raw_size.height)) {
		mRawWidth = raw_size.width;
		mRawHeight = raw_size.height;
		mRestartFlag = true;
	}

	return NO_ERROR;
}

int SprdCamera3OEMIf::SetDimensionCapture(cam_dimension_t capture_size)
{
	if((mCaptureWidth != capture_size.width) || (mCaptureHeight != capture_size.height)) {
		mCaptureWidth = capture_size.width;
		mCaptureHeight = capture_size.height;
		mRestartFlag = true;
	}

	return NO_ERROR;
}

int SprdCamera3OEMIf::PushPreviewbuff(buffer_handle_t * buff_handle)
{
	SprdCamera3RegularChannel* channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	cmr_uint addr_vir = NULL, addr_phy = NULL, priv_data = NULL;
	SprdCamera3Stream *stream = NULL;
	int ret = NO_ERROR;

	HAL_LOGV("E");

	if(channel &&
		(getPreviewState() == SPRD_INTERNAL_PREVIEW_REQUESTED
		|| getPreviewState() == SPRD_PREVIEW_IN_PROGRESS)) {
		channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &stream);

		if(stream) {
			ret = stream->getQBufForHandle(buff_handle, &addr_vir, &addr_phy, &priv_data);
			HAL_LOGV("addr_phy = 0x%lx, addr_vir = 0x%lx, ret = %d",addr_phy, addr_vir, ret);
			if(ret == NO_ERROR) {
				if(addr_vir != NULL)
					mHalOem->ops->camera_set_preview_buffer(mCameraHandle, addr_phy, addr_vir);
				else
					stream->getQBufNumForVir(addr_vir, &mDropPreviewFrameNum);
			}
		}
	}

	HAL_LOGV("X");

	return NO_ERROR;
}

int SprdCamera3OEMIf::PushVideobuff(buffer_handle_t * buff_handle)
{
	SprdCamera3RegularChannel* channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	cmr_uint addr_vir = NULL, addr_phy = NULL, priv_data = NULL;
	SprdCamera3Stream *stream = NULL;
	int ret = NO_ERROR;

	HAL_LOGV("E, previewState = %d", getPreviewState());

	if(channel &&
		(getPreviewState() == SPRD_INTERNAL_PREVIEW_REQUESTED
		|| getPreviewState() == SPRD_PREVIEW_IN_PROGRESS)) {
		channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &stream);

		if(stream) {
			ret = stream->getQBufForHandle(buff_handle, &addr_vir, &addr_phy, &priv_data);
			HAL_LOGV("addr_phy = 0x%lx, addr_vir = 0x%lx, ret = %d",addr_phy, addr_vir, ret);
			if(ret == NO_ERROR) {
				if(addr_vir != NULL)
					mHalOem->ops->camera_set_video_buffer(mCameraHandle, addr_phy, addr_vir);
				else
					stream->getQBufNumForVir(addr_vir, &mDropVideoFrameNum);
			}
		}
	}

	HAL_LOGV("X");

	return NO_ERROR;
}

int SprdCamera3OEMIf::PushZslbuff(buffer_handle_t * buff_handle)
{
	SprdCamera3RegularChannel* channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	cmr_uint addr_vir = NULL, addr_phy = NULL, zsl_private = NULL;
	SprdCamera3Stream *stream = NULL;
	int ret = NO_ERROR;

	HAL_LOGV("E, previewState = %d", getPreviewState());

	if(channel &&
		(getPreviewState() == SPRD_INTERNAL_PREVIEW_REQUESTED
		|| getPreviewState() == SPRD_PREVIEW_IN_PROGRESS)) {
		channel->getStream(CAMERA_STREAM_TYPE_CALLBACK, &stream);

		if(stream) {
			ret = stream->getQBufForHandle(buff_handle, &addr_vir, &addr_phy, &zsl_private);
			HAL_LOGV("addr_phy = 0x%lx, addr_vir = 0x%lx, zsl_private = 0x%lx ret = %d",addr_phy, addr_vir, zsl_private, ret);
			if(ret == NO_ERROR) {
				if(addr_vir != NULL)
					mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, addr_phy, addr_vir, zsl_private);
				else
					stream->getQBufNumForVir(addr_vir, &mDropZslFrameNum);
			}
		}
	}

	HAL_LOGV("X");

	return NO_ERROR;
}

int SprdCamera3OEMIf::PushFirstPreviewbuff()
{
	SprdCamera3RegularChannel* channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	cmr_uint addr_vir = NULL, addr_phy = NULL;
	SprdCamera3Stream *stream = NULL;
	int ret = NO_ERROR;

//	HAL_LOGD("E");

	if(channel) {
		ret = channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &stream);

		if(stream) {
			ret = stream->getQBuffFirstVir(&addr_vir);
			stream->getQBuffFirstPhy(&addr_phy);
			HAL_LOGD("addr_phy = 0x%lx, addr_vir = 0x%lx, ret = %d",addr_phy, addr_vir, ret);
			if(ret == NO_ERROR) {
				if(addr_vir != NULL)
					mHalOem->ops->camera_set_preview_buffer(mCameraHandle, addr_phy, addr_vir);
				else
					stream->getQBuffFirstNum(&mDropPreviewFrameNum);

			}
		}
	}

//	HAL_LOGD("X");

	return ret;
}

int SprdCamera3OEMIf::PushFirstVideobuff()
{
	SprdCamera3RegularChannel* channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	cmr_uint addr_vir = NULL, addr_phy = NULL;
	SprdCamera3Stream *stream = NULL;
	uint32_t frame_number = 0;
	int ret = NO_ERROR;

//	HAL_LOGD("E");

	if(channel) {
		channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &stream);

		if(stream) {
			ret = stream->getQBuffFirstVir(&addr_vir);
			stream->getQBuffFirstPhy(&addr_phy);
			HAL_LOGD("addr_phy = 0x%lx, addr_vir = 0x%lx, ret = %d",addr_phy, addr_vir, ret);
			if(ret == NO_ERROR) {
				if(addr_vir != NULL)
					mHalOem->ops->camera_set_video_buffer(mCameraHandle, addr_phy, addr_vir);
				else
					stream->getQBuffFirstNum(&mDropVideoFrameNum);
			}
		}
	}

//	HAL_LOGD("X");

	return NO_ERROR;
}

int SprdCamera3OEMIf::PushFirstZslbuff()
{
	SprdCamera3RegularChannel* channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	cmr_uint addr_vir = NULL, addr_phy = NULL, zsl_private = NULL;
	SprdCamera3Stream *stream = NULL;
	uint32_t frame_number = 0;
	int ret = NO_ERROR;

//	HAL_LOGD("E");

	if(channel) {
		channel->getStream(CAMERA_STREAM_TYPE_CALLBACK, &stream);

		if(stream) {
			ret = stream->getQBuffFirstVir(&addr_vir);
			stream->getQBuffFirstPhy(&addr_phy);
			stream->getQBuffFirstPhy(&zsl_private);
			HAL_LOGD("addr_phy = 0x%lx, addr_vir = 0x%lx, ret = %d",addr_phy, addr_vir, ret);
			if(ret == NO_ERROR)
				if(addr_vir != NULL)
					mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, addr_phy, addr_vir, zsl_private);
				else
					stream->getQBuffFirstNum(&mDropZslFrameNum);
		}
	}

//	HAL_LOGD("X");

	return NO_ERROR;
}

int SprdCamera3OEMIf::PushVideoSnapShotbuff(int32_t frame_number, camera_stream_type_t type)
{
	SprdCamera3RegularChannel* channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	cmr_uint addr_vir = NULL, addr_phy = NULL;
	SprdCamera3Stream *stream = NULL;
	int ret = NO_ERROR;

	HAL_LOGD("E");

	if(mVideoShotPushFlag == 0 && frame_number == mVideoShotNum)
		mVideoShotWait.wait(mPreviewCbLock);

	if(mVideoShotPushFlag) {
		channel->getStream(type, &stream);

		if(stream) {
			ret = stream->getQBufAddrForNum(frame_number, &addr_vir, &addr_phy);
			HAL_LOGD("addr_phy = 0x%lx, addr_vir = 0x%lx, frame_number = %d",addr_phy, addr_vir, frame_number);
			if(ret == NO_ERROR && addr_vir != NULL)
				mHalOem->ops->camera_set_video_snapshot_buffer(mCameraHandle, addr_phy, addr_vir);

			mVideoShotPushFlag = 0;
			mVideoShotFlag = 0;
		}
	}

	HAL_LOGD("X");

	return NO_ERROR;
}

int SprdCamera3OEMIf::PushZslSnapShotbuff()
{
	SprdCamera3RegularChannel* channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	cmr_uint addr_vir = NULL, addr_phy = NULL, zsl_private = NULL;
	int ret = NO_ERROR;

	HAL_LOGD("E");

	if(channel) {
		ret = channel->getZSLInputBuff(&addr_vir, &addr_phy, &zsl_private);
		if(ret == NO_ERROR && addr_vir != NULL)
			mHalOem->ops->camera_set_zsl_snapshot_buffer(mCameraHandle, addr_phy, addr_vir);
		channel->releaseZSLInputBuff();
	}

	HAL_LOGD("X");

	return NO_ERROR;
}

#ifdef CONFIG_SPRD_PRIVATE_ZSL
ZslBufferQueue SprdCamera3OEMIf::popZSLQueue()
{
	List<ZslBufferQueue>::iterator frame;
	ZslBufferQueue ret;
	if(mZSLQueue.size() == 0)
		return ret;
	frame = mZSLQueue.begin()++;
	ret = static_cast<ZslBufferQueue>(*frame);
	mZSLQueue.erase(frame);
	return ret;
}

int SprdCamera3OEMIf::getZSLQueueFrameNum()
{
	int ret = 0;
	ret = mZSLQueue.size();
	HAL_LOGV("%d",ret);
	return ret;
}

void SprdCamera3OEMIf::pushZSLQueue(ZslBufferQueue frame)
{
	mZSLQueue.push_back(frame);
}

void SprdCamera3OEMIf::releaseZSLQueue()
{
	List<ZslBufferQueue>::iterator round;
	HAL_LOGD("para changed.size : %d", mZSLQueue.size());
	while (mZSLQueue.size() > 0) {
		round  = mZSLQueue.begin()++;
		mZSLQueue.erase(round);
	}
}

int SprdCamera3OEMIf::getZSLSnapshotFrame(hal_mem_info_t *mem_info)
{
	int ret = -1;
	int j = 0;
	int buf_id;
	ZslBufferQueue zsl_frame;

	for (List < ZslBufferQueue >::iterator i = mZSLQueue.begin(); i != mZSLQueue.end(); i++) {
		zsl_frame = static_cast<ZslBufferQueue>(*i);
		HAL_LOGD("j = %d", j);
		if (1 == i->valid && j >= mZslHeapNum - mZslMaxFrameNum - 1) {
			mZSLQueue.erase(i);

			buf_id = zsl_frame.frame.buf_id;
			ret = map(zsl_frame.heap_array, mem_info);
			if (ret < 0) {
				HAL_LOGE("map failed");
				return ret;
			}
			mZslHeapArray_phy[buf_id] = (uintptr_t)mem_info->addr_phy;
			mZslHeapArray_vir[buf_id] = (uintptr_t)mem_info->addr_vir;
			mem_info->valid = zsl_frame.valid;
			ret = 0;
			break;
		}
		j++;
	}

	return ret;
}

void SprdCamera3OEMIf::PushAllZslBuffer()
{
	cmr_uint i = 0;

	LOGI("PushAllZslBuffer mZslHeapNum %d", mZslHeapNum);

#ifdef CONFIG_NEED_UNMAP
	//ZslBufferQueue zsl_buffer_q;
	for (i = 0; i < mZslHeapNum; i++) {
		if (0 != mZslHeapArray[i]->phys_addr) {
			HAL_LOGD("i %d phys_addr 0x%x", i, (cmr_uint)mZslHeapArray[i]->phys_addr);
			mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, (cmr_uint)mZslHeapArray[i]->phys_addr, (cmr_uint)mZslHeapArray[i]->data, (cmr_uint)mZslHeapArray[i]);
		}/*else {
				memset(&zsl_buffer_q, 0, sizeof(zsl_buffer_q));
				zsl_buffer_q.valid = 0;
				zsl_buffer_q.heap_array = mZslHeapArray[i];
				zsl_buffer_q.frame.buf_id = i;
				pushZSLQueue(zsl_buffer_q);
		}*/
	}
#else
	releaseZSLQueue();
	for (i = 0; i < mZslHeapNum; i++) {
		mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, (cmr_uint)mZslHeapArray[i]->phys_addr, (cmr_uint)mZslHeapArray[i]->data, (cmr_uint)mZslHeapArray[i]);
	}
#endif

}

void SprdCamera3OEMIf::SprdZslTakePicture()
{
	mZslShotPushFlag = 1;
}

int SprdCamera3OEMIf::map(sprd_camera_memory_t* camera_memory, hal_mem_info_t *mem_info)
{
	if (NULL == camera_memory || !mem_info) {
		HAL_LOGE("Fatal error! memory pointer is null camera_memory 0x%lx mem_info 0x%lx.", camera_memory, mem_info);
		return -1;
	}

	unsigned long paddr = 0;
	uintptr_t tmp_fd = 0;
	size_t psize = 0;
	int result = 0;
	HAL_LOGD("E: camera_memory 0x%x", camera_memory);

	if (0 == s_mem_method) {
		result = camera_memory->ion_heap->get_phy_addr_from_ion(&paddr, &psize);
	} else {
		result = camera_memory->ion_heap->get_iova(ION_MM, &paddr, &psize);
	}

	if (result < 0) {
		HAL_LOGE("error get pHeapIon addr - method %d result 0x%x ",s_mem_method, result);
		return result;
	}

	mem_info->addr_phy = (void*)paddr;
	mem_info->addr_vir = camera_memory->ion_heap->getBase();
	tmp_fd = (uintptr_t)camera_memory;
	mem_info->fd = tmp_fd;

	camera_memory->phys_addr = (uintptr_t)mem_info->addr_phy;
	camera_memory->phys_size = (uint32_t)psize;

	HAL_LOGD("phys_addr 0x%lx, data: 0x%lx", (unsigned long)mem_info->addr_phy, (unsigned long)mem_info->addr_vir);
	HAL_LOGV("X");

	return result;
}

int SprdCamera3OEMIf::unmap(sprd_camera_memory_t* camera_memory, hal_mem_info_t *mem_info)
{
	if (NULL == camera_memory || !mem_info) {
		HAL_LOGE("fatal error! memory pointer is null camera_memory 0x%lx mem_info 0x%lx.", camera_memory, mem_info);
		return -1;
	}
	int result = 0;
	if(camera_memory->ion_heap) {
		if (0 != s_mem_method) {
			HAL_LOGD("free_mm_iova: 0x%lx,data: 0x%lx, 0x%x", (unsigned long)camera_memory->phys_addr, (unsigned long)camera_memory->data,camera_memory->phys_size);
			result = camera_memory->ion_heap->free_iova(ION_MM, camera_memory->phys_addr, camera_memory->phys_size);
			if (0 == result) {
				camera_memory->phys_addr = 0;
			}
		}
	}
	HAL_LOGD("result %d.", result);
	return result;
}

uint32_t SprdCamera3OEMIf::getZslBufferIDForPhy(cmr_uint phy_addr)
{
	uint32_t id = 0xFFFFFFFF;
	int i = 0;
	for (i = 0; i < kPreviewBufferCount+kPreviewRotBufferCount+1; i++) {
		if ((NULL != mZslHeapArray_phy[i]) &&
			(mZslHeapArray_phy[i] == phy_addr)) {
			id = i;
			break;
		}
	}

	return id;
}

int SprdCamera3OEMIf::releaseZslBuffer(struct camera_frame_type *frame)
{
	int ret = 0;
	hal_mem_info_t mem_info;
	ZslBufferQueue zsl_buffer_q;
	if (NULL == frame) {
		HAL_LOGE("error param frame 0x%lx.", frame);
		return -1;
	}

	frame->buf_id = getZslBufferIDForPhy(frame->y_phy_addr);

	memset(&mem_info, 0, sizeof(mem_info));
	memset(&zsl_buffer_q, 0, sizeof(zsl_buffer_q));
	ret = unmap(mZslHeapArray[frame->buf_id], &mem_info);
	if (ret != 0) {
		HAL_LOGE("unmap failed");
		return -1;
	}
	mZslHeapArray_phy[frame->buf_id] = (uintptr_t)0;//(uintptr_t)mem_info.addr_phy;
	mZslHeapArray_vir[frame->buf_id] = (uintptr_t)0;//(uintptr_t)mem_info.addr_vir;
	zsl_buffer_q.frame = *frame;
	zsl_buffer_q.heap_array = mZslHeapArray[frame->buf_id];
	zsl_buffer_q.valid = 1;
	pushZSLQueue(zsl_buffer_q);

	return 0;
}

int SprdCamera3OEMIf::getZslBuffer(hal_mem_info_t *mem_info)
{
	int ret = 0;
	int buf_id;
	ZslBufferQueue zsl_frame;

	if (NULL == mem_info) {
		HAL_LOGE("Error param mem_info 0x%lx.", mem_info);
		return -1;
	}

	zsl_frame = popZSLQueue();
	buf_id = zsl_frame.frame.buf_id;
	ret = map(zsl_frame.heap_array, mem_info);
	if (ret < 0) {
		HAL_LOGE("map failed.");
		return ret;
	}

	mZslHeapArray_phy[buf_id] = (uintptr_t)mem_info->addr_phy;
	mZslHeapArray_vir[buf_id] = (uintptr_t)mem_info->addr_vir;
	mem_info->valid = zsl_frame.valid;

	return ret;
}

void SprdCamera3OEMIf::receiveZslFrame(struct camera_frame_type *frame)
{
	ZslBufferQueue zsl_frame;
	ZslBufferQueue zsl_buffer_q;
	hal_mem_info_t mem_info;
	cmr_int need_pause;
	int i;

	mHalOem->ops->camera_zsl_snapshot_need_pause(mCameraHandle, &need_pause);
	if (PREVIEW_ZSL_FRAME == frame->type) {
		if (1 == mZslShotPushFlag &&1 == mZslChannelStatus) {
			LOGV("receiveZslFrame getZSLQueueFrameNum %d", getZSLQueueFrameNum());
			for (i=getZSLQueueFrameNum(); i>mZslMaxFrameNum; i--) {
				LOGV("receiveZslFrame getZSLQueueFrameNum %d", getZSLQueueFrameNum());
				zsl_frame = popZSLQueue();
				if (!need_pause) {
					mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, zsl_frame.frame.y_phy_addr, zsl_frame.frame.y_vir_addr, (cmr_uint)zsl_frame.heap_array);
				}
			}

		if(0 != mFlashMode && 0!= mZslShotSkipNum)
		{
			zsl_buffer_q.frame = *frame;
			pushZSLQueue(zsl_buffer_q);
			if (mZslMaxFrameNum < getZSLQueueFrameNum() && 1 == mZslChannelStatus) {
				zsl_frame = popZSLQueue();
				mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, zsl_frame.frame.y_phy_addr, zsl_frame.frame.y_vir_addr, (cmr_uint)zsl_frame.heap_array);
			}
			mZslShotSkipNum--;
		}else {
			if (0 == getZSLQueueFrameNum()) {
				Mutex::Autolock zsllock(&mZslBufLock);
				mHalOem->ops->camera_set_zsl_snapshot_buffer(mCameraHandle,frame->y_phy_addr, frame->y_vir_addr);
			} else {
				Mutex::Autolock zsllock(&mZslBufLock);
				zsl_frame = popZSLQueue();
				mHalOem->ops->camera_set_zsl_snapshot_buffer(mCameraHandle,zsl_frame.frame.y_phy_addr, zsl_frame.frame.y_vir_addr);
				if (!need_pause) {
					mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, zsl_frame.frame.y_phy_addr, zsl_frame.frame.y_vir_addr, (cmr_uint)zsl_frame.heap_array);
				}

			}
			mZslShotPushFlag = 0;
			mZslChannelStatus = 0;
			mZslShotSkipNum = 2;
			}
		} else {
			if (SPRD_INTERNAL_RAW_REQUESTED == getCaptureState()) {
				zsl_buffer_q.frame = *frame;
				pushZSLQueue(zsl_buffer_q);
				if (mZslMaxFrameNum < getZSLQueueFrameNum() && 1 == mZslChannelStatus) {
					zsl_frame = popZSLQueue();
					mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, zsl_frame.frame.y_phy_addr, zsl_frame.frame.y_vir_addr, (cmr_uint)zsl_frame.heap_array);
				}
			}
		}
		if (!isCapturing() || !need_pause) {
			zsl_buffer_q.frame = *frame;
			pushZSLQueue(zsl_buffer_q);
			if (mZslMaxFrameNum < getZSLQueueFrameNum() && 1 == mZslChannelStatus) {
				zsl_frame = popZSLQueue();
				mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, zsl_frame.frame.y_phy_addr, zsl_frame.frame.y_vir_addr, (cmr_uint)zsl_frame.heap_array);
			}
		}
	} else if (PREVIEW_ZSL_CANCELED_FRAME == frame->type) {
		if (!isCapturing() || !need_pause) {
			mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, frame->y_phy_addr, frame->y_vir_addr, frame->zsl_private);
		}
	}
}

void SprdCamera3OEMIf::processZslFrame(void *p_data)
{
	SprdCamera3OEMIf * obj = (SprdCamera3OEMIf *)p_data;
	ZslBufferQueue zsl_buffer_q;
	hal_mem_info_t mem_info;
	cmr_int need_pause;
	int i;
	int ret = 0;

	memset(&mem_info, 0, sizeof(mem_info));
	mHalOem->ops->camera_zsl_snapshot_need_pause(obj->mCameraHandle, &need_pause);
	if (1 == obj->mZslShotPushFlag && 1 == obj->mZslChannelStatus) {
		HAL_LOGD("processZslFrame getZSLQueueFrameNum %d", obj->getZSLQueueFrameNum());
		ret = obj->getZSLSnapshotFrame(&mem_info);
		if (0 == ret) {
			mHalOem->ops->camera_set_zsl_snapshot_buffer(obj->mCameraHandle, (cmr_uint)mem_info.addr_phy, (cmr_uint)mem_info.addr_vir);
			obj->mZslShotPushFlag = 0;
			obj->mZslChannelStatus = 0;
		}
	} else {
		if (SPRD_INTERNAL_RAW_REQUESTED == obj->getCaptureState()) {
			if (obj->mZslMaxFrameNum < obj->getZSLQueueFrameNum() && 1 == obj->mZslChannelStatus) {
				ret = obj->getZslBuffer(&mem_info);
				if(0 == ret) {
					mHalOem->ops->camera_set_zsl_buffer(obj->mCameraHandle, (cmr_uint)mem_info.addr_phy, (cmr_uint)mem_info.addr_vir, (cmr_uint)mem_info.fd);
				}
			}
		}
	}
	if (!obj->isCapturing() || !need_pause) {
		if (obj->mZslMaxFrameNum < obj->getZSLQueueFrameNum() && 1 == obj->mZslChannelStatus) {
			ret = obj->getZslBuffer(&mem_info);
			if(0 == ret) {
				mHalOem->ops->camera_set_zsl_buffer(obj->mCameraHandle, (cmr_uint)mem_info.addr_phy, (cmr_uint)mem_info.addr_vir, (cmr_uint)mem_info.fd);
			}
		}
	}
}

int SprdCamera3OEMIf::ZSLMode_monitor_thread_init(void *p_data)
{
    CMR_MSG_INIT(message);
    int ret = NO_ERROR;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    if (!obj->mZSLModeMonitorInited) {
		ret = cmr_thread_create((cmr_handle*)&obj->mZSLModeMonitorMsgQueHandle,
					ZSLMode_MONITOR_QUEUE_SIZE,
					ZSLMode_monitor_thread_proc,
					(void*)obj);
		if (ret) {
			HAL_LOGE("create send msg failed!");
			return ret;
		}
		obj->mZSLModeMonitorInited = 1;

		message.msg_type  = CMR_EVT_ZSL_MON_INIT;
		message.sync_flag = CMR_MSG_SYNC_RECEIVED;
		ret = cmr_thread_msg_send((cmr_handle)obj->mZSLModeMonitorMsgQueHandle, &message);
		if (ret) {
			HAL_LOGE("send msg failed!");
			return ret;
		}
    }
    return ret;
}

int SprdCamera3OEMIf::ZSLMode_monitor_thread_deinit(void *p_data)
{
    CMR_MSG_INIT(message);
    int ret = NO_ERROR;
    SprdCamera3OEMIf * obj = (SprdCamera3OEMIf *)p_data;

    if (obj->mZSLModeMonitorInited) {
		message.msg_type  = CMR_EVT_ZSL_MON_EXIT;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		ret = cmr_thread_msg_send((cmr_handle)obj->mZSLModeMonitorMsgQueHandle, &message);
		if (ret) {
			HAL_LOGE("send msg failed!");
		}

		ret = cmr_thread_destroy((cmr_handle)obj->mZSLModeMonitorMsgQueHandle);
		obj->mZSLModeMonitorMsgQueHandle = 0;
		obj->mZSLModeMonitorInited = 0;
    }
    return ret ;
}

cmr_int SprdCamera3OEMIf::ZSLMode_monitor_thread_proc(struct cmr_msg *message, void *p_data)
{
	//CMR_MSG_INIT(message);
	int exit_flag = 0;
	int ret = NO_ERROR;
	SprdCamera3OEMIf * obj = (SprdCamera3OEMIf *)p_data;
	camera_frame_type *zsl_frame;
	int i = 0;
	cmr_int need_pause;

	HAL_LOGD("zsl thread message.msg_type 0x%x, sub-type 0x%x, ret %d",
			message->msg_type,
			message->sub_msg_type,
			ret);

	switch (message->msg_type) {
		case CMR_EVT_ZSL_MON_INIT:
			HAL_LOGD("zsl thread msg INITED!");
			break;

		case CMR_EVT_ZSL_MON_PUSH:
			if(obj->getPreviewState() != SPRD_INTERNAL_PREVIEW_STOPPING)
			{
				HAL_LOGD("zsl thread CMR_EVT_ZSL_MON_PUSH!");
				obj->processZslFrame(p_data);
			}
			break;
		case CMR_EVT_ZSL_MON_SNP:
			HAL_LOGD("zsl thread CMR_EVT_ZSL_MON_SNP!");
#ifdef CONFIG_NEED_UNMAP
			obj->processZslFrame(p_data);
#endif
			break;

		case CMR_EVT_ZSL_MON_EXIT:
			HAL_LOGD("zsl thread msg EXIT!");
			exit_flag = 1;
			break;

		default:
			HAL_LOGE("Unsupported ZSLMode monitorMSG");
			break;
	}

	if(exit_flag){
		HAL_LOGD("ZSLMode monitor thread exit ");
	}
	return NULL;
}
#endif

void SprdCamera3OEMIf::decZslMapNum()
{
#ifdef CONFIG_SPRD_PRIVATE_ZSL
#ifdef CONFIG_NEED_UNMAP
		mZslMapNum = 1;
#else
		mZslMapNum = 1;
		mZslMaxBuffNum = 1;
#endif
#endif
}

uint32_t SprdCamera3OEMIf::isPreAllocCapMem()
{
	if (0 == mIsPreAllocCapMem) {
		return 0;
	} else {
		return 1;
	}
}

int SprdCamera3OEMIf::pre_alloc_cap_mem_thread_init(void *p_data)
{
	int ret = NO_ERROR;
	pthread_attr_t attr;

	SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

	if (!obj) {
		HAL_LOGE("obj null  error");
		return -1;
	}

	HAL_LOGD("inited=%d", obj->mPreAllocCapMemInited);

	if (!obj->mPreAllocCapMemInited) {
		obj->mPreAllocCapMemInited = 1;
		obj->mIsPreAllocCapMemDone = 0;
		sem_init(&obj->mPreAllocCapMemSemDone, 0, 0);
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		ret = pthread_create(&obj->mPreAllocCapMemThread,
							&attr,
							pre_alloc_cap_mem_thread_proc,
							(void *)obj);
		if (ret) {
			obj->mPreAllocCapMemInited = 0;
			sem_destroy(&obj->mPreAllocCapMemSemDone);
			HAL_LOGE("fail to send init msg");
		}
	}

	return ret;
}

int SprdCamera3OEMIf::pre_alloc_cap_mem_thread_deinit(void *p_data)
{
	int ret = NO_ERROR;
	SprdCamera3OEMIf * obj = (SprdCamera3OEMIf *)p_data;

	if (!obj) {
		HAL_LOGE("obj null  error");
		return -1;
	}

	HAL_LOGD("inited=%d", obj->mPreAllocCapMemInited);

	if (obj->mPreAllocCapMemInited) {
		sem_wait(&obj->mPreAllocCapMemSemDone);
		sem_destroy(&obj->mPreAllocCapMemSemDone);
		obj->mPreAllocCapMemInited = 0;
		obj->mIsPreAllocCapMemDone = 0;
	}
	return ret ;
}

void * SprdCamera3OEMIf::pre_alloc_cap_mem_thread_proc(void *p_data)
{
	uint32_t mem_size = 0;
	int32_t buffer_id = 0;
	cmr_u32 sum = 0;
	SprdCamera3OEMIf * obj = (SprdCamera3OEMIf *)p_data;
	HAL_LOGD("E");

	if (!obj) {
		HAL_LOGE("obj null  error");
		return NULL;
	}

	buffer_id = mHalOem->ops->camera_pre_capture_get_buffer_id(obj->mCameraId);

	if (mHalOem->ops->camera_pre_capture_get_buffer_size(obj->mCameraId,
		buffer_id, &mem_size, &sum)) {
		obj->mIsPreAllocCapMem = 0;
		HAL_LOGE("buffer size error, using normal alloc cap buffer mode");
	} else {
		cmr_uint phy_addr[MAX_SUB_RAWHEAP_NUM] = {0,0,0,0,0,0,0,0,0,0};
		cmr_uint virt_addr[MAX_SUB_RAWHEAP_NUM] = {0,0,0,0,0,0,0,0,0,0};
		//obj->mSubRawHeapSize = mem_size;
		if (!obj->Callback_CaptureMalloc(mem_size, sum, &phy_addr[0], &virt_addr[0])) {
			obj->mIsPreAllocCapMemDone = 1;
			HAL_LOGD("pre alloc capture mem sum %d, phy addr:0x%lx 0x%lx 0x%lx 0x%lx",
				sum, phy_addr[0], phy_addr[1], phy_addr[2], phy_addr[3]);
		} else {
			obj->mIsPreAllocCapMem = 0;
			HAL_LOGE("buffer alloc error, using normal alloc cap buffer mode");
		}
	}

	sem_post(&obj->mPreAllocCapMemSemDone);

	HAL_LOGD("X");

	return NULL;
}

void SprdCamera3OEMIf::setSensorCloseFlag()
{
	if (NULL == mCameraHandle) {
		HAL_LOGE("oem is null or oem ops is null");
		return;
	}

	mHalOem->ops->camera_set_sensor_close_flag(mCameraHandle);
}

}
