#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <dlfcn.h>

#include <linux/ion.h>
#include "MemoryHeapIon.h"
#include "ion_sprd.h"
using namespace android;

#include "m4v_h263_enc_api.h"

//#define CALCULATE_PSNR

#define CLIP_16(x)	(((x) + 15) & (~15))

#define MP4ENC_INTERNAL_BUFFER_SIZE  (0x200000)  //(MP4ENC_OR_RUN_SIZE+MP4ENC_OR_INTER_MALLOC_SIZE)  
#define ONEFRAME_BITSTREAM_BFR_SIZE	(1500*1024)  //for bitstream size of one encoded frame.

static void usage() {
    INFO("usage:\n");
    INFO("utest_vsp_enc -i filename_yuv -w width -h height -o filename_bitstream [OPTIONS]\n");
    INFO("-i                string : input yuv filename\n");
    INFO("-w                integer: intput yuv width\n");
    INFO("-h                integer: intput yuv height\n");
    INFO("-o                string : output bitstream filename\n");
    INFO("[OPTIONS]:\n");
    INFO("-format           integer: video format(0 is h263 / 1 is mpeg4), default is 1\n");
    INFO("-framerate        integer: framerate, default is 25\n");
    INFO("-max_key_interval integer: maximum keyframe interval, default is 30\n");
    INFO("-bitrate          integer: target bitrate in kbps if cbr(default), default is 512\n");
    INFO("-qp               integer: qp[1...31] if vbr, default is 8\n");
    INFO("-frames           integer: number of frames to encode, default is 0(all frames)\n");
    INFO("-help                    : show this help message\n");
    INFO("Built on %s %s, Written by XiaoweiLuo(xiaowei.luo@spreadtrum.com)\n", __DATE__, __TIME__);
}

void *mLibHandle;
FT_MP4EncPreInit        mMP4EncPreInit;
FT_MP4EncInit        mMP4EncInit;
FT_MP4EncSetConf        mMP4EncSetConf;
FT_MP4EncGetConf        mMP4EncGetConf;
FT_MP4EncStrmEncode        mMP4EncStrmEncode;
FT_MP4EncGenHeader        mMP4EncGenHeader;
FT_MP4EncRelease        mMP4EncRelease;
static bool mIOMMUEnabled = false;

static int32 enc_init(MP4Handle *mHandle, uint32 width, uint32 height, int32 format,
                      uint8* pbuf_inter, uint_32or64 pbuf_inter_phy, uint32 size_inter,
                      uint8* pbuf_extra, uint_32or64 pbuf_extra_phy, uint32 size_extra,
                      uint8* pbuf_stream, uint_32or64 pbuf_stream_phy, uint32 size_stream) {
    MMCodecBuffer InterMemBfr;
    MMCodecBuffer ExtraMemBfr;
    MMCodecBuffer StreamMemBfr;
    MMEncVideoInfo encInfo;

    INFO("enc_init IN\n");

    InterMemBfr.common_buffer_ptr = pbuf_inter;
    InterMemBfr.common_buffer_ptr_phy = pbuf_inter_phy;
    InterMemBfr.size = size_inter;
    if ((*mMP4EncPreInit)(mHandle, &InterMemBfr) != MMENC_OK) {
        ERR ("mMP4EncPreInit error.\n ");
        return -1;
    }

    ExtraMemBfr.common_buffer_ptr = pbuf_extra;
    ExtraMemBfr.common_buffer_ptr_phy = pbuf_extra_phy;
    ExtraMemBfr.size	= size_extra;

    StreamMemBfr.common_buffer_ptr = pbuf_stream;
    StreamMemBfr.common_buffer_ptr_phy = pbuf_stream_phy;
    StreamMemBfr.size	= size_stream;

    encInfo.is_h263 = (format == 0) ? 1 : 0;
    encInfo.frame_width = width;
    encInfo.frame_height = height;
    encInfo.yuv_format = MMENC_YUV420SP_NV21;
    encInfo.time_scale = 1000;
    encInfo.b_anti_shake = 0;
    if ((*mMP4EncInit)(mHandle, &ExtraMemBfr,&StreamMemBfr, &encInfo) != MMENC_OK) {
        ERR ("mMP4EncInit error.\n ");
        return -1;
    }

    INFO("enc_init OUT\n");

    return 0;
}

