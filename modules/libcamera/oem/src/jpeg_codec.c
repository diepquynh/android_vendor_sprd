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
#include "exif_writer.h"
#include "cmr_msg.h"
#include "jpeg_codec.h"
#include "cmr_common.h"
#include "jpegdec_api.h"
#include "jpegenc_api.h"

/*#define JPEG_CODE_DEBUG*/
#undef LOG_TAG
#define LOG_TAG "jpeg_codec"

#define JPEG_MSG_QUEUE_SIZE                   40
#define JPEG_EXIT_THREAD_FLAG                 1
#define JPEG_SLICE_HEIGHT                     4224
#define JPEG_BUF_RES_SIZE                     256
#define JPEG_DECODE_FW_BUF_SIZE               (20*1024)
#define JPEG_WEXIF_TEMP_MARGIN                (21*1024)
#define JPEG_EVT_ENC_START                    (1 << 16)
#define JPEG_EVT_ENC_NEXT                     (1 << 17)
#define JPEG_EVT_DEC_START                    (1 << 18)
#define JPEG_EVT_DEC_NEXT                     (1 << 19)
#define JPEG_EVT_STOP                         (1 << 20)
#define JPEG_EVT_KILL                         (1 << 21)
#define JPEG_EVT_ENC_EXIF                     (1 << 22)
#define JPEG_EVT_ENC_THUMB                    (1 << 23)

#define JPEG_EVT_MASK_BITS                    (cmr_u32)(JPEG_EVT_ENC_START | JPEG_EVT_ENC_NEXT | \
		                                               JPEG_EVT_DEC_START | JPEG_EVT_DEC_NEXT |  \
		                                               JPEG_EVT_STOP | JPEG_EVT_KILL | \
		                                               JPEG_EVT_ENC_EXIF | JPEG_EVT_ENC_THUMB)

enum cmr_jpeg_ret {
	JPEG_CODEC_SUCCESS = 0,
	JPEG_CODEC_PARAM_ERR,
	JPEG_CODEC_INVALID_HANDLE,
	JPEG_CODEC_NO_MEM,
	JPEG_CODEC_ENC_WAIT_SRC,
	JPEG_CODEC_ERROR,
	JPEG_CODEC_STOP
};

struct jpeg_codec_context {
	cmr_handle                 oem_handle;
	cmr_handle                 active_handle;
	void                       *fw_decode_buf;
	cmr_u32                    fw_decode_buf_size;
	cmr_u32                    err_code;
	cmr_handle                 thread_handle;
	cmr_u32                    sync_flag;
	cmr_u32                    type;
	cmr_u32                    is_stop;
	cmr_u32                    padding;
	sem_t                      access_sem;
	cmr_evt_cb                 event_cb;
	struct jpeg_wexif_cb_param exif_output;
	struct jpeg_enc_cb_param   thumbnail_info;
	struct jpeg_dec_cb_param   dec_output;
};

struct jpeg_dec {
	cmr_uint                   stream_buf_phy;
	cmr_uint                   stream_buf_vir;
	cmr_u32             	   stream_buf_size;
	cmr_u32             	   slice_height;/*slice height must be  8X*/
	cmr_u32             	   set_slice_height;
	cmr_u32             	   slice_mod;
	cmr_u32             	   dst_fmt;
	cmr_u32             	   fw_decode_buf_size;
	void *              	   fw_decode_buf;
	cmr_uint            	   temp_buf_phy;
	cmr_uint            	   temp_buf_vir;
	cmr_u32             	   temp_buf_size;
	cmr_u32             	   cur_line_num;
	cmr_u32             	   handle_line_num;
	cmr_u32             	   is_finish;
	struct img_addr     	   dst_addr_phy;
	struct img_addr     	   dst_addr_vir;
	struct img_data_end		   dst_endian;
	struct img_size     	   size;
};

struct jpeg_enc {
	cmr_u32             	   src_fmt;
	cmr_u32             	   slice_height;/*slice height must be  8X*/
	cmr_u32             	   slice_mod;
	cmr_u32             	   quality_level;
	cmr_uint            	   stream_buf_phy;
	cmr_uint            	   stream_buf_vir;
	cmr_u32              	   stream_buf_size;
	cmr_u32             	   stream_real_size;
	cmr_u32             	   cur_line_num;
	cmr_u32                    is_finish;
	cmr_u32                    is_thumbnail;
	cmr_u32                    padding;
	struct img_size            size;
	struct img_size            out_size;
	struct img_addr            src_addr_phy;
	struct img_addr            src_addr_vir;
	struct img_data_end        src_endian;
};

static cmr_int _dec_next(cmr_handle dec_handle, struct jpeg_codec_context  *jcontext, struct jpeg_dec_next_param *param_ptr);
static cmr_int jpeg_thread_proc(struct cmr_msg *message, void* data);
static cmr_int _jpeg_stop(cmr_handle jpeg_handle);


static void savedata(cmr_uint buf_addr, cmr_u32 size)
{
	FILE                  *fp = NULL;

	CMR_LOGV("jpeg:savedata");
	fp = fopen("/data/out.raw", "wb");
	if (0 != fp) {
		fwrite((void*)buf_addr, 1, size, fp);
		fclose(fp);
	} else {
		CMR_LOGE("jpeg:can not create savedata");
	}
}

static void save_inputdata(cmr_uint y_buf_addr, cmr_uint uv_buf_addr, cmr_u32 size)
{
	FILE                  *fp = NULL;

	CMR_LOGV("jpeg: save input data,size=%d.",size);
	fp = fopen("/data/in_y.raw", "wb");
	if (0 != fp) {
		fwrite((void*)y_buf_addr, 1, size, fp);
		fclose(fp);
	} else {
		CMR_LOGE("jpeg:can not create savedata");
	}
	fp = fopen("/data/in_uv.raw", "wb");
	if (0 != fp) {
		fwrite((void*)uv_buf_addr, 1, size/2, fp);
		fclose(fp);
	} else {
		CMR_LOGW("jpeg:can not create savedata");
	}
}

static cmr_u32 _format_covert(cmr_u32 format)
{
	cmr_u32                  jfmt = JPEGENC_YUV_420;

	switch(format) {
	case IMG_DATA_TYPE_YUV422:
		jfmt = JPEGENC_YUV_422;
		break;

	default:
		CMR_LOGW("JPEG, unknow format");
		break;
	}
	return jfmt;
}

static cmr_u32 _quality_covert(cmr_u32 quality)
{
	cmr_u32                  jq = JPEGENC_QUALITY_HIGH;

	if (quality <= 70) {
		jq = JPEGENC_QUALITY_LOW;
	} else if (quality <= 80) {
		jq = JPEGENC_QUALITY_MIDDLE_LOW;
	} else if (quality <= 85) {
		jq = JPEGENC_QUALITY_MIDDLE;
	} else if (quality <= 90) {
		jq = JPEGENC_QUALITY_MIDDLE_HIGH;
	} else {
		jq = JPEGENC_QUALITY_HIGH;
	}

	return jq;
}

static void _prc_enc_cbparam(cmr_handle handle, struct jpeg_enc_cb_param *parm_ptr)
{
	struct jpeg_enc                  *enc_cxt_ptr = NULL;
	/*FILE                           *fp = NULL;*/

	enc_cxt_ptr = (struct jpeg_enc*)handle;
	parm_ptr->stream_buf_phy = enc_cxt_ptr->stream_buf_phy;
	parm_ptr->stream_buf_vir = enc_cxt_ptr->stream_buf_vir;
	parm_ptr->stream_size = enc_cxt_ptr->stream_real_size;
	parm_ptr->slice_height = enc_cxt_ptr->slice_height;
	parm_ptr->total_height = enc_cxt_ptr->cur_line_num;

	if ((enc_cxt_ptr->cur_line_num == enc_cxt_ptr->size.height) && (1 != parm_ptr->is_thumbnail)) {
		CMR_LOGI("jpeg:adjust %d,%d.",enc_cxt_ptr->out_size.width,enc_cxt_ptr->out_size.height);
		adjust_jpg_resolution((void*)parm_ptr->stream_buf_vir, parm_ptr->stream_size,
								enc_cxt_ptr->out_size.width, enc_cxt_ptr->out_size.height);
	}
#ifdef JPEG_CODE_DEBUG
	CMR_LOGV("slice_height %d,total_height %d,stream_size 0x%x.",
				parm_ptr->slice_height,parm_ptr->total_height,parm_ptr->stream_size);
#endif
	/*fp = fopen("/data/1.aw", "wb");
	if(0 != fp) {
		fwrite((void*)parm_ptr->stream_buf_vir, 1, parm_ptr->stream_size, fp);
		CMR_LOGV("save jpg.");
		fclose(fp);
	}*/
	return;
}

static cmr_u32 _jpeg_enc_is_done(struct jpeg_enc  *enc_cxt_ptr)
{
	CMR_LOGI("jpeg:%d", enc_cxt_ptr->is_finish);
	return enc_cxt_ptr->is_finish;
}

static cmr_u32 _jpeg_dec_is_done(struct jpeg_dec  *dec_cxt_ptr)
{
	CMR_LOGI("jpeg:%d", dec_cxt_ptr->is_finish);
	return dec_cxt_ptr->is_finish;
}

static cmr_u32 _jpeg_is_stop(struct jpeg_codec_context  *jcontext)
{
	CMR_LOGI("jpeg:%d", jcontext->is_stop);
	return jcontext->is_stop;
}

