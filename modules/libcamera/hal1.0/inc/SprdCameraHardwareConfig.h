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
#ifndef _SPRD_CAMERA_HARDWARE_CONFIG_H_
#define _SPRD_CAMERA_HARDWARE_CONFIG_H_
#include "cmr_common.h"


 /* Effect type, used for CAMERA_PARM_EFFECT */
enum {
	CAMERA_EFFECT_NONE = 0,
	CAMERA_EFFECT_MONO,
	CAMERA_EFFECT_RED,
	CAMERA_EFFECT_GREEN,
	CAMERA_EFFECT_BLUE,
	CAMERA_EFFECT_YELLOW,
	CAMERA_EFFECT_NEGATIVE,
	CAMERA_EFFECT_SEPIA,
	CAMERA_EFFECT_MAX
};

enum {
	CAMERA_CAMERA_ID_BACK = 0,
	CAMERA_CAMERA_ID_FRONT,
	CAMERA_CAMERA_ID_MAX
};

enum {
	CAMERA_ANTIBANDING_50HZ,
	CAMERA_ANTIBANDING_60HZ,
	CAMERA_ANTIBANDING_OFF,
	CAMERA_ANTIBANDING_AUTO,
	CAMERA_MAX_ANTIBANDING,
};

enum {
	CAMERA_DC_MODE = 0,
	CAMERA_DV_MODE,
	CAMERA_DCDV_MODE_MAX
};

enum {
	CAMERA_ZOOM_1X = 0,
	CAMERA_ZOOM_2X,
	CAMERA_ZOOM_3X,
	CAMERA_ZOOM_4X,
	CAMERA_ZOOM_5X,
	CAMERA_ZOOM_6X,
	CAMERA_ZOOM_7X,
	CAMERA_ZOOM_8X,
	CAMERA_ZOOM_9X,
	CAMERA_ZOOM_10X,
	CAMERA_ZOOM_11X,
	CAMERA_ZOOM_12X,
	CAMERA_ZOOM_13X,
	CAMERA_ZOOM_14X,
	CAMERA_ZOOM_15X,
	CAMERA_ZOOM_16X,
	CAMERA_ZOOM_MAX
};

enum {
	CAMERA_BRIGHTNESS_0 = 0,
	CAMERA_BRIGHTNESS_1 = 1,
	CAMERA_BRIGHTNESS_2 = 2,
	CAMERA_BRIGHTNESS_DEFAULT = 3,
	CAMERA_BRIGHTNESS_3 = 3,
	CAMERA_BRIGHTNESS_4 = 4,
	CAMERA_BRIGHTNESS_5 = 5,
	CAMERA_BRIGHTNESS_6 = 6,
	CAMERA_BRIGHTNESS_MAX
};

enum {
	CAMERA_SHARPNESS_0 = 0,
	CAMERA_SHARPNESS_1 = 1,
	CAMERA_SHARPNESS_2 = 2,
	CAMERA_SHARPNESS_DEFAULT = 3,
	CAMERA_SHARPNESS_3 = 3,
	CAMERA_SHARPNESS_4 = 4,
	CAMERA_SHARPNESS_5 = 5,
	CAMERA_SHARPNESS_6 = 6,
	CAMERA_SHARPNESS_MAX
};

enum {
	CAMERA_PREVIEWFRAMERATE_0 = 10,
	CAMERA_PREVIEWFRAMERATE_1 = 15,
	CAMERA_PREVIEWFRAMERATE_2 = 20,
	CAMERA_PREVIEWFRAMERATE_3 = 25,
	CAMERA_PREVIEWFRAMERATE_DEFAULT = 30,
	CAMERA_PREVIEWFRAMERATE_4 = 30,
	CAMERA_PREVIEWFRAMERATE_5 = 31,
	CAMERA_PREVIEWFRAMERATE_6 = 60,
	CAMERA_PREVIEWFRAMERATE_7 = 70,
	CAMERA_PREVIEWFRAMERATE_8 = 80,
	CAMERA_PREVIEWFRAMERATE_9 = 90,
	CAMERA_PREVIEWFRAMERATE_MAX
};

enum {
	CAMERA_CONTRAST_0 = 0,
	CAMERA_CONTRAST_1 = 1,
	CAMERA_CONTRAST_2 = 2,
	CAMERA_CONTRAST_DEFAULT = 3,
	CAMERA_CONTRAST_3 = 3,
	CAMERA_CONTRAST_4 = 4,
	CAMERA_CONTRAST_5 = 5,
	CAMERA_CONTRAST_6 = 6,
	CAMERA_CONTRAST_MAX
};

