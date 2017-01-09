/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _UAPI_VIDEO_gsp_r1p1_CFG_H
#define _UAPI_VIDEO_gsp_r1p1_CFG_H

#include <linux/ioctl.h>
#include <linux/types.h>
#include "gsp_cfg.h"

/*Original: B3B2B1B0*/
enum gsp_r1p1_word_endian {
	GSP_WORD_ENDN_0 = 0x00,     /*B3B2B1B0*/
	GSP_WORD_ENDN_1,            /*B0B1B2B3*/
	GSP_WORD_ENDN_2,            /*B2B3B0B1*/
	GSP_WORD_ENDN_3,            /*B1B0B3B2*/
	GSP_WORD_ENDN_MAX_NUM,
};

enum gsp_r1p1_rgb_swap_mod {
	GSP_RGB_SWP_RGB = 0x00,
	GSP_RGB_SWP_RBG,
	GSP_RGB_SWP_GRB,
	GSP_RGB_SWP_GBR,
	GSP_RGB_SWP_BGR,
	GSP_RGB_SWP_BRG,
	GSP_RGB_SWP_MAX,
};

enum gsp_r1p1_a_swap_mod {
	GSP_A_SWAP_ARGB,
	GSP_A_SWAP_RGBA,
	GSP_A_SWAP_MAX,
};

enum gsp_r1p1_src_layer_format {
	GSP_SRC_FMT_ARGB888 = 0x00,
	GSP_SRC_FMT_RGB888,
	GSP_SRC_FMT_ARGB565,
	GSP_SRC_FMT_RGB565,
	GSP_SRC_FMT_YUV420_2P,
	GSP_SRC_FMT_YUV420_3P,
	GSP_SRC_FMT_YUV400_1P,
	GSP_SRC_FMT_YUV422_2P,
	GSP_SRC_FMT_8BPP,
	GSP_SRC_FMT_MAX_NUM,
};

enum gsp_r1p1_des_layer_format {
	GSP_DST_FMT_ARGB888 = 0x00,
	GSP_DST_FMT_RGB888,
	GSP_DST_FMT_ARGB565,
	GSP_DST_FMT_RGB565,
	GSP_DST_FMT_YUV420_2P,
	GSP_DST_FMT_YUV420_3P,
	GSP_DST_FMT_YUV422_2P,
	GSP_DST_FMT_MAX_NUM,
};

struct gsp_r1p1_endian {
	__u32 y_word_endn;
	__u32 uv_word_endn;
	__u32 va_word_endn;
	__u32 rgb_swap_mode;
	__u32 a_swap_mode;
};

struct gsp_r1p1_img_layer_params {
	__u32			pitch;
	struct gsp_rect                 clip_rect;
	struct gsp_rect                 des_rect;
	struct gsp_rgb                  grey;
	struct gsp_rgb                  colorkey;
	struct gsp_r1p1_endian		endian;
	__u32				img_format;
	__u32				rot_angle;
	__u8				row_tap_mode;
	__u8				col_tap_mode;
	__u8				alpha;
	__u8				colorkey_en;
	__u8				pallet_en;
	__u8				scaling_en;
	__u8				pmargb_en;
	__u8				pmargb_mod;
	/* optimize narrow yuv format */
	__u8				y2r_opt;
	/* 4k boundary burst partition bypass signal */
	__u8				bnd_bypass;
};

struct gsp_r1p1_img_layer_user {
	struct gsp_layer_user		common;
	struct gsp_r1p1_img_layer_params	params;
};

struct gsp_r1p1_osd_layer_params {
	__u32                        pitch;
	struct gsp_rect                 clip_rect;
	struct gsp_pos                  des_pos;
	struct gsp_rgb                  grey;
	struct gsp_rgb                  colorkey;
	struct gsp_r1p1_endian		endian;
	__u32				osd_format;
	__u32				rot_angle;
	__u8				row_tap_mode;
	__u8				col_tap_mode;
	__u8				alpha;
	__u8				colorkey_en;
	__u8				pallet_en;
	__u8				pmargb_en;
	__u8				pmargb_mod;
};

struct gsp_r1p1_osd_layer_user {
	struct gsp_layer_user		common;
	struct gsp_r1p1_osd_layer_params	params;
};

struct gsp_r1p1_des_layer_params {
	__u32				pitch;
	struct gsp_r1p1_endian		endian;
	__u32				img_format;
	__u8				compress_r8_en;
};

struct gsp_r1p1_des_layer_user {
	struct gsp_layer_user		common;
	struct gsp_r1p1_des_layer_params	params;
};

struct gsp_r1p1_misc_cfg {
	__u8 dither_en;
	/* gsp ddr gap(0~255) */
	__u8 gsp_gap;
	/* gsp clock(0:96M 1:153.6M 2:192M 3:256M) */
	__u8 gsp_clock;
	/* ahb clock(0:26M 1:76M 2:128M 3:192M) */
	__u8 ahb_clock;
};

struct gsp_r1p1_cfg_user {
	struct gsp_r1p1_img_layer_user l0;
	struct gsp_r1p1_osd_layer_user l1;
	struct gsp_r1p1_des_layer_user ld;
	struct gsp_r1p1_misc_cfg misc;
};

struct gsp_r1p1_capability {
	struct gsp_capability common;
	/* 1: means 1/16, 64 means 4*/
	__u16 scale_range_up;
	/* 1: means 1/16, 64 means 4*/
	__u16 scale_range_down;
	__u32 yuv_xywh_even;
	__u32 max_video_size;
};

#endif