static cmr_int _enc_start(cmr_handle handle, struct jpeg_codec_context *jcontext)
{
	cmr_int                   ret = JPEG_CODEC_SUCCESS;
	cmr_uint                  jpeg_enc_buf_phys_addr;
	cmr_uint                  *jpeg_enc_buf_virt_addr;
	cmr_u32                   jpeg_enc_buf_len;
	cmr_u32                   i = 0;
	cmr_u32                   jpeg_ret = 0;
	struct jpeg_enc           *enc_cxt_ptr = NULL;
	JPEGENC_SLICE_OUT_T       slice_out;
	JPEGENC_SLICE_NEXT_T      next_slice_parm;
	JPEGENC_PARAMS_T          *jenc_parm_ptr = (JPEGENC_PARAMS_T *)malloc(sizeof(JPEGENC_PARAMS_T));

	CMR_LOGV("jpeg:_encoder_start: S");

	memset((void*)&slice_out,0,sizeof(JPEGENC_SLICE_OUT_T));
	memset((void*)&next_slice_parm,0,sizeof(JPEGENC_SLICE_NEXT_T));
	if (NULL == jenc_parm_ptr) {
		CMR_LOGE("jpeg:malloc fail");
		return JPEG_CODEC_NO_MEM;
	}

	enc_cxt_ptr = (struct jpeg_enc *)handle;
	if (enc_cxt_ptr->slice_height == enc_cxt_ptr->size.height ) {
		jenc_parm_ptr->set_slice_height = JPEG_SLICE_HEIGHT;
	} else {
		jenc_parm_ptr->set_slice_height = enc_cxt_ptr->slice_height;
	}
	if (1 == enc_cxt_ptr->is_thumbnail) {
		jenc_parm_ptr->set_slice_height = enc_cxt_ptr->size.height;
		CMR_LOGV("jpeg:thumbnail, set_slice_height = %d", jenc_parm_ptr->set_slice_height);
	}

	jenc_parm_ptr->format = JPEGENC_YUV_420;
	jenc_parm_ptr->quality = _quality_covert(enc_cxt_ptr->quality_level);
	jenc_parm_ptr->width = enc_cxt_ptr->size.width;
	jenc_parm_ptr->height = enc_cxt_ptr->size.height;
	jenc_parm_ptr->yuv_virt_buf = (void *)enc_cxt_ptr->src_addr_vir.addr_y;
	jenc_parm_ptr->yuv_phy_buf = enc_cxt_ptr->src_addr_phy.addr_y;
	jenc_parm_ptr->yuv_u_virt_buf = (void *)enc_cxt_ptr->src_addr_vir.addr_u;
	jenc_parm_ptr->yuv_u_phy_buf = enc_cxt_ptr->src_addr_phy.addr_u;
	jenc_parm_ptr->yuv_v_virt_buf = (void*)0;
	jenc_parm_ptr->yuv_v_phy_buf = 0;
	jenc_parm_ptr->y_interleaved = enc_cxt_ptr->src_endian.y_endian;
	jenc_parm_ptr->uv_interleaved = enc_cxt_ptr->src_endian.uv_endian;
	CMR_LOGI("uv endian, %d y endian %d", enc_cxt_ptr->src_endian.uv_endian, enc_cxt_ptr->src_endian.y_endian);
#ifdef JPEG_CODE_DEBUG
	CMR_LOGV("jpeg:enc yuv phy addr,0x%x 0x%x 0x%x,slice height %d.",
			 jenc_parm_ptr->yuv_phy_buf, jenc_parm_ptr->yuv_u_phy_buf,
			 jenc_parm_ptr->yuv_v_phy_buf, enc_cxt_ptr->slice_height);
#endif
	jpeg_enc_buf_virt_addr = (void *)enc_cxt_ptr->stream_buf_vir;
	jpeg_enc_buf_len = enc_cxt_ptr->stream_buf_size;
	jpeg_enc_buf_phys_addr = enc_cxt_ptr->stream_buf_phy;
	jenc_parm_ptr->stream_virt_buf[0] = jpeg_enc_buf_virt_addr;
	jenc_parm_ptr->stream_phy_buf[0] = jpeg_enc_buf_phys_addr;
#ifdef JPEG_CODE_DEBUG
	/*save_inputdata(enc_cxt_ptr->src_addr_vir.addr_y,
		enc_cxt_ptr->src_addr_vir.addr_u,
		enc_cxt_ptr->size.width*enc_cxt_ptr->size.height);*/
	CMR_LOGV("jpeg:jpegenc_params[%d]: virt: %x, phys: %x,size %d.",
			  i,(cmr_uint)jenc_parm_ptr->stream_virt_buf[i],
			  jenc_parm_ptr->stream_phy_buf[i],jpeg_enc_buf_len);
#endif
	jenc_parm_ptr->stream_buf_len = jpeg_enc_buf_len;
	jenc_parm_ptr->stream_size = 0;

	if (_jpeg_is_stop(jcontext)) {
		CMR_LOGI("jpeg: cancel");
		free(jenc_parm_ptr);
		return JPEG_CODEC_STOP;
	}

	/*start jpeg enc for both slice and frame*/
	if (0 != JPEGENC_Slice_Start(jenc_parm_ptr, &slice_out)) {
		ret = JPEG_CODEC_ERROR;
		goto enc_start_end;
	}
	enc_cxt_ptr->cur_line_num = jenc_parm_ptr->set_slice_height;
	/*if frame, and still use slice mode for sc8810 to keep the same  interface to top layer*/
	if (enc_cxt_ptr->slice_height == enc_cxt_ptr->size.height ) {
		cmr_u32   cur_slice_height = jenc_parm_ptr->set_slice_height;
		cmr_s32   slice_num = (cmr_u32)(enc_cxt_ptr->size.height/cur_slice_height);
		cmr_u32   cur_ver_pos = enc_cxt_ptr->cur_line_num;
		cmr_u32   buf_id = 1;
		cmr_u32   cur_y_buf_adr = 0;
		cmr_u32   cur_u_buf_adr = 0;
#ifdef JPEG_CODE_DEBUG
		CMR_LOGV("jpeg:slice mode for frame");
#endif
		if (0 != enc_cxt_ptr->size.height%cur_slice_height) {
			slice_num = slice_num + 1;
		}

		CMR_LOGV("jpeg:slice num: %d", slice_num);
		slice_num--;/*start has process one slice*/
		if (0 != slice_num) {
			do {
				if (_jpeg_is_stop(jcontext)) {
					CMR_LOGI("jpeg:cancel jpeg");
					return JPEG_CODEC_STOP;
				}
				next_slice_parm.slice_height = jenc_parm_ptr->set_slice_height;
				next_slice_parm.yuv_phy_buf = enc_cxt_ptr->src_addr_phy.addr_y+enc_cxt_ptr->cur_line_num* enc_cxt_ptr->size.width;
				next_slice_parm.yuv_u_phy_buf = enc_cxt_ptr->src_addr_phy.addr_u+enc_cxt_ptr->cur_line_num* enc_cxt_ptr->size.width/2;
				next_slice_parm.yuv_v_phy_buf = 0;
				jpeg_ret = JPEGENC_Slice_Next(&next_slice_parm, &slice_out);
				if (0 != jpeg_ret && 1 != jpeg_ret) {
					CMR_LOGE("jpeg:error %d.", jpeg_ret);
					ret =  JPEG_CODEC_ERROR;
					break;
				}
				enc_cxt_ptr->cur_line_num += cur_slice_height;
				if (1 == slice_out.is_over) {
					enc_cxt_ptr->is_finish = 1;
					enc_cxt_ptr->stream_real_size = slice_out.stream_size;
					enc_cxt_ptr->cur_line_num = enc_cxt_ptr->size.height;
					break;
				}
				slice_num--;
			}while(0 < slice_num);
		} else {
			enc_cxt_ptr->is_finish = 1;
			enc_cxt_ptr->stream_real_size = slice_out.stream_size;
			enc_cxt_ptr->cur_line_num = enc_cxt_ptr->size.height;
		}
		CMR_LOGV("jpeg:slice_num %d",slice_num);
	}

	CMR_LOGV("jpeg:buf addr:0x%lx, size: %d", enc_cxt_ptr->stream_buf_vir, enc_cxt_ptr->stream_real_size);

	/*savedata(enc_cxt_ptr->stream_buf_vir, enc_cxt_ptr->stream_real_size);*/
enc_start_end:
	free(jenc_parm_ptr);
	CMR_LOGV("jpeg:_encoder_start E.");
	return ret;
}


static void _enc_start_post(cmr_handle handle, struct jpeg_codec_context  *jcontext, cmr_int ret)
{
	struct jpeg_enc           *enc_cxt_ptr = (struct jpeg_enc*)handle;

	if (JPEG_CODEC_SUCCESS == ret) {
		if (!_jpeg_is_stop(jcontext)) {
			struct jpeg_enc_cb_param   param;
			memset((void*)&param,0,sizeof(struct jpeg_enc_cb_param));
			param.is_thumbnail = 0;
			_prc_enc_cbparam(handle, &param);
			if (NULL != jcontext->event_cb) {
				jcontext->event_cb(CMR_JPEG_ENC_DONE, &param, (void*)jcontext->oem_handle);
			} else {
				CMR_LOGE("jpeg:even cb is NULL.");
			}
			if (_jpeg_enc_is_done((struct jpeg_enc  *)handle)) {
				if (_jpeg_stop((cmr_handle)jcontext)) {
					CMR_LOGE("stop fail");
				}
			}
		} else {
			if (_jpeg_stop((cmr_handle)jcontext)) {
				CMR_LOGE("jpeg:stop fail");
			}
		}
	} else {
		if (JPEG_CODEC_STOP != ret) {
			if (NULL != jcontext->event_cb) {
				jcontext->event_cb(CMR_JPEG_ENC_ERR, NULL , (void*)jcontext->oem_handle);
			} else {
				CMR_LOGE("jpeg:even cb is NULL.");
			}
		} else {
			CMR_LOGI("jpeg:jpeg is stopped");
		}
		if (_jpeg_stop((cmr_handle)jcontext)) {
			CMR_LOGE("jpeg:stop fail");
		}
	}
}