enum {
	CAMERA_SATURATION_0 = 0,
	CAMERA_SATURATION_1 = 1,
	CAMERA_SATURATION_2 = 2,
	CAMERA_SATURATION_DEFAULT = 3,
	CAMERA_SATURATION_3 = 3,
	CAMERA_SATURATION_4 = 4,
	CAMERA_SATURATION_5 = 5,
	CAMERA_SATURATION_6 = 6,
	CAMERA_SATURATION_MAX
};

/* Enum Type for different ISO Mode supported */
enum {
	CAMERA_ISO_AUTO = 0,
	CAMERA_ISO_100,
	CAMERA_ISO_200,
	CAMERA_ISO_400,
	CAMERA_ISO_800,
	CAMERA_ISO_1600,
	CAMERA_ISO_MAX
};

enum {
	CAMERA_EXPOSURW_COMPENSATION_0 = 0,
	CAMERA_EXPOSURW_COMPENSATION_1 = 1,
	CAMERA_EXPOSURW_COMPENSATION_2 = 2,
	CAMERA_EXPOSURW_COMPENSATION_3 = 3,
	CAMERA_EXPOSURW_COMPENSATION_DEFAULT = 4,
	CAMERA_EXPOSURW_COMPENSATION_4 = 4,
	CAMERA_EXPOSURW_COMPENSATION_5 = 5,
	CAMERA_EXPOSURW_COMPENSATION_6 = 6,
	CAMERA_EXPOSURW_COMPENSATION_7 = 7,
	CAMERA_EXPOSURW_COMPENSATION_8 = 8,
	CAMERA_EXPOSURW_COMPENSATION_MAX
};

#if 0
enum {
	CAMERA_FOCUS_MODE_AUTO = 0,
	CAMERA_FOCUS_MODE_AUTO_MULTI = 1,
	CAMERA_FOCUS_MODE_MACRO = 2,
	CAMERA_FOCUS_MODE_INFINITY = 3,
	CAMERA_FOCUS_MODE_CAF =4,
	CAMERA_FOCUS_MODE_MAX
};

enum {
	CAMERA_FLASH_MODE_OFF = 0,
	CAMERA_FLASH_MODE_ON = 1,
	CAMERA_FLASH_MODE_TORCH = 2,
	CAMERA_FLASH_MODE_AUTO = 3,
	CAMERA_FLASH_MODE_MAX
};
#endif


enum {
	CAMERA_RECORDING_FALSE = 0,
	CAMERA_RECORDING_TRUE = 1,
	CAMERA_RECORDING_MAX
};

enum {
	CAMERA_SLOWMOTION_0 = 0,
	CAMERA_SLOWMOTION_1,
	CAMERA_SLOWMOTION_2,
	CAMERA_SLOWMOTION_3,
	CAMERA_SLOWMOTION_MAX
};

enum {
	CAMERA_DC_PREVIEW = 0,
	CAMERA_DV_PREVIEW,
	CAMERA_PREVIEW_ENV_MAX
};

struct str_map {
	const char *const desc;
	int val;
};

const struct str_map wb_map[] = {
	{"auto",            CAMERA_WB_AUTO},
	{"incandescent",    CAMERA_WB_INCANDESCENT},
	{"fluorescent",     CAMERA_WB_FLUORESCENT},
	{"daylight",        CAMERA_WB_DAYLIGHT},
	{"cloudy-daylight", CAMERA_WB_CLOUDY_DAYLIGHT},
	{NULL, 0 }
};

const struct str_map effect_map[] = {
	{"none",            CAMERA_EFFECT_NONE},
	{"mono",            CAMERA_EFFECT_MONO},
	{"red",             CAMERA_EFFECT_RED},
	{"green",           CAMERA_EFFECT_GREEN},
	{"blue",            CAMERA_EFFECT_BLUE},
	{"antique",         CAMERA_EFFECT_YELLOW},
	{"negative",        CAMERA_EFFECT_NEGATIVE},
	{"sepia",           CAMERA_EFFECT_SEPIA},
	{"cold",            CAMERA_EFFECT_BLUE},
	{NULL,              0}
};

