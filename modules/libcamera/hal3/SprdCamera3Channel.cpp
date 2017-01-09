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

#define LOG_TAG "SprdCamera3Channel"
//#define LOG_NDEBUG 0

#include <stdlib.h>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <hardware/camera3.h>
#include <system/camera_metadata.h>
#include <gralloc_priv.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <cutils/properties.h>
#include "SprdCamera3Channel.h"


using namespace android;

#define MIN_STREAMING_BUFFER_NUM 7+11

namespace sprdcamera {


/**************************SprdCamera3Channel************************************************/
SprdCamera3Channel::SprdCamera3Channel(SprdCamera3OEMIf *oem_if,
						channel_cb_routine cb_routine,
						SprdCamera3Setting *setting,
						//SprdCamera3Channel *metadata_channel,
						void *userData)
{
	mOEMIf = oem_if;
	mUserData = userData;
	mChannelCB = cb_routine;
	mSetting = setting;
	//mMetadataChannel = metadata_channel;
}

SprdCamera3Channel::~SprdCamera3Channel()
{
}


/**************************SprdCamera3RegularChannel  start**********************************/
SprdCamera3RegularChannel::SprdCamera3RegularChannel(SprdCamera3OEMIf *oem_if,
								channel_cb_routine cb_routine,
								SprdCamera3Setting *setting,
								SprdCamera3Channel *metadata_channel,
								camera_channel_type_t channel_type,
								void *userData) :
								SprdCamera3Channel(oem_if, cb_routine,
										setting, userData)
{
	HAL_LOGD("E");

	mChannelType = channel_type;

	for(size_t i=0; i<CHANNEL_MAX_STREAM_NUM; i++)
	{
		mCamera3Stream[i] = NULL;
	}
	stream_num = 0;
	mZSLInputBuff = NULL;
	mMemory = new SprdCamera3GrallocMemory();
	if (NULL == mMemory) {
		HAL_LOGE("no mem!");
	}
	memset(&mInputBufInfo, 0, sizeof(hal_mem_info_t));

	mMetadataChannel = metadata_channel;
	HAL_LOGD("X");
}

SprdCamera3RegularChannel::~SprdCamera3RegularChannel()
{
	for(size_t i=0; i<CHANNEL_MAX_STREAM_NUM; i++)
	{
		if(mCamera3Stream[i]) {
			delete mCamera3Stream[i];
			mCamera3Stream[i] = NULL;
		}
	}
	if (mMemory) {
		delete mMemory;
		mMemory = NULL;
	}
}

int SprdCamera3RegularChannel::start(uint32_t frame_number)
{
	int ret = NO_ERROR;
	size_t i = 0;

	ret = mOEMIf->start(mChannelType, frame_number);
	return ret;
}

int SprdCamera3RegularChannel::stop(uint32_t frame_number)
{
	int ret;
	ret = mOEMIf->stop(mChannelType, frame_number);

	return NO_ERROR;
}

int SprdCamera3RegularChannel::channelCbRoutine(uint32_t frame_number, int64_t timestamp, camera_stream_type_t stream_type)
{
	int ret = NO_ERROR;
	cam_result_data_info_t  result_info;
	int8_t index = stream_type - REGULAR_STREAM_TYPE_BASE;

	if(index < 0) {
		HAL_LOGE("stream_type %d is not valied type",stream_type);
		return BAD_VALUE;
	}

	if(mCamera3Stream[index] == NULL)
	{
		HAL_LOGE("channel has no valied stream");
		return INVALID_OPERATION;
	}

	camera3_stream_t *stream;
	buffer_handle_t* buffer;

	mCamera3Stream[index]->getStreamInfo(&stream);
	ret = mCamera3Stream[index]->buffDoneDQ(frame_number, &buffer);
	if(ret != NO_ERROR) {
		HAL_LOGE("dq error, stream_type = %d", stream_type);
		return BAD_VALUE;
	}

	result_info.is_urgent = false;
	result_info.buffer = buffer;
	result_info.frame_number = frame_number;
	result_info.stream = stream;
	result_info.timestamp = timestamp;
	result_info.buff_status = CAMERA3_BUFFER_STATUS_OK;
	result_info.msg_type = CAMERA3_MSG_SHUTTER;

	mChannelCB(&result_info, mUserData);

	return NO_ERROR;
}

int SprdCamera3RegularChannel::channelClearInvalidQBuff(uint32_t frame_num, int64_t timestamp, camera_stream_type_t stream_type)
{
	camera3_stream_t *stream;
	buffer_handle_t* buffer;
	int32_t buff_num;
	uint32_t buff_frame_number;
	int8_t index = stream_type - REGULAR_STREAM_TYPE_BASE;
	cam_result_data_info_t  result_info;
	int ret =NO_ERROR;
	int i = 0;

	if(index < 0) {
		HAL_LOGE("stream_type %d is not valied type",stream_type);
		return BAD_VALUE;
	}

	if(mCamera3Stream[index] == NULL)
	{
		HAL_LOGW("channel has no valied stream, stream_type = %d",stream_type);
		return INVALID_OPERATION;
	}

	mCamera3Stream[index]->getQBufListNum(&buff_num);

	while(i < buff_num)
	{
		ret = mCamera3Stream[index]->getQBuffFirstNum(&buff_frame_number);
		if(ret != NO_ERROR || buff_frame_number > frame_num) {
			return ret;
		}

		mCamera3Stream[index]->getStreamInfo(&stream);
		mCamera3Stream[index]->buffDoneDQ(buff_frame_number, &buffer);
		result_info.is_urgent = false;
		result_info.buffer = buffer;
		result_info.frame_number = buff_frame_number;
		result_info.stream = stream;
		result_info.timestamp = timestamp;
		result_info.buff_status = CAMERA3_BUFFER_STATUS_ERROR;
		result_info.msg_type = CAMERA3_MSG_ERROR;

		mChannelCB(&result_info, mUserData);

		i++;
		HAL_LOGD("drop buff frame_number %d, stream_type %d", buff_frame_number, stream_type);
	}

	return NO_ERROR;
}

int SprdCamera3RegularChannel::channelClearAllQBuff(int64_t timestamp, camera_stream_type_t stream_type)
{
	int ret = NO_ERROR;
	cam_result_data_info_t  result_info;
	int8_t index = stream_type - REGULAR_STREAM_TYPE_BASE;

	if(index < 0) {
		HAL_LOGE("stream_type %d is not valied type",stream_type);
		return BAD_VALUE;
	}

	if(mCamera3Stream[index] == NULL)
	{
		HAL_LOGW("channel has no valied stream");
		return INVALID_OPERATION;
	}

	uint32_t frame_number;
	camera3_stream_t *stream;
	buffer_handle_t* buffer;
	int buff_num;
	mCamera3Stream[index]->getQBufListNum(&buff_num);

	for(int i =0; i<buff_num; i++)
	{
		mCamera3Stream[index]->getStreamInfo(&stream);
		ret = mCamera3Stream[index]->buffFirstDoneDQ(&frame_number, &buffer);
		if(ret != NO_ERROR) {
			HAL_LOGD("dq, bufId = %d",i);
			continue;
		}

		result_info.is_urgent = false;
		result_info.buffer = buffer;
		result_info.frame_number = frame_number;
		result_info.stream = stream;
		result_info.timestamp = timestamp;
		result_info.buff_status = CAMERA3_BUFFER_STATUS_ERROR;
		result_info.msg_type = CAMERA3_MSG_ERROR;

		mChannelCB(&result_info, mUserData);
	}
	return NO_ERROR;
}

int SprdCamera3RegularChannel::channelUnmapCurrentQBuff(uint32_t frame_num, int64_t timestamp, camera_stream_type_t stream_type)
{
	int ret = NO_ERROR;
	cam_result_data_info_t  result_info;
	buffer_handle_t* buffer;
	int8_t index = stream_type - REGULAR_STREAM_TYPE_BASE;

	if(index < 0) {
		HAL_LOGE("stream_type %d is not valied type",stream_type);
		return BAD_VALUE;
	}

	if(mCamera3Stream[index] == NULL)
	{
		HAL_LOGE("channel has no valied stream");
		return INVALID_OPERATION;
	}

	ret = mCamera3Stream[index]->buffDoneDQ(frame_num, &buffer);

	return NO_ERROR;
}

int SprdCamera3RegularChannel::registerBuffers(const camera3_stream_buffer_set_t *buffer_set)
{
	return NO_ERROR;
}

int SprdCamera3RegularChannel::request(camera3_stream_t *stream, buffer_handle_t *buffer, uint32_t frameNumber)
{
	int ret = NO_ERROR;
	int i;

	for(i=0; i<CHANNEL_MAX_STREAM_NUM; i++)
	{
		if(mCamera3Stream[i])
		{
			camera3_stream_t *new_stream;
			mCamera3Stream[i]->getStreamInfo(&new_stream);

			if(new_stream == stream)
			{
				ret = mCamera3Stream[i]->buffDoneQ(frameNumber, buffer);
				if(ret != NO_ERROR) {
					HAL_LOGE("err do Q, stream id = %d",i);
					return ret;
				}
				if(i == 0)
					mOEMIf->PushPreviewbuff(buffer);
				else if(i == (CAMERA_STREAM_TYPE_VIDEO - REGULAR_STREAM_TYPE_BASE))
					mOEMIf->PushVideobuff(buffer);
				else if(i == (CAMERA_STREAM_TYPE_CALLBACK - REGULAR_STREAM_TYPE_BASE))
					mOEMIf->PushZslbuff(buffer);

				break;
			}
		}
	}

	if(i == CHANNEL_MAX_STREAM_NUM) {
		HAL_LOGE("buff request failed, has no stream 0x%lx",stream);
		return INVALID_OPERATION;
	}

	return ret;
}

int SprdCamera3RegularChannel::addStream(camera_stream_type_t stream_type, camera3_stream_t * stream)
{
	camera_stream_type_t oldstream_type;
	camera3_stream_t* oldstream;
	int8_t index = stream_type - REGULAR_STREAM_TYPE_BASE;
	camera_dimension_t stream_size;
	HAL_LOGD("E: index = %d",index);
	if(index < 0) {
		HAL_LOGE("stream_type %d is not valied type",stream_type);
		return BAD_VALUE;
	}

	if(mCamera3Stream[index])
	{
		mCamera3Stream[index]->getStreamType(&oldstream_type);
		mCamera3Stream[index]->getStreamInfo(&oldstream);
		if(stream_type == oldstream_type)
		{
			if(oldstream == stream && oldstream->width == stream->width && oldstream->height == stream->height) {
				return NO_ERROR;
			}
			else {
				delete mCamera3Stream[index];
				mCamera3Stream[index] = NULL;
			}
		}
	}

	stream_size.width = stream->width;
	stream_size.height = stream->height;
	mCamera3Stream[index] = new SprdCamera3Stream(stream,
											stream_size, stream_type,
											this);
	if(mCamera3Stream[index] == NULL) {
		HAL_LOGE("stream create failed");
		return INVALID_OPERATION;
	}

	stream_num++;

	HAL_LOGD("X");
	return NO_ERROR;
}

int SprdCamera3RegularChannel::deleteStream(void)
{
	int8_t index = 0;
	while (mCamera3Stream[index]) {
		delete mCamera3Stream[index];
		HAL_LOGD("index=%d", index);
		mCamera3Stream[index++] = NULL;
		stream_num--;
	}
	return NO_ERROR;
}

int SprdCamera3RegularChannel::getStream(camera_stream_type_t stream_type, SprdCamera3Stream** stream)
{
	int8_t index = stream_type - REGULAR_STREAM_TYPE_BASE;

	if(index < 0) {
		HAL_LOGE("stream_type %d is not valied type",stream_type);
		return BAD_VALUE;
	}

	if(mCamera3Stream[index] == NULL)
	{
		HAL_LOGW("channel has no valied stream(type is %d)", stream_type);
		return INVALID_OPERATION;
	}

	*stream = mCamera3Stream[index];

	return NO_ERROR;
}

int SprdCamera3RegularChannel::setCapturePara(camera_capture_mode_t cap_mode)
{
	mOEMIf->setCapturePara(cap_mode, 0);
	return NO_ERROR;
}

int SprdCamera3RegularChannel::setZSLInputBuff(buffer_handle_t* buff)
{
	mZSLInputBuff = buff;

	return NO_ERROR;
}

int SprdCamera3RegularChannel::getZSLInputBuff(cmr_uint* addr_vir, cmr_uint* addr_phy, cmr_uint* priv_data)
{
	int ret = NO_ERROR;

	ret = mMemory->map(mZSLInputBuff, &mInputBufInfo);
	if(ret != NO_ERROR)
		return ret;
	else {
		*addr_vir = (cmr_uint)mInputBufInfo.addr_vir;
		*addr_phy = (cmr_uint)mInputBufInfo.addr_phy;
		*priv_data = (cmr_uint)mInputBufInfo.fd;
	}

	return NO_ERROR;
}

int SprdCamera3RegularChannel::releaseZSLInputBuff()
{
	mMemory->unmap(mZSLInputBuff, &mInputBufInfo);
	mZSLInputBuff = NULL;
	return NO_ERROR;
}

int SprdCamera3RegularChannel::kMaxBuffers = 4;
/**************************SprdCamera3PicChannel  start**********************************/
SprdCamera3PicChannel::SprdCamera3PicChannel(SprdCamera3OEMIf *oem_if,
							channel_cb_routine cb_routine,
							SprdCamera3Setting *setting,
							SprdCamera3Channel *metadata_channel,
							camera_channel_type_t channel_type,
							void *userData) :
							SprdCamera3Channel(oem_if, cb_routine,
								setting, userData)
{
	HAL_LOGD("E");
	mChannelType = channel_type;

	for(size_t i=0; i<CHANNEL_MAX_STREAM_NUM; i++)
	{
		mCamera3Stream[i] = NULL;
	}
	stream_num = 0;
	buff_index = 0;

	mMetadataChannel = metadata_channel;
	HAL_LOGD("X");
}

SprdCamera3PicChannel::~SprdCamera3PicChannel()
{
	for(size_t i=0; i<CHANNEL_MAX_STREAM_NUM; i++)
	{
		if(mCamera3Stream[i]) {
			delete mCamera3Stream[i];
			mCamera3Stream[i] = NULL;
		}
	}
}

int SprdCamera3PicChannel::start(uint32_t frame_number)
{
	int ret = NO_ERROR;
	size_t i=0;

	ret = mOEMIf->start(mChannelType, frame_number);
	return ret;
}

int SprdCamera3PicChannel::stop(uint32_t frame_number)
{
	int ret;
	ret = mOEMIf->stop(mChannelType, frame_number);

	return NO_ERROR;
}

int SprdCamera3PicChannel::channelCbRoutine(uint32_t frame_number, int64_t timestamp, camera_stream_type_t stream_type)
{
	int ret = NO_ERROR;
	cam_result_data_info_t  result_info;
	int8_t index = stream_type - PIC_STREAM_TYPE_BASE;

	if(index < 0) {
		HAL_LOGE("stream_type %d is not valied type",stream_type);
		return BAD_VALUE;
	}

	if(mCamera3Stream[index] == NULL)
	{
		HAL_LOGE("channel has no valied stream");
		return INVALID_OPERATION;
	}

	camera3_stream_t *stream;
	buffer_handle_t* buffer;

	mCamera3Stream[index]->getStreamInfo(&stream);
	ret = mCamera3Stream[index]->buffDoneDQ(frame_number, &buffer);
	if(ret != NO_ERROR) {
		HAL_LOGE("dq error, stream_type = %d", stream_type);
		return BAD_VALUE;
	}

	result_info.is_urgent = false;
	result_info.buffer = buffer;
	result_info.frame_number = frame_number;
	result_info.stream = stream;
	result_info.timestamp = timestamp;
	result_info.buff_status = CAMERA3_BUFFER_STATUS_OK;
	result_info.msg_type = CAMERA3_MSG_SHUTTER;

	mChannelCB(&result_info, mUserData);

	return NO_ERROR;
}

int SprdCamera3PicChannel::channelClearAllQBuff(int64_t timestamp, camera_stream_type_t stream_type)
{
	int ret = NO_ERROR;
	cam_result_data_info_t  result_info;
	int8_t index = stream_type - PIC_STREAM_TYPE_BASE;

	if(index < 0) {
		HAL_LOGE("stream_type %d is not valied type",stream_type);
		return BAD_VALUE;
	}

	if(mCamera3Stream[index] == NULL)
	{
		HAL_LOGW("channel has no valied stream");
		return INVALID_OPERATION;
	}

	uint32_t frame_number;
	camera3_stream_t *stream;
	buffer_handle_t* buffer;
	int buff_num;
	mCamera3Stream[index]->getQBufListNum(&buff_num);

	for(int i =0; i<buff_num; i++)
	{
		mCamera3Stream[index]->getStreamInfo(&stream);
		ret = mCamera3Stream[index]->buffFirstDoneDQ(&frame_number, &buffer);
		if(ret != NO_ERROR) {
			HAL_LOGD("dq, bufId = %d",i);
			continue;
		}

		result_info.is_urgent = false;
		result_info.buffer = buffer;
		result_info.frame_number = frame_number;
		result_info.stream = stream;
		result_info.timestamp = timestamp;
		result_info.buff_status = CAMERA3_BUFFER_STATUS_ERROR;
		result_info.msg_type = CAMERA3_MSG_ERROR;

		mChannelCB(&result_info, mUserData);
	}
	return NO_ERROR;
}

int SprdCamera3PicChannel::registerBuffers(const camera3_stream_buffer_set_t *buffer_set)
{
	return NO_ERROR;
}

int32_t SprdCamera3PicChannel::request(camera3_stream_t *stream, buffer_handle_t *buffer, uint32_t frameNumber)
{
	int ret;
	int i;
	camera_stream_type_t stream_type;

	for(i=0; i<CHANNEL_MAX_STREAM_NUM; i++)
	{
		if(mCamera3Stream[i])
		{
			camera3_stream_t *new_stream;
			mCamera3Stream[i]->getStreamInfo(&new_stream);

			if(new_stream == stream) {
				if (i == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT - PIC_STREAM_TYPE_BASE) {
					ret = mCamera3Stream[i]->buffDoneQ2(frameNumber, buffer);
					return ret;
				} else if (i == CAMERA_STREAM_TYPE_PICTURE_CALLBACK - PIC_STREAM_TYPE_BASE) {
					ret = mCamera3Stream[i]->buffDoneQ(frameNumber, buffer);
					return ret;
				}
			}
		}
	}

	HAL_LOGE("buff request failed, has no stream 0x%lx",stream);
	return INVALID_OPERATION;
}

int SprdCamera3PicChannel::addStream(camera_stream_type_t stream_type, camera3_stream_t * stream)
{
	camera_stream_type_t oldstream_type;
	camera3_stream_t* oldstream;
	int8_t index = stream_type - PIC_STREAM_TYPE_BASE;
	camera_dimension_t stream_size;
	HAL_LOGD("E: index = %d",index);

	if(index < 0) {
		HAL_LOGE("stream_type %d is not valied type",stream_type);
		return BAD_VALUE;
	}

	if(mCamera3Stream[index])
	{
		mCamera3Stream[index]->getStreamType(&oldstream_type);
		mCamera3Stream[index]->getStreamInfo(&oldstream);
		if(stream_type == oldstream_type)
		{
			if(oldstream == stream && oldstream->width == stream->width && oldstream->height == stream->height) {
				return NO_ERROR;
			}
			else {
				delete mCamera3Stream[index];
				mCamera3Stream[index] = NULL;
			}
		}
	}

	stream_size.width = stream->width;
	stream_size.height = stream->height;
	mCamera3Stream[index] = new SprdCamera3Stream(stream,
											stream_size, stream_type,
											this);
	if(mCamera3Stream[index] == NULL) {
		HAL_LOGE("stream create failed");
		return INVALID_OPERATION;
	}

	stream_num++;

	HAL_LOGD("X");
	return NO_ERROR;
}

int SprdCamera3PicChannel::deleteStream(void)
{
	int8_t index = 0;
	while (mCamera3Stream[index]) {
		delete mCamera3Stream[index];
		HAL_LOGD("index=%d", index);
		mCamera3Stream[index++] = NULL;
		stream_num--;
	}
	return NO_ERROR;
}

int SprdCamera3PicChannel::getStream(camera_stream_type_t stream_type, SprdCamera3Stream** stream)
{
	int8_t index = stream_type - PIC_STREAM_TYPE_BASE;

	if(index < 0) {
		HAL_LOGE("stream_type %d is not valied type",stream_type);
		return BAD_VALUE;
	}

	if(mCamera3Stream[index] == NULL)
	{
		HAL_LOGW("channel has no valied stream(type is %d)", stream_type);
		return INVALID_OPERATION;
	}

	*stream = mCamera3Stream[index];

	return NO_ERROR;
}

int SprdCamera3PicChannel::setCapturePara(camera_capture_mode_t cap_mode)
{
	mOEMIf->setCapturePara(cap_mode, 0);
	return NO_ERROR;
}

int SprdCamera3PicChannel::kMaxBuffers = 1;
/**************************SprdCamera3MetadataChannel  start**********************************/
SprdCamera3MetadataChannel::SprdCamera3MetadataChannel(SprdCamera3OEMIf *oem_if,
								channel_cb_routine cb_routine,
								SprdCamera3Setting *setting,
								//SprdCamera3Channel *metadata_channel,
								void *userData) :
								SprdCamera3Channel(oem_if,
									cb_routine, setting, userData)
{
}

SprdCamera3MetadataChannel::~SprdCamera3MetadataChannel()
{
}

int SprdCamera3MetadataChannel::request(const CameraMetadata &metadata)
{
	CONTROL_Tag controlInfo;

	mSetting->updateWorkParameters(metadata);
	/*ae precapture ae state callback to framework*/
	mSetting->getCONTROLTag(&controlInfo);
	/*if (controlInfo.ae_state == ANDROID_CONTROL_AE_STATE_PRECAPTURE
		&& controlInfo.ae_precap_trigger == ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_START) {
		mOEMIf->setAePrecaptureSta(ANDROID_CONTROL_AE_STATE_CONVERGED);
	}*/
	return 0;
}

int SprdCamera3MetadataChannel::channelCbRoutine(uint32_t frame_number, int64_t timestamp, camera_stream_type_t stream_type)
{
	HAL_LOGD(" E");

	cam_result_data_info_t  result_info;
	memset(&result_info, 0, sizeof(result_info));

	result_info.is_urgent = true;
	result_info.timestamp = timestamp;
	result_info.frame_number = 0;
	mChannelCB(&result_info, mUserData);
	HAL_LOGD(" X");

	return NO_ERROR;
}

int SprdCamera3MetadataChannel::start(uint32_t frame_number)
{
	CONTROL_Tag controlInfo;
	SPRD_DEF_Tag sprddefInfo;
	JPEG_Tag jpegInfo;
	STATISTICS_Tag statisticsInfo;
	int tag = 0;

	while ((tag = mSetting->popAndroidParaTag()) != -1) {
		switch (tag) {
		case ANDROID_CONTROL_AF_TRIGGER:
			mSetting->getCONTROLTag(&controlInfo);
			HAL_LOGD("AF_TRIGGER %d", controlInfo.af_trigger);
			if (controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_START) {
				mOEMIf->autoFocus(this);
			} else if (controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
				mOEMIf->cancelAutoFocus();
			}
			break;

		case ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER:
			HAL_LOGV("ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER");
			mSetting->getCONTROLTag(&controlInfo);
			/*if (controlInfo.ae_precap_trigger == ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_START) {
				mOEMIf->setAePrecaptureSta(ANDROID_CONTROL_AE_STATE_CONVERGED);
			}*/
			break;

		case ANDROID_SCALER_CROP_REGION:
			HAL_LOGD("SCALER_CROP_REGION");
			mOEMIf->setCameraConvertCropRegion();
			break;
		case ANDROID_CONTROL_CAPTURE_INTENT:
			mOEMIf->SetCameraParaTag(ANDROID_CONTROL_CAPTURE_INTENT);
			break;
		case ANDROID_LENS_FOCAL_LENGTH:
			HAL_LOGV("ANDROID_LENS_FOCAL_LENGTH");
			mOEMIf->SetCameraParaTag(ANDROID_LENS_FOCAL_LENGTH);
			break;
		case ANDROID_JPEG_QUALITY:
			HAL_LOGV("ANDROID_JPEG_QUALITY");
			mOEMIf->SetCameraParaTag(ANDROID_JPEG_QUALITY);
			break;
		case ANDROID_JPEG_GPS_COORDINATES:
			HAL_LOGV("ANDROID_JPEG_GPS_COORDINATES");
			mOEMIf->SetJpegGpsInfo(true);
			break;
		case ANDROID_JPEG_GPS_PROCESSING_METHOD:
			HAL_LOGV("ANDROID_JPEG_GPS_PROCESSING_METHOD");
			break;
		case ANDROID_JPEG_GPS_TIMESTAMP:
			HAL_LOGD("ANDROID_JPEG_GPS_TIMESTAMP");
			break;
		case ANDROID_JPEG_ORIENTATION:
			mOEMIf->SetCameraParaTag(ANDROID_JPEG_ORIENTATION);
			break;
		case ANDROID_CONTROL_SCENE_MODE:
			mOEMIf->SetCameraParaTag(ANDROID_CONTROL_SCENE_MODE);
			break;
		case ANDROID_CONTROL_EFFECT_MODE:
			mOEMIf->SetCameraParaTag(ANDROID_CONTROL_EFFECT_MODE);
			break;
		case ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION:
			mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
			break;
		case ANDROID_CONTROL_AWB_MODE:
			mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AWB_MODE);
			break;
		case ANDROID_CONTROL_AE_MODE:
			HAL_LOGV("ANDROID_CONTROL_AE_MODE");
			mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AE_MODE);
			break;
		case ANDROID_CONTROL_AE_ANTIBANDING_MODE:
			mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AE_ANTIBANDING_MODE);
			break;
		case ANDROID_FLASH_MODE:
			HAL_LOGV("ANDROID_FLASH_MODE");
			mOEMIf->SetCameraParaTag(ANDROID_FLASH_MODE);
			break;
		case ANDROID_CONTROL_AF_MODE:
			mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AF_MODE);
			break;
		case ANDROID_CONTROL_AF_REGIONS:
			mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AF_REGIONS);
			break;
		case ANDROID_STATISTICS_FACE_DETECT_MODE:
			HAL_LOGV("FACE DECTION");