/*assume the slice height is the same, except the last one*/
static cmr_int _enc_next(cmr_handle handle, struct jpeg_codec_context *jcontext, struct jpeg_enc_next_param *param_ptr)
{
	cmr_int                  ret = JPEG_CODEC_SUCCESS;
	cmr_uint                 jpeg_enc_buf_phys_addr;
	cmr_uint                 *jpeg_enc_buf_virt_addr;
	cmr_u32                  jpeg_enc_buf_len;
	cmr_u32                  i;
	cmr_u32                  cur_line_num = 0;
	cmr_u32                  cur_slice_height;
	cmr_u32                  slice_num = 0;
	cmr_u32                  cur_ver_pos = 0;
	cmr_u32                  buf_id = 0;
	cmr_uint                 cur_y_buf_adr = 0;
	cmr_uint                 cur_u_buf_adr = 0;
	JPEGENC_SLICE_NEXT_T     update_parm;
	struct jpeg_enc          *enc_cxt_ptr = NULL;
	JPEGENC_SLICE_OUT_T      slice_out;

	CMR_LOGV("jpeg:_enc_next: S");

	enc_cxt_ptr = (struct jpeg_enc *)handle;
	if (((enc_cxt_ptr->cur_line_num + enc_cxt_ptr->slice_height)>param_ptr->ready_line_num)
		&& (param_ptr->ready_line_num != enc_cxt_ptr->size.height)) {
		CMR_LOGI("jpeg:ready line num don't enough for a slice.enc line num %d,read line num %d",
			      enc_cxt_ptr->cur_line_num,param_ptr->ready_line_num);
		return JPEG_CODEC_ENC_WAIT_SRC;
	}
	memset((void*)&update_parm,0,sizeof(JPEGENC_SLICE_NEXT_T));
	memset((void*)&slice_out,0,sizeof(JPEGENC_SLICE_OUT_T));

	if (0 != param_ptr->src_addr_phy.addr_y) {
		update_parm.yuv_phy_buf = param_ptr->src_addr_phy.addr_y;
		update_parm.yuv_u_phy_buf = param_ptr->src_addr_phy.addr_u;
		update_parm.yuv_v_phy_buf = param_ptr->src_addr_phy.addr_v;
		cur_slice_height = param_ptr->slice_height;
	} else {
		cur_line_num = enc_cxt_ptr->cur_line_num;
		if (0 != param_ptr->slice_height) {
			cur_slice_height = param_ptr->slice_height;
		} else {
			cur_slice_height = enc_cxt_ptr->slice_height;
		}

		if (enc_cxt_ptr->cur_line_num >= enc_cxt_ptr->size.height) {
			return JPEG_CODEC_ERROR;
		}

		if (((cur_line_num + enc_cxt_ptr->slice_height) > param_ptr->ready_line_num)
				&& (param_ptr->ready_line_num == enc_cxt_ptr->size.height)) {
			cur_slice_height = param_ptr->ready_line_num - cur_line_num;
		}

		update_parm.yuv_phy_buf = enc_cxt_ptr->src_addr_phy.addr_y + cur_line_num*enc_cxt_ptr->size.width;
		update_parm.yuv_u_phy_buf = enc_cxt_ptr->src_addr_phy.addr_u + cur_line_num*enc_cxt_ptr->size.width/2;
		update_parm.yuv_v_phy_buf = enc_cxt_ptr->src_addr_phy.addr_v;
	}
	update_parm.slice_height = cur_slice_height;
	CMR_LOGI("jpeg:cur_line_num %d, addr y 0x%x,addr u 0x%x", cur_line_num,
			 update_parm.yuv_phy_buf, update_parm.yuv_u_phy_buf);

	if (_jpeg_is_stop(jcontext)) {
		CMR_LOGI("jpeg:cancel");
		return JPEG_CODEC_STOP;
	}

 	/*encode the jpeg picture by HW.*/
	if (0 != JPEGENC_Slice_Next(&update_parm, &slice_out)) {
		ret = JPEG_CODEC_ERROR;
	}
	enc_cxt_ptr->cur_line_num += cur_slice_height;
	enc_cxt_ptr->slice_height = cur_slice_height;
	if (enc_cxt_ptr->cur_line_num == enc_cxt_ptr->size.height) {
		enc_cxt_ptr->is_finish = 1;
		enc_cxt_ptr->stream_real_size = slice_out.stream_size;
		enc_cxt_ptr->cur_line_num = enc_cxt_ptr->size.height;
	}

	CMR_LOGV("jpeg:_encoder_start E.");
	return ret;
}

static void _enc_next_post(cmr_handle handle, struct jpeg_codec_context  *jcontext, cmr_int ret)
{
	struct jpeg_enc                  *enc_cxt_ptr = (struct jpeg_enc*)handle;

	if (JPEG_CODEC_ENC_WAIT_SRC != ret) {
		if (JPEG_CODEC_SUCCESS == ret) {
			if (!_jpeg_is_stop(jcontext)) {
				struct jpeg_enc_cb_param param;
				memset((void*)&param,0,sizeof(struct jpeg_enc_cb_param));
				param.is_thumbnail = 0;
				_prc_enc_cbparam((cmr_handle)enc_cxt_ptr, &param);
				if (NULL != jcontext->event_cb) {
					jcontext->event_cb(CMR_JPEG_ENC_DONE, &param, (void*)jcontext->oem_handle);
				} else {
					CMR_LOGE("jpeg:even cb is NULL.");
				}
				if (_jpeg_enc_is_done((struct jpeg_enc*)handle)) {
					if (_jpeg_stop((cmr_handle)jcontext)) {
						CMR_LOGE("jpeg:stop fail");
					}
				}
			} else {
				if (_jpeg_stop((cmr_handle)jcontext)) {
					CMR_LOGE("jpeg:stop fail");
				}
			}
		} else {
			if (NULL != jcontext->event_cb) {
				jcontext->event_cb(CMR_JPEG_ENC_ERR, NULL, (void*)jcontext->oem_handle);
			} else {
				CMR_LOGE("jpeg:even cb is NULL.");
			}
			if (_jpeg_stop((cmr_handle)jcontext)) {
				CMR_LOGE("jpeg:stop fail");
			}
		}
	} else {
		CMR_LOGI("jpeg:receive enc next message.");
	}
}

static void _dec_callback(cmr_u32 buf_id, cmr_u32 stream_size,
							     cmr_u32 is_last_slice, struct jpeg_codec_context *jcontext)
{
	struct jpeg_dec            *dec_cxt_ptr = (struct jpeg_dec*)jcontext->active_handle;
	cmr_u32                    cpy_height = 0;
	cmr_uint                   src, dst;
	cmr_u32                    i;
	struct img_frm             img_frm;
	struct jpeg_dec_cb_param   param;

	param.data_endian.y_endian = 1;
	param.data_endian.uv_endian = 1;

	param.src_img = &img_frm;
	if (dec_cxt_ptr->cur_line_num + dec_cxt_ptr->set_slice_height > dec_cxt_ptr->size.height) {
		cpy_height = dec_cxt_ptr->size.height - dec_cxt_ptr->cur_line_num;
		dec_cxt_ptr->is_finish = 1;
	} else {
		cpy_height = dec_cxt_ptr->set_slice_height;
	}
	param.slice_height = cpy_height;
	CMR_LOGV("jpeg:dec handle 0x%p, line number %d, cpy_height %d",
			 dec_cxt_ptr,dec_cxt_ptr->cur_line_num, cpy_height);

	dst = dec_cxt_ptr->dst_addr_vir.addr_u;
	param.src_img->addr_vir.addr_u = dst;
	param.src_img->addr_phy.addr_u = dec_cxt_ptr->dst_addr_phy.addr_u;
	src = dec_cxt_ptr->temp_buf_vir;
	CMR_LOGV("jpeg:copy uv,src 0x%x, dst 0x%x", (cmr_u32)src, (cmr_u32)dst);
	for (i = 0; i < (cpy_height >> 1); i++) {
		memcpy((void*)dst,(void*)src,dec_cxt_ptr->size.width);
		dst += dec_cxt_ptr->size.width;
		src += (dec_cxt_ptr->size.width << 1);
	}

	dec_cxt_ptr->cur_line_num += cpy_height;
	dec_cxt_ptr->handle_line_num = cpy_height;
	param.total_height = dec_cxt_ptr->cur_line_num;
	param.src_img->data_end = dec_cxt_ptr->dst_endian;
	param.src_img->fmt = dec_cxt_ptr->dst_fmt;
	param.src_img->size = dec_cxt_ptr->size;
	if ((dec_cxt_ptr->slice_height != dec_cxt_ptr->size.height)
		||(param.total_height == dec_cxt_ptr->size.height)) {
		if ((NULL != jcontext->event_cb) && (1 != jcontext->sync_flag)) {
			jcontext->event_cb(CMR_JPEG_DEC_DONE, &param, (void*)jcontext->oem_handle);
		} else {
			memcpy((void*)&jcontext->dec_output,(void*)&param,
					sizeof(struct jpeg_dec_cb_param));
			CMR_LOGE("jpeg:even cb is NULL.");
		}
	}

	return;
}