const struct str_map scene_mode_map[] = {
	{"auto",            CAMERA_SCENE_MODE_AUTO},
	{"night",           CAMERA_SCENE_MODE_NIGHT},
	{"portrait",        CAMERA_SCENE_MODE_PORTRAIT},
	{"landscape",       CAMERA_SCENE_MODE_LANDSCAPE},
	{"action",          CAMERA_SCENE_MODE_ACTION},
	{"normal",          CAMERA_SCENE_MODE_NORMAL},
	{"hdr",             CAMERA_SCENE_MODE_HDR},
	{NULL,              0}
};

const struct str_map camera_id_map[] = {
	{"back_camera",     CAMERA_CAMERA_ID_BACK},
	{"front_camera",    CAMERA_CAMERA_ID_FRONT},
	{NULL,              0}
};

const struct str_map camera_dcdv_mode[] = {
	{"true",            CAMERA_DV_MODE},
	{"false",           CAMERA_DC_MODE},
	{NULL,              0}
};


const struct str_map zoom_map[] = {
	{"0",               CAMERA_ZOOM_1X},
	{"1",               CAMERA_ZOOM_2X},
	{"2",               CAMERA_ZOOM_3X},
	{"3",               CAMERA_ZOOM_4X},
	{"4",               CAMERA_ZOOM_5X},
	{"5",               CAMERA_ZOOM_6X},
	{"6",               CAMERA_ZOOM_7X},
	{"7",               CAMERA_ZOOM_8X},
	{"8",               CAMERA_ZOOM_9X},
	{"9",               CAMERA_ZOOM_10X},
	{"10",              CAMERA_ZOOM_11X},
	{"11",              CAMERA_ZOOM_12X},
	{"12",              CAMERA_ZOOM_13X},
	{"13",              CAMERA_ZOOM_14X},
	{"14",              CAMERA_ZOOM_15X},
	{"15",              CAMERA_ZOOM_16X},
	{"16",              CAMERA_ZOOM_MAX},
	{NULL,              0}
};

const struct str_map brightness_map[] = {
	{"0",               CAMERA_BRIGHTNESS_0},
	{"1",               CAMERA_BRIGHTNESS_1},
	{"2",               CAMERA_BRIGHTNESS_2},
	{"3",               CAMERA_BRIGHTNESS_3},
	{"4",               CAMERA_BRIGHTNESS_4},
	{"5",               CAMERA_BRIGHTNESS_5},
	{"6",               CAMERA_BRIGHTNESS_6},
	{NULL,              0}
};

const struct str_map sharpness_map[] = {
	{"0",               CAMERA_SHARPNESS_0},
	{"1",               CAMERA_SHARPNESS_1},
	{"2",               CAMERA_SHARPNESS_2},
	{"3",               CAMERA_SHARPNESS_3},
	{"4",               CAMERA_SHARPNESS_4},
	{"5",               CAMERA_SHARPNESS_5},
	{"6",               CAMERA_SHARPNESS_6},
	{NULL,              0}
};

const struct str_map previewframerate_map[] = {
	{"10",              CAMERA_PREVIEWFRAMERATE_0},
	{"15",              CAMERA_PREVIEWFRAMERATE_1},
	{"20",              CAMERA_PREVIEWFRAMERATE_2},
	{"25",              CAMERA_PREVIEWFRAMERATE_3},
	{"30",              CAMERA_PREVIEWFRAMERATE_4},
	{"31",              CAMERA_PREVIEWFRAMERATE_5},
	{"60",              CAMERA_PREVIEWFRAMERATE_6},
	{"70",              CAMERA_PREVIEWFRAMERATE_7},
	{"80",              CAMERA_PREVIEWFRAMERATE_8},
	{"90",              CAMERA_PREVIEWFRAMERATE_9},
	{NULL,              0}
};

const struct str_map iso_map[] = {
	{"auto",            CAMERA_ISO_AUTO},
	{"100",             CAMERA_ISO_100},
	{"200",             CAMERA_ISO_200},
	{"400",             CAMERA_ISO_400},
	{"800",             CAMERA_ISO_800},
	{"1600",            CAMERA_ISO_1600},
	{NULL,              0}
};

const struct str_map contrast_map[] = {
	{"0",               CAMERA_CONTRAST_0},
	{"1",               CAMERA_CONTRAST_1},
	{"2",               CAMERA_CONTRAST_2},
	{"3",               CAMERA_CONTRAST_3},
	{"4",               CAMERA_CONTRAST_4},
	{"5",               CAMERA_CONTRAST_5},
	{"6",               CAMERA_CONTRAST_6},
	{NULL,              0}
};

