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
#ifndef SPRD_CAMERA_HAL2_SPRDCAMERA2_H
#define SPRD_CAMERA_HAL2_SPRDCAMERA2_H


namespace android {

#ifdef __cplusplus
	extern "C"
	{
#endif

#include "SprdOEMCamera.h"
int androidAfModeToDrvAfMode(camera_metadata_enum_android_control_af_mode_t androidAfMode, int8_t *convertDrvMode);
int androidFlashModeToDrvFlashMode(camera_metadata_enum_android_flash_mode_t androidFlashMode, int8_t *convertDrvMode);
int androidAeModeToDrvAeMode(camera_metadata_enum_android_control_ae_mode_t androidAeMode, int8_t *convertDrvMode);
int androidAwbModeToDrvAwbMode(camera_metadata_enum_android_control_awb_mode_t androidAwbMode, int8_t *convertDrvMode);

int androidSceneModeToDrvMode(camera_metadata_enum_android_control_scene_mode_t androidScreneMode, int8_t *convertDrvMode);

#ifdef __cplusplus
}
#endif


#define CAMERA2_MAX_FACES 10

typedef enum cam_hal_metadata_section {
    ANDROID_ADD_PARAMETERS,
    ANDROID_VENDOR_SECTION_COUNT
} cam_hal_metadata_section_t;

typedef struct tags_info {
    const char *tag_name;
    uint8_t     tag_type;
} tags_info_t;


const int32_t jpegResolutionSensorBack[] = {/*must order from bigger to smaller*/
    2592, 1944,
    2048, 1536,
    1920, 1088,
    1600, 1200,
    1280,  960,
    1280,  720,
     720,  480,
     640,  480,
};

const int32_t jpegResolutionSensorFront[] = {
     720,  480,
     640,  480,
     320,  240,
};

const int32_t PreviewResolutionSensorBack[] = {/*supported video size must be same as supported preview size*/
#ifndef CONFIG_CAMERA_SMALL_PREVSIZE
    1920,  1088,
    1280,  720, // 16:9
#endif
     720,  480, // 3:2
     640,  480, // 4:3
     352,  288, // 11:9
     320,  240, // 4:3
     176,  144  // 6:5
};

const int32_t PreviewResolutionSensorFront[] = {
     960,  540,
     720,  540,
     720,  480, // 3:2
     640,  480, // 4:3
     352,  288, // 11:9
     320,  240, // 4:3
     176,  144, // 6:5
};

const uint8_t availableAfModesSensorBack[] = {
    ANDROID_CONTROL_AF_MODE_OFF,
    ANDROID_CONTROL_AF_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_MACRO,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO
};

const uint8_t sceneModeOverridesSensorBack[] = {
    // ANDROID_CONTROL_SCENE_MODE_ACTION
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AWB_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE,
    // ANDROID_CONTROL_SCENE_MODE_NIGHT
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AWB_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE,
    // ANDROID_CONTROL_SCENE_MODE_SUNSET
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AWB_MODE_DAYLIGHT,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE,
    // ANDROID_CONTROL_SCENE_MODE_PARTY
    ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH,
    ANDROID_CONTROL_AWB_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE,
    // ANDROID_CONTROL_SCENE_MODE_HDR
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AWB_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE
};

const uint8_t availableAeModesSensorBack[] = {
    ANDROID_CONTROL_AE_MODE_OFF,
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH
};

const uint32_t kAvailableFormats[4] = {//for preview
        HAL_PIXEL_FORMAT_RAW16,
        HAL_PIXEL_FORMAT_BLOB,
        HAL_PIXEL_FORMAT_YV12,
        HAL_PIXEL_FORMAT_YCrCb_420_SP
};

const uint8_t availableSceneModes[] = {
            ANDROID_CONTROL_SCENE_MODE_ACTION,
            ANDROID_CONTROL_SCENE_MODE_NIGHT,
            ANDROID_CONTROL_SCENE_MODE_SUNSET,
            ANDROID_CONTROL_SCENE_MODE_PARTY,
            ANDROID_CONTROL_SCENE_MODE_HDR
};

const uint32_t kAvailableSensitivities[5] =
    {100, 200, 400, 800, 1600};

const int64_t kFrameDurationRange[2] =
    {33331760L, 30000000000L}; // ~1/30 s - 30 sec

const int64_t kExposureTimeRange[2] =
    {1000L, 30000000000L} ; // 1 us - 30 sec

const uint64_t kAvailableRawMinDurations[1] = {
    (uint64_t)kFrameDurationRange[0]
};

const int64_t kAvailableProcessedMinDurations[1] = {
    (int64_t)kFrameDurationRange[0]
};
const uint64_t kAvailableJpegMinDurations[1] = {
    (uint64_t)kFrameDurationRange[0]
};

const char *cam_hal_metadata_section_names[ANDROID_VENDOR_SECTION_COUNT] = {
    "com.addParameters",
};

static tags_info_t android_add_parameters[VENDOR_SECTION_END - VENDOR_SECTION_START] = {
    { "brightness",                          TYPE_BYTE   },
    { "contrast",                          TYPE_BYTE   },
    { "saturation",                          TYPE_BYTE   },
    { "capMode",                        TYPE_BYTE   },
    { "sensororientation",		       TYPE_BYTE },
	{ "sensorrotation",		           TYPE_INT32 },

    { "perfectskinlevel",                   TYPE_INT32 },

    { "burstcap-cnt",                        TYPE_BYTE   },
    { "record-buf",                        TYPE_BYTE   },
    { "iso",                                        TYPE_BYTE   },
    { "slowMotion",                               TYPE_BYTE   },
    { "meteringMode",                          TYPE_BYTE },
    { "meteringArea",                            TYPE_INT32 },
    { "videoSnapshotSupport",              TYPE_BYTE },
    { "bigPrevRecSizeSupport",              TYPE_BYTE },
    { "availableBrightness",                   TYPE_BYTE },
    { "availableContrast",                       TYPE_BYTE },
    { "availableSaturation",                    TYPE_BYTE },
    { "availableCapMode",                    TYPE_BYTE },
    { "availableISO",                                 TYPE_BYTE },
    { "availableSlowMotion",                   TYPE_BYTE },
    { "availableMeteringMode",             TYPE_BYTE },
    { "flash-mode-supported",             TYPE_BYTE },
    { "prv-rec-difsize-supported",         TYPE_BYTE },
    { "notify-flag-rec -sync",         TYPE_BYTE },
    { "sprdZslEnabled",         TYPE_BYTE },
    { "mirror",                TYPE_BYTE },
    { "availableSmileEnable",                TYPE_BYTE },
};

tags_info_t *cam_tag_info[ANDROID_VENDOR_SECTION_COUNT] = {
    android_add_parameters,
};

int cam_hal_metadata_section_bounds[ANDROID_VENDOR_SECTION_COUNT][2] = {
    { VENDOR_SECTION_START, VENDOR_SECTION_END },
};

}

#endif

