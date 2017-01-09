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
#ifndef _CMR_COMMON_H_
#define _CMR_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include "cmr_type.h"
#include "sensor_raw.h"
#include "isp_app.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_k.h"

//#define NCMRDBG 1
#ifndef LOG_NDEBUG
#ifdef NCMRDBG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif

#define UNUSED(x) (void)x

#define DEBUG_STR     "L %d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

extern volatile uint32_t gCMRLogLevel;

#define CMR_LOGE(fmt, args...) \
  ALOGE("%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)

#define CMR_LOGW(fmt, args...) \
  ALOGW_IF(gCMRLogLevel >= 2, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)

#define CMR_LOGI(fmt, args...) \
  ALOGI_IF(gCMRLogLevel >= 3, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)

#define CMR_LOGD(fmt, args...) \
  ALOGD_IF(gCMRLogLevel >= 4, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)

#define CMR_LOGV(fmt, args...) \
  ALOGV_IF(gCMRLogLevel >= 5, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)


#define CMR_EVT_GRAB_BASE                  (1 << 16)
#define CMR_EVT_CVT_BASE                   (1 << 17)
#define CMR_EVT_ISP_BASE                   (1 << 18)
#define CMR_EVT_SENSOR_BASE                (1 << 19)
#define CMR_EVT_JPEG_BASE                  (1 << 20)
#define CMR_EVT_OEM_BASE                   (1 << 21)
#define CMR_EVT_SETTING_BASE               (1 << 22)
#define CMR_EVT_IPM_BASE                   (1 << 23)
#define CMR_EVT_FOCUS_BASE                 (1 << 24)
#define CMR_EVT_PREVIEW_BASE               (1 << 25)
#define CMR_EVT_SNAPSHOT_BASE              (1 << 26)

#define CMR_PREV_ID_BASE                   0x1000
#define CMR_CAP0_ID_BASE                   0x2000
#define CMR_CAP1_ID_BASE                   0x4000
#define CMR_VIDEO_ID_BASE                  0x8000
#define CMR_BASE_ID(x)                     ((x) & 0xF000)
#define JPEG_EXIF_SIZE	                   (64*1024)
#define RAWRGB_BIT_WIDTH                   10
#define CMR_ZOOM_FACTOR                    2
#define CMR_SLICE_HEIGHT                   256
#define CMR_SHARK_SCALING_TH               2048
#define CMR_DOLPHIN_SCALING_TH             1280
#define GRAB_BUF_MAX                       8
#define GRAB_CHANNEL_MAX                   6
#define SESNOR_NAME_LEN                    40
#define CMR_CAPTURE_MEM_SUM                4
#define CAMERA_PIXEL_ALIGNED               4
#define CMR_MAX_SKIP_NUM                   10


#define HDR_CAP_NUM                        3
#define FACE_DETECT_NUM                    10
#define FRAME_NUM_MAX                     0xFFFFFFFF
#define FRAME_FLASH_MAX                   0x0000FFFF
#define FRAME_STOP                        0
#define FRAME_CONTINUE                    1
#define FRAME_IMAGE_PROC                  2
#define INVALID_FORMAT_PATTERN            255

#define CAMERA_SAFE_SCALE_DOWN(w)          (cmr_u32)((w)*11/10)
#define CAMERA_START(w)                    ((w)& ~(8 - 1))
#define CAMERA_WIDTH(w)                    ((w)& ~(16 - 1))
#define CAMERA_HEIGHT(h)                   ((h)& ~(16 - 1))
#define CMR_ADDR_ALIGNED(x)                ((((x) + 256 - 1) >> 8) << 8)

#define CMR_JPEG_SZIE(w,h)                 (cmr_u32)((w)*(h)*3/2)
#define CMR_EVT_MASK_BITS                  (cmr_u32)(CMR_EVT_GRAB_BASE | CMR_EVT_CVT_BASE | \
					                                  CMR_EVT_ISP_BASE | CMR_EVT_SENSOR_BASE | \
					                                  CMR_EVT_JPEG_BASE | CMR_EVT_OEM_BASE | \
					                                  CMR_EVT_SETTING_BASE | CMR_EVT_IPM_BASE | \
					                                  CMR_EVT_FOCUS_BASE)