const struct str_map saturation_map[] = {
	{"0",               CAMERA_SATURATION_0},
	{"1",               CAMERA_SATURATION_1},
	{"2",               CAMERA_SATURATION_2},
	{"3",               CAMERA_SATURATION_3},
	{"4",               CAMERA_SATURATION_4},
	{"5",               CAMERA_SATURATION_5},
	{"6",               CAMERA_SATURATION_6},
	{NULL,              0}
};

const struct str_map exposure_compensation_map[] = {
	{"-4",              CAMERA_EXPOSURW_COMPENSATION_0},
	{"-3",              CAMERA_EXPOSURW_COMPENSATION_1},
	{"-2",              CAMERA_EXPOSURW_COMPENSATION_2},
	{"-1",              CAMERA_EXPOSURW_COMPENSATION_3},
	{"0",               CAMERA_EXPOSURW_COMPENSATION_4},
	{"1",               CAMERA_EXPOSURW_COMPENSATION_5},
	{"2",               CAMERA_EXPOSURW_COMPENSATION_6},
	{"3",               CAMERA_EXPOSURW_COMPENSATION_7},
	{"4",               CAMERA_EXPOSURW_COMPENSATION_8},
	{NULL,              0}
};

const struct str_map antibanding_map[] = {
	{"50hz",            CAMERA_ANTIBANDING_50HZ},
	{"60hz",            CAMERA_ANTIBANDING_60HZ},
	{"off",             CAMERA_ANTIBANDING_OFF},
	{"auto",            CAMERA_ANTIBANDING_AUTO},
	{NULL,              0}
};

const struct str_map focus_mode_map[] = {
	{"auto",               CAMERA_FOCUS_MODE_AUTO},
	{"auto-multi",         CAMERA_FOCUS_MODE_AUTO_MULTI},
	{"macro",              CAMERA_FOCUS_MODE_MACRO},
	{"infinity",           CAMERA_FOCUS_MODE_INFINITY},
	{"continuous-picture", CAMERA_FOCUS_MODE_CAF},
	{"continuous-video",   CAMERA_FOCUS_MODE_CAF_VIDEO},
	{NULL,                 0}
};

const struct str_map flash_mode_map[] = {
	{"on",              CAMERA_FLASH_MODE_ON},
	{"off",             CAMERA_FLASH_MODE_OFF},
	{"torch",           CAMERA_FLASH_MODE_TORCH},
	{"auto",            CAMERA_FLASH_MODE_AUTO},
	{NULL,              0}
};

const struct str_map recording_hint_map[] = {
	{"false",           CAMERA_RECORDING_FALSE},
	{"true",            CAMERA_RECORDING_TRUE},
	{NULL,              0}
};

const struct str_map slowmotion_map[] = {
	{"0",               CAMERA_SLOWMOTION_0},
	{"1",               CAMERA_SLOWMOTION_1},
	{"2",               CAMERA_SLOWMOTION_2},
	{"3",               CAMERA_SLOWMOTION_3},
	{NULL,              0}
};

const struct str_map previewenv_map[] = {
	{"0",               CAMERA_DC_PREVIEW},
	{"1",               CAMERA_DV_PREVIEW},
	{NULL,              0}
};

const struct str_map auto_exposure_mode_map[] = {
	{"frame-average",   CAMERA_AE_FRAME_AVG},
	{"center-weighted", CAMERA_AE_CENTER_WEIGHTED},
	{"spot-metering",   CAMERA_AE_SPOT_METERING},
	{NULL,              0}
};


struct config_element{
	const char *const key;
	const char *const value;
};

struct config_element sprd_front_camera_hardware_config[] = {
	{"whitebalance-values", "auto,incandescent,fluorescent,daylight,cloudy-daylight"},
	{"whitebalance", "auto"},
#if defined(CONFIG_FRONT_CAMERA_SUPPORT_5M)
	{"picture-size-values", "2880x1920,2576x1932,2048x1536,1600x1200,1280x960,640x480"},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_3M)
	{"picture-size-values", "2048x1536,1600x1200,1280x960,640x480"},
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_2M)
	{"picture-size-values", "1600x1200,1280x960,640x480"},
#else
	{"picture-size-values", "640x480,320x240"},
