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
#ifndef _JPEG_CODEC_H_
#define _JPEG_CODEC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "cmr_common.h"
#include "jpeg_exif_header.h"


enum cmr_jpeg_evt {
	CMR_JPEG_ENC_DONE = CMR_EVT_JPEG_BASE,
	CMR_JPEG_DEC_DONE,
	CMR_JPEG_WEXIF_DONE,
	CMR_JPEG_ENC_ERR,
	CMR_JPEG_DEC_ERR,
	CMR_JPEG_ERR,
};

struct jpeg_wexif_cb_param {
	cmr_uint                 output_buf_virt_addr;
	cmr_uint                 output_buf_size;
};

//only support YUV slice, do not support stream slice for simplicity.
struct jpeg_enc_in_param {
	cmr_u32                  src_fmt;
	cmr_u32                  slice_height;
	cmr_u32                  slice_mod;
	cmr_u32                  quality_level;
	cmr_uint                 stream_buf_phy;
	cmr_uint                 stream_buf_vir;
	cmr_u32                  stream_buf_size;
	cmr_u32                  padding;
	cmr_handle               jpeg_handle;
	struct img_size          size;
	struct img_size          out_size;
	struct img_addr          src_addr_phy;
	struct img_addr          src_addr_vir;
	struct img_data_end      src_endian;
};

struct jpeg_enc_next_param {
	cmr_uint                  jpeg_handle;
	cmr_u32                  ready_line_num;
	cmr_u32                  slice_height;
	cmr_u32			         padding;
	struct img_addr          src_addr_phy;
	struct img_addr          src_addr_vir;
};

struct jpeg_dec_in_param {
	cmr_uint                 stream_buf_phy;
	cmr_uint                 stream_buf_vir;
	cmr_u32                  stream_buf_size;
	cmr_u32                  slice_height;
	cmr_u32                  slice_mod;
	cmr_u32                  dst_fmt;
	cmr_uint                 temp_buf_phy;
	cmr_uint                 temp_buf_vir;
	cmr_u32                  temp_buf_size;
	cmr_u32                  padding;
	cmr_handle               jpeg_handle;
	struct img_size          size;
	struct img_addr          dst_addr_phy;
	struct img_addr          dst_addr_vir;
	struct img_data_end      dst_endian;
};

struct jpeg_dec_next_param {
	cmr_handle               jpeg_handle;
	cmr_u32                  slice_height;
	cmr_u32                  padding;
	struct img_addr          dst_addr_phy;
	struct img_addr          dst_addr_vir;
};

struct jpeg_enc_exif_param {
	cmr_handle               jpeg_handle;
	cmr_uint                 src_jpeg_addr_virt;
	cmr_uint                 thumbnail_addr_virt;
	cmr_uint                 target_addr_virt;
	cmr_u32                  src_jpeg_size;
	cmr_u32                  thumbnail_size;
	cmr_u32                  target_size;
	cmr_u32                  padding;
	JINF_EXIF_INFO_T         *exif_ptr;
	EXIF_ISP_INFO_T          *exif_isp_info;
};

cmr_int jpeg_init(cmr_handle oem_handle, cmr_handle *jpeg_handle);
cmr_int jpeg_enc_start(struct jpeg_enc_in_param *start_in_parm_ptr);
cmr_int jpeg_enc_next(struct jpeg_enc_next_param *nxt_param_ptr);
cmr_int jpeg_dec_start(struct jpeg_dec_in_param *start_in_parm_ptr);
cmr_int jpeg_dec_next(struct jpeg_dec_next_param *next_param_ptr);
cmr_int jpeg_stop(cmr_handle jpeg_handle);
cmr_int jpeg_deinit(cmr_handle jpeg_handle);
cmr_int jpeg_enc_add_eixf(struct jpeg_enc_exif_param *param_ptr, struct jpeg_wexif_cb_param *output_ptr);
cmr_int jpeg_enc_thumbnail(struct jpeg_enc_in_param *in_parm_ptr, cmr_uint *stream_size_ptr);
cmr_int jpeg_dec_start_sync( struct jpeg_dec_in_param *in_parm_ptr, struct jpeg_dec_cb_param *out_parm_ptr);
void jpeg_evt_reg(cmr_handle jpeg_handle, cmr_evt_cb  adp_event_cb);

#ifdef __cplusplus
}
#endif

#endif //for _JPEG_CODEC_H_


