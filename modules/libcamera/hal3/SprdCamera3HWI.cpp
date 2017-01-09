/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#define LOG_TAG "SprdCamera3HWI"
//#define LOG_NDEBUG 0

#include <cutils/properties.h>
#include <hardware/camera3.h>
#include <camera/CameraMetadata.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#ifndef MINICAMERA
#include <ui/Fence.h>
#endif
#include <gralloc_priv.h>
#include "SprdCamera3HWI.h"

#include "SprdCamera3Channel.h"

#include "SprdCamera3OEMIf.h"
#include "SprdCamera3Setting.h"

#ifdef CONFIG_CAMERA_ISP
extern "C" {
#include "isp_video.h"
}
#endif

using namespace android;

namespace sprdcamera {

unsigned int SprdCamera3HWI::mCameraSessionActive = 0;

// err log is always show
// gHALLogLevel(default is 4):
//   1 - only show ALOGE
//   2 - show ALOGE and ALOGW
//   3 - show ALOGE, ALOGW and ALOGI
//   4 - show ALOGE, ALOGW, ALOGI and ALOGD
//   5 - show ALOGE, ALOGW, ALOGI and ALOGD, ALOGV
// use the following command to change gHALLogLevel:
//   adb shell setprop persist.sys.camera.hal.log 1
volatile uint32_t gHALLogLevel = 4;

camera3_device_ops_t SprdCamera3HWI::mCameraOps = {
	initialize:	                        SprdCamera3HWI::initialize,
	configure_streams:                  SprdCamera3HWI::configure_streams,
	register_stream_buffers:            NULL,//SprdCamera3HWI::register_stream_buffers,
	construct_default_request_settings: SprdCamera3HWI::construct_default_request_settings,
	process_capture_request:            SprdCamera3HWI::process_capture_request,
	get_metadata_vendor_tag_ops:        NULL,//SprdCamera3HWI::get_metadata_vendor_tag_ops,
	dump:                               SprdCamera3HWI::dump,
	flush:                              SprdCamera3HWI::flush,
	reserved:{0},
};

static camera3_device_t *g_cam_device;

SprdCamera3Setting *SprdCamera3HWI::mSetting = NULL;

#define SPRD_CONTROL_CAPTURE_INTENT_FLUSH   0xFE
#define SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE   0xFF

SprdCamera3HWI::SprdCamera3HWI(int cameraId)
:	mCameraId(cameraId),
	mOEMIf(NULL),
	mCameraOpened(false),
	mCameraInitialized(false),
	mLastFrmNum(0),
	mCallbackOps(NULL),
	mInputStream(NULL),
	mMetadataChannel(NULL),
	mPictureChannel(NULL),
	mDeqBufNum(0),
	mRecSkipNum(0),
	mIsSkipFrm(false)
{
	getLogLevel();
	HAL_LOGD("E");
	mCameraDevice.common.tag = HARDWARE_DEVICE_TAG;
	mCameraDevice.common.version = CAMERA_DEVICE_API_VERSION_3_2;//CAMERA_DEVICE_API_VERSION_3_0;
	mCameraDevice.common.close = close_camera_device;
	mCameraDevice.ops = &mCameraOps;
	mCameraDevice.priv = this;
	g_cam_device = &mCameraDevice;
	mPendingRequest = 0;
	mCurrentRequestId = -1;
	mCurrentCapIntent = 0;

	mMetadataChannel = NULL;

	mRegularChan = NULL;
	mFirstRegularRequest = false;

	mPicChan = NULL;
	mPictureRequest = false;

	mCallbackChan = NULL;

	mBurstCapCnt = 1;

	mOldCapIntent = 0;
	mOldRequesId = 0;
	mPrvTimerID = NULL;
	mFrameNum = 0;

	mReciveQeqMax = 0;
	mHDRProcessFlag = false;

	HAL_LOGD("X");
}

int SprdCamera3HWI::getNumberOfCameras()
{
	int num = SprdCamera3Setting::getNumberOfCameras();

	return num;
}

SprdCamera3HWI::~SprdCamera3HWI()
{
	SprdCamera3RegularChannel* regularChannel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	SprdCamera3PicChannel* picChannel = reinterpret_cast<SprdCamera3PicChannel *>(mPicChan);
	int64_t timestamp = systemTime();

	if(mHDRProcessFlag == true){
		Mutex::Autolock l(mLock);
	}
	HAL_LOGV("E");

	if (mMetadataChannel) {
		mMetadataChannel->stop(mFrameNum);
		delete mMetadataChannel;
		mMetadataChannel = NULL;
	}

	if (mRegularChan) {
		mRegularChan->stop(mFrameNum);
		regularChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_PREVIEW);
		regularChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_VIDEO);
		regularChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_CALLBACK);
		delete mRegularChan;
		mRegularChan = NULL;
	}

	if (mPicChan) {
		mPicChan->stop(mFrameNum);
		picChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_PICTURE_CALLBACK);
		picChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
		delete mPicChan;
		mPicChan = NULL;
	}

	if (mCallbackChan) {
		mCallbackChan->stop(mFrameNum);
		delete mCallbackChan;
		mCallbackChan = NULL;
	}

	if (mCameraOpened)
		closeCamera();

	timer_stop();

	HAL_LOGV("X");
}

SprdCamera3RegularChannel*  SprdCamera3HWI::getRegularChan()
{
	return mRegularChan;
}

SprdCamera3PicChannel* SprdCamera3HWI::getPicChan()
{
	return mPicChan;
}

SprdCamera3OEMIf* SprdCamera3HWI::getOEMif()
{
	return mOEMIf;
}

static int ispVideoStartPreview(uint32_t param1, uint32_t param2)
{
	SprdCamera3HWI *dev = reinterpret_cast<SprdCamera3HWI *>(g_cam_device->priv);
	SprdCamera3RegularChannel* regularChannel = dev->getRegularChan();
	int rtn = 0x00;

	HAL_LOGD("ISP_TOOL, regularChannel 0x%x", regularChannel);
	if (regularChannel != NULL) {
		regularChannel->setCapturePara(CAMERA_CAPTURE_MODE_PREVIEW);
		rtn = regularChannel->start(dev->mFrameNum);
	}

	return rtn;
}

static int ispVideoStopPreview(uint32_t param1, uint32_t param2)
{

	SprdCamera3HWI *dev = reinterpret_cast<SprdCamera3HWI *>(g_cam_device->priv);
	SprdCamera3RegularChannel* regularChannel = dev->getRegularChan();
	int rtn = 0x00;

	HAL_LOGD("ISP_TOOL, regularChannel 0x%x", regularChannel);
	if (regularChannel != NULL) {
		int64_t timestamp = 0;
		timestamp = systemTime();
		regularChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_PREVIEW);
		regularChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_VIDEO);
		regularChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_CALLBACK);
		rtn = regularChannel->stop(dev->mFrameNum);
	}

	return rtn;
}

