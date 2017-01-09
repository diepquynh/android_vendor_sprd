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

#ifndef __SPRDCAMERA3_SETTING_H__
#define __SPRDCAMERA3_SETTING_H__

#include <pthread.h>
#include <utils/Mutex.h>
#include <utils/List.h>
#include <utils/KeyedVector.h>
#include <hardware/camera3.h>
#include <camera/CameraMetadata.h>
#include "include/SprdCamera3Tags.h"
#include "SprdCamera3HALHeader.h"
#include "SprdCameraParameters.h"


using namespace android;

namespace sprdcamera {

#define LOGV ALOGV
#define LOGE ALOGE
#define LOGI ALOGI
#define LOGW ALOGW
#define LOGD ALOGD
#define PRINT_TIME 0
#define ROUND_TO_PAGE(x) (((x)+0xfff)&~0xfff)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))
#define METADATA_SIZE (2*sizeof(unsigned long)+5*sizeof(unsigned int))//28/* (7 * 4) */

#define SET_PARM(h,x,y,z) do {\
			HAL_LOGV("%s: set camera param: %s, %d", __func__, #x, y);\
                        if (NULL != h && NULL != h->ops)\
			    h->ops->camera_set_param (x, y, z);\
		} while(0)
#define SIZE_ALIGN(x) (((x)+15)&(~15))
#define UNUSED(x) (void)x

#define CAMERA_ID_COUNT   3

#ifdef CONFIG_CAMERA_SUPPORT_13M
#define BACK_SENSOR_ORIG_WIDTH 4160
#define BACK_SENSOR_ORIG_HEIGHT 3120
#elif CONFIG_CAMERA_SUPPORT_8M
#define BACK_SENSOR_ORIG_WIDTH 3264
#define BACK_SENSOR_ORIG_HEIGHT 2448
#else
#define BACK_SENSOR_ORIG_WIDTH 2592
#define BACK_SENSOR_ORIG_HEIGHT 1952
#endif

#ifdef CONFIG_FRONT_CAMERA_SUPPORT_5M
#define FRONT_SENSOR_ORIG_WIDTH 2592 //select according to max jpg resolution
#define FRONT_SENSOR_ORIG_HEIGHT 1944
#elif CONFIG_FRONT_CAMERA_SUPPORT_3M
#define FRONT_SENSOR_ORIG_WIDTH 2048 //select according to max jpg resolution
#define FRONT_SENSOR_ORIG_HEIGHT 1536
#else

#ifndef CONFIG_FRONT_CAMERA_SUPPORT_720P
#define FRONT_SENSOR_ORIG_WIDTH 1600 //select according to max jpg resolution
#define FRONT_SENSOR_ORIG_HEIGHT 1200
#else
#define FRONT_SENSOR_ORIG_WIDTH 720 //select according to max jpg resolution
#define FRONT_SENSOR_ORIG_HEIGHT 480
#endif

#endif

/* Time related macros */
typedef int64_t nsecs_t;
#define NSEC_PER_SEC 1000000000LL
#define NSEC_PER_USEC 1000
#define NSEC_PER_33MSEC 33000000LL
#define NSEC_PER_40MSEC 40000000LL
#define MIN_FPS_RANGE 5
#define MAX_FPS_RANGE_BACK_CAM 30
#define MAX_FPS_RANGE_FRONT_CAM 30
#define MIDDLE_FPS_RANGE 20
#define SPRD_SHADING_FACTOR_NUM    (2*2)//(>1*1*4,<=64*64*4)
#define SPRD_MAX_TONE_CURVE_POINT  64  //>=64

#define CAMERA_SETTINGS_CONFIG_ARRAYSIZE 70


typedef struct {
	uint8_t correction_mode;
	uint8_t aberration_mode;
	uint8_t available_aberration_modes[3];
	float gains[4];
	camera_metadata_rational_t transform[9];
} COLOR_Tag;

typedef struct {
	uint8_t capture_intent;
	uint8_t available_video_stab_modes[2];
	int32_t max_regions[3];
	uint8_t available_effects[9];
	uint8_t available_scene_modes[18];
	uint8_t mode;
	uint8_t effect_mode;
	uint8_t scene_mode;
	uint8_t video_stabilization_mode;
	uint8_t antibanding_mode;

	uint8_t af_trigger;
	int32_t af_trigger_Id;
	uint8_t af_state;
	uint8_t af_mode;
	int32_t af_regions[5];
	uint8_t af_available_modes[6];

	int32_t ae_available_fps_ranges[14];
	int32_t ae_compensation_range[2];
	uint8_t ae_available_abtibanding_modes[4];
	uint8_t ae_available_modes[5];
	int32_t ae_target_fps_range[2];
	int32_t ae_regions[5];

	uint8_t am_available_modes[4];
	camera_metadata_rational ae_compensation_step;
	int32_t org_ae_exposure_compensation;
	int32_t ae_exposure_compensation;
	uint8_t ae_lock;
	uint8_t ae_mode;
	uint8_t ae_abtibanding_mode;
	uint8_t ae_precap_trigger;
	uint8_t ae_state;
	int32_t ae_precapture_id;

	uint8_t awb_available_modes[9];
	uint8_t awb_lock;
	uint8_t awb_mode;
	uint8_t awb_state;
	int32_t awb_regions[5];
} CONTROL_Tag;

typedef struct {
	uint8_t mode;
	uint8_t strength;
	uint8_t available_edge_modes[3];
} EDGE_Tag;

typedef struct {
	uint8_t firing_power;
	uint8_t mode;
	uint8_t state;
} FLASH_Tag;

typedef struct {
	int64_t charge_duration;
	uint8_t available;
} FLASH_INFO_Tag;

typedef struct {
} GEOMETRIC_Tag;

typedef struct {
	uint8_t mode;
} HOT_PIXEL_Tag;

typedef struct {
	uint8_t reduction_mode;
	uint8_t reduction_available_noise_reduction_modes[3];
} NOISE_Tag;

typedef struct {
	float focus_range[4];
	float focus_distance;
	float focal_length;
	uint8_t state;
	uint8_t facing;
	float aperture;
	float filter_density;
	uint8_t optical_stabilization_mode;
} LENS_Tag;

typedef struct {
	uint8_t quality;
	uint8_t thumbnail_quality;
	int32_t thumbnail_size[2];
	int32_t orientation_original;
	int32_t orientation;
	int32_t available_thumbnail_sizes[8];
	int32_t max_size;
	double gps_coordinates[3];
	uint8_t gps_processing_method[36];
	int64_t gps_timestamp;
} JPEG_Tag;

typedef struct {
	float mini_focus_distance;
	float hyperfocal_distance;
	float available_focal_lengths;
	float available_apertures;
	float filter_density;
	uint8_t optical_stabilization;
	int32_t shading_map_size[2];
	uint8_t focus_distance_calibration;
} LENS_INFO_Tag;

typedef struct {
	float physical_size[2];
	int64_t exposupre_time_range[2];
	int64_t max_frame_duration;
	uint8_t color_filter_arrangement;
	int32_t pixer_array_size[2];
	int32_t active_array_size[4];
	int32_t white_level;
	int32_t sensitivity_range[2];
	uint8_t timestamp_source;
} SENSOR_INFO_Tag;

typedef struct {
	uint8_t mode;
	uint8_t factor_count;
	float   gain_factor[SPRD_SHADING_FACTOR_NUM];
} SHADING_Tag;

typedef struct {
	uint8_t face_detect_mode;
	uint8_t lens_shading_map_mode;
	uint8_t hot_pixel_map_mode;
	uint8_t scene_flicker;
} STATISTICS_Tag;

typedef struct {
} LED_Tag;

typedef struct {
	int32_t max_curve_points;
	uint8_t mode;
	uint8_t available_tone_map_modes[3];
	float   curve_blue[SPRD_MAX_TONE_CURVE_POINT];
	float   curve_green[SPRD_MAX_TONE_CURVE_POINT];
	float   curve_red[SPRD_MAX_TONE_CURVE_POINT];
} TONEMAP_Tag;

typedef struct {
	int32_t max_face_count;
	int32_t histogram_bucket_count;
	int32_t max_histogram_count;
	int32_t sharpness_map_size[2];
	int32_t max_sharpness_map_size;
	uint8_t available_face_detect_modes[2];
} STATISTICS_INFO_Tag;

typedef struct {
	int64_t raw_min_duration[1];
	int32_t formats[4];
	int32_t processed_size[20];
	int64_t processed_min_durations[1];
	float max_digital_zoom;
	int32_t jpeg_size[20];
	int64_t jpeg_min_durations[2];
	int32_t crop_region[4];
	int32_t available_stream_configurations[CAMERA_SETTINGS_CONFIG_ARRAYSIZE*4];
	int64_t min_frame_durations[CAMERA_SETTINGS_CONFIG_ARRAYSIZE*4];
	int64_t stall_durations[CAMERA_SETTINGS_CONFIG_ARRAYSIZE*4];
	uint8_t cropping_type;
} SCALER_Tag;

typedef struct {
	int64_t rollingShutterSkew;
	int64_t exposure_time;
	int64_t frame_duration;
	int32_t sensitivity;
	int64_t timestamp;
	int32_t orientation;
	int32_t black_level_pattern[4];
	int32_t available_test_pattern_modes[6];
	int32_t max_analog_sensitivity;
	int32_t test_pattern_mode;
} SENSOR_Tag;

typedef struct {
	uint8_t use_parital_result;
} QUIRKS_Tag;

typedef struct {
	int32_t max_num_output_streams[3];
	uint8_t type;
	int32_t id;
	int32_t frame_count;
	int32_t available_characteristics_keys[100];
	int32_t available_request_keys[50];
	int32_t available_result_keys[50];
	uint8_t available_capabilites[5];
	int32_t partial_result_count;
	uint8_t pipeline_max_depth;
	uint8_t pipeline_depth;
} REQUEST_Tag;

typedef struct {
	uint8_t availabe_brightness[7];
	uint8_t availabe_contrast[7];
	uint8_t availabe_saturation[7];
	uint8_t availabe_iso[7];
	uint8_t availabe_slow_motion[4];
	uint8_t flash_mode_support;
	uint8_t prev_rec_size_diff_support;
	uint8_t rec_snap_support;
	uint8_t big_pre_rec_size_support;
	uint8_t burst_cap_cnt;
	uint8_t capture_mode;
	uint8_t brightness;
	uint8_t contrast;
	uint8_t saturation;
	uint8_t slowmotion;
	uint8_t iso;
	uint8_t am_mode;
	uint8_t sensor_orientation;
	int32_t sensor_rotation;
	int32_t am_regions[5];
	int32_t perfect_skin_level;
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	uint8_t sprd_zsl_enabled;
#endif
	uint8_t flip_on;
	uint8_t availabe_smile_enable;
	uint8_t availabe_antiband_auto_supported;
} SPRD_DEF_Tag;

typedef struct {
	camera_face_t face[10];
	uint8_t face_num;
} FACE_Tag;

typedef struct {
	int32_t max_latency;
} SYNC_Tag;

typedef struct {
	cam_dimension_t preview_size;
	cam_dimension_t picture_size;

	COLOR_Tag colorInfo;
	CONTROL_Tag controlInfo;
	EDGE_Tag edgeInfo;
	FLASH_Tag flashInfo;
	FLASH_INFO_Tag flash_InfoInfo;
	GEOMETRIC_Tag geometricInfo;
	HOT_PIXEL_Tag hotpixerInfo;
	JPEG_Tag jpgInfo;
	LENS_Tag lensInfo;
	LENS_INFO_Tag lens_InfoInfo;
	NOISE_Tag noiseInfo;
	QUIRKS_Tag quirksInfo;
	REQUEST_Tag requestInfo;
	SCALER_Tag scalerInfo;
	SENSOR_Tag sensorInfo;
	SENSOR_INFO_Tag sensor_InfoInfo;
	SHADING_Tag shadingInfo;
	STATISTICS_Tag statisticsInfo;
	STATISTICS_INFO_Tag statis_InfoInfo;
	TONEMAP_Tag toneInfo;
	LED_Tag ledInfo;
	FACE_Tag faceInfo;
	SYNC_Tag syncInfo;
	meta_info_t metaInfo;
	uint8_t info_supported_hardware_level;
	uint8_t black_level_lock;

	SPRD_DEF_Tag sprddefInfo;

	uint8_t supported_hardware_level;
	uint8_t demosaic_mode;
} sprd_setting_info_t;

class SprdCamera3Setting {
public:
	SprdCamera3Setting(int cameraId);
	virtual ~SprdCamera3Setting();