static cmr_int _dec_start(cmr_handle handle, struct jpeg_codec_context *jcontext)
{
	cmr_int                        ret = JPEG_CODEC_SUCCESS;
	struct jpeg_dec                *dec_cxt_ptr = (struct jpeg_dec *)handle;
	JPEGDEC_PARAMS_T               jpegdec_params;
	JPEGDEC_SLICE_OUT_T            slice_out;
	struct jpeg_dec_next_param     next_param;

	CMR_LOGI("jpeg:dec slice height %d.", dec_cxt_ptr->slice_height);

	dec_cxt_ptr->cur_line_num = 0;
	dec_cxt_ptr->is_finish = 0;
	dec_cxt_ptr->fw_decode_buf_size = jcontext->fw_decode_buf_size;
	dec_cxt_ptr->fw_decode_buf = jcontext->fw_decode_buf;

	jpegdec_params.format = JPEGDEC_YUV_422;
	jpegdec_params.width = dec_cxt_ptr->size.width;
	jpegdec_params.height = dec_cxt_ptr->size.height;
	jpegdec_params.stream_size = dec_cxt_ptr->stream_buf_size;
	CMR_LOGI(" %d", jpegdec_params.stream_size);
	jpegdec_params.src_buf = (void *)dec_cxt_ptr->stream_buf_vir;
	jpegdec_params.src_phy_buf = dec_cxt_ptr->stream_buf_phy;
	jpegdec_params.target_buf_Y = (void *)dec_cxt_ptr->dst_addr_vir.addr_y;
	jpegdec_params.target_phy_buf_Y = dec_cxt_ptr->dst_addr_phy.addr_y;
	jpegdec_params.target_buf_UV = (void *)dec_cxt_ptr->temp_buf_vir;
	jpegdec_params.target_phy_buf_UV = dec_cxt_ptr->temp_buf_phy;

	jpegdec_params.fw_decode_buf = dec_cxt_ptr->fw_decode_buf;
	jpegdec_params.fw_decode_buf_size = dec_cxt_ptr->fw_decode_buf_size;

	jpegdec_params.stream_virt_buf[0] = jpegdec_params.src_buf;
	jpegdec_params.stream_phy_buf[0] = jpegdec_params.src_phy_buf;

	jpegdec_params.stream_virt_buf[1] = 0;
	jpegdec_params.stream_buf_len = jpegdec_params.stream_size;
	jpegdec_params.yuv_virt_buf = jpegdec_params.target_buf_Y;
	jpegdec_params.yuv_phy_buf = jpegdec_params.target_phy_buf_Y;
	jpegdec_params.set_slice_height = dec_cxt_ptr->slice_height;
	dec_cxt_ptr->set_slice_height = dec_cxt_ptr->slice_height;
	if (dec_cxt_ptr->slice_height == dec_cxt_ptr->size.height) {
		jpegdec_params.set_slice_height = JPEG_SLICE_HEIGHT;
		dec_cxt_ptr->set_slice_height = JPEG_SLICE_HEIGHT;
	}
	if (_jpeg_is_stop(jcontext)) {
		CMR_LOGI("jpeg:cancel jpeg");
		return JPEG_CODEC_STOP;
	}
	memset(&slice_out,0,sizeof(JPEGDEC_SLICE_OUT_T));
	if (0 != JPEGDEC_Slice_Start(&jpegdec_params, &slice_out)) {
		ret = JPEG_CODEC_ERROR;
	}

	if (JPEG_CODEC_SUCCESS == ret) {
		if (dec_cxt_ptr->slice_height == dec_cxt_ptr->size.height) {
			while(1 != slice_out.is_over) {
				if (_jpeg_is_stop(jcontext)) {
					CMR_LOGI("jpeg:cancel");
					return JPEG_CODEC_STOP;
				}
				_dec_callback(0,0,0,jcontext);
				next_param.dst_addr_phy.addr_y = 0;
				next_param.slice_height = JPEG_SLICE_HEIGHT;
				if (JPEG_CODEC_SUCCESS != _dec_next(handle, jcontext, &next_param)) {
					CMR_LOGE("jpeg:dec next error!");
					return JPEG_CODEC_ERROR;
				}
			}
		}
	}
	if (1 == slice_out.is_over) {
		dec_cxt_ptr->is_finish = 1;
		CMR_LOGI("jpeg:dec finish.");
	}
	CMR_LOGI("jpeg:dec start end.");
	return ret;
}

static void _dec_start_post(cmr_handle handle, struct jpeg_codec_context *jcontext, cmr_int ret)
{
	if (JPEG_CODEC_SUCCESS == ret) {
		if (!_jpeg_is_stop(jcontext)) {
			_dec_callback(0,0,0,jcontext);
			if (_jpeg_dec_is_done((struct jpeg_dec  *)handle)) {
				if (_jpeg_stop((cmr_handle)jcontext)) {
					CMR_LOGE("jpeg:stop fail");
				}
			}
		} else {
			if (_jpeg_stop((cmr_handle)jcontext)) {
				CMR_LOGE("jpeg:stop fail");
			}
		}
	} else {
		if ((NULL != jcontext->event_cb)&&(1 != jcontext->sync_flag)) {
			jcontext->event_cb(CMR_JPEG_DEC_ERR, NULL, (void*)jcontext->oem_handle);
		} else {
			jcontext->err_code = ret;
			CMR_LOGE("jpeg:even cb is NULL.");
		}
		if (_jpeg_stop((cmr_handle)jcontext)) {
			CMR_LOGE("jpeg:stop fail");
		}
	}
}

static cmr_int _dec_next(cmr_handle handle, struct jpeg_codec_context  *jcontext, struct jpeg_dec_next_param *param_ptr)
{
	cmr_int                  ret = JPEG_CODEC_SUCCESS;
	JPEGDEC_SLICE_OUT_T      slice_out;
	JPEGDEC_SLICE_NEXT_T     slice_param;
	struct jpeg_dec          *dec_cxt_ptr = NULL;
	cmr_u32                  dec_line_num = 0;

	CMR_LOGI("jpeg:handle :0x%p", handle);

	if (0 == handle) {
		CMR_LOGE("jpeg:handle is NULL.");
		return JPEG_CODEC_PARAM_ERR;
	}

	dec_cxt_ptr = (struct jpeg_dec * )handle;
	memset(&slice_param,0,sizeof(slice_param));
	memset(&slice_out,0,sizeof(JPEGDEC_SLICE_OUT_T));
	if (0 == param_ptr->dst_addr_phy.addr_y) {
		CMR_LOGI("jpeg:one buffer.");
		slice_param.slice_height = dec_cxt_ptr->slice_height;
		slice_param.yuv_phy_buf = dec_cxt_ptr->dst_addr_phy.addr_y + dec_cxt_ptr->cur_line_num*dec_cxt_ptr->size.width;
		dec_cxt_ptr->dst_addr_phy.addr_u += dec_cxt_ptr->handle_line_num*dec_cxt_ptr->size.width>>1;
		dec_cxt_ptr->dst_addr_vir.addr_u += dec_cxt_ptr->handle_line_num*dec_cxt_ptr->size.width>>1;
	} else {
		slice_param.slice_height = param_ptr->slice_height;
		slice_param.yuv_phy_buf = param_ptr->dst_addr_phy.addr_y;
		dec_cxt_ptr->dst_addr_phy.addr_u = param_ptr->dst_addr_phy.addr_u;
		dec_cxt_ptr->dst_addr_vir.addr_u = param_ptr->dst_addr_vir.addr_u;
		dec_cxt_ptr->dst_addr_phy.addr_v = param_ptr->dst_addr_phy.addr_v;
		dec_cxt_ptr->dst_addr_vir.addr_v = param_ptr->dst_addr_vir.addr_v;
	}
	slice_param.yuv_u_phy_buf = dec_cxt_ptr->temp_buf_phy;
	if (_jpeg_is_stop(jcontext)) {
		CMR_LOGI("jpeg:cancel");
		return JPEG_CODEC_STOP;
	}
	CMR_LOGI("jpeg:update addr:0x%x,0x%x.",slice_param.yuv_phy_buf,slice_param.yuv_u_phy_buf);
	if (JPEG_CODEC_SUCCESS != JPEGDEC_Slice_Next(&slice_param,&slice_out)) {
		ret = JPEG_CODEC_ERROR;
		CMR_LOGE("jpeg:dec next error!");
	}

	if (1 == slice_out.is_over) {
		dec_cxt_ptr->is_finish = 1;
		CMR_LOGI("jpeg:dec finish.");
	}

	CMR_LOGI("jpeg:dec next done, height:%d.",dec_cxt_ptr->cur_line_num);
	return ret;
}

static void _dec_next_post(cmr_handle handle, struct jpeg_codec_context  *jcontext, cmr_int ret)
{
	if (JPEG_CODEC_SUCCESS == ret) {
		if (!_jpeg_is_stop(jcontext)) {
			_dec_callback(0,0,0,jcontext);
			if (_jpeg_dec_is_done((struct jpeg_dec  *)handle)) {
				if (_jpeg_stop((cmr_handle)jcontext)) {
					CMR_LOGE("jpeg:stop fail");
				}
			}
		} else {
			if (_jpeg_stop((cmr_handle)jcontext)) {
				CMR_LOGE("jpeg:stop fail");
			}
		}
	} else {
		if (NULL != jcontext->event_cb) {
			jcontext->event_cb(CMR_JPEG_DEC_ERR, NULL, (void*)jcontext->oem_handle);
		} else {
			CMR_LOGE("jpeg:even cb is NULL.");
		}
		if (_jpeg_stop((cmr_handle)jcontext)) {
			CMR_LOGE("jpeg:stop fail");
		}
	}
}

