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

#ifndef ANDROID_HARDWARE_SPRD_CAMERA_PARAMETERS_H
#define ANDROID_HARDWARE_SPRD_CAMERA_PARAMETERS_H

#include <camera/CameraParameters.h>

namespace android {

class SprdCameraParameters : public CameraParameters {
public:
	enum ConfigType {
		kFrontCameraConfig,
		kBackCameraConfig
	};

	typedef struct {
		int width;
		int height;
	} Size;

	typedef struct {
		int x;
		int y;
		int width;
		int height;
	}Rect;

public:
    SprdCameraParameters();
    SprdCameraParameters(const String8 &params);
    ~SprdCameraParameters();

	void setDefault(ConfigType config);
	const char *getDefaultValue(ConfigType config, const char * const KEY) const;

	int chekFocusAreas(int max_num) const;
	void getFocusAreas(int *area, int *count);
	void getFocusAreas(int *area, int *count, Size *preview_size,
					 Rect *preview_rect, int orientation, bool mirror);
	const char* get_FocusAreas() const;
	void setFocusAreas(const char* value);
	int getFocusMode();
	const char* get_FocusMode() const;
	void setFocusMode(const char* value);
	int getWhiteBalance();
	const char* get_WhiteBalance() const;
	void setWhiteBalance(const char* value);
	int getCameraId();
	const char* get_CameraId() const;
	void setCameraId(const char* value);
	int getJpegQuality();
	void setJpegQuality(const char* value);
	int getJpegThumbnailQuality();
	void setJpegThumbnailQuality(const char* value);
	const char* get_Effect() const;
	int getEffect();
	void setEffect(const char* value);
	int getSceneMode();
	const char* get_SceneMode() const;
	void setSceneMode(const char* value);
	int getZoom();
	const char* get_Zoom() const;
	void setZoom(const char* value);
	int getBrightness();
	const char* get_Brightness() const;
	void setBrightness(const char* value);
	int getSharpness();
	const char* get_Sharpness() const;
	void setSharpness(const char* value);
	int getContrast();
	const char* get_Contrast() const;
	void setContrast(const char* value);
	int getSaturation();
	const char* get_Saturation() const;
	void setSaturation(const char* value);
	int getExposureCompensation();
	const char* get_ExposureCompensation() const;
	void setExposureCompensation(const char* value);
	int getAntiBanding();
	const char*  get_AntiBanding() const;
	void setAntiBanding(const char* value);
	int getIso();
	const char* get_Iso() const;
	void setIso(const char* value);
	int getRecordingHint();
	const char* get_RecordingHint() const;
	void setRecordingHint(const char* value);
	int getFlashMode();
	const char* get_FlashMode() const;
	void setFlashMode(const char* value);
	int getSlowmotion();
	const char* get_Slowmotion() const;
	void setSlowmotion(const char* value);
	const char* get_SlowmotionSupported() const;
	void setSlowmotionSupported(const char* value);
	int getPreviewEnv();
	const char* get_PreviewEnv() const;
	void setPreviewEnv(const char* value);
	int getPreviewFameRate();
	const char* get_PreviewFameRate() const;
	void setPreviewFameRate(const char* value);
	int getAutoExposureMode();
	const char* get_AutoExposureMode() const;
	void setAutoExposureMode(const char* value);
	int chekMeteringAreas(int max_num) const;
	void getMeteringAreas(int *area, int *count, Size *preview_size,
					 Rect *preview_rect, int orientation, bool mirror);
	const char* get_MeteringAreas() const;
	void setMeteringAreas(const char* value);
	const char* get_PreviewFpsRange() const;
	void setPreviewFpsRange(const char* value);
	const char* get_GPS_Processing_Method() const;
	void setGPSProcessingMethod(const char* value);
	void removeGPSProcessingMethod(void);
	const char* get_FocalLength() const;
	void setFocalLength(const char* value);
	const char* get_ExposureCompensationStep() const;
	void setExposureCompensationStep(const char* value);
	const char* get_MaxExposureCompensation() const;
	void setMaxExposureCompensation(const char* value);
	const char* get_MinExposureCompensation() const;
	void setMinExposureCompensation(const char* value);
	const char* get_SupportedSceneModes() const;
	void setSupportedSceneModes(const char* value);