	static int getCameraInfo(int32_t cameraId, struct camera_info *cameraInfo);
	static int getNumberOfCameras();
	static int initDefaultParameters(int32_t cameraId);
	static int getStaticMetadata(int32_t cameraId, camera_metadata_t **static_metadata);

	static const char *getVendorSectionName(
				const vendor_tag_query_ops_t *v,
				uint32_t tag);
	static int getVendorTagType(
				const vendor_tag_query_ops_t *v,
				uint32_t tag);
	static const char *getVendorTagName(
				const vendor_tag_query_ops_t *v,
				uint32_t tag);
	static int getVendorTagCnt(const vendor_tag_query_ops_t *v);
	static void getVendorTags(
				const vendor_tag_query_ops_t *v,
				uint32_t *tag_array);

	/*HAL 3.2*/
	static int get_tag_count(const vendor_tag_ops_t* ops);
	static void get_all_tags(const vendor_tag_ops_t* ops, uint32_t* tag_array);
	static const char* get_section_name(const vendor_tag_ops_t* ops, uint32_t tag);
	static const char* get_tag_name(const vendor_tag_ops_t* ops, uint32_t tag);
	static int get_tag_type(const vendor_tag_ops_t* ops, uint32_t tag);

	int constructDefaultMetadata(int type, camera_metadata_t** metadata);
	int UpdateWorkParameters(const CameraMetadata &frame_settings);
	int initialize(const camera3_callback_ops_t *callback_ops);
	camera_metadata_t *translateLocalToFwMetadata();
	int updateWorkParameters(const CameraMetadata &frame_settings);
	int getDefaultParameters(SprdCameraParameters &params);
	int popAndroidParaTag();
	int popSprdParaTag();
	void releaseAndroidParaTag();