static int ispVideoTakePicture(uint32_t param1, uint32_t param2)
{
	int rtn=0x00;
	SprdCamera3HWI *dev = reinterpret_cast<SprdCamera3HWI *>(g_cam_device->priv);
	SprdCamera3PicChannel* picChannel = dev->getPicChan();

	HAL_LOGD("ISP_TOOL, regularChannel 0x%x", picChannel);
	if (NULL != picChannel) {
		// param1 = 1 for simulation case
		if (1 == param1) {
			picChannel->setCapturePara(CAMERA_CAPTURE_MODE_ISP_SIMULATION_TOOL);
		} else {
			picChannel->setCapturePara(CAMERA_CAPTURE_MODE_ISP_TUNING_TOOL);
		}
		rtn = picChannel->start(dev->mFrameNum);
	}

	return rtn;
}

static int ispVideoSetParam(uint32_t width, uint32_t height)
{
	int rtn=0x00;
	SprdCamera3HWI *dev = reinterpret_cast<SprdCamera3HWI *>(g_cam_device->priv);
	SprdCamera3PicChannel* picChannel = dev->getPicChan();
	SprdCamera3OEMIf* oemIf = dev->getOEMif();
	cam_dimension_t capture_size;

	HAL_LOGD("ISP_TOOL, width:%d, height:%d, picChannel 0x%x", width, height, picChannel);
	if (NULL != picChannel) {
		//picChannel->setCaptureSize(width,height);
		capture_size.width = width;
		capture_size.height = height;
		oemIf->SetDimensionCapture(capture_size);
		oemIf->setCaptureRawMode(1);
	}

	return rtn;
}

static int ispCtrlFlash(uint32_t param, uint32_t status)
{
    SprdCamera3HWI *dev = reinterpret_cast<SprdCamera3HWI *>(g_cam_device->priv);
    SprdCamera3OEMIf* oemIf = dev->getOEMif();
    oemIf->setIspFlashMode(status);

    return 0;
}

int SprdCamera3HWI::openCamera(struct hw_device_t **hw_device)
{
	int ret = 0;
	Mutex::Autolock l(mLock);

	if (mCameraSessionActive) {
		HAL_LOGE("multiple simultaneous camera instance not supported");
		return -EUSERS;
	}

	if (mCameraOpened) {
		*hw_device = NULL;
		return PERMISSION_DENIED;
	}

	ret = openCamera();
	if (ret == 0) {
		*hw_device = &mCameraDevice.common;
		mCameraSessionActive = 1;
#ifdef CONFIG_CAMERA_ISP
		startispserver();
		ispvideo_RegCameraFunc(1, ispVideoStartPreview);
		ispvideo_RegCameraFunc(2, ispVideoStopPreview);
		ispvideo_RegCameraFunc(3, ispVideoTakePicture);
		ispvideo_RegCameraFunc(4, ispVideoSetParam);
		ispvideo_RegCameraFunc(REG_CTRL_FLASH, ispCtrlFlash);
#endif
	} else
		*hw_device = NULL;

	return ret;
}

int SprdCamera3HWI::openCamera()
{
	int ret = NO_ERROR;

	if (mOEMIf) {
		HAL_LOGE("Failure: Camera already opened");
		return ALREADY_EXISTS;
	}

	mSetting = new SprdCamera3Setting(mCameraId);
	if (mSetting==NULL) {
		HAL_LOGE("alloc setting failed.");
		return NO_MEMORY;
	}

	mOEMIf = new SprdCamera3OEMIf(mCameraId, mSetting);
	if (!mOEMIf) {
		HAL_LOGE("alloc oemif failed.");
		return NO_MEMORY;
	}
	ret = mOEMIf->openCamera();
	if (NO_ERROR != ret) {
		HAL_LOGE("camera_open failed.");
		return ret;
	}

	mCameraOpened = true;

	return NO_ERROR;
}

int SprdCamera3HWI::closeCamera()
{
	int ret = NO_ERROR;

	if (mOEMIf) {
		mOEMIf->closeCamera();
		delete mOEMIf;
		mOEMIf = NULL;
	}

	if (mSetting) {
		delete mSetting;
		mSetting = NULL;
	}

	mCameraOpened = false;

	return ret;
}

int SprdCamera3HWI::initialize(const struct camera3_callback_ops *callback_ops)
{
	int ret = 0;
	Mutex::Autolock l(mLock);

	mCallbackOps = callback_ops;
	mCameraInitialized = true;
	mFrameNum = 0;
	if (mOEMIf)
		mOEMIf->initialize();

	return ret;
}

camera_metadata_t *SprdCamera3HWI::constructDefaultMetadata(int type)
{
	camera_metadata_t *metadata = NULL;

	HAL_LOGD("S, type = %d",type);

	if (!mSetting) {
		HAL_LOGE("NULL camera device");
		return NULL;
	}
	mSetting->constructDefaultMetadata(type, &metadata);

	if(mOldCapIntent == SPRD_CONTROL_CAPTURE_INTENT_FLUSH) {
		mOldCapIntent = SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE;
	}

	HAL_LOGD("X");
	return metadata;
}

int SprdCamera3HWI::checkStreamList(camera3_stream_configuration_t *streamList)
{
	int ret = NO_ERROR;

	// Sanity check stream_list
	if (streamList == NULL) {
		HAL_LOGE("NULL stream configuration");
		return BAD_VALUE;
	}
	if (streamList->streams == NULL) {
		HAL_LOGE("NULL stream list");
		return BAD_VALUE;
	}

	if (streamList->num_streams < 1) {
		HAL_LOGE("Bad number of streams requested: %d",
			 streamList->num_streams);
		return BAD_VALUE;
	}

	return ret;
}