#define CMR_RTN_IF_ERR(n)                                              \
		do {                                                   \
			if (n) {                                       \
				CMR_LOGW("ret %ld", n);                 \
				goto exit;                             \
			}                                              \
		} while(0)

#define CMR_PRINT_TIME                                                     \
		do {                                                       \
                        nsecs_t timestamp = systemTime(CLOCK_MONOTONIC);   \
                        CMR_LOGI("timestamp = %lld.", timestamp/1000000);  \
		} while(0)

#define CMR_PRINT_TIME_V                                                     \
		do {                                                       \
                        nsecs_t timestamp = systemTime(CLOCK_MONOTONIC);   \
                        CMR_LOGV("timestamp = %lld.", timestamp/1000000);  \
		} while(0)


#define CMR_PERROR                                                     \
		do {                                                       \
                        CMR_LOGE("device.errno %d %s", errno, strerror(errno)); \
		} while(0)

#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif

#ifndef MAX
#define MAX(x,y) (((x)>(y))?(x):(y))
#endif
/*********************************** common* **********************************/
#define cmr_bzero(b, len)                     memset((b), '\0', (len))
#define cmr_copy(b, a, len)                   memcpy((b), (a), (len))
#define CAMERA_ALIGNED_16(w)                  ((((w) + 16 -1) >> 4) << 4)
#define cmr_array_size(x)                     (sizeof(x)/sizeof((x)[0]))
#define CMR_YUV_BUF_GAP                        4
/******************************************************************************/


/********************************** fun type **********************************/
struct after_set_cb_param {
	cmr_u32        re_mode;
	cmr_u32        skip_mode;
	cmr_u32        skip_number;
	cmr_u32        padding;
	nsecs_t        timestamp;
};

enum preview_param_mode {
	PARAM_NORMAL = 0,
	PARAM_ZOOM,
	PARAM_MODE_MAX
};

typedef void (*cmr_evt_cb)(cmr_int evt, void* data, void* privdata);
typedef void (*cmr_malloc)(cmr_u32 mem_type, cmr_handle oem_handle, cmr_u32 *size,
	                         cmr_u32 *sum, cmr_uint *phy_addr, cmr_uint *vir_addr);
typedef void (*cmr_free)(cmr_u32 mem_type, cmr_handle oem_handle, cmr_uint *phy_addr,
	                       cmr_uint *vir_addr, cmr_u32 sum);
typedef cmr_int  (*cmr_before_set_cb)(cmr_handle oem_handle, enum preview_param_mode mode);
typedef cmr_int  (*cmr_after_set_cb)(cmr_handle oem_handle, struct after_set_cb_param *param);
/******************************************************************************/
enum camera_index {
	CAMERA_ID_0 = 0,
	CAMERA_ID_1,
	CAMERA_ID_2,
	CAMERA_ID_MAX
};

enum img_angle {
	IMG_ANGLE_0 = 0,
	IMG_ANGLE_90,
	IMG_ANGLE_270,
	IMG_ANGLE_180,
	IMG_ANGLE_MIRROR,
	IMG_ANGLE_MAX
};

enum img_data_type {
	IMG_DATA_TYPE_YUV422 = 0,
	IMG_DATA_TYPE_YUV420,
	IMG_DATA_TYPE_YVU420,
	IMG_DATA_TYPE_YUV420_3PLANE,
	IMG_DATA_TYPE_RAW,
	IMG_DATA_TYPE_RGB565,
	IMG_DATA_TYPE_RGB666,
	IMG_DATA_TYPE_RGB888,
	IMG_DATA_TYPE_JPEG,
	IMG_DATA_TYPE_YV12,
	IMG_DATA_TYPE_MAX
};

/*0: 3 plane;  1: 2 plane uvuv;  2: 2 plane vuvu*/
enum img_data_endian {
	IMG_DATA_ENDIAN_3PLANE = 0,
	IMG_DATA_ENDIAN_2PLANE_UVUV,
	IMG_DATA_ENDIAN_2PLANE_VUVU,
	IMG_DATA_ENDIAN_MAX
};