	int setPreviewSize(cam_dimension_t size);
	int getPreviewSize(cam_dimension_t *size);

	int setPictureSize(cam_dimension_t size);
	int getPictureSize(cam_dimension_t *size);

	int setCOLORTag(COLOR_Tag colorInfo);
	int getCOLORTag(COLOR_Tag* colorInfo);

	int setCONTROLTag(CONTROL_Tag controlInfo);
	int getCONTROLTag(CONTROL_Tag* controlInfo);

	int setAeCONTROLTag(CONTROL_Tag controlInfo);
	int setAfCONTROLTag(CONTROL_Tag controlInfo);
	int setAwbCONTROLTag(CONTROL_Tag controlInfo);

	int setEDGETag(EDGE_Tag edgeInfo);
	int getEDGETag(EDGE_Tag* edgeInfo);

	int setLENSTag(LENS_Tag lensInfo);
	int getLENSTag(LENS_Tag* lensInfo);

	int setJPEGTag(JPEG_Tag jpgInfo);
	int getJPEGTag(JPEG_Tag* jpgInfo);

	int setLENSINFOTag(LENS_INFO_Tag lens_InfoInfo);
	int getLENSINFOTag(LENS_INFO_Tag* lens_InfoInfo);

	int setSENSORINFOTag(SENSOR_INFO_Tag sensor_InfoInfo);
	int getSENSORINFOTag(SENSOR_INFO_Tag* sensor_InfoInfo);