static int32 enc_set_parameter(MP4Handle *mHandle, int format, int framerate, int cbr, int bitrate, int qp) {
    MMEncConfig encConfig;

    if ((*mMP4EncGetConf)(mHandle, &encConfig) != MMENC_OK) {
        ERR ("mMP4EncGetConf error.\n ");
        return -1;
    }

    encConfig.h263En = (format == 0) ? 1 : 0;
    encConfig.RateCtrlEnable = cbr;
    encConfig.targetBitRate = bitrate;
    encConfig.FrameRate = framerate;
    encConfig.QP_IVOP = qp;
    encConfig.QP_PVOP = qp;
    encConfig.vbv_buf_size = bitrate/2;
    encConfig.profileAndLevel = 1;
    if ((*mMP4EncSetConf)(mHandle, &encConfig) != MMENC_OK) {
        ERR ("mMP4EncSetConf error.\n ");
        return -1;
    }

    return 0;
}

static int32 enc_get_header(MP4Handle *mHandle, uint8 *pheader, uint32 *size) {
    MMEncOut encOut;

    if (((*mMP4EncGenHeader)(mHandle, &encOut) != MMENC_OK) ||
            (encOut.strmSize > (int)(*size)) || (encOut.strmSize <= 0)) {
        ERR ("mMP4EncSetConf error.\n ");
        return -1;
    }

    memcpy(pheader, encOut.pOutBuf, encOut.strmSize);
    *size = encOut.strmSize;

    return 0;
}


static int32 enc_encode_frame(MP4Handle *mHandle,uint32 width,uint32 height, uint8* py, uint8* py_phy, uint8* puv, uint8* puv_phy, uint8* pframe, uint32* size, uint8**pyuv_rec, uint32 timestamp, int32* type, int32 bs_remain_len = 0)
{
    MMEncIn vid_in;
    MMEncOut vid_out;

    vid_in.time_stamp = timestamp;
    vid_in.vopType = *type;
    vid_in.bs_remain_len = bs_remain_len;
    vid_in.channel_quality = 1;
    vid_in.p_src_y = py;
    vid_in.p_src_u = puv;
    vid_in.p_src_v = 0;
    vid_in.p_src_y_phy = py_phy;
    vid_in.p_src_u_phy = puv_phy;
    vid_in.p_src_v_phy = 0;
    vid_in.org_img_width = width;
    vid_in.org_img_height =height;
    vid_in.crop_x = 0;
    vid_in.crop_y = 0;

    if (((*mMP4EncStrmEncode)(mHandle, &vid_in, &vid_out) != MMENC_OK)) {
        ERR ("mMP4EncStrmEncode error.\n ");
        return -1;
    }

    if((vid_out.strmSize > ONEFRAME_BITSTREAM_BFR_SIZE) || (vid_out.strmSize <= 0)) {
        ERR ("vid_out.strmSize error.\n ");
        return -1;
    }

    memcpy(pframe, vid_out.pOutBuf, vid_out.strmSize);
    *size = vid_out.strmSize;

//	*pyuv_rec = vid_out.pRecYUV;

    *type = vid_in.vopType;

    return 0;
}

static int32 enc_release(MP4Handle *mHandle) {
    if ((*mMP4EncRelease)(mHandle) != MMENC_OK ) {
        ERR ("mMP4EncRelease error.\n ");
        return -1;
    }

    return 0;
}

static const char* type2str(int type) {
    if (type == 0) {
        return "I";
    } else if (type == 1) {
        return "P";
    } else if (type == 2) {
        return "B";
    } else {
        return "S";
    }
}

#if defined(CALCULATE_PSNR)
static float psnr(uint8 *pframe_org, uint8 *pframe_rec, uint32 width, uint32 height) {
    uint32 sse = 0;
    for (uint32 i=0; i<width*height; i++) {
        int32 dif = (*pframe_org ++) - (*pframe_rec ++);
        sse += dif * dif;
    }
    return (sse == 0) ? 99.9f : (48.131f - 10.0f * log10f((float)sse / (float)(width*height)));
}
#endif