enum img_skip_mode {
	IMG_SKIP_HW = 0,
	IMG_SKIP_SW
};

enum restart_mode {
	RESTART_LIGHTLY = 0,
	RESTART_MIDDLE,
	RESTART_HEAVY,
	RESTART_ZOOM,
	RESTART_ZOOM_RECT,
	RESTART_MAX
};

enum restart_trigger {
	RESTART_BEFORE = 0,
	RESTART_AFTER,
	RESTART_ERROR,
	RESTART_SEQUENCE_MAX
};

enum raw_video_mode {
	VIDEO_CONTINUE = 0,
	VIDEO_SINGLE,
	VIDEO_MODE_MAX
};

enum camera_mem_type {
	CAMERA_MEM_PREVIEW = 0,
	CAMERA_MEM_SNAPSHOT,
	CAMERA_MEM_TYPE_MAX
};

enum camera_mem_aligned {
	CAMERA_MEM_ALIGNED = 0,
	CAMERA_MEM_NO_ALIGNED,
	CAMERA_MEM_ALIGNED_MAX
};

enum camera_states {
	IDLE = 0x00,
	INITED,
	PREVIEWING,
	SNAPSHOTING,
	RECOVERING_IDLE,
	RECOVERING,
	WORKING,
	CODEC_WORKING,
	POST_PROCESSING,
	IPM_WORKING,
	WRITE_EXIF,
	ERROR,
	CAMERA_STATE_MAX
};

enum common_isp_cmd_type {
	COM_ISP_SET_AE_MODE,
	COM_ISP_SET_AE_MEASURE_LUM,
	COM_ISP_SET_AE_METERING_AREA,
	COM_ISP_SET_BRIGHTNESS,
	COM_ISP_SET_CONTRAST,
	COM_ISP_SET_SATURATION,
	COM_ISP_SET_SHARPNESS,
	COM_ISP_SET_SPECIAL_EFFECT,
	COM_ISP_SET_EV,
	COM_ISP_SET_AWB_MODE,
	COM_ISP_SET_ANTI_BANDING,
	COM_ISP_SET_ISO,
	COM_ISP_SET_VIDEO_MODE,
	COM_ISP_SET_FPS_LLS_MODE,
	COM_ISP_SET_FLASH_EG,
	COM_ISP_SET_AF,
	COM_ISP_SET_AF_MODE,/*15*/
	COM_ISP_SET_AF_STOP,
	COM_ISP_GET_EXIF_IMAGE_INFO,
	COM_ISP_GET_LOW_LUX_EB,/*capability*/
	COM_ISP_SET_FLASH_NOTICE,
	COM_ISP_SET_FLASH_LEVEL,
	COM_ISP_SET_FACE_AREA,
	COM_ISP_SET_SNAPSHOT_NOTICE,
	COM_ISP_SET_RANGE_FPS,
	COM_ISP_GET_AE_LUM,
	COM_ISP_SET_HDR,
	COM_ISP_SET_AE_LOCK_UNLOCK,
	COM_ISP_TYPE_MAX
};

enum common_sn_cmd_type {
	COM_SN_GET_AUTO_FLASH_STATE,
	COM_SN_SET_BRIGHTNESS,
	COM_SN_SET_CONTRAST,
	COM_SN_SET_SATURATION,
	COM_SN_SET_SHARPNESS,
	COM_SN_SET_IMAGE_EFFECT,
	COM_SN_SET_EXPOSURE_COMPENSATION,
	COM_SN_SET_EXPOSURE_LEVEL,
	COM_SN_SET_WB_MODE,
	COM_SN_SET_PREVIEW_MODE,
	COM_SN_SET_ANTI_BANDING,
	COM_SN_SET_ISO,
	COM_SN_SET_VIDEO_MODE,
	COM_SN_SET_FPS_LLS_MODE,
	COM_SN_SET_BEFORE_SNAPSHOT,
	COM_SN_SET_AFTER_SNAPSHOT,
	COM_SN_SET_EXT_FUNC,
	COM_SN_SET_AE_ENABLE,
	COM_SN_SET_EXIF_FOCUS,
	COM_SN_SET_FOCUS,
	COM_SN_GET_RESOLUTION_IMAGE_FORMAT,
	COM_SN_GET_FRAME_SKIP_NUMBER,
	COM_SN_GET_PREVIEW_MODE,
	COM_SN_GET_CAPTURE_MODE,
	COM_SN_GET_SENSOR_ID,
	COM_SN_GET_VIDEO_MODE,
	COM_SN_GET_EXIF_IMAGE_INFO,
	COM_SN_SET_HDR_EV,
	COM_SN_GET_INFO,
	COM_SN_GET_FLASH_LEVEL,
	COM_SN_SET_YUV_FPS,
	COM_SN_TYPE_MAX,
};