	int setFLASHTag(FLASH_Tag flashInfo);
	int getFLASHTag(FLASH_Tag* flashInfo);

	int setFLASHINFOTag(FLASH_INFO_Tag flash_InfoInfo);
	int getFLASHINFOTag(FLASH_INFO_Tag* flash_InfoInfo);
	int androidAeModeToDrvAeMode(uint8_t androidAeMode, int8_t *convertDrvMode);
	int androidFlashModeToDrvFlashMode(uint8_t androidFlashMode, int8_t *convertDrvMode);
	int androidSceneModeToDrvMode(uint8_t androidScreneMode, int8_t *convertDrvMode);
	int androidIsoToDrvMode(uint8_t androidIso, int8_t *convertDrvMode);
	int androidEffectModeToDrvMode(uint8_t androidEffectMode, int8_t *convertDrvMode);
	int androidAwbModeToDrvAwbMode(uint8_t androidAwbMode, int8_t *convertDrvMode);
	int androidAntibandingModeToDrvAntibandingMode(uint8_t androidAntibandingMode, int8_t *convertAntibandingMode);
	int androidAmModeToDrvAwbMode(uint8_t androidAmMode, struct cmr_ae_param* ae_param);
	int androidAfModeToDrvAfMode(uint8_t androidAfMode, int8_t *convertDrvMode);
	int setTONEMAPTag(TONEMAP_Tag toneInfo);
	int getTONEMAPTag(TONEMAP_Tag* toneInfo);