int32_t SprdCamera3HWI::tranStreamAndChannelType(camera3_stream_t* new_stream, camera_stream_type_t* stream_type, camera_channel_type_t* channel_type)
{

	if(new_stream->stream_type == CAMERA3_STREAM_OUTPUT)
	{
		if(new_stream->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
			new_stream->format =  HAL_PIXEL_FORMAT_YCrCb_420_SP;

		switch (new_stream->format) {
			case HAL_PIXEL_FORMAT_YCrCb_420_SP:
				if (new_stream->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
					*stream_type = CAMERA_STREAM_TYPE_VIDEO;
					*channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
				} else if (new_stream->usage & GRALLOC_USAGE_SW_READ_OFTEN) {
					*stream_type = CAMERA_STREAM_TYPE_CALLBACK;
					*channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
				} else {
					*stream_type = CAMERA_STREAM_TYPE_PREVIEW;
					*channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
					new_stream->usage |= GRALLOC_USAGE_SW_READ_OFTEN;
				}
				break;
			case HAL_PIXEL_FORMAT_YV12:
			case HAL_PIXEL_FORMAT_YCbCr_420_888:
				if (new_stream->usage & GRALLOC_USAGE_SW_READ_OFTEN) {
					*stream_type =  CAMERA_STREAM_TYPE_DEFAULT;
					*channel_type = CAMERA_CHANNEL_TYPE_RAW_CALLBACK;
				}
				break;
			case HAL_PIXEL_FORMAT_BLOB:
				*stream_type =  CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT;
				*channel_type = CAMERA_CHANNEL_TYPE_PICTURE;
				break;
			default:
				*stream_type =  CAMERA_STREAM_TYPE_DEFAULT;
				break;
		}
	}
	else if(new_stream->stream_type== CAMERA3_STREAM_BIDIRECTIONAL) {
		*stream_type =  CAMERA_STREAM_TYPE_CALLBACK;
		*channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
		new_stream->format =  HAL_PIXEL_FORMAT_YCbCr_420_888;
		mInputStream = new_stream;
	}

	HAL_LOGD("stream_type = %d, channel_type = %d w=%d h=%d",*stream_type, *channel_type, new_stream->width, new_stream->height);

	return NO_ERROR;
}

int32_t SprdCamera3HWI::checkStreamSizeAndFormat(camera3_stream_t* new_stream)
{
	SCALER_Tag scaleInfo;
	int i;
	mSetting->getSCALERTag(&scaleInfo);
	HAL_LOGD("newStream->width = %d, newStream->height = %d, new_stream->format = 0x%lx, new_stream->usage = 0x%lx", new_stream->width, new_stream->height, new_stream->format, new_stream->usage);
	HAL_LOGD("sizeof(scaleInfo.available_stream_configurations) = %d, 4*sizeof(int32_t) = %d", sizeof(scaleInfo.available_stream_configurations), 4*sizeof(uint32_t));
	for(i = 0; i< sizeof(scaleInfo.available_stream_configurations)/(4*sizeof(int32_t)); i++)
	{
		if (new_stream->format == scaleInfo.available_stream_configurations[4*i]
			&& new_stream->width == scaleInfo.available_stream_configurations[4*i+1]
			&& new_stream->height == scaleInfo.available_stream_configurations[4*i+2])
			return NO_ERROR;
	}

		return BAD_VALUE;
}

int SprdCamera3HWI::configureStreams(camera3_stream_configuration_t *streamList)
{
	int ret = NO_ERROR;
	Mutex::Autolock l(mLock);
	bool preview_stream_flag = false;
	bool callback_stream_flag = false;
	bool support_bigsize_flag = false;
	cam_dimension_t preview_size = {0, 0};
	cam_dimension_t video_size = {0, 0};
	cam_dimension_t raw_size = {0, 0};
	cam_dimension_t capture_size = {0, 0};

	ret = checkStreamList(streamList);
	if (ret) {
		HAL_LOGE("check failed ret=%d", ret);
		return ret;
	}

	//Create metadata channel and initialize it
	if (mMetadataChannel == NULL) {
		mMetadataChannel = new SprdCamera3MetadataChannel(mOEMIf, captureResultCb,
								mSetting, this);
		if(mMetadataChannel == NULL) {
			HAL_LOGE("failed to allocate metadata channel");
		}
	} else {
		mMetadataChannel->stop(mFrameNum);
	}

	//regular channel
	if(mRegularChan == NULL)
	{
		mRegularChan = new SprdCamera3RegularChannel(mOEMIf,
											captureResultCb,
											mSetting,
											mMetadataChannel,
											CAMERA_CHANNEL_TYPE_REGULAR,
											this);
		if(mRegularChan == NULL) {
			HAL_LOGE("channel created failed");
			return INVALID_OPERATION;
		}
	}

	//picture channel
	if(mPicChan == NULL)
	{
		mPicChan = new SprdCamera3PicChannel(mOEMIf,
									captureResultCb,
									mSetting,
									mMetadataChannel,
									CAMERA_CHANNEL_TYPE_PICTURE,
									this);
		if(mPicChan == NULL)
		{
			HAL_LOGE("channel created failed");
			return INVALID_OPERATION;
		}
	}

	//callback channel
	if(mCallbackChan == NULL)
	{
		mCallbackChan = new SprdCamera3RegularChannel(mOEMIf,
											captureResultCb,
											mSetting,
											mMetadataChannel,
											CAMERA_CHANNEL_TYPE_RAW_CALLBACK,
											this);
		if(mCallbackChan == NULL)
		{
			HAL_LOGE("channel created failed");
			return INVALID_OPERATION;
		}
	}

	/* Allocate channel objects for the requested streams */
	for (size_t i = 0; i < streamList->num_streams; i++) {
		camera3_stream_t *newStream = streamList->streams[i];
		camera_stream_type_t stream_type;
		camera_channel_type_t channel_type;
		cam_dimension_t channel_size;
		bool chan_created = false;
		/*
		ret = checkStreamSizeAndFormat(newStream);
		HAL_LOGD("@check checkStreamSizeAndFormat ret %d", ret);
		if(ret) {
			return ret;
		}
		*/
		tranStreamAndChannelType(newStream, &stream_type, &channel_type);

		if(newStream->width > 2048 && newStream->format != HAL_PIXEL_FORMAT_BLOB) {
			if(support_bigsize_flag == false)
				support_bigsize_flag = true;
			else {
				HAL_LOGE("Hardware not support two big size stream");
				return BAD_VALUE;
			}
		}
		if(stream_type == CAMERA_STREAM_TYPE_PREVIEW) { //for CTS MultiViewTest
			if(preview_stream_flag == false)
				preview_stream_flag = true;
			else {
				stream_type =  CAMERA_STREAM_TYPE_DEFAULT;
				channel_type = CAMERA_CHANNEL_TYPE_RAW_CALLBACK;
			}
		}
		if((stream_type == CAMERA_STREAM_TYPE_DEFAULT) && (channel_type == CAMERA_CHANNEL_TYPE_RAW_CALLBACK)) { //for CTS
			if(callback_stream_flag == false)
				callback_stream_flag = true;
			else {
				stream_type = CAMERA_STREAM_TYPE_PREVIEW;
				channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
			}
		}

		switch(channel_type)
		{
			case CAMERA_CHANNEL_TYPE_REGULAR:
			{
				mRegularChan->stop(mFrameNum);

				ret = mRegularChan->addStream(stream_type, newStream);
				if (stream_type == CAMERA_STREAM_TYPE_PREVIEW) {
					preview_size.width = newStream->width;
					preview_size.height = newStream->height;
				} else if (stream_type == CAMERA_STREAM_TYPE_VIDEO) {
					video_size.width = newStream->width;
					video_size.height = newStream->height;
				} else if(stream_type == CAMERA_STREAM_TYPE_CALLBACK) {
					raw_size.width = newStream->width;
					raw_size.height = newStream->height;
				}
				newStream->priv = mRegularChan;
				newStream->max_buffers = SprdCamera3RegularChannel::kMaxBuffers;
				break;
			}
			case CAMERA_CHANNEL_TYPE_PICTURE:
			{
				mPicChan->stop(mFrameNum);

				ret = mPicChan->addStream(stream_type, newStream);
				if (stream_type == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
					capture_size.width = newStream->width;
					capture_size.height = newStream->height;
				} else if (stream_type == CAMERA_STREAM_TYPE_PICTURE_CALLBACK) {
					raw_size.width = newStream->width;
					raw_size.height = newStream->height;
				}

				newStream->priv = mPicChan;
				newStream->max_buffers = SprdCamera3PicChannel::kMaxBuffers;
				mPictureRequest = false;
				break;
			}
			case CAMERA_CHANNEL_TYPE_RAW_CALLBACK:
			{
				mRegularChan->stop(mFrameNum);
				mPicChan->stop(mFrameNum);

				ret = mRegularChan->addStream(CAMERA_STREAM_TYPE_CALLBACK, newStream);
				ret = mPicChan->addStream(CAMERA_STREAM_TYPE_PICTURE_CALLBACK, newStream);
				raw_size.width = newStream->width;
				raw_size.height = newStream->height;

				newStream->priv = mCallbackChan;
				newStream->max_buffers = SprdCamera3RegularChannel::kMaxBuffers;
				mPictureRequest = false;

				break;
			}
			default:
				HAL_LOGE("channel type is invalid channel");
				break;
		}
	}

	mOldCapIntent = SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE;
	mOEMIf->SetChannelHandle(mRegularChan, mPicChan);

	mOEMIf->SetDimensionPreview(preview_size);
	mOEMIf->SetDimensionVideo(video_size);
	mOEMIf->SetDimensionRaw(raw_size);
	mOEMIf->SetDimensionCapture(capture_size);

	// for cts
	if(preview_size.height * preview_size.width > 3264 * 2448 ||
		raw_size.height * raw_size.width > 3264 * 2448 ||
		(video_size.height != 0 && video_size.width != 0 &&
			capture_size.height * capture_size.width > 3264 * 2448)) {
		mReciveQeqMax = SprdCamera3PicChannel::kMaxBuffers;
		mOEMIf->decZslMapNum();
	} else {
		mReciveQeqMax = SprdCamera3RegularChannel::kMaxBuffers;
	}

	/* Initialize mPendingRequestInfo and mPendnigBuffersMap */
	mPendingRequestsList.clear();

	HAL_LOGD("ret = %d, mCallbackChan = 0x%lx", ret, mCallbackChan);
	return ret;
}

int SprdCamera3HWI::registerStreamBuffers(const camera3_stream_buffer_set_t *buffer_set)
{
	int ret = 0;
	Mutex::Autolock l(mLock);

	if (buffer_set == NULL) {
		HAL_LOGE("Invalid buffer_set parameter.");
		return -EINVAL;
	}
	if (buffer_set->stream == NULL) {
		HAL_LOGE("Invalid stream parameter.");
		return -EINVAL;
	}
	if (buffer_set->num_buffers < 1) {
		HAL_LOGE("Invalid num_buffers %d.", buffer_set->num_buffers);
		return -EINVAL;
	}
	if (buffer_set->buffers == NULL) {
		HAL_LOGE("Invalid buffers parameter.");
		return -EINVAL;
	}

	SprdCamera3Channel *channel =  reinterpret_cast<SprdCamera3Channel *>(buffer_set->stream->priv);//(SprdCamera3Channel *) stream->priv;
	if (channel) {
		ret = channel->registerBuffers(buffer_set);
		if (ret < 0) {
			HAL_LOGE("registerBUffers for stream %p failed", buffer_set->stream);
			return -ENODEV;
		}
	}
	return NO_ERROR;
}

int SprdCamera3HWI::validateCaptureRequest(camera3_capture_request_t *request)
{
	size_t idx = 0;
	const camera3_stream_buffer_t *b = NULL;

	/* Sanity check the request */
	if (request == NULL) {
		HAL_LOGE("NULL capture request");
		return BAD_VALUE;
	}

	uint32_t frameNumber = request->frame_number;
	if (request->input_buffer != NULL &&
			request->input_buffer->stream != mInputStream) {
		HAL_LOGE("Request %d: Input buffer not from input stream!", frameNumber);
		return BAD_VALUE;
	}
	if (request->num_output_buffers < 1
			|| request->output_buffers == NULL) {
		HAL_LOGE("Request %d: No output buffers provided!", frameNumber);
		return BAD_VALUE;
	}
	if (request->input_buffer != NULL) {
		b = request->input_buffer;
		SprdCamera3Channel *channel = static_cast<SprdCamera3Channel * >(b->stream->priv);
		if (channel == NULL) {
			HAL_LOGE("Request %d: Buffer %d: Unconfigured stream!", frameNumber, idx);
			return BAD_VALUE;
		}
		if (b->status != CAMERA3_BUFFER_STATUS_OK) {
			HAL_LOGE("Request %d: Buffer %d: Status not OK!", frameNumber, idx);
			return BAD_VALUE;
		}
		if (b->release_fence != -1) {
			HAL_LOGE("Request %d: Buffer %d: Has a release fence!", frameNumber, idx);
			return BAD_VALUE;
		}
		if (b->buffer == NULL) {
			HAL_LOGE("Request %d: Buffer %d: NULL buffer handle!", frameNumber, idx);
			return BAD_VALUE;
		}
	}
	// Validate all buffers
	b = request->output_buffers;
	do {
		//SprdCamera3Channel *channel = static_cast <SprdCamera3Channel * >(b->stream->priv);
		HAL_LOGV("strm=0x%lx hdl=0x%lx b=0x%lx", (cmr_uint)b->stream, (cmr_uint)b->buffer,(cmr_uint)b);
		SprdCamera3Channel *channel = static_cast <SprdCamera3Channel * >(b->stream->priv);
		if (channel == NULL) {
			HAL_LOGE("Request %d: Buffer %d: Unconfigured stream!", frameNumber, idx);
			return BAD_VALUE;
		}
		if (b->status != CAMERA3_BUFFER_STATUS_OK) {
			HAL_LOGE("Request %d: Buffer %d: Status not OK!", frameNumber, idx);
			return BAD_VALUE;
		}
		if (b->release_fence != -1) {
			HAL_LOGE("Request %d: Buffer %d: Has a release fence!", frameNumber, idx);
			return BAD_VALUE;
		}
		if (b->buffer == NULL) {
			HAL_LOGE("Request %d: Buffer %d: NULL buffer handle!", frameNumber, idx);
			return BAD_VALUE;
		}
		idx++;
		b = request->output_buffers + idx;
	} while (idx < (size_t) request->num_output_buffers);

	return NO_ERROR;
}

void SprdCamera3HWI::flushRequest(uint32_t frame_num)
{
	SprdCamera3RegularChannel* regularChannel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
	SprdCamera3PicChannel* picChannel = reinterpret_cast<SprdCamera3PicChannel *>(mPicChan);
	int64_t timestamp = 0;
	HAL_LOGD("ent");
	if(mMetadataChannel)
		mMetadataChannel->stop(mFrameNum);
	if(mRegularChan)
		mRegularChan->stop(mFrameNum);
	if(mPicChan)
		mPicChan->stop(mFrameNum);

	timestamp = systemTime();
	HAL_LOGD("clear stream");
	if(regularChannel) {
		regularChannel->channelUnmapCurrentQBuff(frame_num, timestamp, CAMERA_STREAM_TYPE_PREVIEW);
		regularChannel->channelUnmapCurrentQBuff(frame_num, timestamp, CAMERA_STREAM_TYPE_VIDEO);
		regularChannel->channelUnmapCurrentQBuff(frame_num,timestamp, CAMERA_STREAM_TYPE_CALLBACK);
		regularChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_PREVIEW);
		regularChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_VIDEO);
		regularChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_CALLBACK);
	}

	if(picChannel){
		picChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_PICTURE_CALLBACK);
		picChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
	}
	HAL_LOGD("exit");
}