static cmr_int _jpeg_stop(cmr_handle jpeg_handle)
{
	struct jpeg_codec_context                *jcontext = (struct jpeg_codec_context*)jpeg_handle;
	CMR_LOGV("jpeg:start");

	if (!jcontext || !jcontext->active_handle) {
		CMR_LOGE("jpeg:jpeg is deinited");
		return JPEG_CODEC_PARAM_ERR;
	}

	if (0 == jcontext->type) {
		struct jpeg_enc *enc_cxt_ptr = (struct jpeg_enc*)jcontext->active_handle;
		if (NULL != enc_cxt_ptr) {
			free(enc_cxt_ptr);
			jcontext->active_handle = 0;
		}
	} else if (1 == jcontext->type) {
		struct jpeg_dec *dec_cxt_ptr = (struct jpeg_dec*)jcontext->active_handle;
		if (NULL != dec_cxt_ptr) {
			free(dec_cxt_ptr);
			jcontext->active_handle = 0;
		}
	} else {
		return JPEG_CODEC_PARAM_ERR;
	}

	return JPEG_CODEC_SUCCESS;
}

static cmr_int _jpeg_enc_wexif(struct jpeg_enc_exif_param *param_ptr, struct jpeg_wexif_cb_param *out_ptr)
{
	cmr_int                    ret = JPEG_CODEC_SUCCESS;
	JINF_WEXIF_IN_PARAM_T      input_param;
	JINF_WEXIF_OUT_PARAM_T     output_param;

	input_param.exif_info_ptr = param_ptr->exif_ptr;
	input_param.src_jpeg_buf_ptr = (cmr_u8*)param_ptr->src_jpeg_addr_virt;
	input_param.src_jpeg_size = param_ptr->src_jpeg_size;
	input_param.thumbnail_buf_ptr = (cmr_u8*)param_ptr->thumbnail_addr_virt;
	input_param.thumbnail_buf_size = param_ptr->thumbnail_size;
	input_param.target_buf_ptr = (cmr_u8*)param_ptr->target_addr_virt;
	input_param.target_buf_size = param_ptr->target_size;
	input_param.temp_buf_size = param_ptr->thumbnail_size + JPEG_WEXIF_TEMP_MARGIN;
	input_param.temp_buf_ptr = (cmr_u8*)malloc(input_param.temp_buf_size);
	input_param.exif_isp_info = param_ptr->exif_isp_info;
	if (PNULL == input_param.temp_buf_ptr) {
		CMR_LOGE("jpeg:malloc temp buf for wexif fail.");
		return JPEG_CODEC_NO_MEM;
	}
	input_param.temp_exif_isp_buf_size = 4 * 1024;
	input_param.temp_exif_isp_buf_ptr = (uint8_t*)malloc(input_param.temp_exif_isp_buf_size);
	input_param.wrtie_file_func = NULL;
	if (PNULL == input_param.temp_exif_isp_buf_ptr) {
		free(input_param.temp_buf_ptr);
		return JPEG_CODEC_NO_MEM;
	}

	CMR_LOGI("jpeg:src jpeg addr 0x%p, size %d thumbnail addr 0x%p, size %d target addr 0x%p,size %d",
			 input_param.src_jpeg_buf_ptr,input_param.src_jpeg_size,
			 input_param.thumbnail_buf_ptr,input_param.thumbnail_buf_size,
			 input_param.target_buf_ptr,input_param.target_buf_size);

	ret = IMGJPEG_WriteExif(&input_param,&output_param);

	out_ptr->output_buf_virt_addr = (cmr_uint)output_param.output_buf_ptr;
	out_ptr->output_buf_size = output_param.output_size;
	free(input_param.temp_buf_ptr);
	free(input_param.temp_exif_isp_buf_ptr);
	input_param.temp_buf_ptr = PNULL;
	CMR_LOGI("jpeg:output: addr 0x%lx,size %d", out_ptr->output_buf_virt_addr, (cmr_u32)out_ptr->output_buf_size);

	return ret;
}

static cmr_int jpeg_thread_proc(struct cmr_msg *message, void* data)
{
	cmr_int                        ret = JPEG_CODEC_SUCCESS;
	cmr_int                        evt_id;
	cmr_u32                        evt;
	cmr_u32                        restart_cnt = 0;
	cmr_handle                     handle = NULL;
	struct jpeg_enc_next_param     *param_ptr = NULL;
	struct jpeg_dec_next_param     *dec_param_ptr = NULL;
	struct jpeg_enc                *enc_cxt_ptr = NULL;
	struct jpeg_wexif_cb_param     wexif_out_param;
	struct jpeg_codec_context      *jcontext = (struct jpeg_codec_context*)data;

	CMR_LOGV("jpeg:JPEG Thread Proc In \n");

	if (!message || !data) {
		CMR_LOGE("param error");
		goto jpeg_proc_end;
	}

	CMR_LOGV("jpeg: message.msg_type 0x%x", message->msg_type);
	evt = (cmr_u32)(message->msg_type & JPEG_EVT_MASK_BITS);
	if ((1 == jcontext->is_stop) && (JPEG_EVT_STOP != evt)) {
		CMR_LOGI("jpeg:discard message 0x%x.",evt);
		goto jpeg_proc_end;
	}

	switch(evt) {
	case  JPEG_EVT_ENC_START:
enc_start:
		handle = (cmr_handle )message->data;
		ret = _enc_start(handle, jcontext);
		if (ret && (JPEG_CODEC_STOP != ret) && (0 == restart_cnt)) {
			ret = JPEGCODEC_Close();
			if (ret) {
				CMR_LOGE("failed to close jpeg codec %ld", ret);
			} else {
				ret = JPEGCODEC_Open();
				if (ret) {
					CMR_LOGE("failed to open jpeg codec %ld", ret);
				} else {
					restart_cnt++;
					goto enc_start;
				}
			}
		}
		_enc_start_post(handle, jcontext, ret);
		restart_cnt = 0;
		CMR_LOGV("jpeg:receive JPEG_EVT_ENC_START message");
		break;

	case  JPEG_EVT_ENC_NEXT:
		enc_cxt_ptr = (struct jpeg_enc *)jcontext->active_handle;
		param_ptr = (struct jpeg_enc_next_param*)message->data;
		if (NULL == param_ptr) break;
		do {
			ret = _enc_next((cmr_handle)enc_cxt_ptr, jcontext, param_ptr);
			if (JPEG_CODEC_SUCCESS != ret) {
				CMR_LOGE("jpeg:enc next err %d.",(cmr_u32)ret);
				break;
			}
		} while((param_ptr->ready_line_num >= enc_cxt_ptr->size.height) && (enc_cxt_ptr->cur_line_num<enc_cxt_ptr->size.height));

		_enc_next_post(enc_cxt_ptr, jcontext, ret);
		break;

	case JPEG_EVT_DEC_START:
		handle = (cmr_handle )message->data;
		jcontext->err_code = 0;
		ret = _dec_start(handle, jcontext);
		_dec_start_post(handle, jcontext, ret);
		CMR_LOGI("jpeg:receive JPEG_EVT_DEC_START message");
		break;

	case  JPEG_EVT_DEC_NEXT:
		handle = (cmr_handle )message->data;
		if (0 != message->data) {
			ret = _dec_next(jcontext->active_handle, jcontext, (struct jpeg_dec_next_param *)message->data);
		} else {
			ret = JPEG_CODEC_PARAM_ERR;
			CMR_LOGE("jpeg:para error.");
		}

		_dec_next_post(handle, jcontext, ret);
		CMR_LOGI("jpeg:receive dec next message.");
		break;

	case  JPEG_EVT_STOP:
		ret = _jpeg_stop((cmr_handle)jcontext);
		break;

	case JPEG_EVT_KILL:
		ret = _jpeg_stop((cmr_handle)jcontext);
		break;

	case JPEG_EVT_ENC_EXIF:
		if (NULL != message->data) {
			ret = _jpeg_enc_wexif((struct jpeg_enc_exif_param*)message->data, &wexif_out_param);
			if (JPEG_CODEC_SUCCESS == ret) {
				jcontext->exif_output = wexif_out_param;
			} else {
				jcontext->exif_output.output_buf_size = 0;
			}
		}
		break;

	case JPEG_EVT_ENC_THUMB:
thumb_start:
		handle = (cmr_handle )message->data;
		ret = _enc_start(handle,jcontext);
		memset((void*)&jcontext->thumbnail_info, 0, sizeof(struct jpeg_enc_cb_param));
		if (JPEG_CODEC_SUCCESS == ret) {
			jcontext->thumbnail_info.is_thumbnail = 1;
			_prc_enc_cbparam(handle, &jcontext->thumbnail_info);
		} else {
			if (!restart_cnt) {
				ret = JPEGCODEC_Close();
				if (ret) {
					CMR_LOGE("failed to close jpeg codec %ld", ret);
				} else {
					ret = JPEGCODEC_Open();
					if (ret) {
						CMR_LOGE("failed to open jpeg codec %ld", ret);
					} else {
						restart_cnt++;
						goto thumb_start;
					}
				}
			}
		}
		restart_cnt = 0;
		CMR_LOGI("jpeg:enc thumbnail done,ret = %d.", (cmr_u32)ret);
		break;

	default:
		CMR_LOGE("jpeg:not correct message");
		break;
	}

jpeg_proc_end:
	CMR_LOGV("jpeg:JPEG Thrad Proc Out %d", (cmr_u32)ret);
	return ret;
}


