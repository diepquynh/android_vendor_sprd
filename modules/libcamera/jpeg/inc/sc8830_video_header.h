/******************************************************************************
 ** File Name:      sc8801h_video_header.h                                    *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           3/10/2009                                                 *
 ** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    common header file for sc6800h video(jpeg).               *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 4/23/2007    Binggo Zhou     Create.                                      *
 *****************************************************************************/
#ifndef _SC8825_VIDEO_HEADER_H_
#define _SC8825_VIDEO_HEADER_H_

#ifdef   __cplusplus
    extern   "C"
    {
#endif
#include "sci_types.h"
#include "video_common.h"
#include "jpg_global_define.h"
//#include "vsp_axim.h"
//#include "vsp_dcam.h"
#include "jpg_dct.h"
#include "jpg_mbio.h"
//#include "vsp_dbk.h"
//#include "vsp_mca.h"
#include "jpg_bsm.h"
#include "jpg_drv_sc8830.h"
#include "jpg_vld.h"
#include "jpg_vlc.h"
//#include "vsp_mea.h"
//#include "vsp_glb_ctrl.h"
/*#if defined(MPEG4_DEC)
	#include "mmcodec.h"
	#include "mp4dec_basic.h"
	#include "mp4dec_bfrctrl.h"
	#include "mp4dec_bitstream.h"
	#include "mp4dec_datapartitioning.h"
	#include "mp4dec_mode.h"
	#include "mp4dec_global.h"
	#include "mp4dec_header.h"
	#include "mp4dec_malloc.h"
	#include "mp4dec_mb.h"
	#include "mp4dec_mc.h"
	#include "mp4dec_mv.h"
	#include "mp4dec_session.h"
	#include "mp4dec_transpose.h"
	#include "mp4dec_vop.h"
	#include "mp4dec_vld.h"
	#include "mpeg4dec.h"
#elif defined(MPEG4_ENC)
	#include "mmcodec.h"
	#include "mp4enc_bitstrm.h"
	#include "mp4enc_command.h"
	#include "mp4enc_constdef.h"
	#include "mp4enc_dct.h"
	#include "mp4enc_global.h"
	#include "mp4enc_header.h"
	#include "mp4enc_init.h"
	#include "mp4enc_malloc.h"
	#include "mp4enc_mb.h"
	#include "mp4enc_me.h"
	#include "mp4enc_mode.h"
	#include "mp4enc_mv.h"
	#include "mp4enc_ratecontrol.h"
	#include "mp4enc_trace.h"
	#include "mp4enc_vlc.h"
	#include "mp4enc_vop.h"
#else*/
	#include "jpegcodec_global.h"
	#include "jpegcodec_table.h"
	#include "jpegcodec_def.h"
	#include "jpeg_fw_def.h"
	#include "jpeg_common.h"
	//#if defined(JPEG_ENC)
		#include "jpegenc_init.h"
		#include "jpegenc_header.h"
		#include "jpegenc_frame.h"
		#include "jpegenc_bitstream.h"
		#include "jpegenc_malloc.h"
	//#endif

	//#if defined(JPEG_DEC)
		#include "jpegdec_out.h"
		#include "jpegdec_init.h"
		#include "jpegdec_vld.h"
		#include "jpegdec_dequant.h"
		#include "jpegdec_malloc.h"
		#include "jpegdec_bitstream.h"
		#include "jpegdec_parse.h"
		#include "jpegdec_frame.h"
		#include "jpegdec_pvld.h"
	//#endif
//#endif

#if defined(SIM_IN_WIN)
#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif //SIM_IN_WIN

#if _CMODEL_
#include "ahbm_global.h"
#include "bsm_global.h"
#include "buffer_global.h"
#include "common_global.h"
#include "dcam_global.h"
#include "dct_global.h"
#include "global_global.h"
#include "dct_global.h"
#include "iict_global.h"
#include "mbc_global.h"
#include "ipred_global.h"
#include "mca_global.h"
#include "vld_global.h"
#include "hvld_global.h"
#include "hvld_mode.h"
#include "hvld_test_vector.h"
#include "hvld_trace.h"
#include "h264dbk_trace.h"
#include "hdbk_global.h"
#include "hdbk_mode.h"
#include "hdbk_test_vector.h"
#include "vlc_global.h"
#include "mea_global.h"
#endif //_CMODEL_

#ifdef RUN_IN_PLATFORM
//#include "os_api.h"
#endif //RUN_IN_PLATFORM

#ifdef SIM_IN_ADS
#include <stdlib.h>
#endif //SIM_IN_ADS

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_SC8825_VIDEO_HEADER_H_