void SprdCamera3HWI::getLogLevel()
{
	char prop[PROPERTY_VALUE_MAX];
	int val = 0;

	property_get("persist.sys.camera.hal.log", prop, "0");
	val = atoi(prop);
	if (val > 0) {
		gHALLogLevel = (uint32_t)val;
	}
}

int SprdCamera3HWI::processCaptureRequest(camera3_capture_request_t *request)
{
	int ret = NO_ERROR;
	CapRequestPara capturePara;
	CameraMetadata meta;
	bool invaildRequest = false;
	SprdCamera3Stream *pre_stream = NULL;
	int receive_req_max = SprdCamera3RegularChannel::kMaxBuffers;
	int32_t width = 0, height = 0;
	uint8_t OldCapIntentForVideoSnp = 0;
	Mutex::Autolock l(mLock);

	ret = validateCaptureRequest(request);
	if (ret != NO_ERROR) {
		HAL_LOGE("incoming request is not valid");
		return ret;
	}
	mFrameNum = request->frame_number;
	meta = request->settings;
	mMetadataChannel->request(meta);
	mMetadataChannel->getCapRequestPara(meta, &capturePara);

	if(capturePara.cap_intent == ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE
		&& capturePara.scene_mode == ANDROID_CONTROL_SCENE_MODE_HDR) {//for BUG459753 HDR capture
		mHDRProcessFlag = true;
	} else {
		mHDRProcessFlag = false;
	}

#ifdef CONFIG_CAMERA_GSP_SCALING
	//stop video, we need to notify GSP to close HWC
	if((ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD!=capturePara.cap_intent
		&& ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT!=capturePara.cap_intent)
	|| (request->num_output_buffers==1 &&
	(ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD==capturePara.cap_intent
	|| ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT==capturePara.cap_intent))
	){
		if(camera_notify_closing_gsp_hwc(0))
			HAL_LOGE("processCaptureRequest: stop GSP failed!");
	}
#endif

	switch(capturePara.cap_intent)
	{
		case ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW:
			if(mOldCapIntent != capturePara.cap_intent) {
				mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_PREVIEW, mFrameNum);
				mFirstRegularRequest = true;
			}
#ifdef CONFIG_SPRD_PRIVATE_ZSL
			if(capturePara.sprd_zsl_enabled == true && mOldCapIntent != capturePara.cap_intent) {
				mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_SPRD_ZSL_PREVIEW, mFrameNum);
				if(mOldCapIntent == ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE) {
					mFirstRegularRequest = false;
				}
				else {
					mFirstRegularRequest = true;
				}
			}