cmr_int jpeg_init(cmr_handle oem_handle, cmr_handle *jpeg_handle)
{
	cmr_int                    ret = JPEG_CODEC_SUCCESS;
	struct jpeg_codec_context  *jcontext = NULL;

	if (!jpeg_handle) {
		CMR_LOGE("jpeg:param error");
		return JPEG_CODEC_PARAM_ERR;
	}

	*jpeg_handle = 0;
	jcontext = (struct jpeg_codec_context*)malloc(sizeof(struct jpeg_codec_context));
	if (!jcontext) {
		CMR_LOGE("jpeg:No mem");
		return JPEG_CODEC_NO_MEM;
	}
	memset(jcontext, 0, sizeof(struct jpeg_codec_context));

	jcontext->fw_decode_buf_size = JPEG_DECODE_FW_BUF_SIZE;
	jcontext->fw_decode_buf = (void*)malloc(jcontext->fw_decode_buf_size);
	if (PNULL == jcontext->fw_decode_buf) {
		CMR_LOGE("jpeg:No mem");
		ret = JPEG_CODEC_NO_MEM;
		goto jpeg_init_end;
	}

	/*create thread*/
	ret = cmr_thread_create(&jcontext->thread_handle, JPEG_MSG_QUEUE_SIZE,
							jpeg_thread_proc,(void*)jcontext);

	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("jpeg:create thread fail");
		ret = JPEG_CODEC_ERROR;
		goto jpeg_init_end;
	}
	*jpeg_handle = (cmr_handle)jcontext;

	ret = JPEGCODEC_Open();
jpeg_init_end:
	if (ret) {
		if (jcontext->fw_decode_buf) {
			free(jcontext->fw_decode_buf);
			jcontext->fw_decode_buf = NULL;
		}

		if (jcontext) {
			free(jcontext);
			jcontext = NULL;
		}
		*jpeg_handle = 0;
	} else {
		sem_init(&jcontext->access_sem, 0, 1);
		jcontext->oem_handle = oem_handle;
	}
	CMR_LOGV("jpeg:ret %d", (cmr_u32)ret);
	return ret;
}

static cmr_int _check_enc_start_param(struct jpeg_enc_in_param *in_parm_ptr)
{
	cmr_int               ret = JPEG_CODEC_SUCCESS;

	CMR_LOGV("jpeg:w h, %d %d, quality level %d",in_parm_ptr->size.width, in_parm_ptr->size.height,
			 in_parm_ptr->quality_level);

#ifdef JPEG_CODE_DEBUG
	CMR_LOGV("jpeg:slice height, %d, slice mode %d",
			 in_parm_ptr->slice_height, in_parm_ptr->slice_mod);
	CMR_LOGV("jpeg:phy addr 0x%lx 0x%lx, vir addr 0x%lx 0x%lx",
		     in_parm_ptr->src_addr_phy.addr_y, in_parm_ptr->src_addr_phy.addr_u,
		     in_parm_ptr->src_addr_vir.addr_y, in_parm_ptr->src_addr_vir.addr_u);
	CMR_LOGV("jpeg:endian %d %d",
			 in_parm_ptr->src_endian.y_endian, in_parm_ptr->src_endian.uv_endian);
	CMR_LOGV("jpeg:stream phy 0x%lx vir 0x%lx, size 0x%x",
			 in_parm_ptr->stream_buf_phy,
			 in_parm_ptr->stream_buf_vir,
			 in_parm_ptr->stream_buf_size);
#endif
	return ret;

}


static cmr_int _get_enc_start_param(struct jpeg_enc *cxt_ptr,
												struct jpeg_enc_in_param *in_parm_ptr)
{
	cmr_int               ret = JPEG_CODEC_SUCCESS;

	CMR_LOGV("jpeg:all param");

#ifdef JPEG_CODE_DEBUG
	CMR_LOGI("jpeg:stream_buf_phy: 0x%x", in_parm_ptr->stream_buf_phy);
	CMR_LOGI("jpeg:stream_buf_vir: 0x%x", in_parm_ptr->stream_buf_vir);
	CMR_LOGI("jpeg:stream_buf_size: 0x%x", in_parm_ptr->stream_buf_size);
	CMR_LOGI("jpeg:img_size: w:%d, h:%d", in_parm_ptr->size.width, in_parm_ptr->size.height);
#endif
	CMR_LOGI("jpeg:slice_height:%d", in_parm_ptr->slice_height);

	cxt_ptr->stream_buf_phy = in_parm_ptr->stream_buf_phy;
	cxt_ptr->stream_buf_vir = in_parm_ptr->stream_buf_vir;
	cxt_ptr->stream_buf_size = in_parm_ptr->stream_buf_size;

	cxt_ptr->src_addr_phy = in_parm_ptr->src_addr_phy;
	cxt_ptr->src_addr_vir = in_parm_ptr->src_addr_vir;
	cxt_ptr->src_endian = in_parm_ptr->src_endian;

	cxt_ptr->src_fmt = in_parm_ptr->src_fmt;

	cxt_ptr->quality_level = in_parm_ptr->quality_level;

	cxt_ptr->size = in_parm_ptr->size;
	cxt_ptr->out_size = in_parm_ptr->out_size;

	cxt_ptr->slice_height = in_parm_ptr->slice_height;
	cxt_ptr->slice_mod = in_parm_ptr->slice_mod;

	return ret;
}

static cmr_int _get_dec_start_param(struct jpeg_dec *cxt_ptr,
											     struct jpeg_dec_in_param *in_parm_ptr)
{
	cmr_int                 ret = JPEG_CODEC_SUCCESS;

	cxt_ptr->stream_buf_phy = in_parm_ptr->stream_buf_phy;
	cxt_ptr->stream_buf_vir = in_parm_ptr->stream_buf_vir;
	cxt_ptr->stream_buf_size = in_parm_ptr->stream_buf_size;

	cxt_ptr->temp_buf_phy = in_parm_ptr->temp_buf_phy;
	cxt_ptr->temp_buf_vir = in_parm_ptr->temp_buf_vir;
	cxt_ptr->temp_buf_size = in_parm_ptr->temp_buf_size;

	cxt_ptr->dst_addr_phy = in_parm_ptr->dst_addr_phy;
	cxt_ptr->dst_addr_vir = in_parm_ptr->dst_addr_vir;
	cxt_ptr->dst_endian = in_parm_ptr->dst_endian;

	cxt_ptr->dst_fmt = in_parm_ptr->dst_fmt;

	cxt_ptr->size = in_parm_ptr->size;

	cxt_ptr->slice_height = in_parm_ptr->slice_height;
	cxt_ptr->slice_mod = in_parm_ptr->slice_mod;

	CMR_LOGV("jpeg:stream phy 0x%x vir 0x%x, temp_buf phy 0x%x vir 0x%x",
			 (cmr_u32)cxt_ptr->stream_buf_phy, (cmr_u32)cxt_ptr->stream_buf_vir,
			 (cmr_u32)cxt_ptr->temp_buf_phy, (cmr_u32)cxt_ptr->temp_buf_vir);
	CMR_LOGV("jpeg:dst phy  0x%x 0x%x, vir 0x%x 0x%x",
			 (cmr_u32)cxt_ptr->dst_addr_phy.addr_y, (cmr_u32)cxt_ptr->dst_addr_phy.addr_u,
			 (cmr_u32)cxt_ptr->dst_addr_vir.addr_y, (cmr_u32)cxt_ptr->dst_addr_vir.addr_u);

	return ret;

}

static cmr_int _check_wexif_param(struct jpeg_enc_exif_param *param_ptr)
{
	cmr_int                 ret = JPEG_CODEC_SUCCESS;

	if ((NULL == (cmr_uint*)param_ptr->target_addr_virt)
		|| (NULL == (cmr_uint*)param_ptr->src_jpeg_addr_virt)) {
		ret = JPEG_CODEC_PARAM_ERR;
	}

	CMR_LOGI("jpeg:src addr 0x%x size %d thumb addr 0x%x,size %d target addr 0x%x,size %d.",
			 (cmr_u32)param_ptr->src_jpeg_addr_virt,param_ptr->src_jpeg_size,
			 (cmr_u32)param_ptr->thumbnail_addr_virt,param_ptr->thumbnail_size,
			 (cmr_u32)param_ptr->target_addr_virt,param_ptr->target_size);

	return ret;
}