#endif
	{"picture-size", "640x480"},
	{"preview-size-values",	"720x480,640x480,352x288,320x240,176x144"},
	{"preview-size", "720x480"},
	{"video-size-values",  ""},
	{"video-size", "720x480"},
	{"video-picture-size-values", "1280x960,1280x960,1280x960"},
	{"preferred-preview-size-for-video", ""},
	{"video-frame-format-values", "yuv420sp,yuv420p"},
	{"video-frame-format", "yuv420sp"},
	{"preview-format-values", "yuv420sp,yuv420p"},
	{"preview-format", "yuv420sp"},
	{"picture-format-values", "jpeg"},
	{"picture-format", "jpeg"},
	{"jpeg-quality", "100"},
	{"preview-frame-rate-values", "5,10,12,15,20,25,30"},
	{"preview-frame-rate", "25"},
	{"preview-fps-range-values", "(1000,30000)"},
	{"preview-fps-range", "1000,30000"},
	{"jpeg-thumbnail-size-values", "320x240,0x0"},
	{"jpeg-thumbnail-width","320"},
	{"jpeg-thumbnail-height", "240"},
	{"jpeg-thumbnail-quality", "80"},
	{"effect-values", "none,mono,negative,sepia,cold,antique"},
	{"effect", "none"},
	{"scene-mode-values", "auto,night"},
	{"scene-mode", "auto"},
	{"cameraid-values",
	"back_camera,front_camera"},
	{"cameraid", "front_camera"},
	{"zoom-supported", "true"},
	{"smooth-zoom-supported", "false"},
	{"max-zoom", "7"},
	{"zoom-ratios", "100,120,140,170,200,230,260,300"},
	//{"zoom", "100"},
	{"zoom", "0"},
	{"brightness-supported", "true"},
	{"smooth-brightness-supported", "false"},
	{"max-brightness", "6"},
	{"brightness-values", "0,1,2,3,4,5,6"},
	{"brightness", "3"},
	{"contrast-supported", "true"},
	{"smooth-contrast-supported", "false"},
	{"max-contrast", "6"},
	{"contrast-values", "0,1,2,3,4,5,6"},
	{"contrast", "3"},

	{"saturation-supported", "true"},
	{"saturation-values", "0,1,2,3,4,5,6"},
	{"saturation", "3"},
#if 0
	{"sharpness-supported", "true"},
	{"max-sharpness", "6"},
	{"sharpness-values", "0,1,2,3,4,5,6"},
	{"sharpness", "3"},
#endif
	{"min-exposure-compensation", "0"},
	{"max-exposure-compensation", "0"},
	{"exposure-compensation","0"},
	{"exposure-compensation-step", "0"},
	{"focal-length", "3.75"},
	{"horizontal-view-angle", "54"},
	{"vertical-view-angle", "54"},
#ifndef CONFIG_CAMERA_FLASH_NOT_SUPPORT
	{"flash-mode-values", "off"},
	{"flash-mode", "off"},
#endif
	{"flash-mode-supported", "false"},
	//CTS will test front camera focus mode,but it don't support,so delete it.
	/** SPRD: add { */
	{"focus-mode-values", "fixed"},
	{"focus-mode", "fixed"},
	{"focus-distances", "2.0,2.5,Infinity"},
	/** SPRD: add { */
#if defined(CONFIG_CAMERA_FACE_DETECT)
        {"max-num-detected-faces-hw", "10"},
#else
        {"max-num-detected-faces-hw", "0"},
#endif
	{"iso-supported", "false"},
	{"max-iso", "1"},
	{"iso-values", "auto"},
	{"iso", "auto"},
	{"smile-snap-mode","0"},
	{"hdr-supported","false"},
	{"hdr","0"},
#if	defined(CONFIG_CAMERA_ZSL_CAPTURE)
	{"zsl-supported","true"},
#else
	{"zsl-supported","false"},
#endif
#ifdef CONFIG_CAMERA_FORCE_ZSL_CAPTURE
	{"zsl","1"},
#else
	{"zsl","0"},
#endif
	{"capture-mode", "1"},
	{"slow-motion-supported","false"},
	{"max-slow-motion","3"},
	{"slow-motion-values", "1"},
	{"slow-motion", "1"},
	{"max-num-metering-areas", "0"},
#ifndef CONFIG_EXPOSURE_METERING_NOT_SUPPORT
	{"auto-exposure","center-weighted"},
	{"auto-exposure-values", "frame-average,center-weighted,spot-metering"},