#if defined(CONFIG_CAMERA_FACE_DETECT)
			mOEMIf->faceDectect_enable(1);
#endif
			mSetting->getSTATISTICSTag(&statisticsInfo);
			if (statisticsInfo.face_detect_mode == ANDROID_STATISTICS_FACE_DETECT_MODE_OFF) {
				mOEMIf->faceDectect(0);
			} else
				mOEMIf->faceDectect(1);
			break;
		case ANDROID_CONTROL_AE_TARGET_FPS_RANGE:
			mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
			break;

		case ANDROID_CONTROL_AE_LOCK:
			HAL_LOGD("ANDROID_CONTROL_AE_LOCK");
			mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AE_LOCK);
			break;
		default:
			HAL_LOGV("other tag");
			break;
		}
	}
	while ((tag = mSetting->popSprdParaTag()) != -1) {
		switch (tag) {
		case ANDROID_SPRD_BRIGHTNESS:
			HAL_LOGV("ANDROID_SPRD_BRIGHTNESS");
			mOEMIf->SetCameraParaTag(ANDROID_SPRD_BRIGHTNESS);
			break;
		case ANDROID_SPRD_CONTRAST:
			HAL_LOGV("contrast");
			mOEMIf->SetCameraParaTag(ANDROID_SPRD_CONTRAST);
			break;
		case ANDROID_SPRD_SATURATION:
			HAL_LOGV("ANDROID_SPRD_SATURATION");
			mOEMIf->SetCameraParaTag(ANDROID_SPRD_SATURATION);
			break;
		case ANDROID_SPRD_CAPTURE_MODE:
			HAL_LOGD("ANDROID_SPRD_CAPTURE_MODE");
			mOEMIf->SetCameraParaTag(ANDROID_SPRD_CAPTURE_MODE);
			break;
		case ANDROID_SPRD_SENSOR_ORIENTATION:
			mOEMIf->SetCameraParaTag(ANDROID_SPRD_SENSOR_ORIENTATION);
			break;
		case ANDROID_SPRD_SENSOR_ROTATION:
			mOEMIf->SetCameraParaTag(ANDROID_SPRD_SENSOR_ROTATION);
			break;
		case ANDROID_SPRD_UCAM_SKIN_LEVEL:
			HAL_LOGV("ANDROID_SPRD_UCAM_SKIN_LEVEL");
			mOEMIf->SetCameraParaTag(ANDROID_SPRD_UCAM_SKIN_LEVEL);
			break;
		case ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR:
			HAL_LOGV("ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR");
			mOEMIf->SetCameraParaTag(ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR);
			break;
		case ANDROID_SPRD_METERING_MODE:
			mOEMIf->SetCameraParaTag(ANDROID_SPRD_METERING_MODE);
			break;
		case ANDROID_SPRD_ISO:
			mOEMIf->SetCameraParaTag(ANDROID_SPRD_ISO);
			break;
#ifdef CONFIG_SPRD_PRIVATE_ZSL
		case ANDROID_SPRD_ZSL_ENABLED:
			HAL_LOGV("ANDROID_SPRD_ZSL_ENABLED");
			mOEMIf->SetCameraParaTag(ANDROID_SPRD_ZSL_ENABLED);
			break;
#endif
		default:
			HAL_LOGV("other tag");
			break;
		}
	}
	HAL_LOGV("set parameter out");
	return 0;
}