cmr_int jpeg_enc_start(struct jpeg_enc_in_param *in_parm_ptr)
{
	cmr_int                      ret = JPEG_CODEC_SUCCESS;
	struct jpeg_enc              *enc_cxt_ptr = 0;
	struct jpeg_codec_context    *jcontext = (struct jpeg_codec_context*)in_parm_ptr->jpeg_handle;
	CMR_MSG_INIT(message);

	if (!jcontext) {
		CMR_LOGE("jpeg:param error");
		return JPEG_CODEC_PARAM_ERR;
	}

	sem_wait(&jcontext->access_sem);
	if (jcontext->active_handle) {
		CMR_LOGE("jpeg:param error");
		ret = JPEG_CODEC_PARAM_ERR;
		goto enc_start_end;
	}
	if (JPEG_SUCCESS != _check_enc_start_param(in_parm_ptr)) {
		ret = JPEG_CODEC_PARAM_ERR;
		goto enc_start_end;
	}
	enc_cxt_ptr = (struct jpeg_enc *)malloc(sizeof(struct jpeg_enc));
	CMR_LOGV("jpeg:0x%p", enc_cxt_ptr);
	if (NULL == enc_cxt_ptr) {
		ret = JPEG_CODEC_NO_MEM;
		goto enc_start_end;
	}
	memset(enc_cxt_ptr, 0, sizeof(struct jpeg_enc));

	if (JPEG_SUCCESS != _get_enc_start_param(enc_cxt_ptr,in_parm_ptr)) {
		ret = JPEG_CODEC_PARAM_ERR;
		goto enc_start_end;
	}
	jcontext->active_handle = (cmr_handle)enc_cxt_ptr;
	message.msg_type = JPEG_EVT_ENC_START;
	message.data = enc_cxt_ptr;
	message.alloc_flag = 0;
	message.sync_flag  = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(jcontext->thread_handle, &message);

	if (CMR_MSG_SUCCESS != ret) {
		ret = JPEG_CODEC_ERROR;
		goto enc_start_end;
	}

	jcontext->type = 0;
	CMR_LOGV("jpeg:handle 0x%p.",enc_cxt_ptr);
enc_start_end:
	if (ret) {
		if (enc_cxt_ptr) {
			free(enc_cxt_ptr);
			enc_cxt_ptr = NULL;
			jcontext->active_handle = 0;
		}
	}
	sem_post(&jcontext->access_sem);
	CMR_LOGV("jpeg:ret %d", (cmr_u32)ret);
	return JPEG_CODEC_SUCCESS;
}

cmr_int jpeg_enc_next(struct jpeg_enc_next_param *param_ptr)
{
	cmr_int                     ret = JPEG_CODEC_SUCCESS;
	struct jpeg_enc             *enc_cxt_ptr = 0;
	struct jpeg_enc_next_param  *data_ptr = NULL;
	struct jpeg_codec_context   *jcontext;
	CMR_MSG_INIT(message);

	if (!param_ptr || !param_ptr->jpeg_handle){
		CMR_LOGE("jpeg:param error");
		return JPEG_CODEC_PARAM_ERR;
	}

	CMR_LOGV("jpeg:start,handle 0x%lx.",param_ptr->jpeg_handle);
	jcontext = (struct jpeg_codec_context*)param_ptr->jpeg_handle;
	sem_wait(&jcontext->access_sem);
	enc_cxt_ptr = (struct jpeg_enc *)jcontext->active_handle;
	if (!enc_cxt_ptr) {
		CMR_LOGE("jpeg:param error");
		ret = JPEG_CODEC_PARAM_ERR;
		goto enc_next_end;
	}
	if (1 == enc_cxt_ptr->is_finish) {
		CMR_LOGE("jpeg:encode finish.");
		ret = JPEG_CODEC_ERROR;
		goto enc_next_end;
	}

	if (JPEG_YUV_SLICE_MUTI_BUF == enc_cxt_ptr->slice_mod) {
		if (0 == param_ptr->src_addr_phy.addr_y|| 0 == param_ptr->src_addr_vir.addr_y ||
			0 == param_ptr->src_addr_phy.addr_u || 0 == param_ptr->src_addr_vir.addr_u) {
			ret = JPEG_CODEC_PARAM_ERR;
			goto enc_next_end;
		}
	}

	data_ptr = (struct jpeg_enc_next_param *)malloc(sizeof(struct jpeg_enc_next_param ));
	if (0 == data_ptr) {
		CMR_LOGE("jpeg:No mem");
		ret = JPEG_CODEC_NO_MEM;
		goto enc_next_end;
	}
	memcpy(data_ptr, param_ptr, sizeof(struct jpeg_enc_next_param));
	message.msg_type = JPEG_EVT_ENC_NEXT;
	message.data = data_ptr;
	message.alloc_flag = 1;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(jcontext->thread_handle, &message);

enc_next_end:
	if (ret) {
		if (data_ptr) {
			free(data_ptr);
		}
	}
	sem_post(&jcontext->access_sem);
	CMR_LOGV("jpeg:ret %d", (cmr_u32)ret);
	return ret;
}


cmr_int jpeg_dec_start(struct jpeg_dec_in_param *in_parm_ptr)
{
	cmr_int                    ret = JPEG_CODEC_SUCCESS;
	struct jpeg_dec            *dec_cxt_ptr = 0;
	struct jpeg_codec_context  *jcontext;
	CMR_MSG_INIT(message);

	if (!in_parm_ptr || !in_parm_ptr->jpeg_handle) {
		CMR_LOGE("jpeg:param error");
		return JPEG_CODEC_PARAM_ERR;
	}
	jcontext = (struct jpeg_codec_context*)in_parm_ptr->jpeg_handle;
	sem_wait(&jcontext->access_sem);
	if (jcontext->active_handle) {
		CMR_LOGE("jpeg:param error");
		ret = JPEG_CODEC_PARAM_ERR;
		goto dec_start_end;
	}
	dec_cxt_ptr = (struct jpeg_dec *)malloc(sizeof(struct jpeg_dec));
	if (NULL == dec_cxt_ptr) {
		CMR_LOGE("jpeg:No mem");
		ret = JPEG_CODEC_NO_MEM;
		goto dec_start_end;
	}
	memset(dec_cxt_ptr, 0, sizeof(struct jpeg_dec));
	if (JPEG_CODEC_SUCCESS != _get_dec_start_param(dec_cxt_ptr,in_parm_ptr)) {
		ret = JPEG_CODEC_PARAM_ERR;
		goto dec_start_end;
	}
	jcontext->active_handle = (cmr_handle)dec_cxt_ptr;
	message.msg_type = JPEG_EVT_DEC_START;
	message.data = dec_cxt_ptr;
	message.alloc_flag = 0;
	message.sync_flag  = CMR_MSG_SYNC_NONE;

	ret = cmr_thread_msg_send(jcontext->thread_handle, &message);
	if (ret) {
		ret = JPEG_CODEC_ERROR;
		goto dec_start_end;
	}

	jcontext->type = 1;/*decode*/
	CMR_LOGV("jpeg:dec handle 0x%p", dec_cxt_ptr);
dec_start_end:
	if (ret) {
		if (dec_cxt_ptr) {
			free(dec_cxt_ptr);
			dec_cxt_ptr = NULL;
			jcontext->active_handle = 0;
		}
	}
	sem_post(&jcontext->access_sem);
	CMR_LOGV("jpeg:ret %d", (cmr_u32)ret);
	return ret;
}

/*useless function, some mode can not be support. slice mode can not be supp*/
cmr_int jpeg_dec_next(struct jpeg_dec_next_param *param_ptr)
{
	cmr_int                         ret = JPEG_CODEC_SUCCESS;
	struct jpeg_dec                 *dec_cxt_ptr = 0;
	struct jpeg_dec_next_param      *data_ptr = NULL;
	struct jpeg_codec_context       *jcontext;
	CMR_MSG_INIT(message);

	if (!param_ptr || !param_ptr->jpeg_handle) {
		CMR_LOGE("jpeg:param error");
		return JPEG_CODEC_PARAM_ERR;
	}
	CMR_LOGV("jpeg:start,handle 0x%p", param_ptr->jpeg_handle);
	jcontext = (struct jpeg_codec_context*)param_ptr->jpeg_handle;
	sem_wait(&jcontext->access_sem);
	if (!jcontext->active_handle) {
		CMR_LOGE("jpeg:param error");
		ret = JPEG_CODEC_PARAM_ERR;
		goto dec_next_end;
	}
	dec_cxt_ptr = (struct jpeg_dec*)jcontext->active_handle;
	if (1 == dec_cxt_ptr->is_finish) {
		CMR_LOGE("jpeg:decode finish.");
		ret = JPEG_CODEC_ERROR;
		goto dec_next_end;
	}

	if (JPEG_YUV_SLICE_MUTI_BUF == dec_cxt_ptr->slice_mod){
		if (0 == param_ptr->dst_addr_phy.addr_y|| 0 == param_ptr->dst_addr_vir.addr_y ||
			0 == param_ptr->dst_addr_phy.addr_u || 0 == param_ptr->dst_addr_vir.addr_u) {
			ret = JPEG_CODEC_PARAM_ERR;
			goto dec_next_end;
		}
	}
	data_ptr = (struct jpeg_dec_next_param*)malloc(sizeof(struct jpeg_dec_next_param));
	if (0 == data_ptr) {
		ret = JPEG_CODEC_NO_MEM;
		goto dec_next_end;
	}
	memcpy(data_ptr, param_ptr, sizeof(struct jpeg_dec_next_param));
	message.msg_type = JPEG_EVT_DEC_NEXT;
	message.data = data_ptr;
	message.alloc_flag = 1;
    message.sync_flag  = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(jcontext->thread_handle, &message);

	if (ret) {
		ret = JPEG_CODEC_ERROR;
	}
dec_next_end:
	if (ret) {
		if (data_ptr) {
			free(data_ptr);
			data_ptr = NULL;
		}
	}
	sem_post(&jcontext->access_sem);
	CMR_LOGV("jpeg:ret %d", (cmr_u32)ret);
	return ret;
}

cmr_int jpeg_dec_start_sync(struct jpeg_dec_in_param *in_parm_ptr,
									    struct jpeg_dec_cb_param *out_parm_ptr)
{
	cmr_int                    ret = JPEG_CODEC_SUCCESS;
	struct jpeg_dec            *dec_cxt_ptr = 0;
	struct jpeg_codec_context  *jcontext;
	CMR_MSG_INIT(message);