#endif
			break;
		case ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD:
			if(mOldCapIntent != capturePara.cap_intent) {
				mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_VIDEO, mFrameNum);
				if(mOldCapIntent != ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT)
					mFirstRegularRequest = true;
			}
			break;
		case ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE:
			if(mOldCapIntent != capturePara.cap_intent) {
				mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_CONTINUE_NON_ZSL_SNAPSHOT, mFrameNum);
				mPictureRequest = true;
			}
#ifdef CONFIG_SPRD_PRIVATE_ZSL
			if(capturePara.sprd_zsl_enabled == true) {
				//if(mOldCapIntent != capturePara.cap_intent) {
					mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_SPRD_ZSL_SNAPSHOT, mFrameNum);
					mPictureRequest = true;
				//}else {
				//	mOEMIf->SprdZslTakePicture();
				//	mPictureRequest = false;
				//}
			}
#endif
			break;
		case ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT:
			if(mOldCapIntent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD) { //for CTS
				//mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_VIDEO_SNAPSHOT, mFrameNum);
				//mPictureRequest = true;
				OldCapIntentForVideoSnp = mOldCapIntent;
			}
			else if(mOldCapIntent != ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE && mOldCapIntent != ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
				mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_CONTINUE_NON_ZSL_SNAPSHOT, mFrameNum);
				mPictureRequest = true;
			}
			break;
		case ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG:
			if(mOldCapIntent != capturePara.cap_intent) {
				mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_ZSL_PREVIEW, mFrameNum);
				mFirstRegularRequest = true;
			}
			break;
		default:
			break;
	}

	if(mOldCapIntent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD && capturePara.cap_intent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
		mOldCapIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
	} else if(mOldCapIntent != SPRD_CONTROL_CAPTURE_INTENT_FLUSH) {
		mOldCapIntent = capturePara.cap_intent;
	} else {
                mOldCapIntent = SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE;
		invaildRequest = true;
        }

	if(capturePara.cap_request_id == 0)
		capturePara.cap_request_id = mOldRequesId;
	else
		mOldRequesId = capturePara.cap_request_id;

	HAL_LOGD("output_buffers = %d, frame_num = %d, input_buffer = 0x%lx PicReq %d CaptureIntent = %d prvReq=%d",
			request->num_output_buffers, request->frame_number, request->input_buffer, mPictureRequest,
					capturePara.cap_intent, mFirstRegularRequest);

#ifndef MINICAMERA
	for (size_t i = 0; i < request->num_output_buffers; i++) {
		const camera3_stream_buffer_t & output = request->output_buffers[i];
		sp < Fence > acquireFence = new Fence(output.acquire_fence);

		ret = acquireFence->wait(Fence::TIMEOUT_NEVER);
		if (ret != OK) {
			HAL_LOGE("%s: fence wait failed %d", __func__, ret);
			return ret;
		}

		acquireFence = NULL;
	}