	const char* get_SupportedPreviewSizes() const;
	void setSupportedPreviewSizes(const char* value);
	const char* get_SupportedPreviewFrameRate() const;
	void setSupportedPreviewFrameRate(const char* value);

	const char* get_SupportedPreviewFpsRange() const;
	void setSupportedPreviewFpsRange(const char* value);

	const char* get_SupportedPictureSizes() const;
	void setSupportedPictureSizes(const char* value);

	const char* get_SupportedFocusModes() const;
	void setSupportedFocusModes(const char* value);
	const char* get_AutoExposureLock() const;
	void setAutoExposureLock(const char* value);
	const char* get_AutoExposureLockSupported() const;
	void setAutoExposureLockSupported(const char* value);
	const char* get_AutoWhiteBalanceLock() const;
	void setAutoWhiteBalanceLock(const char* value);
	const char* get_AutoWhiteBalanceLockSupported() const;
	void setAutoWhiteBalanceLockSupported(const char* value);
	const char* get_HorizontalViewAngle() const;
	void setHorizontalViewAngle(const char* value);
	const char* get_VerticalViewAngle() const;
	void setVerticalViewAngle(const char* value);
	const char* get_VideoFrameFormat() const;
	void setVideoFrameFormat(const char* value);
	const char* get_SupportedVideoSizes() const;
	void setSupportedVideoSizes(const char* value);
	const char* get_JpegQuality() const;
	const char* get_JpegThumbnailQuality() const;
	const char* get_JpegThumbnailSize() const;
	void setJpegThumbnailSize(const char* value);
	const char* get_JpegThumbnailSizeValue() const;
	void setJpegThumbnailSizeValue(const char* value);
	const char* get_JpegThumbnailWidth() const;
	void setJpegThumbnailWidth(const char* value);
	const char* get_JpegThumbnailHeight() const;
	void setJpegThumbnailHeight(const char* value);
	const char* get_Rotation() const;
	void setRotation(const char* value);
	const char* get_GpsLatitude() const;
	void setGpsLatitude(const char* value);
	void removeGpsLatitude(void);
	const char* get_GpsLongitude() const;
	void setGpsLongitude(const char* value);
	void removeGpsLongitude(void);
	const char* get_GpsAltitude() const;
	void setGpsAltitude(const char* value);
	void removeGpsAltitude(void);
	const char* get_GpsTimestamp() const;
	void setGpsTimestamp(const char* value);
	void removeGpsTimestamp(void);
	const char* get_MaxNumDetectedFacesHW() const;
	void setMaxNumDetectedFacesHW(const char* value);
	const char* get_MaxNumDetectedFacesSW() const;
	void setMaxNumDetectedFacesSW(const char* value);
	const char* get_MaxZoom() const;
	void setMaxZoom(const char* value);
	const char* get_ZoomRatios() const;
	void setZoomRatios(const char* value);
	const char* get_ZoomSupported() const;
	void setZoomSupported(const char* value);
	const char* get_SmoothZoomSupported() const;
	void setSmoothZoomSupported(const char* value);
	const char* get_SupportedFlashMode() const;
	void setSupportedFlashMode(const char* value);
	const char* get_SupportedWhiteBalance() const;
	void setSupportedWhiteBalance(const char* value);
	const char* get_SupportedIso() const;
	const char *get_Supported_Auto_Exposure_Values()const;
	void set_Supported_Auto_Exposure_Values(const char* value);
	void setSupportedIso(const char* value);
	const char* get_SupportedContrast() const;
	void setSupportedContrast(const char* value);
	const char* get_SupportedSaturation() const;
	void setSupportedSaturation(const char* value);
	const char* get_SupportedBrightness() const;
	void setSupportedBrightness(const char* value);
	const char* get_SupportedSharpness() const;
	void setSupportedSharpness(const char* value);
	const char* get_SupportedAntibanding() const;
	void setSupportedAntibanding(const char* value);
	const char* get_SupportedEffects() const;
	void setSupportedEffects(const char* value);
	const char* get_PictureFormat() const;
	void setPictureFormat(const char* value);
	const char* get_SupportedPictureFormat() const;
	void setSupportedPictureFormat(const char* value);
	void setSensorRotation(int value);
	void setSensorOrientation(int value);
	const char* get_VideoStabilition() const;
	void setVideoStabilition(const char* value);
	const char* get_VideoStabilitionSupported() const;
	void setVideoStabilitionSupported(const char* value);
	const char* get_FocusDistances() const;
	void setFocusDistances(const char* value);
	const char* get_MaxNumFocusAreas() const;
	void setMaxNumFocusAreas(const char* value);
	const char* get_SupportedPreviewFormat() const;
	void setSupportedPreviewFormat(const char* value);
	const char* get_VideoSnapshotSupported() const;
	void setVideoSnapshotSupported(const char* value);
	const char * get_maxNumMeteringArea() const;
	void setMaxNumMeteringArea(const char* value);
	void setZsl(int value);
	void setCapMode(int value);
	void setZSLSupport(const char* value);
	void updateSupportedPreviewSizes(int width, int height);
	void setFlashModeSupport(const char* value);
	void setPerfectSkinLevel(const char* value);
	const char* getPerfectSkinLevel() const;