enum cmr_jpeg_buf_type {
	JPEG_YUV_SLICE_ONE_BUF =  0,
	JPEG_YUV_SLICE_MUTI_BUF,
};

enum take_picture_flag {
	TAKE_PICTURE_NO = 0,
	TAKE_PICTURE_NEEDED
};

struct img_addr {
	cmr_uint                                addr_y;
	cmr_uint                                addr_u;
	cmr_uint                                addr_v;
};

struct img_frm {
	cmr_u32                                 fmt;
	cmr_u32                                 buf_size;
	struct img_rect                         rect;
	struct img_size                         size;
	struct img_addr                         addr_phy;
	struct img_addr                         addr_vir;
	struct img_data_end                     data_end;
	cmr_u32                                 format_pattern;
	void*                                   reserved;
};

struct ccir_if {
	cmr_u8                                  v_sync_pol;
	cmr_u8                                  h_sync_pol;
	cmr_u8                                  pclk_pol;
	cmr_u8                                  res1;
	cmr_u32                                 padding;
};

struct mipi_if {
	cmr_u8                                  use_href;
	cmr_u8                                  bits_per_pxl;
	cmr_u8                                  is_loose;
	cmr_u8                                  lane_num;
	cmr_u32                                 pclk;
};

struct cmr_op_mean {
	cmr_u32                                 slice_height;
	cmr_u32                                 slice_mode;
	cmr_u32                                 ready_line_num;
	cmr_u32                                 slice_num;
	cmr_u32                                 is_sync;
	cmr_u32                                 is_thumb;
	cmr_u32                                 rot;
	cmr_u32                                 quality_level;
	cmr_uint                                out_param;
	struct img_frm                          temp_buf;
};

struct video_start_param {
	struct img_size                         size;
	cmr_u32                                 img_format;
	cmr_u32                                 video_mode;
	cmr_u32                                 work_mode;
	cmr_u32                                 is_need_flash;
	cmr_u32                                 capture_skip_num;
};

struct memory_param {
	cmr_malloc                              alloc_mem;
	cmr_free                                free_mem;
};

struct isptool_scene_param {
	cmr_u32            width;
	cmr_u32            height;
	cmr_u32            gain;
	cmr_u32            awb_gain_r;
	cmr_u32            awb_gain_g;
	cmr_u32            awb_gain_b;
	cmr_u32            smart_ct;
	cmr_u32            smart_bv;
};

/********************************* v4l2 start *********************************/

/********************************* v4l2 start *********************************/
enum cmr_v4l2_evt {
	CMR_GRAB_TX_DONE = CMR_EVT_GRAB_BASE,
	CMR_GRAB_TX_ERROR,
	CMR_GRAB_TX_NO_MEM,
	CMR_GRAB_CSI2_ERR,
	CMR_GRAB_TIME_OUT,
	CMR_GRAB_CANCELED_BUF,
	CMR_GRAB_MAX,
};
enum channel_num {
	CHN_0 = 0,
	CHN_1,
	CHN_2,
	CHN_MAX
};

enum cmr_buf_flag {
	BUF_FLAG_INIT,
	BUF_FLAG_RUNNING,
	BUF_FLAG_MAX
};


struct  sensor_if {
	cmr_u8                              if_type;
	cmr_u8                              img_fmt;
	cmr_u8                              img_ptn;
	cmr_u8                              frm_deci;
	cmr_u8                              res[4];
	union  {
	struct ccir_if                      ccir;
	struct mipi_if                      mipi;
	}if_spec;
};