#endif

	{
		Mutex::Autolock lr(mRequestLock);
		/* Update pending request list and pending buffers map */
		uint32_t frameNumber = request->frame_number;
		FLASH_Tag flashInfo;
		CONTROL_Tag controlInfo;
		PendingRequestInfo pendingRequest;
		int32_t buf_num = 0, buf_num1 = 0;

		mSetting->getFLASHTag(&flashInfo);
		mSetting->getCONTROLTag(&controlInfo);
		pendingRequest.meta_info.flash_mode = flashInfo.mode;
		memcpy(pendingRequest.meta_info.ae_regions, controlInfo.ae_regions, 5 * sizeof(controlInfo.ae_regions[0]));
		memcpy(pendingRequest.meta_info.af_regions, controlInfo.af_regions, 5 * sizeof(controlInfo.af_regions[0]));
		pendingRequest.frame_number = frameNumber;
		pendingRequest.num_buffers = request->num_output_buffers;
		pendingRequest.request_id = capturePara.cap_request_id;
		pendingRequest.bNotified = 0;
		pendingRequest.input_buffer = request->input_buffer;
		for (size_t i = 0; i < request->num_output_buffers; i++) {
			const camera3_stream_buffer_t &output =  request->output_buffers[i];
			camera3_stream_t *stream = output.stream;
			SprdCamera3Channel *channel = (SprdCamera3Channel *)stream->priv;

			if (channel == NULL) {
				HAL_LOGE("invalid channel pointer for stream");
				continue;
			}

			if(channel != mCallbackChan) {
				ret = channel->request(stream, output.buffer, frameNumber);
				if(ret){
					HAL_LOGE("request buff 0x%lx (%d)failed", output.buffer, frameNumber);
					continue;
				}

				if (capturePara.cap_intent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD && channel == mPicChan) {
					mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_VIDEO_SNAPSHOT, frameNumber);
					mPictureRequest = true;
				} else if(capturePara.cap_intent == ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW && channel == mPicChan) {
					if(request->num_output_buffers >=2) {
						mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_PREVIEW_SNAPSHOT, frameNumber);
						if (mOEMIf->GetCameraStatus(CAMERA_STATUS_PREVIEW) == CAMERA_PREVIEW_IDLE)
							mFirstRegularRequest = true;
						receive_req_max = SprdCamera3PicChannel::kMaxBuffers;
					} else {
						mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_ONLY_SNAPSHOT, frameNumber);
						mPictureRequest = true;
						mFirstRegularRequest = false;
					}
					}else if (capturePara.cap_intent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT && channel == mPicChan) {
						if(OldCapIntentForVideoSnp == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD) {//for CTS
						mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_VIDEO_SNAPSHOT, mFrameNum);
						mPictureRequest = true;
					}
				}
			} else {
				if(capturePara.cap_intent == ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW
					|| capturePara.cap_intent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD
					|| capturePara.cap_intent == ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG) {
					ret = mRegularChan->request(stream, output.buffer, frameNumber);
					if(ret){
						HAL_LOGE("request buff 0x%lx (%d)failed", output.buffer, frameNumber);
						continue;
					}
				} else if(capturePara.cap_intent == ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE
					|| capturePara.cap_intent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
					ret = mPicChan->request(stream, output.buffer, frameNumber);
					if(ret){
						HAL_LOGE("request buff 0x%lx (%d)failed", output.buffer, frameNumber);
						continue;
					}
				}
			}

			RequestedBufferInfo requestedBuf;
			requestedBuf.stream = output.stream;
			requestedBuf.buffer = output.buffer;
			pendingRequest.buffers.push_back(requestedBuf);
		}
		pendingRequest.receive_req_max = receive_req_max;

		mPendingRequestsList.push_back(pendingRequest);
		mPendingRequest++;

		if(request->input_buffer != NULL) {
			const camera3_stream_buffer_t* input =  request->input_buffer;
			camera3_stream_t *stream = input->stream;
			SprdCamera3Channel *channel = (SprdCamera3Channel *)stream->priv;

			if (channel == NULL) {
				HAL_LOGE("invalid channel pointer for stream");
				return INVALID_OPERATION;
			}

			HAL_LOGD("zsl input buff 0x%lx (%d)", input->buffer, frameNumber);
			mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_ZSL_SNAPSHOT, frameNumber);
			mPictureRequest = true;
			ret = mRegularChan->setZSLInputBuff(input->buffer);
			if(ret){
				HAL_LOGE("zsl input buff 0x%lx (%d)failed", input->buffer, frameNumber);
				return INVALID_OPERATION;
			}
		}
	}

	if(invaildRequest) {
		HAL_LOGE("Invalid request");
		mFirstRegularRequest = false;
		mPictureRequest = false;
		timer_set(this, 3);
		usleep(200*1000);
		return NO_ERROR;
	}

	mMetadataChannel->start(mFrameNum);

	// For first regular channel request
	if (mFirstRegularRequest) {
		SprdCamera3Channel *channel;
		for (size_t i = 0; i < request->num_output_buffers; i++) {
			camera3_stream_t *stream = request->output_buffers[i].stream;
			channel = (SprdCamera3Channel *)stream->priv;
			HAL_LOGD("channel = 0x%lx, mRegularChan = 0x%lx, mCallbackChan = 0x%lx",channel, mRegularChan, mCallbackChan);
			if(channel == mRegularChan || channel == mCallbackChan) {
				ret = mRegularChan->start(mFrameNum);
				if(ret != NO_ERROR) {
					return ret;
				}
				mFirstRegularRequest = false;
				break;
			}
		}
	}

	// For take picture channel request
	if(mPictureRequest) {
		SprdCamera3Channel *channel;

		for (size_t i = 0; i < request->num_output_buffers; i++) {
			camera3_stream_t *stream = request->output_buffers[i].stream;
			channel = (SprdCamera3Channel *)stream->priv;

			if(channel == mPicChan || channel == mCallbackChan) {
				ret = mPicChan->start(mFrameNum);
				if(ret != NO_ERROR) {
					return ret;
				}
				mPictureRequest = false;
				break;
			}
		}
	}

	{
		Mutex::Autolock lr(mRequestLock);
		while (mPendingRequest >= receive_req_max) {
			ret = mRequestSignal.waitRelative(mRequestLock, kPendingTime);
			if (ret == TIMED_OUT) {
				ret = -ENODEV;
				break;
			}
		}
	}
	if (ret == -ENODEV)
		flushRequest(request->frame_number);
	mHDRProcessFlag = false;

	HAL_LOGV("X");
	return ret;
}