int SprdCamera3MetadataChannel::stop(uint32_t frame_number)
{
	CONTROL_Tag controlInfo;

	HAL_LOGD("E");
	mSetting->getCONTROLTag(&controlInfo);
	if((controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_START)||(controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_IDLE))
		mOEMIf->cancelAutoFocus();
	HAL_LOGD("X");
	return 0;
}

int SprdCamera3MetadataChannel::getCapRequestPara(const CameraMetadata &metadata, CapRequestPara *request_para)
{
	if (metadata.exists(ANDROID_CONTROL_CAPTURE_INTENT)) {
		request_para->cap_intent = metadata.find(ANDROID_CONTROL_CAPTURE_INTENT).data.u8[0];
	}
	else {
		CONTROL_Tag controlInfo;
		mSetting->getCONTROLTag(&controlInfo);
		request_para->cap_intent = controlInfo.capture_intent;
	}

	if (metadata.exists(ANDROID_REQUEST_ID)) {
		request_para->cap_request_id = metadata.find(ANDROID_REQUEST_ID).data.i32[0];
	}
	else {
		request_para->cap_request_id = 0;
	}

	if (metadata.exists(ANDROID_SPRD_CAPTURE_MODE)) {
		request_para->takepicture_cnt = metadata.find(ANDROID_SPRD_CAPTURE_MODE).data.u8[0];
	}
	else {
		SPRD_DEF_Tag sprddefInfo;
		mSetting->getSPRDDEFTag(&sprddefInfo);
		request_para->takepicture_cnt = sprddefInfo.capture_mode;
	}

	if (metadata.exists(ANDROID_CONTROL_SCENE_MODE)) {
		request_para->scene_mode = metadata.find(ANDROID_CONTROL_SCENE_MODE).data.u8[0];
	}
	else {
		CONTROL_Tag controlInfo;
		mSetting->getCONTROLTag(&controlInfo);
		request_para->scene_mode = controlInfo.scene_mode;
	}

#ifdef CONFIG_SPRD_PRIVATE_ZSL
	if (metadata.exists(ANDROID_SPRD_ZSL_ENABLED)) {
		request_para->sprd_zsl_enabled= metadata.find(ANDROID_SPRD_ZSL_ENABLED).data.u8[0];
	}
	else {
		request_para->sprd_zsl_enabled = 0;
	}
#endif
	return NO_ERROR;
}

/**************************SprdCamera3PicChannel  end**********************************/

}; // namespace sprdcamera