	int setSTATISTICSINFOTag(STATISTICS_INFO_Tag statis_InfoInfo);
	int getSTATISTICSINFOTag(STATISTICS_INFO_Tag* statis_InfoInfo);

	int setSCALERTag(SCALER_Tag scalerInfo);
	int getSCALERTag(SCALER_Tag* scalerInfo);

	int setQUIRKSTag(QUIRKS_Tag quirksInfo);
	int getQUIRKSTag(QUIRKS_Tag* quirksInfo);

	int setREQUESTTag(REQUEST_Tag requestInfo);
	int getREQUESTTag(REQUEST_Tag* requestInfo);

	int setSPRDDEFTag(SPRD_DEF_Tag sprddefInfo);
	int getSPRDDEFTag(SPRD_DEF_Tag* sprddefInfo);

	int setGEOMETRICTag(GEOMETRIC_Tag geometricInfo);
	int getGEOMETRICTag(GEOMETRIC_Tag* geometricInfo);

	int setHOTPIXERTag(HOT_PIXEL_Tag hotpixerInfo);
	int getHOTPIXERTag(HOT_PIXEL_Tag* hotpixerInfo);

	int setNOISETag(NOISE_Tag noiseInfo);
	int getNOISETag(NOISE_Tag* noiseInfo);

	int setSENSORTag(SENSOR_Tag sensorInfo);
	int getSENSORTag(SENSOR_Tag* sensorInfo);

	int setSHADINGTag(SHADING_Tag shadingInfo);
	int getSHADINGTag(SHADING_Tag* shadingInfo);

	int setSTATISTICSTag(STATISTICS_Tag statisticsInfo);
	int getSTATISTICSTag(STATISTICS_Tag* statisticsInfo);

	int setLEDTag(LED_Tag ledInfo);
	int getLEDTag(LED_Tag* ledInfo);

	int setFACETag(FACE_Tag faceInfo);
	int getFACETag(FACE_Tag* faceInfo);
	int setMETAInfo(meta_info_t metaInfo);/*   for some metadata,   result meta should be  corresponding with request meta */
	int getMETAInfo(meta_info_t* metaInfo);
	static uint8_t mMaxCameraCount;
	static camera_metadata_t *mStaticMetadata[CAMERA_ID_COUNT];
	static SprdCameraParameters mDefaultParameters;
	static camera_metadata_t *mDefaultMetadata[CAMERA3_TEMPLATE_COUNT];
	static sprd_setting_info_t      s_setting[CAMERA_ID_COUNT];
	static CameraMetadata mStaticInfo[CAMERA_ID_COUNT];

private:
	void pushAndroidParaTag(camera_metadata_tag_t tag);
	void pushAndroidParaTag(sprd_camera_metadata_tag_t tag);

	Mutex                           mLock;
	List<camera_metadata_tag_t>     mParaChangedTagQueue;
	List<sprd_camera_metadata_tag_t>     mSprdParaChangedTagQueue;
	uint8_t mCameraId;
	bool mbTouchAe_doing;
	int32_t mTouchAe_region_bak[5];
	uint8_t mTouchAe_am_mode_bak;
	static int parse_int(const char *str, int *data, char delim, char **endptr = NULL);
	static int parse_float(const char *str, float *data, char delim, char **endptr = NULL);
	static int parse_string(char *str, char *data, char delim, char **endptr = NULL);
	static void parseStringInt(const char *sizesStr, int *fdStr);
	static void parseStringfloat(const char *sizesStr, float *fdStr);

	static int setDefaultParaInfo(int32_t cameraId);
	static bool getLcdSize(uint32_t *width, uint32_t *height);
	static int initStaticParameters(int32_t cameraId);
	static int initStaticMetadata(int32_t cameraId, camera_metadata_t **static_metadata);
	static void convertToRegions(int32_t* rect, int32_t* region, int weight);

	void coordinate_struct_convert(int *rect_arr,int arr_size);
	int coordinate_convert(int *rect_arr,int arr_size,int angle,int is_mirror, struct img_size *preview_size,
					struct img_rect *preview_rect);
	static int GetFovParam(int32_t cameraId);
};

}; // namespace sprdcamera

#endif
