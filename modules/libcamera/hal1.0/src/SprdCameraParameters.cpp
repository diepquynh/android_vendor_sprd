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
#define LOG_TAG "SprdCameraParameters"
#include <utils/Log.h>
#include <string.h>
#include <stdlib.h>
#include "SprdCameraParameters.h"
#include "SprdCameraHardwareConfig.h"

namespace android {

/*#define LOG_TAG "SprdCameraParameters"*/
#define LOGV ALOGD
#define LOGE ALOGE
#define LOGI ALOGI
#define LOGW ALOGW
#define LOGD ALOGD
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define SIZE_ALIGN_16(x) (((x)+15)&(~15))

static int lookup(const struct str_map *const arr, const char *name, int def);
/*Parse rectangle from string like "(100,100,200,200, weight)" . if exclude_weight is true: the weight is not write to rect array*/
static int parse_rect(int *rect, int *count, const char *str, bool exclude_weight);
static void coordinate_struct_convert(int *rect_arr,int arr_size);
static int coordinate_convert(int *rect_arr,int arr_size,int angle,int is_mirror, SprdCameraParameters::Size *preview_size,
					SprdCameraParameters::Rect *preview_rect);

const int SprdCameraParameters::kDefaultPreviewSize = 0;
const SprdCameraParameters::Size SprdCameraParameters::kPreviewSizes[] = {
	{ 640, 480 },
	{ 480, 320 },/* HVGA*/
	{ 432, 320 },/*1.35-to-1, for photos. (Rounded up from 1.3333 to 1)*/
	{ 352, 288 },/*CIF*/
	{ 320, 240 },/*QVGA*/
	{ 240, 160 },/*SQVGA*/
	{ 176, 144 },/*QCIF*/
};
const int SprdCameraParameters::kPreviewSettingCount = sizeof(kPreviewSizes)/sizeof(Size);
const int SprdCameraParameters::kFrontCameraConfigCount = ARRAY_SIZE(sprd_front_camera_hardware_config);
const int SprdCameraParameters::kBackCameraConfigCount = ARRAY_SIZE(sprd_back_camera_hardware_config);
/*Parameter keys to communicate between camera application and driver.*/
const char SprdCameraParameters::KEY_FOCUS_AREAS[] = "focus-areas";
const char SprdCameraParameters::KEY_FOCUS_MODE[] = "focus-mode";
const char SprdCameraParameters::KEY_WHITE_BALANCE[] = "whitebalance";
const char SprdCameraParameters::KEY_CAMERA_ID[] = "cameraid";
const char SprdCameraParameters::KEY_JPEG_QUALITY[] = "jpeg-quality";
const char SprdCameraParameters::KEY_JPEG_THUMBNAIL_QUALITY[] = "jpeg-thumbnail-quality";
const char SprdCameraParameters::KEY_JPEG_THUMBNAIL_SIZE[] = "jpeg-thumbnail-size";
const char SprdCameraParameters::KEY_JPEG_THUMBNAIL_WIDTH[] = "jpeg-thumbnail-width";
const char SprdCameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT[] = "jpeg-thumbnail-height";
const char SprdCameraParameters::KEY_EFFECT[] = "effect";
const char SprdCameraParameters::KEY_SCENE_MODE[] = "scene-mode";
const char SprdCameraParameters::KEY_ZOOM[] = "zoom";
const char SprdCameraParameters::KEY_BRIGHTNESS[] = "brightness";
const char SprdCameraParameters::KEY_SUPPORTED_BRIGHTNESS[] = "brightness-values";
const char SprdCameraParameters::KEY_CONTRAST[] = "contrast";
const char SprdCameraParameters::KEY_SUPPORTED_CONTRAST[] = "contrast-values";
const char SprdCameraParameters::KEY_EXPOSURE_COMPENSATION[] = "exposure-compensation";
const char SprdCameraParameters::KEY_ANTI_BINDING[] = "antibanding";
const char SprdCameraParameters::KEY_ISO[] = "iso";
const char SprdCameraParameters::KEY_SUPPORTED_ISO[] = "iso-values";
const char SprdCameraParameters::KEY_RECORDING_HINT[] = "recording-hint";
const char SprdCameraParameters::KEY_FLASH_MODE[] = "flash-mode";
const char SprdCameraParameters::KEY_SLOWMOTION[] = "slow-motion";
const char SprdCameraParameters::KEY_SUPPORTED_SLOWMOTION[] = "slow-motion-values";
const char SprdCameraParameters::KEY_SATURATION[] = "saturation";
const char SprdCameraParameters::KEY_SUPPORTED_SATURATION[] = "saturation-values";
const char SprdCameraParameters::KEY_SHARPNESS[] = "sharpness";
const char SprdCameraParameters::KEY_SUPPORTED_SHARPNESS[] = "sharpness-values";
const char SprdCameraParameters::KEY_PREVIEWFRAMERATE[] = "preview-frame-rate";
const char SprdCameraParameters::KEY_AUTO_EXPOSURE[] = "auto-exposure";
const char SprdCameraParameters::KEY_METERING_AREAS[] = "metering-areas";
const char SprdCameraParameters::KEY_PREVIEW_ENV[] = "preview-env";
const char SprdCameraParameters::KEY_SENSOR_ROTATION[] = "sensorrotation";
const char SprdCameraParameters::KEY_SENSOR_ORIENTATION[] = "sensororientation";
const char SprdCameraParameters::KEY_ZSL[] = "zsl";
const char SprdCameraParameters::KEY_CAPMODE[] = "capture-mode";
const char SprdCameraParameters::KEY_SUPPORTED_ZSL[] = "zsl-supported";
const char SprdCameraParameters::KEY_SUPPORTED_FLASH_MODE[] = "flash-mode-supported";
const char SprdCameraParameters::KEY_PERFECT_SKIN_LEVEL[] = "perfectskinlevel";

SprdCameraParameters::SprdCameraParameters():CameraParameters()
{

}

SprdCameraParameters::SprdCameraParameters(const String8 &params):CameraParameters(params)
{

}

SprdCameraParameters::~SprdCameraParameters()
{

}

void SprdCameraParameters::setDefault(ConfigType config)
{
	struct config_element *element = NULL;
	int count = 0;

	setPreviewSize(kPreviewSizes[kDefaultPreviewSize].width,
			kPreviewSizes[kDefaultPreviewSize].height);
	setPreviewFrameRate(15);
	setPreviewFormat("yuv420sp");
	setPictureFormat("jpeg");

	set("jpeg-quality", "100");/*maximum quality*/
	set("jpeg-thumbnail-width", "320");
	set("jpeg-thumbnail-height", "240");
	set("jpeg-thumbnail-quality", "80");
	set("focus-mode", "auto");

	switch (config) {
	case kFrontCameraConfig:
		element = sprd_front_camera_hardware_config;
		count = kFrontCameraConfigCount;
		break;

	case kBackCameraConfig:
		element = sprd_back_camera_hardware_config;
		count = kBackCameraConfigCount;
		break;
	}

	LOGI("setDefault: config = %d, count = %d", config, count);

	for (int i=0; i<count; i++) {
		set(element[i].key, element[i].value);
//		LOGI("SetDefault: key = %s, value = %s", element[i].key, element[i].value);
	}
}

const char *SprdCameraParameters::getDefaultValue(ConfigType config, const char * const KEY) const
{
	struct config_element *element = NULL;
	int count = 0;
	int i = 0;

	switch (config) {
	case kFrontCameraConfig:
		element = sprd_front_camera_hardware_config;
		count = kFrontCameraConfigCount;
		break;

	case kBackCameraConfig:
		element = sprd_back_camera_hardware_config;
		count = kBackCameraConfigCount;
		break;
	default:
		break;
	}


	for (i=0; i<count; i++) {
		if (0 == strcmp(element[i].key, KEY)) {
			LOGV("got Default: key = %s, value = %s", element[i].key, element[i].value);
			break;
		}
	}

	if (i >= count) {
		LOGW("can't found the default value return NULL");
		return  NULL;
	} else {
		return element[i].value;
	}

}

/*return rectangle: (x1, y1, x2, y2, weight), the values are on the screen's coordinate*/
void SprdCameraParameters::getFocusAreas(int *area, int *count)
{
	const char *p = get(KEY_FOCUS_AREAS);

	parse_rect(area, count, p, false);
}

/*return rectangle: (x1, y1, x2, y2), the values are on the sensor's coordinate*/
void SprdCameraParameters::getFocusAreas(int *area, int *count, Size *preview_size,
						Rect *preview_rect,
						int orientation, bool mirror)
{
	const char *p = get(KEY_FOCUS_AREAS);
	int focus_area[4 * kFocusZoneMax] = {0};
	int area_count = 0;

	parse_rect(&focus_area[0], &area_count, p, true);

	if (area_count > 0) {
		int ret = coordinate_convert(&focus_area[0], area_count, orientation, mirror,
					preview_size, preview_rect);
		if (ret) {
			area_count = 0;
			LOGV("error: coordinate_convert error, ignore focus \n");
		} else {
			coordinate_struct_convert(&focus_area[0], area_count * 4);
			for (int i=0; i<area_count * 4; i++) {
				area[i] = focus_area[i];
				if (focus_area[i+1] < 0) {
					area_count = 0;
					LOGV("error: focus area %d < 0, ignore focus \n", focus_area[i+1]);
				}
			}
		}
	}
	*count = area_count;
}

int SprdCameraParameters::chekFocusAreas(int max_num) const
{
	const char *p = get(KEY_FOCUS_AREAS);
	int focus_area[4 * kFocusZoneMax] = {0};
	int area_count = 0;
	int i =0;
	int left = 0,top=0,right=0,bottom=0;
	int ret = 0;
	int weight = 0;

	parse_rect(&focus_area[0], &area_count, p, false);

	if (area_count > 0) {
		if (area_count > max_num) {
			return 1;
		}
		for (i=0;i<area_count;i++) {
			left = focus_area[i*5];
			top = focus_area[i*5+1];
			right = focus_area[i*5+2];
			bottom = focus_area[i*5+3];
			weight = focus_area[i*5+4];
			if ((left != 0) && (right != 0) && (top != 0) && (bottom != 0)) {
				if ((left >= right) || (top >= bottom) || (left < -1000) || (top < -1000)
					|| (right > 1000) || (bottom > 1000) || (weight < 1) || (weight > 1000)) {
					LOGI("chekFocusAreas: left=%d,top=%d,right=%d,bottom=%d, weight=%d \n",
					left, top, right, bottom, weight);
					return 1;
				}
			}
		}
	}

	return 0;
}

/*return rectangle: (x1, y1, x2, y2), the values are on the sensor's coordinate*/
void SprdCameraParameters::getMeteringAreas(int *area, int *count, Size *preview_size,
							Rect *preview_rect,
							int orientation, bool mirror)
{
	const char *p = get(KEY_METERING_AREAS);
	int metering_area[4 * kMeteringAreasMax] = {0};
	int area_count = 0;

	LOGI("getMeteringAreas: %s", p);

	parse_rect(&metering_area[0], &area_count, p, true);

	if (area_count > 0) {
		int ret = coordinate_convert(&metering_area[0], area_count, orientation, mirror,
					preview_size, preview_rect);
		if (ret) {
			area_count = 0;
			LOGE("error: coordinate_convert error, ignore focus \n");
		} else {
			coordinate_struct_convert(&metering_area[0], area_count * 4);
			for (int i=0; i<area_count * 4; i++) {
				area[i] = metering_area[i];
				if (metering_area[i+1] < 0) {
					area_count = 0;
					LOGE("error: focus area %d < 0, ignore focus \n", metering_area[i+1]);
				}
			}
		}
	}
	*count = area_count;
}

int SprdCameraParameters::chekMeteringAreas(int max_num) const
{
	const char *p = get(KEY_METERING_AREAS);
	int metering_area[4 * kMeteringAreasMax] = {0};
	int area_count = 0;
	int i =0;
	int left = 0,top=0,right=0,bottom=0;
	int ret = 0;
	int weight = 0;

	parse_rect(&metering_area[0], &area_count, p, false);

	if (area_count > 0) {
		if (area_count > max_num) {
			return 1;
		}
		for (i=0;i<area_count;i++) {
			left = metering_area[i*5];
			top = metering_area[i*5+1];
			right  = metering_area[i*5+2];
			bottom = metering_area[i*5+3];
			weight = metering_area[i*5+4];
			if ((left != 0) && (right != 0) && (top != 0) && (bottom != 0)) {
				if ((left >= right) || (top >= bottom) || (left < -1000) || (top < -1000)
					|| (right > 1000) || (bottom > 1000) || (weight < 1) || (weight > 1000)) {
					LOGI("checkMeteringAreas: left=%d,top=%d,right=%d,bottom=%d, weight=%d \n",
					left, top, right, bottom, weight);
					return 1;
				}
			}
		}
	}

	return 0;
}

const char *SprdCameraParameters::get_FocusAreas() const
{
	return get(KEY_FOCUS_AREAS);
}

void SprdCameraParameters::setFocusAreas(const char* value)
{
	set("focus-areas",value);
}

const char *SprdCameraParameters::get_MeteringAreas() const
{
	return get(KEY_METERING_AREAS);
}

void SprdCameraParameters::setMeteringAreas(const char* value)
{
	set("metering-areas",value);
}

int SprdCameraParameters::getFocusMode()
{
	const char *p = get(KEY_FOCUS_MODE);

	return lookup(focus_mode_map, p, CAMERA_FOCUS_MODE_AUTO);
}

const char *SprdCameraParameters::get_FocusMode() const
{
	return get(KEY_FOCUS_MODE);
}

void SprdCameraParameters::setFocusMode(const char* value)
{
	set("focus-mode",value);
}

int SprdCameraParameters::getWhiteBalance()
{
	const char *p = get(KEY_WHITE_BALANCE);

	return lookup(wb_map, p, CAMERA_WB_AUTO);
}

const char *SprdCameraParameters::get_WhiteBalance() const
{
	return get(KEY_WHITE_BALANCE);

}

void SprdCameraParameters::setWhiteBalance(const char* value)
{
	set("whitebalance",value);
}

int SprdCameraParameters::getCameraId()
{
	const char *p = get(KEY_CAMERA_ID);

	return lookup(camera_id_map, p, CAMERA_CAMERA_ID_BACK);
}

const char *SprdCameraParameters::get_CameraId() const
{
	return get(KEY_CAMERA_ID);
}

void SprdCameraParameters::setCameraId(const char* value)
{
	set("cameraid",value);
}

int SprdCameraParameters::getJpegQuality()
{
	return getInt(KEY_JPEG_QUALITY);
}

const char *SprdCameraParameters::get_JpegQuality() const
{
	return get(KEY_JPEG_QUALITY);
}

void SprdCameraParameters::setJpegQuality(const char* value)
{
	set("jpeg-quality",value);
}

int SprdCameraParameters::getJpegThumbnailQuality()
{
	return getInt(KEY_JPEG_THUMBNAIL_QUALITY);
}

const char *SprdCameraParameters::get_JpegThumbnailQuality() const
{
	return get(KEY_JPEG_THUMBNAIL_QUALITY);
}

void SprdCameraParameters::setJpegThumbnailQuality(const char* value)
{
	set("jpeg-thumbnail-quality",value);
}

const char *SprdCameraParameters::get_JpegThumbnailSize() const
{
	return get(KEY_JPEG_THUMBNAIL_SIZE);
}

void SprdCameraParameters::setJpegThumbnailSize(const char* value)
{
	set(KEY_JPEG_THUMBNAIL_SIZE,value);
}

const char *SprdCameraParameters::get_JpegThumbnailSizeValue() const
{
	return get("jpeg-thumbnail-size-values");
}

void SprdCameraParameters::setJpegThumbnailSizeValue(const char* value)
{
	set("jpeg-thumbnail-size-values",value);
}

const char *SprdCameraParameters::get_JpegThumbnailWidth() const
{
	return get(KEY_JPEG_THUMBNAIL_WIDTH);
}

void SprdCameraParameters::setJpegThumbnailWidth(const char* value)
{
	set(KEY_JPEG_THUMBNAIL_WIDTH,value);
}

void SprdCameraParameters::setJpegThumbnailHeight(const char* value)
{
	set(KEY_JPEG_THUMBNAIL_HEIGHT,value);
}
const char *SprdCameraParameters::get_JpegThumbnailHeight() const
{
	return get(KEY_JPEG_THUMBNAIL_HEIGHT);
}

int SprdCameraParameters::getEffect()
{
	const char *p = get(KEY_EFFECT);

	return lookup(effect_map, p, CAMERA_EFFECT_NONE);
}

const char *SprdCameraParameters::get_Effect() const
{
	return get(KEY_EFFECT);
}

void SprdCameraParameters::setEffect(const char* value)
{
	set("effect",value);
}

int SprdCameraParameters::getSceneMode()
{
	const char *p = get(KEY_SCENE_MODE);

	return lookup(scene_mode_map, p, CAMERA_SCENE_MODE_AUTO);
}

const char *SprdCameraParameters::get_SceneMode() const
{
	return get(KEY_SCENE_MODE);

}

void SprdCameraParameters::setSceneMode(const char* value)
{
	set("scene-mode",value);
}

int SprdCameraParameters::getZoom()
{
	const char *p = get(KEY_ZOOM);

	return lookup(zoom_map, p, CAMERA_ZOOM_1X);
}

const char *SprdCameraParameters::get_Zoom() const
{
	return get(KEY_ZOOM);
}

void SprdCameraParameters::setZoom(const char* value)
{
	set("zoom",value);
}

int SprdCameraParameters::getBrightness()
{
	const char *p = get(KEY_BRIGHTNESS);

	return lookup(brightness_map, p, CAMERA_BRIGHTNESS_DEFAULT);
}

const char *SprdCameraParameters::get_Brightness() const
{
	return get(KEY_BRIGHTNESS);
}

void SprdCameraParameters::setBrightness(const char* value)
{
	set("brightness",value);
}

int SprdCameraParameters::getSharpness()
{
	const char *p = get(KEY_SHARPNESS);

	return lookup(sharpness_map, p, CAMERA_SHARPNESS_DEFAULT);
}

const char *SprdCameraParameters::get_Sharpness() const
{
	return get(KEY_SHARPNESS);
}

void SprdCameraParameters::setSharpness(const char* value)
{
	set("sharpness",value);
}

int SprdCameraParameters::getPreviewFameRate()
{
	const char *p = get(KEY_PREVIEWFRAMERATE);

	return lookup(previewframerate_map, p, CAMERA_PREVIEWFRAMERATE_DEFAULT);
}

const char *SprdCameraParameters::get_PreviewFameRate() const
{
	return get(KEY_PREVIEWFRAMERATE);
}

void SprdCameraParameters::setPreviewFameRate(const char* value)
{
	set("preview-frame-rate",value);
}

int SprdCameraParameters::getContrast()
{
	const char *p = get(KEY_CONTRAST);

	return lookup(contrast_map, p, CAMERA_CONTRAST_DEFAULT);
}

const char *SprdCameraParameters::get_Contrast() const
{
	return get(KEY_CONTRAST);
}

void SprdCameraParameters::setContrast(const char* value)
{
	set("contrast",value);
}

int SprdCameraParameters::getSaturation()
{
	const char *p = get(KEY_SATURATION);

	return lookup(saturation_map, p, CAMERA_SATURATION_DEFAULT);
}

const char *SprdCameraParameters::get_Saturation() const
{
	return get(KEY_SATURATION);
}

void SprdCameraParameters::setSaturation(const char* value)
{
	set("saturation",value);
}

int SprdCameraParameters::getExposureCompensation()
{
	const char *p = get(KEY_EXPOSURE_COMPENSATION);

	return lookup(exposure_compensation_map, p, CAMERA_EXPOSURW_COMPENSATION_DEFAULT);
}

const char *SprdCameraParameters::get_ExposureCompensation() const
{
	return get(KEY_EXPOSURE_COMPENSATION);
}

void SprdCameraParameters::setExposureCompensation(const char* value)
{
	set("exposure-compensation",value);
}

int SprdCameraParameters::getAntiBanding()
{
	const char *p = get(KEY_ANTI_BINDING);

	return lookup(antibanding_map, p, CAMERA_ANTIBANDING_50HZ);
}

const char *SprdCameraParameters::get_AntiBanding() const
{
	return get(KEY_ANTI_BINDING);
}

void SprdCameraParameters::setAntiBanding(const char* value)
{
	set("antibanding",value);
}

int SprdCameraParameters::getIso()
{
	const char *p = get(KEY_ISO);

	return lookup(iso_map, p, CAMERA_ISO_AUTO);
}

const char *SprdCameraParameters::get_Iso() const
{
	return get(KEY_ISO);
}

void SprdCameraParameters::setIso(const char* value)
{
	set("iso",value);
}

int SprdCameraParameters::getRecordingHint()
{
	const char *p = get(KEY_RECORDING_HINT);

	return lookup(camera_dcdv_mode, p, CAMERA_DC_MODE);
}

const char *SprdCameraParameters::get_RecordingHint() const
{
	return get(KEY_RECORDING_HINT);
}

void SprdCameraParameters::setRecordingHint(const char* value)
{
	set("recording-hint",value);
}

int SprdCameraParameters::getFlashMode()
{
	const char *p = get(KEY_FLASH_MODE);

	return lookup(flash_mode_map, p, CAMERA_FLASH_MODE_OFF);
}

const char *SprdCameraParameters::get_FlashMode() const
{
	return get(KEY_FLASH_MODE);
}

void SprdCameraParameters::setFlashMode(const char* value)
{
	set("flash-mode",value);
}

int SprdCameraParameters::getSlowmotion()
{
	const char *p = get(KEY_SLOWMOTION);

	return lookup(slowmotion_map, p, CAMERA_SLOWMOTION_0);
}

const char *SprdCameraParameters::get_Slowmotion() const
{
	return get(KEY_SLOWMOTION);
}

void SprdCameraParameters::setSlowmotion(const char* value)
{
	set("slow-motion",value);
}

const char *SprdCameraParameters::get_SlowmotionSupported() const
{
	return get(KEY_SUPPORTED_SLOWMOTION);
}

void SprdCameraParameters::setSlowmotionSupported(const char* value)
{
	set(KEY_SUPPORTED_SLOWMOTION,value);
}


int SprdCameraParameters::getPreviewEnv()
{
	const char *p = get(KEY_PREVIEW_ENV);

	return lookup(previewenv_map, p, CAMERA_DC_PREVIEW);
}

const char *SprdCameraParameters::get_PreviewEnv() const
{
	return get(KEY_PREVIEW_ENV);
}

void SprdCameraParameters::setPreviewEnv(const char* value)
{
	set("preview-env",value);
}

int SprdCameraParameters::getAutoExposureMode()
{
	const char *p = get(KEY_AUTO_EXPOSURE);

	return lookup(auto_exposure_mode_map, p, CAMERA_AE_FRAME_AVG);
}

const char *SprdCameraParameters::get_AutoExposureMode() const
{
	return get(KEY_AUTO_EXPOSURE);
}

void SprdCameraParameters::setAutoExposureMode(const char* value)
{
	set("auto-exposure",value);
}

const char *SprdCameraParameters::get_PreviewFpsRange() const
{
	return get(KEY_PREVIEW_FPS_RANGE);
}

void SprdCameraParameters::setPreviewFpsRange(const char* value)
{
	set(KEY_PREVIEW_FPS_RANGE,value);
}

const char *SprdCameraParameters::get_GPS_Processing_Method() const
{
	return get(KEY_GPS_PROCESSING_METHOD);
}

void SprdCameraParameters::setGPSProcessingMethod(const char* value)
{
	set(KEY_GPS_PROCESSING_METHOD, value);
}

void SprdCameraParameters::removeGPSProcessingMethod(void)
{
	remove(KEY_GPS_PROCESSING_METHOD);
}

const char *SprdCameraParameters::get_FocalLength() const
{
	return get(KEY_FOCAL_LENGTH);
}

void SprdCameraParameters::setFocalLength(const char* value)
{
	set(KEY_FOCAL_LENGTH, value);
}

const char *SprdCameraParameters::get_ExposureCompensationStep() const
{
	return get(KEY_EXPOSURE_COMPENSATION_STEP);
}

void SprdCameraParameters::setExposureCompensationStep(const char* value)
{
	set(KEY_EXPOSURE_COMPENSATION_STEP, value);
}

const char *SprdCameraParameters::get_MaxExposureCompensation() const
{
	return get(KEY_MAX_EXPOSURE_COMPENSATION);
}

void SprdCameraParameters::setMaxExposureCompensation(const char* value)
{
	set(KEY_MAX_EXPOSURE_COMPENSATION, value);
}

const char *SprdCameraParameters::get_MinExposureCompensation() const
{
	return get(KEY_MIN_EXPOSURE_COMPENSATION);
}

void SprdCameraParameters::setMinExposureCompensation(const char* value)
{
	set(KEY_MIN_EXPOSURE_COMPENSATION, value);
}

const char *SprdCameraParameters::get_SupportedSceneModes() const
{
	return get(KEY_SUPPORTED_SCENE_MODES);
}

void SprdCameraParameters::setSupportedSceneModes(const char* value)
{
	set(KEY_SUPPORTED_SCENE_MODES, value);
}

const char *SprdCameraParameters::get_SupportedPreviewSizes() const
{
	return get(KEY_SUPPORTED_PREVIEW_SIZES);
}

void SprdCameraParameters::setSupportedPreviewSizes(const char* value)
{
	set(KEY_SUPPORTED_PREVIEW_SIZES, value);
}

const char *SprdCameraParameters::get_SupportedPreviewFrameRate() const
{
	return get(KEY_SUPPORTED_PREVIEW_FRAME_RATES);
}

void SprdCameraParameters::setSupportedPreviewFrameRate(const char* value)
{
	set(KEY_SUPPORTED_PREVIEW_FRAME_RATES, value);
}

const char *SprdCameraParameters::get_SupportedPreviewFpsRange() const
{
	return get(KEY_SUPPORTED_PREVIEW_FPS_RANGE);
}

void SprdCameraParameters::setSupportedPreviewFpsRange(const char* value)
{
	set(KEY_SUPPORTED_PREVIEW_FPS_RANGE, value);
}

const char *SprdCameraParameters::get_SupportedPictureSizes() const
{
	return get(KEY_SUPPORTED_PICTURE_SIZES);
}

void SprdCameraParameters::setSupportedPictureSizes(const char* value)
{
	set(KEY_SUPPORTED_PICTURE_SIZES, value);
}

const char *SprdCameraParameters::get_SupportedFocusModes() const
{
	return get(KEY_SUPPORTED_FOCUS_MODES);
}

void SprdCameraParameters::setSupportedFocusModes(const char* value)
{
	set(KEY_SUPPORTED_FOCUS_MODES, value);
}

const char *SprdCameraParameters::get_AutoExposureLock() const
{
	return get(KEY_AUTO_EXPOSURE_LOCK);
}

void SprdCameraParameters::setAutoExposureLock(const char* value)
{
	set(KEY_AUTO_EXPOSURE_LOCK, value);
}

const char *SprdCameraParameters::get_AutoExposureLockSupported() const
{
	return get(KEY_AUTO_EXPOSURE_LOCK_SUPPORTED);
}

void SprdCameraParameters::setAutoExposureLockSupported(const char* value)
{
	set(KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, value);
}

const char *SprdCameraParameters::get_AutoWhiteBalanceLock() const
{
	return get(KEY_AUTO_WHITEBALANCE_LOCK);
}

void SprdCameraParameters::setAutoWhiteBalanceLock(const char* value)
{
	set(KEY_AUTO_WHITEBALANCE_LOCK, value);
}

const char *SprdCameraParameters::get_AutoWhiteBalanceLockSupported() const
{
	return get(KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED);
}

void SprdCameraParameters::setAutoWhiteBalanceLockSupported(const char* value)
{
	set(KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, value);
}

const char *SprdCameraParameters::get_HorizontalViewAngle() const
{
	return get(KEY_HORIZONTAL_VIEW_ANGLE);
}

void SprdCameraParameters::setHorizontalViewAngle(const char* value)
{
	set(KEY_HORIZONTAL_VIEW_ANGLE, value);
}

const char *SprdCameraParameters::get_VerticalViewAngle() const
{
	return get(KEY_VERTICAL_VIEW_ANGLE);
}

void SprdCameraParameters::setVerticalViewAngle(const char* value)
{
	set(KEY_VERTICAL_VIEW_ANGLE, value);
}

const char *SprdCameraParameters::get_VideoFrameFormat() const
{
	return get(KEY_VIDEO_FRAME_FORMAT);
}

void SprdCameraParameters::setVideoFrameFormat(const char* value)
{
	set(KEY_VIDEO_FRAME_FORMAT, value);
}

const char *SprdCameraParameters::get_SupportedVideoSizes() const
{
	return get(KEY_SUPPORTED_VIDEO_SIZES);
}

void SprdCameraParameters::setSupportedVideoSizes(const char* value)
{
	set(KEY_SUPPORTED_VIDEO_SIZES, value);
}

const char *SprdCameraParameters::get_Rotation() const
{
	return get(KEY_ROTATION);
}

void SprdCameraParameters::setRotation(const char* value)
{
	set(KEY_ROTATION, value);
}

const char *SprdCameraParameters::get_GpsLatitude() const
{
	return get(KEY_GPS_LATITUDE);
}

void SprdCameraParameters::setGpsLatitude(const char* value)
{
	set(KEY_GPS_LATITUDE, value);
}

void SprdCameraParameters::removeGpsLatitude(void)
{
	remove(KEY_GPS_LATITUDE);
}

const char *SprdCameraParameters::get_GpsLongitude() const
{
	return get(KEY_GPS_LONGITUDE);
}

void SprdCameraParameters::setGpsLongitude(const char* value)
{
	set(KEY_GPS_LONGITUDE, value);
}

void SprdCameraParameters::removeGpsLongitude(void)
{
	remove(KEY_GPS_LONGITUDE);
}

const char *SprdCameraParameters::get_GpsAltitude() const
{
	return get(KEY_GPS_ALTITUDE);
}

void SprdCameraParameters::setGpsAltitude(const char* value)
{
	set(KEY_GPS_ALTITUDE, value);
}

void SprdCameraParameters::removeGpsAltitude(void)
{
	remove(KEY_GPS_ALTITUDE);
}

const char *SprdCameraParameters::get_GpsTimestamp() const
{
	return get(KEY_GPS_TIMESTAMP);
}

void SprdCameraParameters::setGpsTimestamp(const char* value)
{
	set(KEY_GPS_TIMESTAMP, value);
}

void SprdCameraParameters::removeGpsTimestamp(void)
{
	remove(KEY_GPS_TIMESTAMP);
}

const char *SprdCameraParameters::get_MaxNumDetectedFacesHW() const
{
	return get(KEY_MAX_NUM_DETECTED_FACES_HW);
}

void SprdCameraParameters::setMaxNumDetectedFacesHW(const char* value)
{
	set(KEY_MAX_NUM_DETECTED_FACES_HW, value);
}

const char *SprdCameraParameters::get_MaxNumDetectedFacesSW() const
{
	return get(KEY_MAX_NUM_DETECTED_FACES_SW);
}

void SprdCameraParameters::setMaxNumDetectedFacesSW(const char* value)
{
	set(KEY_MAX_NUM_DETECTED_FACES_SW, value);
}

const char *SprdCameraParameters::get_MaxZoom() const
{
	return get(KEY_MAX_ZOOM);
}

void SprdCameraParameters::setMaxZoom(const char* value)
{
	set(KEY_MAX_ZOOM,value);
}

const char *SprdCameraParameters::get_ZoomRatios() const
{
	return get(KEY_ZOOM_RATIOS);
}

void SprdCameraParameters::setZoomRatios(const char* value)
{
	set(KEY_ZOOM_RATIOS,value);
}

const char *SprdCameraParameters::get_ZoomSupported() const
{
	return get(KEY_ZOOM_SUPPORTED);
}

void SprdCameraParameters::setZoomSupported(const char* value)
{
	set(KEY_ZOOM_SUPPORTED,value);
}

const char *SprdCameraParameters::get_SmoothZoomSupported() const
{
	return get(KEY_SMOOTH_ZOOM_SUPPORTED);
}

void SprdCameraParameters::setSmoothZoomSupported(const char* value)
{
	set(KEY_SMOOTH_ZOOM_SUPPORTED,value);
}

const char *SprdCameraParameters::get_SupportedFlashMode() const
{
	return get(KEY_SUPPORTED_FLASH_MODES);
}

void SprdCameraParameters::setSupportedFlashMode(const char* value)
{
	set(KEY_SUPPORTED_FLASH_MODES,value);
}

const char *SprdCameraParameters::get_SupportedWhiteBalance() const
{
	return get(KEY_SUPPORTED_WHITE_BALANCE);
}

void SprdCameraParameters::setSupportedWhiteBalance(const char* value)
{
	set(KEY_SUPPORTED_WHITE_BALANCE, value);
}

const char *SprdCameraParameters::get_Supported_Auto_Exposure_Values() const
{
	return get("auto-exposure-values");
}

void SprdCameraParameters::set_Supported_Auto_Exposure_Values(const char* values)
{
	return set("auto-exposure-values",values);

}

const char *SprdCameraParameters::get_SupportedIso() const
{
	return get(KEY_SUPPORTED_ISO);
}

void SprdCameraParameters::setSupportedIso(const char* value)
{
	set(KEY_SUPPORTED_ISO,value);
}

const char *SprdCameraParameters::get_SupportedContrast() const
{
	return get(KEY_SUPPORTED_CONTRAST);
}

void SprdCameraParameters::setSupportedContrast(const char* value)
{
	set(KEY_SUPPORTED_CONTRAST,value);
}

const char *SprdCameraParameters::get_SupportedSaturation() const
{
	return get(KEY_SUPPORTED_SATURATION);
}

void SprdCameraParameters::setSupportedSaturation(const char* value)
{
	set(KEY_SUPPORTED_SATURATION,value);
}

const char *SprdCameraParameters::get_SupportedBrightness() const
{
	return get(KEY_SUPPORTED_BRIGHTNESS);
}

void SprdCameraParameters::setSupportedBrightness(const char* value)
{
	set(KEY_SUPPORTED_BRIGHTNESS,value);
}

const char *SprdCameraParameters::get_SupportedAntibanding() const
{
	return get(KEY_SUPPORTED_ANTIBANDING);
}

void SprdCameraParameters::setSupportedAntibanding(const char* value)
{
	set(KEY_SUPPORTED_ANTIBANDING, value);
}

const char *SprdCameraParameters::get_SupportedEffects() const
{
	return get(KEY_SUPPORTED_EFFECTS);
}

void SprdCameraParameters::setSupportedEffects(const char* value)
{
	set(KEY_SUPPORTED_EFFECTS, value);
}

const char *SprdCameraParameters::get_SupportedSharpness() const
{
	return get(KEY_SUPPORTED_SHARPNESS);
}

void SprdCameraParameters::setSupportedSharpness(const char* value)
{
	set(KEY_SUPPORTED_SHARPNESS, value);
}

const char *SprdCameraParameters::get_PictureFormat() const
{
	return get(KEY_PICTURE_FORMAT);
}

void SprdCameraParameters::setPictureFormat(const char* value)
{
	set(KEY_PICTURE_FORMAT, value);
}

const char *SprdCameraParameters::get_SupportedPictureFormat() const
{
	return get(KEY_SUPPORTED_PICTURE_FORMATS);
}

void SprdCameraParameters::setSupportedPictureFormat(const char* value)
{
	set(KEY_SUPPORTED_PICTURE_FORMATS, value);
}

const char *SprdCameraParameters::get_VideoStabilition() const
{
	return get(KEY_VIDEO_STABILIZATION);
}

void SprdCameraParameters::setVideoStabilition(const char* value)
{
	set(KEY_VIDEO_STABILIZATION, value);
}

const char *SprdCameraParameters::get_VideoStabilitionSupported() const
{
	return get(KEY_VIDEO_STABILIZATION_SUPPORTED);
}

void SprdCameraParameters::setVideoStabilitionSupported(const char* value)
{
	set(KEY_VIDEO_STABILIZATION_SUPPORTED, value);
}

void SprdCameraParameters::setSensorRotation(int value)
{
	set(KEY_SENSOR_ROTATION,value);
}

void SprdCameraParameters::setSensorOrientation(int value)
{
	set(KEY_SENSOR_ORIENTATION,value);
}

const char *SprdCameraParameters::get_FocusDistances() const
{
	return get(KEY_FOCUS_DISTANCES);
}

void SprdCameraParameters::setFocusDistances(const char* value)
{
	set(KEY_FOCUS_DISTANCES, value);
}

const char *SprdCameraParameters::get_MaxNumFocusAreas() const
{
	return get(KEY_MAX_NUM_FOCUS_AREAS);
}

void SprdCameraParameters::setMaxNumFocusAreas(const char* value)
{
	set(KEY_MAX_NUM_FOCUS_AREAS, value);
}

const char *SprdCameraParameters::get_SupportedPreviewFormat() const
{
	return get(KEY_SUPPORTED_PREVIEW_FORMATS);
}

void SprdCameraParameters::setSupportedPreviewFormat(const char* value)
{
	set(KEY_SUPPORTED_PREVIEW_FORMATS, value);
}

const char *SprdCameraParameters::get_VideoSnapshotSupported() const
{
	return get(KEY_VIDEO_SNAPSHOT_SUPPORTED);
}

void SprdCameraParameters::setVideoSnapshotSupported(const char* value)
{
	set(KEY_VIDEO_SNAPSHOT_SUPPORTED, value);
}

const char *SprdCameraParameters::get_maxNumMeteringArea() const
{
	return get(KEY_MAX_NUM_METERING_AREAS);
}

void SprdCameraParameters::setMaxNumMeteringArea(const char* value)
{
	set(KEY_MAX_NUM_METERING_AREAS, value);
}

void SprdCameraParameters::setZsl(int value)
{
	set(KEY_ZSL,value);
}

void SprdCameraParameters::setCapMode(int value)
{
	set(KEY_CAPMODE,value);
}

void SprdCameraParameters::setZSLSupport(const char* value)
{
	set(KEY_SUPPORTED_ZSL,value);
	LOGI("set ZSL support %s", value);
}

void SprdCameraParameters::setFlashModeSupport(const char* value)
{
	set(KEY_SUPPORTED_FLASH_MODE,value);
}

void SprdCameraParameters::setPerfectSkinLevel(const char* value)
{
       set(KEY_PERFECT_SKIN_LEVEL,value);
}

const char* SprdCameraParameters::getPerfectSkinLevel() const
{
       return get(KEY_PERFECT_SKIN_LEVEL);
}

void SprdCameraParameters::updateSupportedPreviewSizes(int width, int height)
{
	char size_new[32] = {0};
	char vals_new[256] = {0};
	const char *p = get(KEY_PREVIEW_SIZE);
	const char *vals_p = get(KEY_SUPPORTED_PREVIEW_SIZES);
	const char *pos_1 = vals_p, *pos_2 = vals_p, *vals_temp = vals_p, *pos_dst = vals_p;
	unsigned int p_len = strlen(p);
	unsigned int cnt = 0;
	unsigned int i = 0;

	height = width*3/4;
	height = SIZE_ALIGN_16(height);
	width = SIZE_ALIGN_16(width);
	sprintf(size_new, "%dx%d", width, height);
	LOGI("updateSupportedPreviewSizes preview-size %s", size_new);
	pos_1 = strstr(vals_p,",");
	if (!pos_1) return;
	pos_2 = pos_1 + 1;
	strncpy(vals_new, vals_p, pos_1-vals_p);
	pos_dst += strlen(vals_new);

	if (strlen(size_new) > strlen(vals_new) || (strlen(size_new) == strlen(vals_new) && strcmp(size_new,vals_new))) {
		sprintf(vals_new,"%s,%s",size_new,vals_p);
	} else {
		while(pos_1) {
			memset(vals_new,0,128);
			vals_temp = pos_2;
			pos_1 = strstr(vals_temp,",");
			if (!pos_1) {
				strcpy(vals_new, vals_p);
				strcat(vals_new,",");
				strcat(vals_new,size_new);
				break;
			};
			strncpy(vals_new, vals_temp, pos_1-vals_temp);
			pos_2 = pos_1 + 1;
			if (strlen(size_new) > strlen(vals_new) || (strlen(size_new) == strlen(vals_new) && strcmp(size_new,vals_new))) {
				strncpy(vals_new, vals_p, pos_dst-vals_p);
				strcat(vals_new, ",");
				strcat(vals_new, size_new);
				strcat(vals_new, pos_dst);
				break;
			} else {
				pos_dst += strlen(vals_new) + 1;
			}
		}
	}
	LOGI("updateSupportedPreviewSizes preview-size-values %s", vals_new);
	set(KEY_SUPPORTED_PREVIEW_SIZES, vals_new);
	set(KEY_PREVIEW_SIZE, size_new);
}

/*Parse rectangle from string like "(100,100,200,200, weight)" .if exclude_weight is true: the weight is not write to rect array*/
static int parse_rect(int *rect, int *count, const char *str, bool exclude_weight)
{
	char *a = (char *)str;
	char *b = a, *c = a;
	char k[40] = {0};
	char m[40]={0};
	int *rect_arr = rect;
	unsigned int cnt = 0;
	unsigned int i=0;

	if (!a)
		return 0;

	do {
		b = strchr(a,'(');
		if (b == 0)
			goto lookuprect_done;

		a = b + 1;
		b = strchr(a,')');
		if (b == 0)
			goto lookuprect_done;

		strncpy(k, a, (b-a));
		a = b + 1;

		c = strchr(k,',');
		strncpy(m,k,(c-k));
		*rect_arr++ = strtol(m, 0, 0);/*left*/
		memset(m,0,20);

		b = c + 1;
		c = strchr(b, ',');
		strncpy(m, b, (c-b));
		*rect_arr++ = strtol(m, 0, 0);/*top*/
		memset(m, 0, 20);

		b = c + 1;
		c = strchr(b, ',');
		strncpy(m, b, (c-b));
		*rect_arr++ = strtol(m, 0, 0);/*right*/
		memset(m, 0, 20);

		b = c + 1;
		c = strchr(b, ',');
		strncpy(m, b, (c-b));
		*rect_arr++ = strtol(m, 0, 0);/*bottom*/
		memset(m, 0, 20);

		b = c + 1;
		if (!exclude_weight)
			*rect_arr++ =strtol(b, 0, 0);/*weight*/
		memset(m, 0, 20);
		memset(k, 0, 10);

		cnt++;

		if (cnt == SprdCameraParameters::kFocusZoneMax)
			break;

	}while(a);

lookuprect_done:
	*count = cnt;

	return cnt;
}

static int lookupvalue(const struct str_map *const arr, const char *name)
{
	if (name) {
		const struct str_map * trav = arr;
		while (trav->desc) {
			if (!strcmp(trav->desc, name))
				return trav->val;
			trav++;
		}
	}

	return SprdCameraParameters::kInvalidValue;
}

static int lookup(const struct str_map *const arr, const char *name, int def)
{
	int ret = lookupvalue(arr, name);

	return SprdCameraParameters::kInvalidValue == ret ? def : ret;
}

static void discard_zone_weight(int *arr, uint32_t size)
{
	uint32_t i = 0;
	int *dst_arr = &arr[4];
	int *src_arr = &arr[5];

	for (i=0;i<(size-1);i++) {
		*dst_arr++ = *src_arr++;
		*dst_arr++ = *src_arr++;
		*dst_arr++ = *src_arr++;
		*dst_arr++ = *src_arr++;
		src_arr++;
	}

	for (i=0;i<size;i++) {
		LOGI("discard_zone_weight: %d:%d,%d,%d,%d.\n",i,arr[i*4],arr[i*4+1],arr[i*4+2],arr[i*4+3]);
	}
}

static void coordinate_struct_convert(int *rect_arr,int arr_size)
{
	int i =0;
	int left = 0,top=0,right=0,bottom=0;
	int width = 0, height = 0;
	int *rect_arr_copy = rect_arr;

	for (i = 0; i < arr_size/4; i++) {
		left = rect_arr[i*4];
		top = rect_arr[i*4+1];
		right = rect_arr[i*4+2];
		bottom = rect_arr[i*4+3];
		width = (((right-left+3) >> 2)<<2);
		height =(((bottom-top+3) >> 2)<<2);
		rect_arr[i*4+2] = width;
		rect_arr[i*4+3] = height;
		LOGD("test:zone: left=%d,top=%d,right=%d,bottom=%d, w=%d, h=%d \n", left, top, right, bottom, width, height);
	}

	for (i=0;i<arr_size/4;i++) {
		LOGD("test:zone:%d,%d,%d,%d.\n",rect_arr_copy[i*4],rect_arr_copy[i*4+1],rect_arr_copy[i*4+2],rect_arr_copy[i*4+3]);
	}
}

static int coordinate_convert(int *rect_arr,int arr_size,int angle,int is_mirror, SprdCameraParameters::Size *preview_size,
					SprdCameraParameters::Rect *preview_rect)
{
	int i;
	int x1;
	int y1;
	int ret = 0;
	int new_width = preview_rect->width;
	int new_height = preview_rect->height;
	int point_x, point_y;



	LOGD("coordinate_convert: preview_rect x=%d, y=%d, width=%d, height=%d",
		preview_rect->x,preview_rect->y,preview_rect->width,preview_rect->height);
	LOGD("coordinate_convert: arr_size=%d, angle=%d, is_mirror=%d \n",
		arr_size, angle, is_mirror);


	for (i = 0; i < arr_size * 2; i++) {
		x1 = rect_arr[i * 2];
		y1 = rect_arr[i * 2 + 1];

		rect_arr[i * 2] = (1000 + x1) * new_width / 2000;
		rect_arr[i * 2 + 1] = (1000 + y1) * new_height / 2000;

		LOGD("coordinate_convert rect i=%d x=%d y=%d", i, rect_arr[i * 2], rect_arr[i * 2 + 1]);
	}

	/*move to cap image coordinate*/
	point_x = preview_rect->x;
	point_y = preview_rect->y;
	for (i = 0; i < arr_size; i++)
	{

		LOGD("coordinate_convert %d: org: %d, %d, %d, %d.\n",
			i, rect_arr[i * 4], rect_arr[i * 4 + 1], rect_arr[i * 4 + 2], rect_arr[i * 4 + 3]);

		rect_arr[i * 4] += point_x;
		rect_arr[i * 4 + 1] += point_y;
		rect_arr[i * 4 + 2] += point_x;
		rect_arr[i * 4 + 3] += point_y;

		LOGD("coordinate_convert %d: final: %d, %d, %d, %d.\n",
			i, rect_arr[i * 4], rect_arr[i * 4 + 1],rect_arr[i * 4 + 2], rect_arr[i * 4 + 3]);
	}

	return ret;
}

}//namespace android