static int32 openEncoder(const char* libName) {
#if 0
    mMP4EncInit = &MP4EncInit;
    mMP4EncPreInit = &MP4EncPreInit;
    mMP4EncSetConf = &MP4EncSetConf;

    mMP4EncGetConf = &MP4EncGetConf;
    mMP4EncStrmEncode = &MP4EncStrmEncode;
    mMP4EncGenHeader = &MP4EncGenHeader;
    mMP4EncRelease = &MP4EncRelease;
    return 1;
#endif
    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }

    INFO("openEncoder, lib: %s\n",libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ERR("openEncoder, can't open lib: %s",libName);
        return -1;
    }

    mMP4EncPreInit = (FT_MP4EncPreInit)dlsym(mLibHandle, "MP4EncPreInit");
    if(mMP4EncPreInit == NULL) {
        ERR("Can't find MP4EncPreInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP4EncInit = (FT_MP4EncInit)dlsym(mLibHandle, "MP4EncInit");
    if(mMP4EncInit == NULL) {
        ERR("Can't find MP4EncInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP4EncSetConf = (FT_MP4EncSetConf)dlsym(mLibHandle, "MP4EncSetConf");
    if(mMP4EncSetConf == NULL) {
        ERR("Can't find MP4EncSetConf in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP4EncGetConf = (FT_MP4EncGetConf)dlsym(mLibHandle, "MP4EncGetConf");
    if(mMP4EncGetConf == NULL) {
        ERR("Can't find MP4EncGetConf in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP4EncStrmEncode = (FT_MP4EncStrmEncode)dlsym(mLibHandle, "MP4EncStrmEncode");
    if(mMP4EncStrmEncode == NULL) {
        ERR("Can't find MP4EncStrmEncode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP4EncGenHeader = (FT_MP4EncGenHeader)dlsym(mLibHandle, "MP4EncGenHeader");
    if(mMP4EncGenHeader == NULL) {
        ERR("Can't find MP4EncGenHeader in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP4EncRelease = (FT_MP4EncRelease)dlsym(mLibHandle, "MP4EncRelease");
    if(mMP4EncRelease == NULL) {
        ERR("Can't find MP4EncRelease in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    return 0;
}

int32 vsp_enc(char* filename_yuv, char* filename_bs, uint32 width, uint32 height, int32 format,
              uint32 framerate, uint32 max_key_interval, int32 cbr, uint32 bitrate, uint32 qp, uint32 frames = 0) {
    // yuv file and bs file
    FILE* fp_yuv = NULL;
    FILE* fp_bs = NULL;

    // yuv420p buffer, read from yuv file
    uint8* py = NULL;
    uint8* pu = NULL;
    uint8* pv = NULL;

    // yuv420p buffer, rec frame
    uint8* py_rec = NULL;
    uint8* pu_rec = NULL;
    uint8* pv_rec = NULL;

    // yuv420sp buffer, transform from yuv420p and to encode
    sp<MemoryHeapIon> pmem_yuv420sp = NULL;
    int32 size_yuv = 0;
    uint8* pyuv = NULL;
    uint8* pyuv_phy = NULL;

    // yuv420sp buffer, rec frame
    uint8* pyuv_rec = NULL;

    // bitstream buffer, encode from yuv420sp and write to bs file
    uint8 header_buffer[32];
    uint32 header_size = 32;
    uint8 frame_buffer[ONEFRAME_BITSTREAM_BFR_SIZE];
    uint32 frame_size = ONEFRAME_BITSTREAM_BFR_SIZE;

    uint32 framenum_yuv = 0;
    uint32 framenum_bs = 0;
    uint32 bs_total_len = 0;
    uint32 time_total_ms = 0;
    int32 bs_remain_len = bitrate/2;
#if defined(CALCULATE_PSNR)
    float psnr_total[3] = {.0f, .0f, .0f};
    float psnr_y = .0f;
    float psnr_u = .0f;
    float psnr_v = .0f;
#endif

    MP4Handle *mHandle = NULL;

    // VSP buffer
    sp<MemoryHeapIon> pmem_extra = NULL;
    uint8* pbuf_extra = NULL;
    uint_32or64 pbuf_extra_phy = 0;

    uint8* pbuf_inter = NULL;
    uint32 size_inter = 0;

    sp<MemoryHeapIon> pmem_stream = NULL;
    uint8* pbuf_stream = NULL;
    uint_32or64  pbuf_stream_phy = 0;
    uint32 size_stream = 0;

    uint_32or64 phy_addr = 0;
    size_t size = 0;
    uint32 size_extra = 0;
    fp_yuv = fopen(filename_yuv, "rb");
    if (fp_yuv == NULL) {
        ERR("Failed to open file %s\n", filename_yuv);
        goto err;
    }
    fp_bs = fopen(filename_bs, "wb");
    if (fp_bs == NULL) {
        ERR("Failed to open file %s\n", filename_bs);
        goto err;
    }

    /* yuv420p buffer */
    py = (uint8*)vsp_malloc(width * height * sizeof(uint8), 4);
    if (py == NULL) {
        ERR("Failed to alloc yuv buffer\n");
        goto err;
    }
    pu = (uint8*)vsp_malloc(width/2 * height/2 * sizeof(uint8), 4);
    if (pu == NULL) {
        ERR("Failed to alloc yuv buffer\n");
        goto err;
    }
    pv = (uint8*)vsp_malloc(width/2 * height/2 * sizeof(uint8), 4);
    if (pv == NULL) {
        ERR("Failed to alloc yuv buffer\n");
        goto err;
    }
    py_rec = (uint8*)vsp_malloc(width * height * sizeof(uint8), 4);
    if (py_rec == NULL) {
        ERR("Failed to alloc yuv rec buffer\n");
        goto err;
    }
    pu_rec = (uint8*)vsp_malloc(width/2 * height/2 * sizeof(uint8), 4);
    if (pu_rec == NULL) {
        ERR("Failed to alloc yuv rec buffer\n");
        goto err;
    }
    pv_rec = (uint8*)vsp_malloc(width/2 * height/2 * sizeof(uint8), 4);
    if (pv_rec == NULL) {
        ERR("Failed to alloc yuv rec buffer\n");
        goto err;
    }

    /*MMU Enable or not enable.  shark:not enable;dophin:enable */
    mIOMMUEnabled = MemoryHeapIon::Mm_iommu_is_enabled();
    INFO("IOMMU enabled: %d\n", mIOMMUEnabled);

    /* yuv420sp buffer */
    if (mIOMMUEnabled) {
        pmem_yuv420sp = new MemoryHeapIon("/dev/ion", width * height*3/2, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        pmem_yuv420sp = new MemoryHeapIon("/dev/ion", width * height*3/2, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    if (pmem_yuv420sp->getHeapID() < 0) {
        ERR("Failed to alloc yuv pmem buffer\n");
        goto err;
    }

    if (mIOMMUEnabled) {
        pmem_yuv420sp->get_iova(ION_MM, &phy_addr, &size);
    } else {
        pmem_yuv420sp->get_phy_addr_from_ion(&phy_addr, &size);
    }
    pyuv = (uint8*)pmem_yuv420sp->getBase();
    pyuv_phy = (unsigned char*)phy_addr;
    size_yuv = size;
    if (pyuv == NULL) {
        ERR("Failed to alloc yuv pmem buffer\n");
        goto err;
    }

    INFO("Try to encode %s[%dx%d] to %s, format = %s", filename_yuv, width, height, filename_bs, format2str(format));
    if (cbr)
    {
        INFO(", framerate = %d, bitrate = %dkbps\n", framerate, bitrate/1000);
    } else {
        INFO(", framerate = %d, QP = %d\n", framerate, qp);
    }

    mHandle = (MP4Handle *)vsp_malloc(sizeof(MP4Handle), 4);
    if (mHandle == NULL) {
        ERR("Failed to alloc mHandle\n");
        goto err;
    }
    memset(mHandle, 0, sizeof(MP4Handle));

    if (openEncoder("libomx_m4vh263enc_hw_sprd.so") < 0) {
        ERR("Failed to open library.\n");
        goto err;
    }

    /* step 1 - init vsp */
    size_inter = MP4ENC_INTERNAL_BUFFER_SIZE;
    pbuf_inter = (uint8*)vsp_malloc(size_inter, 4);
    if (pbuf_inter == NULL) {
        ERR("Failed to alloc inter memory\n");
        goto err;
    }
    INFO("pbuf_inter: 0x%p\n", pbuf_inter);

    size_extra = width * height * 3/2 * 2 + 10*1024;
    if (mIOMMUEnabled) {
        pmem_extra = new MemoryHeapIon("/dev/ion", size_extra, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        pmem_extra = new MemoryHeapIon("/dev/ion", size_extra, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    if (pmem_extra->getHeapID() < 0) {
        ERR("Failed to alloc extra memory\n");
        goto err;
    }

    if (mIOMMUEnabled) {
        pmem_extra->get_iova(ION_MM, &phy_addr, &size);
    } else {
        pmem_extra->get_phy_addr_from_ion(&phy_addr, &size);
    }
    pbuf_extra = (uint8*)pmem_extra->getBase();
    pbuf_extra_phy = phy_addr;
    if (pbuf_extra == NULL)
    {
        ERR("Failed to alloc extra memory\n");
        goto err;
    }
    INFO("pbuf_extra: 0x%p\n", pbuf_extra);

    size_stream = ONEFRAME_BITSTREAM_BFR_SIZE;
    if (mIOMMUEnabled) {
        pmem_stream = new MemoryHeapIon("/dev/ion", size_stream, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        pmem_stream = new MemoryHeapIon("/dev/ion", size_stream, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    if (mIOMMUEnabled) {
        pmem_stream->get_iova(ION_MM, &phy_addr, &size);
    } else {
        pmem_stream->get_phy_addr_from_ion(&phy_addr, &size);
    }
    pbuf_stream = (uint8*)pmem_stream->getBase();
    pbuf_stream_phy =  phy_addr;
    if (pbuf_stream == NULL) {
        ERR("Failed to alloc stream memory\n");
        goto err;
    }
    INFO("pbuf_stream: 0x%p\n", pbuf_stream);

    if (enc_init(mHandle, width, height, format, pbuf_inter, 0, size_inter, pbuf_extra, pbuf_extra_phy, size_extra, pbuf_stream, pbuf_stream_phy, size_stream) < 0) {
        ERR("Failed to init VSP\n");
        goto err;
    }
    INFO("enc_init end\n");

    /* step 2 - set vsp  and get header */
    if (enc_set_parameter(mHandle, format, framerate, cbr, bitrate, qp) < 0) {
        ERR("Failed to set parameter\n");
        goto err;
    }

    if (format) {	// m4v
        if (enc_get_header(mHandle, header_buffer, &header_size) < 0) {
            ERR("Failed to get header\n");
            goto err;
        }
        INFO("header: size = %d\n", header_size);
    }

    /* step 3 - encode with vsp */
    while (!feof(fp_yuv)) {
        // judge vop type
        int type = 0;
        if (max_key_interval == 0) {
            if (framenum_bs > 0) {
                type = 1;
            }
        } else {
            if ((framenum_bs % max_key_interval) != 0) {
                type = 1;
            }
        }

        // read yuv420p
        if (read_yuv_frame(py, pu, pv, width, height, fp_yuv) < 0) {
            break;
        }
        framenum_yuv ++;

        // yuv420p to yuv420sp
        yuv420p_to_yvu420sp(py, pu, pv, pyuv, pyuv+width*height, width, height);

        // encode yuv420sp to bitstream
        int64_t start = systemTime();
        if (enc_encode_frame(mHandle, width,height,pyuv, pyuv_phy, pyuv+width*height, pyuv_phy+width*height, frame_buffer, &frame_size, &pyuv_rec, framenum_bs*1000/framerate, &type, bs_remain_len) < 0) {
            ERR("Failed to encode frame.\n");
            break;
        }
        int64_t end = systemTime();
        bs_remain_len += (frame_size << 3);
        bs_remain_len -= bitrate / framerate;
        if (bs_remain_len > (int)bitrate) {
            bs_remain_len = bitrate;
        } else if (bs_remain_len < 0) {
            bs_remain_len = 0;
        }
        uint32 duration = (uint32)((end-start) / 1000000L);

        // psnr
#if defined(CALCULATE_PSNR)
        yvu420sp_to_yuv420p(pyuv_rec, pyuv_rec+width*height, py_rec, pu_rec, pv_rec, width, height);
        psnr_y = psnr(py, py_rec, width, height);
        psnr_u = psnr(pu, pu_rec, width/2, height/2);
        psnr_v = psnr(pv, pv_rec, width/2, height/2);
        psnr_total[0] += psnr_y;
        psnr_total[1] += psnr_u;
        psnr_total[2] += psnr_v;
#endif

        // write header if key frame for MPEG4
        if (format && (type == 0)) {
            if (fwrite(header_buffer, sizeof(uint8), header_size, fp_bs) != header_size) {
                break;
            }
            bs_total_len += header_size;
        }

        // write frame
        if (fwrite(frame_buffer, sizeof(uint8), frame_size, fp_bs) != frame_size) {
            break;
        }
        bs_total_len += frame_size;
        time_total_ms += duration;
        framenum_bs++;

        INFO("frame %d: time = %dms, type = %s, size = %d ", framenum_bs, duration, type2str(type), frame_size);
#if defined(CALCULATE_PSNR)
        INFO("psnr (%2.2f %2.2f %2.2f)", psnr_y, psnr_u, psnr_v);
#endif
        INFO("\n");

        if (frames != 0) {
            if (framenum_bs >= frames) {
                break;
            }
        }
    }

    /* step 4 - release vsp */
    enc_release(mHandle);

    INFO("\nFinish encoding %s[%dx%d] to %s(%s, size = %d)\n", filename_yuv, width, height, filename_bs, format2str(format), bs_total_len);
    if (framenum_bs > 0) {
        INFO("average time = %dms, average bitrate = %dkbps", time_total_ms/framenum_bs, bs_total_len*8*framerate/framenum_bs/1000);

#if defined(CALCULATE_PSNR)
        INFO(", average psnr (%2.2f %2.2f %2.2f)", psnr_total[0]/framenum_bs, psnr_total[1]/framenum_bs, psnr_total[2]/framenum_bs);
#endif
        INFO("\n");
    }

err:
    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }

    vsp_free (mHandle);
    vsp_free(pbuf_inter);

    if (pbuf_extra != NULL) {
        if (mIOMMUEnabled) {
            pmem_extra->free_iova(ION_MM, (uint_32or64)pbuf_extra_phy, size_extra);
        }
        pmem_extra.clear();
        pbuf_extra = NULL;
        pbuf_extra_phy = 0;
        size_extra = 0;
    }

    if (pbuf_stream != NULL) {
        if (mIOMMUEnabled) {
            pmem_stream->free_iova(ION_MM, (uint_32or64)pbuf_stream_phy, size_stream);
        }
        pmem_stream.clear();
        pbuf_stream = NULL;
        pbuf_stream_phy = 0;
        size_stream = 0;
    }

    if (pyuv != NULL) {
        if (mIOMMUEnabled) {
            pmem_yuv420sp->free_iova(ION_MM, (uint_32or64)pyuv_phy, size_yuv);
        }
        pmem_yuv420sp.clear();
        pyuv = NULL;
        pyuv_phy = 0;
        size_yuv = 0;
    }

    vsp_free(py_rec);
    vsp_free(pu_rec);
    vsp_free(pv_rec);
    vsp_free(py);
    vsp_free(pu);
    vsp_free(pv);

    if (fp_yuv != NULL) {
        fclose(fp_yuv);
        fp_yuv = NULL;
    }
    if (fp_bs != NULL) {
        fclose(fp_bs);
        fp_bs = NULL;
    }

    return 0;
}

int main(int argc, char **argv) {
    char* filename_yuv = NULL;
    char* filename_bs = NULL;
    uint32 width = 0;
    uint32 height = 0;
    int32 format = MPEG4;
    uint32 framerate = 25;
    uint32 max_key_interval = 30;
    int32 cbr = 1;
    uint32 bitrate = 512;
    uint32 qp = 8;
    uint32 frames = 0;
    int32 i;

    /* parse argument */
    if (argc < 9) {
        usage();
        return -1;
    }

    for (i=1; i<argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && (i < argc-1)) {
            filename_yuv = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && (i < argc-1)) {
            filename_bs = argv[++i];
        } else if (strcmp(argv[i], "-w") == 0 && (i < argc-1)) {
            width = CLIP_16(atoi(argv[++i]));
        } else if (strcmp(argv[i], "-h") == 0 && (i < argc-1)) {
            height = CLIP_16(atoi(argv[++i]));
        } else if (strcmp(argv[i], "-format") == 0 && (i < argc-1)) {
            format = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-framerate") == 0 && (i < argc-1)) {
            framerate = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-max_key_interval") == 0 && (i < argc-1)) {
            max_key_interval = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-bitrate") == 0 && (i < argc-1)) {
            cbr = 1;
            bitrate = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-qp") == 0 && (i < argc-1)) {
            cbr = 0;
            qp = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-frames") == 0 && (i < argc-1)) {
            frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-help") == 0) {
            usage();
            return 0;
        } else {
            usage();
            return -1;
        }
    }

    /* check argument */
    if ((filename_yuv == NULL) || (filename_bs == NULL)) {
        usage();
        return -1;
    }
    if ((width == 0) || (height == 0) || (framerate == 0)) {
        usage();
        return -1;
    }

    return vsp_enc(filename_yuv, filename_bs, width, height, format, framerate, max_key_interval, cbr, bitrate*1000, qp, frames);
}