struct img_frm_cap {
	struct img_rect                     src_img_rect;
	struct img_size                     dst_img_size;
	cmr_u32                             dst_img_fmt;
	cmr_u32                             notice_slice_height;
	cmr_u32                             need_isp;
	cmr_u32                             need_binning;
	cmr_u32                             need_isp_tool;
	cmr_u32                             shrink;
	cmr_u32				                flip_on;
};

struct buffer_cfg {
	cmr_u32                             channel_id;
	cmr_u32                             base_id;
	cmr_u32                             count;
	cmr_u32                             length;
	cmr_u32                             slice_height;
	cmr_u32                             start_buf_id;
	cmr_u32                             is_reserved_buf;
	cmr_u32                             flag;
	cmr_u32                             index[GRAB_BUF_MAX];
	struct img_addr                     addr[GRAB_BUF_MAX];
	struct img_addr                     addr_vir[GRAB_BUF_MAX];
	cmr_uint                            zsl_private;
};

struct cap_cfg {
	cmr_u32                             chn_deci_factor;
	cmr_u32                             frm_num;
	struct img_frm_cap                  cfg;
};

struct channel_start_param {
	cmr_u32                             is_lightly;    /*0:normal, 1:lightly, only update cap_cfg*/
	cmr_u32                             sensor_mode;
	cmr_u32                             frm_num;
	cmr_u32                             skip_num;
	struct sensor_if                    sn_if;
	struct cap_cfg                      cap_inf_cfg;
	struct buffer_cfg                   buffer;
};
struct cmr_path_info {
	uint32_t               line_buf;
	uint32_t               support_yuv;
	uint32_t               support_raw;
	uint32_t               support_jpeg;
	uint32_t               support_scaling;
	uint32_t               is_scaleing_path;;
};
struct cmr_path_capability {
	uint32_t               is_video_prev_diff;
	uint32_t               hw_scale_available;
	uint32_t               yuv_available_cnt;
	uint32_t               capture_no_trim;
	uint32_t               capture_pause;
	uint32_t               zoom_post_proc;
};


/********************************** v4l2 end **********************************/

/******************************** memory start ********************************/
struct cmr_cap_mem {
	struct img_frm                      cap_raw;
	struct img_frm                      cap_yuv;
	struct img_frm                      target_yuv;
	struct img_frm                      target_jpeg;
	struct img_frm                      thum_yuv;
	struct img_frm                      thum_jpeg;
	struct img_frm                      jpeg_tmp;
	struct img_frm                      scale_tmp;
	struct img_frm                      cap_yuv_rot;
	struct img_frm                      isp_tmp;
};
/********************************* memory end *********************************/

/******************************** sensor start ********************************/
enum sensor_mode {
	SENSOR_MODE_COMMON_INIT = 0,
	SENSOR_MODE_PREVIEW_ONE,
	SENSOR_MODE_SNAPSHOT_ONE_FIRST,
	SENSOR_MODE_SNAPSHOT_ONE_SECOND,
	SENSOR_MODE_SNAPSHOT_ONE_THIRD,
	SENSOR_MODE_PREVIEW_TWO,
	SENSOR_MODE_SNAPSHOT_TWO_FIRST,
	SENSOR_MODE_SNAPSHOT_TWO_SECOND,
	SENSOR_MODE_SNAPSHOT_TWO_THIRD,
	SENSOR_MODE_MAX
};

#define SENSOR_VIDEO_MODE_MAX              4

struct sensor_interface {
	cmr_u32                             type;
	cmr_u32                             bus_width;
	cmr_u32                             pixel_width;
	cmr_u32                             is_loose;
};
struct sensor_mode_info {
	cmr_u16                             mode;
	cmr_u16                             width;
	cmr_u16                             height;
	cmr_u16                             trim_start_x;
	cmr_u16                             trim_start_y;
	cmr_u16                             trim_width;
	cmr_u16                             trim_height;
	cmr_u16                             image_format;
	cmr_u32                             line_time;
	cmr_u32                             bps_per_lane;
	cmr_u32                             frame_line;
	cmr_u32                             padding;
	struct img_rect                     scaler_trim;
};