	if (!in_parm_ptr || !in_parm_ptr->jpeg_handle || !out_parm_ptr) {
		CMR_LOGE("jpeg:param error");
		return JPEG_CODEC_PARAM_ERR;
	}
	jcontext = (struct jpeg_codec_context*)in_parm_ptr->jpeg_handle;
	sem_wait(&jcontext->access_sem);
	if (jcontext->active_handle) {
		CMR_LOGE("jpeg:param error");
		ret = JPEG_CODEC_PARAM_ERR;
		goto dec_start_sync_end;
	}
	dec_cxt_ptr = (struct jpeg_dec *)malloc(sizeof(struct jpeg_dec));
	if (NULL == dec_cxt_ptr) {
		CMR_LOGE("jpeg:No mem");
		ret = JPEG_CODEC_NO_MEM;
		goto dec_start_sync_end;
	}
	memset(dec_cxt_ptr, 0, sizeof(struct jpeg_dec));
	if (JPEG_CODEC_SUCCESS != _get_dec_start_param(dec_cxt_ptr,in_parm_ptr)) {
		ret = JPEG_CODEC_PARAM_ERR;
		goto dec_start_sync_end;
	}
	jcontext->sync_flag = 1;
	jcontext->type = 1;
	jcontext->active_handle = (cmr_handle)dec_cxt_ptr;
	message.msg_type = JPEG_EVT_DEC_START;
	message.data = dec_cxt_ptr;
	message.alloc_flag = 1;
	message.sync_flag  = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(jcontext->thread_handle, &message);
	jcontext->sync_flag = 0;
	if (ret) {
		goto dec_start_sync_end;
	}
	memcpy((void*)out_parm_ptr,(void*)&jcontext->dec_output,
			sizeof(struct jpeg_dec_cb_param));

	CMR_LOGV("jpeg:dec handle 0x%p", dec_cxt_ptr);
dec_start_sync_end:
	if (ret) {
		if (dec_cxt_ptr) {
			free(dec_cxt_ptr);
		}
		dec_cxt_ptr = NULL;
	}
	ret = jcontext->err_code;
	CMR_LOGV("jpeg:ret %d", (cmr_u32)ret);
	sem_post(&jcontext->access_sem);
	return ret;
}

cmr_int jpeg_stop(cmr_handle jpeg_handle)
{
	cmr_int                    ret = JPEG_CODEC_SUCCESS;
	struct jpeg_codec_context  *jcontext = (struct jpeg_codec_context*)jpeg_handle;
	CMR_MSG_INIT(message);
	CMR_LOGV("jpeg:start");

	if (!jcontext) {
		CMR_LOGE("jpeg:param error");
		return JPEG_CODEC_PARAM_ERR;
	}
	sem_wait(&jcontext->access_sem);
	if (!jcontext->active_handle) {
		CMR_LOGE("don't need handle");
		goto stop_end;
	}

	if (jcontext->is_stop) {
		CMR_LOGV("jpeg:don't handle");
		ret = JPEG_CODEC_SUCCESS;
		goto stop_end;
	}
	message.msg_type  = JPEG_EVT_STOP;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	jcontext->is_stop = 1;
	message.data = (void*)jcontext;
	ret = cmr_thread_msg_send(jcontext->thread_handle, &message);
	if (ret) {
		ret = JPEG_CODEC_ERROR;
		goto stop_end;
	}
	jcontext->is_stop = 0;
	jcontext->active_handle = 0;
stop_end:
	sem_post(&jcontext->access_sem);
	CMR_LOGV("jpeg:end ret %d", (cmr_u32)ret);
	return ret;
}

cmr_int jpeg_deinit(cmr_handle jpeg_handle)
{
	cmr_int                    ret = JPEG_CODEC_SUCCESS;
	struct jpeg_codec_context  *jcontext = (struct jpeg_codec_context*)jpeg_handle;
	CMR_MSG_INIT(message);

	if (!jcontext) {
		CMR_LOGE("jpeg:param error");
		return JPEG_CODEC_PARAM_ERR;
	}
	sem_wait(&jcontext->access_sem);
	message.msg_type  = JPEG_EVT_KILL;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	CMR_LOGV("jpeg:start");
	ret = cmr_thread_msg_send(jcontext->thread_handle, &message);
	if (ret) {
		ret = JPEG_CODEC_ERROR;
		goto deinit_end;
	}
	if (jcontext->thread_handle) {
		cmr_thread_destroy(jcontext->thread_handle);
		jcontext->thread_handle = 0;
	}

	if (PNULL != jcontext->fw_decode_buf) {
		free(jcontext->fw_decode_buf);
		jcontext->fw_decode_buf = PNULL;
	}
	sem_destroy(&jcontext->access_sem);
	free(jcontext);
	jcontext = NULL;
deinit_end:
	JPEGCODEC_Close();
	CMR_LOGV("jpeg:ret %d", (cmr_u32)ret);
	if (ret) {
		sem_post(&jcontext->access_sem);
	}
	return ret;
}

void jpeg_evt_reg(cmr_handle jpeg_handle, cmr_evt_cb adp_event_cb)
{
	struct jpeg_codec_context    *jcontext = (struct jpeg_codec_context*)jpeg_handle;
	if (!jcontext) {
		CMR_LOGE("jpeg:param error");
		return;
	}
	jcontext->event_cb = adp_event_cb;
	return ;
}

cmr_int jpeg_enc_add_eixf(struct jpeg_enc_exif_param *param_ptr,
								    struct jpeg_wexif_cb_param *output_ptr)
{
	cmr_int                        ret = JPEG_CODEC_SUCCESS;
	struct jpeg_enc_exif_param     *data = NULL;
	struct jpeg_codec_context      *jcontext;

	CMR_MSG_INIT(message);

	if (!param_ptr || !param_ptr->jpeg_handle) {
		CMR_LOGE("jpeg:param error");
		return JPEG_CODEC_PARAM_ERR;
	}

	jcontext = (struct jpeg_codec_context*)param_ptr->jpeg_handle;

	message.msg_type  = JPEG_EVT_ENC_EXIF;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;

	CMR_LOGI("jpeg:enc add exit start.");

	ret = _check_wexif_param(param_ptr);

	if (JPEG_CODEC_SUCCESS != ret) {
		CMR_LOGE("jpeg:input param error.");
		return JPEG_CODEC_PARAM_ERR;
	}

	data = (struct jpeg_enc_exif_param*)malloc(sizeof(struct jpeg_enc_exif_param));

	if (NULL == data) {
		return JPEG_CODEC_NO_MEM;
	}

	*data = *param_ptr;
	message.alloc_flag = 1;
	message.data = data;

	ret = cmr_thread_msg_send(jcontext->thread_handle, &message);

	if (CMR_MSG_SUCCESS == ret) {
		if (0 != jcontext->exif_output.output_buf_size) {
			*output_ptr = jcontext->exif_output;
		} else {
			output_ptr->output_buf_size = 0;
			ret = JPEG_CODEC_ERROR;
			CMR_LOGE("jpeg:write exif fail.");
		}
	} else {
		free(data);
	}

	CMR_LOGI("jpeg:output addr 0x%x,size %d", (cmr_u32)output_ptr->output_buf_virt_addr,(cmr_u32)output_ptr->output_buf_size);
	return ret;
}

cmr_int jpeg_enc_thumbnail(struct jpeg_enc_in_param *in_parm_ptr, cmr_uint *stream_size_ptr)
{
	cmr_int                      ret = JPEG_CODEC_SUCCESS;
	struct jpeg_enc              *enc_cxt_ptr = 0;
	struct jpeg_codec_context    *jcontext;
	CMR_MSG_INIT(message);

	if (!in_parm_ptr || !in_parm_ptr->jpeg_handle) {
		CMR_LOGE("jpeg:param error");
		return JPEG_CODEC_PARAM_ERR;
	}
	jcontext = (struct jpeg_codec_context*)in_parm_ptr->jpeg_handle;

	if (JPEG_SUCCESS != _check_enc_start_param(in_parm_ptr)) {
		return JPEG_CODEC_PARAM_ERR;
	}
	/*save_inputdata(in_parm_ptr->src_addr_vir.addr_y,
					in_parm_ptr->src_addr_vir.addr_u,320*240);*/

	enc_cxt_ptr = (struct jpeg_enc *)malloc(sizeof(struct jpeg_enc));
	CMR_LOGV("jpeg:thumbnail enc: 0x%p", enc_cxt_ptr);

	if (NULL == enc_cxt_ptr) {
		return JPEG_CODEC_NO_MEM;
	}
	memset(enc_cxt_ptr, 0, sizeof(struct jpeg_enc));

	if (JPEG_SUCCESS != _get_enc_start_param(enc_cxt_ptr,in_parm_ptr)) {
		return JPEG_CODEC_PARAM_ERR;
	}
	message.msg_type  = JPEG_EVT_ENC_THUMB;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.msg_type  = JPEG_EVT_ENC_THUMB;
	message.data = enc_cxt_ptr;
	message.alloc_flag = 1;
	enc_cxt_ptr->is_thumbnail = 1;
	ret = cmr_thread_msg_send(jcontext->thread_handle, &message);

	if (CMR_MSG_SUCCESS == ret) {
		*stream_size_ptr = 0;
		if (0 != jcontext->thumbnail_info.stream_size) {
			*stream_size_ptr = jcontext->thumbnail_info.stream_size;
		} else {
			ret = JPEG_CODEC_ERROR;
		}
	} else {
		free(enc_cxt_ptr);
	}
	/*savedata(in_parm_ptr->stream_buf_vir,jcontext.thumbnail_info.stream_size);*/
	CMR_LOGV("jpeg:return %d.", (cmr_u32)ret);

	return ret;
}