	// These sizes have to be a multiple of 16 in each dimension
	static const Size kPreviewSizes[];
	static const int kPreviewSettingCount;
	static const int kDefaultPreviewSize;

	static const unsigned int kFocusZoneMax = 5;
	static const unsigned int kMeteringAreasMax = 5;
	static const int kInvalidValue = 0xffffffff;
	static const int kFrontCameraConfigCount;
	static const int kBackCameraConfigCount;

	static const char KEY_FOCUS_AREAS[];
	static const char KEY_FOCUS_MODE[];
	static const char KEY_WHITE_BALANCE[];
	static const char KEY_CAMERA_ID[];
	static const char KEY_JPEG_QUALITY[];
	static const char KEY_JPEG_THUMBNAIL_QUALITY[];
	static const char KEY_JPEG_THUMBNAIL_SIZE[];
	static const char KEY_JPEG_THUMBNAIL_WIDTH[];
	static const char KEY_JPEG_THUMBNAIL_HEIGHT[];
	static const char KEY_EFFECT[];
	static const char KEY_SCENE_MODE[];
	static const char KEY_ZOOM[];
	static const char KEY_BRIGHTNESS[];
	static const char KEY_SUPPORTED_BRIGHTNESS[];
	static const char KEY_CONTRAST[];
	static const char KEY_SUPPORTED_CONTRAST[];
	static const char KEY_EXPOSURE_COMPENSATION[];
	static const char KEY_ANTI_BINDING[];
	static const char KEY_ISO[];
	static const char KEY_SUPPORTED_ISO[];
	static const char KEY_RECORDING_HINT[];
	static const char KEY_FLASH_MODE[];
	static const char KEY_SLOWMOTION[];
	static const char KEY_SUPPORTED_SLOWMOTION[];
	static const char KEY_SATURATION[];
	static const char KEY_SUPPORTED_SATURATION[];
	static const char KEY_SHARPNESS[];
	static const char KEY_SUPPORTED_SHARPNESS[];
	static const char KEY_PREVIEWFRAMERATE[];
	static const char KEY_AUTO_EXPOSURE[];
	static const char KEY_METERING_AREAS[];
	static const char KEY_PREVIEW_ENV[];
	static const char KEY_SENSOR_ROTATION[];
	static const char KEY_SENSOR_ORIENTATION[];
	static const char KEY_ZSL[];
	static const char KEY_CAPMODE[];
	static const char KEY_SUPPORTED_ZSL[];
	static const char KEY_SUPPORTED_FLASH_MODE[];
	static const char KEY_PERFECT_SKIN_LEVEL[];

private:

};

}//namespace android

#endif