struct sensor_ae_info {
	cmr_u32                             min_frate;
	cmr_u32                             max_frate;
	cmr_u32                             line_time;
	cmr_u32                             gain;
};

struct sensor_video_info {
	struct sensor_ae_info              ae_info[SENSOR_VIDEO_MODE_MAX];
	void                               *setting_pptr;
};

struct sensor_view_angle {
	cmr_u16                             horizontal_val;
	cmr_u16                             vertical_val;
};

struct exif_info {
	float                               aperture;
	float                               focus_distance;
};

struct sensor_exp_info {
	cmr_u32                             image_format;
	cmr_u32                             image_pattern;
	cmr_u32                             change_setting_skip_num;
	cmr_u32                             sensor_image_type;
	cmr_u8                              pclk_polarity;
	cmr_u8                              vsync_polarity;
	cmr_u8                              hsync_polarity;
	cmr_u8                              padding1;
	cmr_u16                             source_width_max;
	cmr_u16                             source_height_max;
	cmr_u32                             image_effect;
	cmr_u32                             step_count;
	cmr_u32                             preview_skip_num;
	cmr_u32                             capture_skip_num;
	cmr_u32                             preview_deci_num;
	cmr_u32                             video_preview_deci_num;
	cmr_u16                             threshold_eb;
	cmr_u16                             threshold_mode;
	cmr_u16                             threshold_start;
	cmr_u16                             threshold_end;
	struct sensor_mode_info             mode_info[SENSOR_MODE_MAX];
	struct sensor_raw_info              *raw_info_ptr;
	struct sensor_interface             sn_interface;
	struct sensor_video_info            video_info[SENSOR_MODE_MAX];
	struct sensor_view_angle            view_angle;
};

struct yuv_sn_af_param {
	uint8_t cmd;
	uint8_t param;
	uint16_t zone_cnt;
	struct img_rect zone[FOCUS_ZONE_CNT_MAX];
};


/********************************* sensor end *********************************/


/***************************** common ctrl start ******************************/
struct common_isp_cmd_param {
	cmr_uint	                                camera_id;
	union {
		cmr_u32                                 cmd_value;
		cmr_u32                                 padding;
		struct isp_af_win                       af_param;
		struct exif_spec_pic_taking_cond_tag    exif_pic_info;
		struct cmr_win_area                     win_area;
		struct isp_flash_notice                 flash_notice;
//#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
		struct isp_face_area                    fd_param;
		struct isp_ae_fps                       fps_param;

//#endif
		struct cmr_range_fps_param              range_fps;
	};
};

struct common_sn_cmd_param {
	cmr_uint                                    camera_id;
	struct sensor_exp_info                      sensor_static_info;
	struct sensor_flash_level                   flash_level;
	union {
		cmr_u32                                 cmd_value;
		cmr_u32                                 padding;
		struct yuv_sn_af_param                  yuv_sn_af_param;
		struct exif_spec_pic_taking_cond_tag    exif_pic_info;
		struct cmr_range_fps_param range_fps;
    };
};
/**************************** common ctrl end *********************************/


/*************************** snapshot post data type **************************/
enum cmr_zoom_mode {
	ZOOM_BY_CAP = 0,
	ZOOM_POST_PROCESS,
	ZOOM_POST_PROCESS_WITH_TRIM
};

struct snp_proc_param {
	cmr_uint                            rot_angle;
	cmr_uint                            channel_zoom_mode;
	struct img_size                     dealign_actual_snp_size;
	struct img_size                     actual_snp_size;
	struct img_size                     snp_size;
	struct img_size                     max_size;
	struct img_data_end                 data_endian;
	cmr_uint                            is_need_scaling;
	struct img_rect                     scaler_src_rect[CMR_CAPTURE_MEM_SUM];
	struct img_frm                      chn_out_frm[CMR_CAPTURE_MEM_SUM];
	struct cmr_cap_mem                  mem[CMR_CAPTURE_MEM_SUM];
};
/*************************** snapshot post data type **************************/

/*************************** ipm data type **************************/