#else
	{"auto-exposure","0"},
	{"auto-exposure-supported", "0"},
#endif
//front	camera add antibanding mode include auto,50HZ\60HZ modes.
/** SPRD: add { */
    {"preview-env","0"},
	{"antibanding-values","auto,50hz,60hz"},
	{"antibanding","auto"},
	{"antibanding-supported","true"},
	{"auto-exposure-lock-supported", "true"}
/** SPRD: add } */
};
struct config_element sprd_back_camera_hardware_config[] = {
	{"whitebalance-values", "auto,incandescent,fluorescent,daylight,cloudy-daylight"},
	{"whitebalance", "auto"},
#if defined(CONFIG_CAMERA_SUPPORT_13M)
	{"picture-size-values", "4208x3120,4000x3000,3264x2448,2592x1936,2048x1536,1600x1200,1280x960,640x480"},
	{"video-picture-size-values", "2592x1944,2592x1944,2592x1944,2592x1944,2592x1944"},
#elif defined(CONFIG_CAMERA_SUPPORT_8M)
	{"picture-size-values", "3264x2448,3264x1836,2448x2448,2592x1936,2048x1536,2048x1152,1600x1200,1280x960,640x480"},
	{"video-picture-size-values", "3264x2448,3264x2448,3264x2448,3264x2448,3264x2448"},
#elif defined(CONFIG_CAMERA_SUPPORT_5M)
	{"picture-size-values", "2592x1936,2048x1536,1600x1200,1280x960,640x480"},
	{"video-picture-size-values", "2592x1936,2592x1936,2592x1936,2592x1936,2592x1936"},
#elif defined(CONFIG_CAMERA_SUPPORT_3M)
	{"picture-size-values", "2048x1536,1600x1200,1280x960,640x480"},
	{"video-picture-size-values", "1600x1200,1600x1200,1600x1200,1600x1200,1600x1200"},
#elif defined(CONFIG_CAMERA_SUPPORT_2M)
	{"picture-size-values", "1920x1088,1600x1200,1280x960,640x480"},
	{"video-picture-size-values", "1280x960,1280x960,1280x960,1280x960,1280x960"},
#else
	{"picture-size-values", "3264x2448,2592x1936,2048x1536,1600x1200,1280x960,640x480"},
	{"video-picture-size-values", "1280x960,1280x960,1280x960,1280x960,1280x960"},
#endif
	{"picture-size", "640x480"},
#if defined(CONFIG_CAMERA_SMALL_PREVSIZE)
	{"preview-size-values", "720x480,640x480,352x288,320x240,176x144"},
	{"preview-size", "640x480"},
#else
#if defined(CONFIG_CAMERA_SUPPORT_720P)
	{"preview-size-values", "1280x720,720x480,640x480,352x288,320x240,176x144"},
	{"preview-size", "720x480"},
#else
	{"preview-size-values", "1920x1088,1280x960,1280x720,720x480,640x480,352x288,320x240,176x144"},
	{"preview-size", "1280x960"},
#endif
#endif
	{"video-size-values", ""},
	{"video-size", "1920x1088"},
	{"preferred-preview-size-for-video", ""},
	{"video-frame-format-values", "android-opaque,yuv420sp,yuv420p"},
	{"video-frame-format", "android-opaque"},
	{"preview-format-values", "yuv420sp,yuv420p"},
	{"preview-format", "yuv420sp"},
	{"picture-format-values", "jpeg"},
	{"picture-format", "jpeg"},
	{"jpeg-quality", "100"},
	{"preview-frame-rate-values", "10,15,20,25,30,31"},
	{"preview-frame-rate", "30"},
	{"preview-fps-range-values", "(1000,30000)"},
	{"preview-fps-range", "1000,30000"},
	{"jpeg-thumbnail-size-values", "320x240,0x0"},
	{"jpeg-thumbnail-width","320"},
	{"jpeg-thumbnail-height", "240"},
	{"jpeg-thumbnail-quality", "80"},
	{"effect-values", "none,mono,negative,sepia,cold,antique"},
	{"effect", "none"},
	{"scene-mode-values", "auto,night,portrait,landscape,action,normal,hdr"},
	{"scene-mode", "auto"},
	{"cameraid-values", "back_camera,front_camera"},
	{"cameraid","back_camera"},
	{"zoom-supported", "true"},
	{"smooth-zoom-supported", "false"},
	{"max-zoom", "7"},
	{"zoom-ratios", "100,120,140,170,200,230,260,300"},
	{"zoom", "0"},
	{"brightness-supported", "true"},
	{"smooth-brightness-supported", "false"},
	{"max-brightness", "6"},
	{"brightness-values", "0,1,2,3,4,5,6"},
	{"brightness", "3"},
	{"contrast-supported", "true"},
	{"smooth-contrast-supported", "false"},
	{"max-contrast", "6"},
	{"contrast-values", "0,1,2,3,4,5,6"},
	{"contrast", "3"}  ,