void SprdCamera3HWI::handleCbDataWithLock(cam_result_data_info_t *result_info)
{
	Mutex::Autolock l(mRequestLock);

	uint32_t frame_number = result_info->frame_number;
	buffer_handle_t* buffer = result_info->buffer;
	int64_t capture_time = result_info->timestamp;
	bool     is_urgent = result_info->is_urgent;
	camera3_stream_t *stream = result_info->stream;
	camera3_buffer_status_t buffer_status = result_info->buff_status;
	camera3_msg_type_t msg_type = result_info->msg_type;
	int  oldrequest = mPendingRequest;
	int num_buffers = 0;
	SprdCamera3Stream *pre_stream = NULL;
	int receive_req_max = SprdCamera3RegularChannel::kMaxBuffers;
	int32_t width = 0, height = 0;

	for (List < PendingRequestInfo >::iterator i = mPendingRequestsList.begin(); i != mPendingRequestsList.end();)
	{
		camera3_capture_result_t result;
		camera3_notify_msg_t notify_msg;
		HAL_LOGD("i->frame_number = %d, frame_number = %d,  i->request_id = %d", i->frame_number, frame_number,  i->request_id);

		if (i->frame_number < frame_number) {
			if (!i->bNotified) {
				notify_msg.type = CAMERA3_MSG_SHUTTER;
				notify_msg.message.shutter.frame_number = i->frame_number;
				notify_msg.message.shutter.timestamp = capture_time;
				mCallbackOps->notify(mCallbackOps, &notify_msg);
				i->bNotified = true;
				HAL_LOGD("drop msg frame_num = %d, timestamp = %lld",i->frame_number, capture_time);

				SENSOR_Tag sensorInfo;
				REQUEST_Tag requestInfo;
				meta_info_t metaInfo;
				mSetting->getSENSORTag(&sensorInfo);
				sensorInfo.timestamp = capture_time;
				mSetting->setSENSORTag(sensorInfo);
				mSetting->getREQUESTTag(&requestInfo);
				requestInfo.id = i->request_id;
				requestInfo.frame_count = i->frame_number;
				mSetting->setREQUESTTag(requestInfo);
				metaInfo.flash_mode = i->meta_info.flash_mode;
				memcpy(metaInfo.ae_regions, i->meta_info.ae_regions, 5 * sizeof(i->meta_info.ae_regions[0]));
				memcpy(metaInfo.af_regions, i->meta_info.af_regions, 5 * sizeof(i->meta_info.af_regions[0]));
				mSetting->setMETAInfo(metaInfo);

				result.result = mSetting->translateLocalToFwMetadata();
				result.frame_number = i->frame_number;
				result.num_output_buffers = 0;
				result.output_buffers = NULL;
				result.input_buffer = NULL;
				result.partial_result = 1;
				mCallbackOps->process_capture_result(mCallbackOps, &result);
				free_camera_metadata(const_cast<camera_metadata_t *>(result.result));
			}
			i++;

		} else if (i->frame_number == frame_number) {
			if (!i->bNotified) {
				notify_msg.type = CAMERA3_MSG_SHUTTER;
				notify_msg.message.shutter.frame_number =  i->frame_number;
				notify_msg.message.shutter.timestamp = capture_time;
				mCallbackOps->notify(mCallbackOps, &notify_msg);
				i->bNotified = true;
				HAL_LOGD("notified frame_num = %d, timestamp = %lld",i->frame_number, notify_msg.message.shutter.timestamp);

				SENSOR_Tag sensorInfo;
				REQUEST_Tag requestInfo;
				meta_info_t metaInfo;
				mSetting->getSENSORTag(&sensorInfo);
				sensorInfo.timestamp = capture_time;
				mSetting->setSENSORTag(sensorInfo);
				mSetting->getREQUESTTag(&requestInfo);
				requestInfo.id = i->request_id;
				requestInfo.frame_count = i->frame_number;
				mSetting->setREQUESTTag(requestInfo);
				metaInfo.flash_mode = i->meta_info.flash_mode;
				memcpy(metaInfo.ae_regions, i->meta_info.ae_regions, 5 * sizeof(i->meta_info.ae_regions[0]));
				memcpy(metaInfo.af_regions, i->meta_info.af_regions, 5 * sizeof(i->meta_info.af_regions[0]));
				mSetting->setMETAInfo(metaInfo);

				result.result = mSetting->translateLocalToFwMetadata();
				result.frame_number = i->frame_number;
				result.num_output_buffers = 0;
				result.output_buffers = NULL;
				result.input_buffer = NULL;
				result.partial_result = 1;
				mCallbackOps->process_capture_result(mCallbackOps, &result);
				free_camera_metadata(const_cast<camera_metadata_t *>(result.result));
			}

			for (List<RequestedBufferInfo>::iterator j = i->buffers.begin(); j != i->buffers.end();)
			{
				if (j->stream == stream && j->buffer == buffer) {
					camera3_stream_buffer_t *result_buffers = new camera3_stream_buffer_t[1];

					result_buffers->stream = stream;
					result_buffers->buffer = buffer;
					result_buffers->status = buffer_status;
					result_buffers->acquire_fence = -1;
					result_buffers->release_fence = -1;

					result.result = NULL;
					result.frame_number = i->frame_number;
					result.num_output_buffers = 1;
					result.output_buffers = result_buffers;
					result.input_buffer = i->input_buffer;
					result.partial_result = 0;

					mCallbackOps->process_capture_result(mCallbackOps, &result);
					HAL_LOGV("data frame_number = %d, input_buffer = 0x%lx", result.frame_number, i->input_buffer);

					delete[]result_buffers;

					i->num_buffers--;
					j = i->buffers.erase(j);

					break;
				} else {
					++j;
				}
			}

			HAL_LOGD("num_buffers =%d, mPendingRequest =%d",i->num_buffers, mPendingRequest);

			if (0 == i->num_buffers) {
				receive_req_max = i->receive_req_max;
				i = mPendingRequestsList.erase(i);
				mPendingRequest--;
				break;
			} else {
				++i;
			}
		}

		if (i->frame_number > frame_number) {
			break;
		}
	}

	if (mPendingRequest != oldrequest
		&& oldrequest >= receive_req_max) {
		HAL_LOGD("signal request=%d", oldrequest);
		mRequestSignal.signal();
	}
	//HAL_LOGD("X");
}

void SprdCamera3HWI::getMetadataVendorTagOps(vendor_tag_query_ops_t * ops)
{
	ops->get_camera_vendor_section_name = mSetting->getVendorSectionName;
	ops->get_camera_vendor_tag_type = mSetting->getVendorTagType;
	ops->get_camera_vendor_tag_name = mSetting->getVendorTagName;
	ops->get_camera_vendor_tag_count = mSetting->getVendorTagCnt;
	ops->get_camera_vendor_tags = mSetting->getVendorTags;
	return;
}

void SprdCamera3HWI::dump(int /*fd */)
{
	/*Enable lock when we implement this function */
	/*
	   pthread_mutex_lock(&mMutex);

	   pthread_mutex_unlock(&mMutex);
	 */
	HAL_LOGD("dump E");
	HAL_LOGD("dump X");
	return;
}

int SprdCamera3HWI::flush()
{
	/*Enable lock when we implement this function */
	int ret = NO_ERROR;
	int64_t timestamp = 0;
	//Mutex::Autolock l(mLock);
	timestamp = systemTime();
	if(mHDRProcessFlag == true) {
		if(mPicChan){
			mPicChan->stop(mFrameNum);
			mPicChan->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_PICTURE_CALLBACK);
			mPicChan->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
		}
	}
	Mutex::Autolock l(mLock);

	if(mMetadataChannel)
		mMetadataChannel->stop(mFrameNum);
	if(mRegularChan)
		mRegularChan->stop(mFrameNum);
	if(mPicChan)
		mPicChan->stop(mFrameNum);

	timer_set(this, 20);
	mOldCapIntent = SPRD_CONTROL_CAPTURE_INTENT_FLUSH;
	ret = mFlushSignal.waitRelative(mLock, 800000000); //800ms
	if (ret == TIMED_OUT)
		return -ENODEV;

	return 0;
}


void SprdCamera3HWI::captureResultCb(cam_result_data_info_t *result_info)
{
	//Mutex::Autolock l(mLock);

	if (NULL == result_info) {
		HAL_LOGE("param invalid");
		return;
	}

	handleCbDataWithLock(result_info);

	return;
}

void SprdCamera3HWI::captureResultCb(cam_result_data_info_t *result_info,
							void *userdata)
{
	SprdCamera3HWI *hw = (SprdCamera3HWI *) userdata;
	if (hw == NULL) {
		HAL_LOGE("Invalid hw %p", hw);
		return;
	}

	hw->captureResultCb(result_info);
	return;
}