enum ipm_class_type {
	IPM_TYPE_NONE = 0x0,
	IPM_TYPE_HDR = 0x00000001,
	IPM_TYPE_FD = 0x00000002,
	IPM_TYPE_UVDE = 0x00000004,
	IPM_TYPE_YDE = 0x00000008,
};

enum img_fmt {
	IMG_FMT_RGB565 = 0x0010,
	IMG_FMT_RGB666,
	IMG_FMT_XRGB8888,
	IMG_FMT_YCBCR420 = 0x0020,
	IMG_FMT_YCRCB420,
	IMG_FMT_YCBCR422P,
	IMG_FMT_YCBYCR422 = 0x0040,
	IMG_FMT_YCRYCB422,
	IMG_FMT_CBYCRY422,
	IMG_FMT_CRYCBY422,
};

struct face_finder_data {
    int face_id;
    int sx;
    int sy;
    int srx;
    int sry;
    int ex;
    int ey;
    int elx;
    int ely;
    int brightness;
    int angle;
    int smile_level;
    int smile_conf;
    int blink_level;
};

struct img_face_area {
	cmr_uint                face_count;
	struct face_finder_data range[FACE_DETECT_NUM];
};
/*************************** ipm data type **************************/


/******************************* jpeg data type *******************************/
struct jpeg_enc_cb_param {
	cmr_uint                 stream_buf_phy;
	cmr_uint                 stream_buf_vir;
	cmr_u32                  stream_size;
	cmr_u32                  slice_height;
	cmr_u32                  total_height;
	cmr_u32                  is_thumbnail;
};

struct jpeg_dec_cb_param {
	cmr_u32                  slice_height;
	cmr_u32                  total_height;
	struct img_frm           *src_img;
	struct img_data_end      data_endian;
};
/******************************* jpeg data type *******************************/


/*********************** camera_takepic_step timestamp  ***********************/
enum CAMERA_TAKEPIC_STEP {
		CMR_STEP_TAKE_PIC = 0,
		CMR_STEP_CAP_S,
		CMR_STEP_CAP_E,
		CMR_STEP_ROT_S,
		CMR_STEP_ROT_E,
		CMR_STEP_ISP_PP_S,
		CMR_STEP_ISP_PP_E,
		CMR_STEP_JPG_DEC_S,
		CMR_STEP_JPG_DEC_E,
		CMR_STEP_SC_S,
		CMR_STEP_SC_E,
		CMR_STEP_UVDENOISE_S,
		CMR_STEP_UVDENOISE_E,
		CMR_STEP_YDENOISE_S,
		CMR_STEP_YDENOISE_E,
		CMR_STEP_JPG_ENC_S,
		CMR_STEP_JPG_ENC_E,
		CMR_STEP_CVT_THUMB_S,
		CMR_STEP_CVT_THUMB_E,
		CMR_STEP_THUM_ENC_S,
		CMR_STEP_THUM_ENC_E,
		CMR_STEP_WR_EXIF_S,
		CMR_STEP_WR_EXIF_E,
		CMR_STEP_CALL_BACK,
		CMR_STEP_MAX
};

struct CAMERA_TAKEPIC_STAT {
		cmr_u8               step_name[24];
		nsecs_t              timestamp;
		cmr_u32              valid;
		cmr_u32              padding;
};

/**********************************************************8*******************/
cmr_int camera_get_trim_rect(struct img_rect *src_trim_rect, cmr_uint zoom_level, struct img_size *dst_size);

cmr_int camera_get_trim_rect2(struct img_rect *src_trim_rect, float zoom_ratio, float dst_ratio,
											cmr_u32 sensor_w, cmr_u32 sensor_h, cmr_u8 rot);

cmr_int camera_save_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width, cmr_u32 height, struct img_addr *addr);

cmr_int camera_get_data_from_file(char *file_name, cmr_u32 img_fmt, cmr_u32 width, cmr_u32 height, struct img_addr *addr);

cmr_int camera_parse_raw_filename(char *file_name, struct isptool_scene_param *scene_param);


void camera_snapshot_step_statisic(struct img_size *img_size);

void camera_take_snapshot_step(enum CAMERA_TAKEPIC_STEP step);
#ifdef __cplusplus
}
#endif

#endif //for _CMR_COMMON_H_