	{"saturation-supported", "true"},
	{"saturation-values", "0,1,2,3,4,5,6"},
	{"saturation", "3"},
#if 0

	{"sharpness-supported", "true"},
	{"max-sharpness", "6"},
	{"sharpness-values", "0,1,2,3,4,5,6"},
	{"sharpness", "3"},
#endif
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#if	defined(CONFIG_CAMERA_CAF)
	{"focus-mode-values", "auto,macro,continuous-picture,continuous-video,infinity"},
	{"focus-mode", "auto"},
#else
	{"focus-mode-values", "auto,macro,infinity"},
	{"focus-mode", "auto"},
#endif
	{"focus-distances", "2.0,2.5,Infinity"},
	{"max-num-focus-areas", "3"},
#else
	{"focus-mode-values", "fixed"},
	{"focus-mode", "fixed"},
	{"focus-distances", "2.0,2.5,Infinity"},
	{"max-num-focus-areas", "0"},
#endif
	{"min-exposure-compensation", "-4"},
	{"max-exposure-compensation", "4"},
	{"exposure-compensation","0"},
	{"exposure-compensation-step", "1"},
//back camera add auto antibanding mode
/** SPRD: add { */
	{"antibanding-values","auto,50hz,60hz"},
	{"antibanding","auto"},
/** SPRD: add } */
	{"antibanding-supported","true"},
	{"focal-length", "3.75"},
	{"horizontal-view-angle", "48"},
	{"vertical-view-angle", "48"},
#ifndef CONFIG_CAMERA_FLASH_NOT_SUPPORT
	{"flash-mode-values", "off,on,torch,auto"},
	{"flash-mode", "off"},
	{"flash-mode-supported", "true"},
#else
        /*{"flash-mode-values", "off"},*/
        /*{"flash-mode", "off"},*/
        {"flash-mode-supported", "false"},
#endif
#if defined(CONFIG_CAMERA_FACE_DETECT)
        {"max-num-detected-faces-hw", "10"},
#else
        {"max-num-detected-faces-hw", "0"},
#endif
#ifdef CONFIG_BACK_CAMERA_ISO_NOT_SUPPORT
	{"iso-supported", "false"},
	{"max-iso", "1"},
	{"iso-values", "auto"},
	{"iso", "auto"},
#else
	{"iso-supported", "true"},
	{"max-iso", "5"},
	{"iso-values", "auto,100,200,400,800,1600"},
	{"iso", "auto"},
#endif
	{"smile-snap-mode","0"},
	{"hdr-supported","true"},
	{"hdr","0"},
#if	defined(CONFIG_CAMERA_ZSL_CAPTURE)
	{"zsl-supported","true"},
#else
	{"zsl-supported","false"},
#endif
#ifdef CONFIG_CAMERA_FORCE_ZSL_CAPTURE
	{"zsl","1"},
#else
	{"zsl","0"},
#endif
	{"capture-mode", "1"},
	{"slow-motion-supported","true"},
	{"max-slow-motion","3"},
	{"slow-motion-values", "1,2,3"},
	{"slow-motion", "1"},
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	{"max-num-metering-areas", "1"},
#else
	{"max-num-metering-areas", "0"},
#endif
#ifndef CONFIG_EXPOSURE_METERING_NOT_SUPPORT
	{"auto-exposure","center-weighted"},
	{"auto-exposure-values", "frame-average,center-weighted,spot-metering"},
#else
	{"auto-exposure","0"},
	{"auto-exposure-supported", "0"},
#endif
	{"preview-env","0"},
#if  defined(CONFIG_CAMERA_ZSL_CAPTURE)
	{"video-snapshot-supported","true"},
#else
	{"video-snapshot-supported","false"},
#endif
	{"auto-exposure-lock-supported", "true"}
};

#endif //_SPRD_CAMERA_HARDWARE_CONFIG_H_