int SprdCamera3HWI::initialize(const struct camera3_device *device,
						const camera3_callback_ops_t *callback_ops)
{
	HAL_LOGD("mCameraOps E");
	SprdCamera3HWI *hw = reinterpret_cast <SprdCamera3HWI *>(device->priv);
	if (!hw) {
		HAL_LOGE("NULL camera device");
		return -ENODEV;
	}

	int ret = hw->initialize(callback_ops);
	HAL_LOGD("mCameraOps X");
	return ret;
}

int SprdCamera3HWI::configure_streams(const struct camera3_device *device,
						camera3_stream_configuration_t * stream_list)
{
	HAL_LOGD("mCameraOps E");
	SprdCamera3HWI *hw = reinterpret_cast <SprdCamera3HWI *>(device->priv);
	if (!hw) {
		HAL_LOGE("NULL camera device");
		return -ENODEV;
	}
	int ret = hw->configureStreams(stream_list);
	HAL_LOGD("mCameraOps X");
	return ret;
}

int SprdCamera3HWI::register_stream_buffers(const struct camera3_device *device,
						const camera3_stream_buffer_set_t *buffer_set)
{
	HAL_LOGD("mCameraOps E");
	SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
	if (!hw) {
		HAL_LOGE("NULL camera device");
		return -ENODEV;
	}
	int ret = hw->registerStreamBuffers(buffer_set);
	HAL_LOGD("mCameraOps X");
	return ret;
}

const camera_metadata_t *
SprdCamera3HWI::construct_default_request_settings(const struct camera3_device *device, int type)
{
	HAL_LOGD("mCameraOps E");
	camera_metadata_t *fwk_metadata = NULL;
	SprdCamera3HWI *hw = reinterpret_cast < SprdCamera3HWI * >(device->priv);
	if (!hw) {
		HAL_LOGE("NULL camera device");
		return NULL;
	}

	fwk_metadata = hw->constructDefaultMetadata(type);
	HAL_LOGD("mCameraOps X");
	return fwk_metadata;
}


int SprdCamera3HWI::process_capture_request(const struct camera3_device *device,
						camera3_capture_request_t * request)
{
	HAL_LOGD("mCameraOps E");
	SprdCamera3HWI *hw = reinterpret_cast < SprdCamera3HWI * >(device->priv);
	if (!hw) {
		HAL_LOGE("NULL camera device");
		return -EINVAL;
	}

	int ret = hw->processCaptureRequest(request);
	HAL_LOGD("mCameraOps X");
	return ret;
}

void SprdCamera3HWI::get_metadata_vendor_tag_ops(const struct camera3_device *device,
						vendor_tag_query_ops_t * ops)
{
	HAL_LOGD("mCameraOps E");
	SprdCamera3HWI *hw = reinterpret_cast <SprdCamera3HWI *>(device->priv);
	if (!hw) {
		HAL_LOGE("NULL camera device");
		return;
	}

	hw->getMetadataVendorTagOps(ops);
	HAL_LOGD("mCameraOps X");
	return;
}

void SprdCamera3HWI::dump(const struct camera3_device *device, int fd)
{
	HAL_LOGV("S");
	SprdCamera3HWI *hw = reinterpret_cast <SprdCamera3HWI *>(device->priv);
	if (!hw) {
		HAL_LOGE("NULL camera device");
		return;
	}

	hw->dump(fd);
	HAL_LOGV("X");
	return;
}

int SprdCamera3HWI::flush(const struct camera3_device *device)
{
	int ret;

	HAL_LOGD("mCameraOps E");
	SprdCamera3HWI *hw = reinterpret_cast <SprdCamera3HWI *>(device->priv);
	if (!hw) {
		HAL_LOGE("NULL camera device");
		return -EINVAL;
	}

	ret = hw->flush();
	HAL_LOGD("mCameraOps X");
	return ret;
}

int SprdCamera3HWI::close_camera_device(struct hw_device_t *device)
{
	int ret = NO_ERROR;

	HAL_LOGD("S");
	SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(reinterpret_cast <camera3_device_t *>(device)->priv);
	if (!hw) {
		HAL_LOGE("NULL camera device");
		return BAD_VALUE;
	}
	delete hw;
   // after delete this object, set it is NULL.
   /** SPRD: add { */
   hw = NULL;
   /** SPRD: add }*/

#ifdef CONFIG_CAMERA_ISP
	stopispserver();
	ispvideo_RegCameraFunc(1, NULL);
	ispvideo_RegCameraFunc(2, NULL);
	ispvideo_RegCameraFunc(3, NULL);
	ispvideo_RegCameraFunc(4, NULL);
	ispvideo_RegCameraFunc(REG_CTRL_FLASH, NULL);
#endif
	g_cam_device = 0;
	mCameraSessionActive = 0;
	HAL_LOGV("X");
	return ret;
}

int SprdCamera3HWI::timer_stop()
{
	HAL_LOGD("E");

	if(mPrvTimerID) {
		timer_delete(mPrvTimerID);
		mPrvTimerID = NULL;
	}

	return NO_ERROR;
}

int SprdCamera3HWI::timer_set(void *obj, int32_t delay_ms)
{
	int status;
	struct itimerspec ts;
	struct sigevent se;
	SprdCamera3HWI *dev = reinterpret_cast<SprdCamera3HWI *>(obj);
	HAL_LOGD("E");

	if(mPrvTimerID == NULL) {
		memset(&se, 0, sizeof(struct sigevent));
		se.sigev_notify = SIGEV_THREAD;
		se.sigev_value.sival_ptr = dev;
		se.sigev_notify_function = timer_handler;
		se.sigev_notify_attributes = NULL;

		status = timer_create(CLOCK_MONOTONIC, &se,&mPrvTimerID);
		if(status != 0) {
			HAL_LOGD("time create fail");
			return status;
		}
	}

	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = delay_ms * 1000000;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;

	status = timer_settime(mPrvTimerID, 0, &ts, NULL);

	return status;
}

void SprdCamera3HWI::timer_handler(union sigval arg)
{
	SprdCamera3HWI *dev = reinterpret_cast<SprdCamera3HWI *>(arg.sival_ptr);
	SprdCamera3RegularChannel* regularChannel = reinterpret_cast<SprdCamera3RegularChannel *>(dev->mRegularChan);
	SprdCamera3PicChannel* picChannel = reinterpret_cast<SprdCamera3PicChannel *>(dev->mPicChan);
	int64_t timestamp = 0;

	HAL_LOGD("E");

	timestamp = systemTime();

	if(regularChannel) {
		regularChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_PREVIEW);
		regularChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_VIDEO);
		regularChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_CALLBACK);
	}

	if(picChannel){
		picChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_PICTURE_CALLBACK);
		picChannel->channelClearAllQBuff(timestamp, CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
	}
	//dev->mOldCapIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
	(dev->mFlushSignal).signal();
	HAL_LOGD("X");
}

};				//end namespace sprdcamera
