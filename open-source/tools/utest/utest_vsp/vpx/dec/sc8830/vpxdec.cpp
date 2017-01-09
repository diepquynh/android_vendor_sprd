#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>

#include <linux/ion.h>
#include "MemoryHeapIon.h"
#include "ion_sprd.h"
using namespace android;

#include "vpx_dec_api.h"
#include "util.h"

#define VP8_DECODER_INTERNAL_BUFFER_SIZE  (0x200000) //(MP4DEC_OR_RUN_SIZE+MP4DEC_OR_INTER_MALLOC_SIZE)  
#define ONEFRAME_BITSTREAM_BFR_SIZE	(1500*1024)  //for bitstream size of one encoded frame.
#define DEC_YUV_BUFFER_NUM   5

static void usage()
{
    INFO("usage:\n");
    INFO("utest_vsp_dec -i filename_bitstream -o filename_yuv [OPTIONS]\n");
    INFO("-i       string: input bitstream filename\n");
    INFO("[OPTIONS]:\n");
    INFO("-o       string: output yuv filename\n");
    INFO("-format integer: video format(0:ITU_H263 / 1:MPEG4 / 2:MJPG / 3:FLVH263 / 4:H264 / 5:VP8 / 6: VP9), auto detection if default\n");
    INFO("-frames integer: number of frames to decode, default is 0 means all frames\n");
    INFO("-help          : show this help message\n");
    INFO("Built on %s %s, Written by XiaoweiLuo(xiaowei.luo@spreadtrum.com)\n", __DATE__, __TIME__);
}

void* mLibHandle;
FT_VPXDecSetCurRecPic mVPXDecSetCurRecPic;
FT_VPXDecInit mVPXDecInit;
FT_VPXDecDecode mVPXDecDecode;
FT_VPXDecRelease mVPXDecRelease;

static int32 dec_init(VPXHandle *mHandle, int32 format, uint8* pheader_buffer, uint32 header_size,
                      uint8* pbuf_inter, uint_32or64 pbuf_inter_phy, uint32 size_inter,
                      uint8* pbuf_extra, uint_32or64 pbuf_extra_phy, uint32 size_extra)
{
    MMCodecBuffer InterMemBfr;
    MMCodecBuffer ExtraMemBfr;
    MMDecVideoFormat video_format;

    INFO("dec_init IN\n");

    InterMemBfr.common_buffer_ptr = pbuf_inter;
    InterMemBfr.common_buffer_ptr_phy = pbuf_inter_phy;
    InterMemBfr.size = size_inter;

    ExtraMemBfr.common_buffer_ptr = pbuf_extra;
    ExtraMemBfr.common_buffer_ptr_phy = pbuf_extra_phy;
    ExtraMemBfr.size	= size_extra;

    video_format.video_std = format;
    video_format.i_extra = header_size;
    video_format.p_extra = pheader_buffer;
    video_format.frame_width = 0;
    video_format.frame_height = 0;
    video_format.yuv_format = YUV420SP_NV12;
    if ((*mVPXDecInit)(mHandle, &InterMemBfr, &ExtraMemBfr, &video_format) != MMDEC_OK) {
        ERR ("mVPXDecInit error.\n ");
        return -1;
    }

    INFO("dec_init OUT\n");

    return MMDEC_OK;
}

static int32 dec_decode_frame(VPXHandle *mHandle, uint8* pframe, uint_32or64 pframe_phy, uint32 size, int32* frame_effective, uint32* width, uint32* height, int32* type)
{
    MMDecInput dec_in;
    MMDecOutput dec_out;

    dec_in.pStream= pframe;
    dec_in.pStream_phy= pframe_phy;
    dec_in.dataLen = size;
    dec_in.beLastFrm = 0;
    dec_in.expected_IVOP = 0;
    dec_in.beDisplayed = 1;
    dec_in.err_pkt_num = 0;

    dec_out.VopPredType = -1;
    dec_out.frameEffective = 0;
    MMDecRet ret = (*mVPXDecDecode)(mHandle, &dec_in, &dec_out);
    if(ret == MMDEC_MEMORY_ALLOCED) {
        ret = (*mVPXDecDecode)(mHandle, &dec_in, &dec_out);
    }
    if (ret != MMDEC_OK) {
        ERR ("mVPXDecDecode error.\n ");
        return -1;
    }

    *width = dec_out.frame_width;
    *height = dec_out.frame_height;
    *type = dec_out.VopPredType;
    *frame_effective = dec_out.frameEffective;

    return 0;
}

static int32 dec_release(VPXHandle *mHandle)
{
    if ((*mVPXDecRelease)(mHandle) != MMDEC_OK) {
        ERR("mVPXDecRelease err.\n");
        return -1;
    }
    return 0;
}

static const char* type2str(int type)
{
    if (type == 0) {
        return "I";
    } else if (type == 1) {
        return "P";
    } else if (type == 2) {
        return "B";
    } else {
        return "N";
    }
}

static int32 openDecoder(const char* libName)
{
    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }

    INFO("openDecoder, lib: %s\n",libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ERR("openDecoder, can't open lib: %s",libName);
        return -1;
    }

    mVPXDecSetCurRecPic = (FT_VPXDecSetCurRecPic)dlsym(mLibHandle, "VP8DecSetCurRecPic");
    if(mVPXDecSetCurRecPic == NULL) {
        ERR("Can't find VPXDecSetCurRecPic in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mVPXDecInit = (FT_VPXDecInit)dlsym(mLibHandle, "VP8DecInit");
    if(mVPXDecInit == NULL) {
        ERR("Can't find VP8DecInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mVPXDecDecode = (FT_VPXDecDecode)dlsym(mLibHandle, "VP8DecDecode");
    if(mVPXDecDecode == NULL) {
        ERR("Can't find VP8DecDecode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mVPXDecRelease = (FT_VPXDecRelease)dlsym(mLibHandle, "VP8DecRelease");
    if(mVPXDecRelease == NULL) {
        ERR("Can't find VP8DecRelease in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    return 0;
}

static bool mIOMMUEnabled = false;

int main(int argc, char **argv)
{
    char *filename_bs = NULL;
    FILE *fp_bs = NULL;
    char *filename_yuv = NULL;
    FILE *fp_yuv = NULL;
    int32 format = VP8;
    uint32 frames = 0;
    uint32 width = 320;
    uint32 height = 240;

    uint32 startcode = 0;
    uint32 maskcode = 0;
    int32 i;

    VPXHandle *mHandle = NULL;

    // bitstream buffer, read from bs file
    uint8 buffer_data[ONEFRAME_BITSTREAM_BFR_SIZE];
    int32 buffer_size = 0;

    // yuv420sp buffer, decode from bs buffer
    sp<MemoryHeapIon> pmem_yuv420sp[DEC_YUV_BUFFER_NUM] = {NULL};
    size_t size_yuv[DEC_YUV_BUFFER_NUM]  = {0};
    uint8* pyuv[DEC_YUV_BUFFER_NUM] = {NULL};
    uint_32or64 pyuv_phy[DEC_YUV_BUFFER_NUM] = {0};

    // yuv420p buffer, transform from yuv420sp and write to yuv file
    uint8* py = NULL;
    uint8* pu = NULL;
    uint8* pv = NULL;

    uint32 framenum_bs = 0;
    uint32 framenum_err = 0;
    uint32 framenum_yuv = 0;
    uint32 time_total_ms = 0;

    // VSP buffer
    sp<MemoryHeapIon> pmem_extra = NULL;
    uint8* pbuf_extra = NULL;
    uint_32or64 pbuf_extra_phy = 0;
    uint32 size_extra = 0;

    uint8* pbuf_inter = NULL;
    uint32 size_inter = 0;

    sp<MemoryHeapIon> pmem_stream = NULL;
    uint8* pbuf_stream = NULL;
    uint_32or64 pbuf_stream_phy = 0;
    size_t size_stream = 0;

    uint_32or64 phy_addr = 0;
    size_t size = 0;

    /* parse argument */
    if (argc < 3) {
        usage();
        return -1;
    }

    for (i=1; i<argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && (i < argc-1)) {
            filename_bs = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && (i < argc-1)) {
            filename_yuv = argv[++i];
        } else if (strcmp(argv[i], "-format") == 0 && (i < argc-1)) {
            format = atoi(argv[++i]);
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
    if (filename_bs == NULL) {
        usage();
        return -1;
    }

    fp_bs = fopen(filename_bs, "rb");
    if (fp_bs == NULL) {
        ERR("Failed to open file %s\n", filename_bs);
        goto err;
    }
    if (filename_yuv != NULL) {
        fp_yuv = fopen(filename_yuv, "wb");
        if (fp_yuv == NULL) {
            ERR("Failed to open file %s\n", filename_yuv);
            goto err;
        }
    }

    if (format != VP8) {
        ERR("error format %s\n", format2str(format));
        goto err;
    }

    /*MMU Enable or not enable.  shark:not enable;dophin:enable */
    mIOMMUEnabled = MemoryHeapIon::Mm_iommu_is_enabled();
    INFO("IOMMU enabled: %d\n", mIOMMUEnabled);

    /* bs buffer */
    if (mIOMMUEnabled) {
        pmem_stream = new MemoryHeapIon("/dev/ion", ONEFRAME_BITSTREAM_BFR_SIZE, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        pmem_stream = new MemoryHeapIon("/dev/ion", ONEFRAME_BITSTREAM_BFR_SIZE, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    if (pmem_stream->getHeapID() < 0) {
        ERR("Failed to alloc bitstream pmem buffer\n");
        goto err;
    }
    if (mIOMMUEnabled) {
        pmem_stream->get_iova(ION_MM, &phy_addr, &size_stream);
    } else {
        pmem_stream->get_phy_addr_from_ion(&phy_addr, &size_stream);
    }
    pbuf_stream = (uint8*)pmem_stream->getBase();
    pbuf_stream_phy = phy_addr;
    if (pbuf_stream == NULL) {
        ERR("Failed to alloc bitstream pmem buffer\n");
        goto err;
    }

    /* yuv420sp buffer */
    for (i=0; i<DEC_YUV_BUFFER_NUM; i++) {
        if (mIOMMUEnabled) {
            pmem_yuv420sp[i] = new MemoryHeapIon("/dev/ion", width*height*3/2, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pmem_yuv420sp[i] = new MemoryHeapIon("/dev/ion", width*height*3/2 * DEC_YUV_BUFFER_NUM, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
        }
        if (pmem_yuv420sp[i]->getHeapID() < 0) {
            ERR("Failed to alloc yuv pmem buffer\n");
            goto err;
        }

        if (mIOMMUEnabled) {
            pmem_yuv420sp[i]->get_iova(ION_MM, &phy_addr, &(size_yuv[i]));
        } else {
            pmem_yuv420sp[i]->get_phy_addr_from_ion(&phy_addr, &(size_yuv[i]));
        }
        pyuv[i] = ((uint8*)pmem_yuv420sp[i]->getBase()) ;
        pyuv_phy[i] = phy_addr ;

        if (pyuv[i] == NULL) {
            ERR("Failed to alloc yuv pmem buffer\n");
            goto err;
        }
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

    INFO("Try to decode %s to %s, format = %s\n", filename_bs, filename_yuv, format2str(format));

    mHandle = (VPXHandle *)vsp_malloc(sizeof(VPXHandle), 4);
    if (mHandle == NULL) {
        ERR("Failed to alloc mHandle.\n");
        goto err;
    }
    memset(mHandle, 0, sizeof(VPXHandle));

    if (openDecoder("libomx_vpxdec_hw_sprd.so") < 0) {
        ERR("Failed to open library.\n");
        goto err;
    }

    /* step 1 - init vsp */
    size_inter = VP8_DECODER_INTERNAL_BUFFER_SIZE;
    pbuf_inter = (uint8*)vsp_malloc(size_inter, 4);
    if (pbuf_inter == NULL) {
        ERR("Failed to alloc inter memory\n");
        goto err;
    }

    size_extra = 5000 * 1024;
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
    if (pbuf_extra == NULL) {
        ERR("Failed to alloc extra memory\n");
        goto err;
    }
    INFO("pbuf_extra: 0x%p\n", pbuf_extra);

    if (dec_init(mHandle, format, NULL, 0, pbuf_inter, 0, size_inter, pbuf_extra, pbuf_extra_phy, size_extra) != 0) {
        ERR("Failed to init VSP\n");
        goto err;
    }

    /* step 2 - decode with vsp */
    startcode = table_startcode2[format];
    maskcode = table_maskcode2[format];
    while (!feof(fp_bs)) {
        int read_size = fread(buffer_data+buffer_size, 1, ONEFRAME_BITSTREAM_BFR_SIZE-buffer_size, fp_bs);
        if (read_size <= 0) {
            break;
        }
        buffer_size += read_size;

        uint8* ptmp = buffer_data ;
        uint32 frame_size = 0;
        uint32 width_header = 0;
        uint32 height_header = 0;

        ptmp += 32;

        while (buffer_size > 0) {
            frame_size = (ptmp[3] <<24)	|  (ptmp[2] <<16)	|  (ptmp[1] <<8)	|  (ptmp[0] <<0);

            INFO("frame_size : %d\t", frame_size);
            ptmp += 12;// Header length.

            if (frame_size == 0) {
                break;
            }

            // read a bitstream frame
            memcpy(pbuf_stream, ptmp, frame_size);
            ptmp += frame_size;
            buffer_size -= frame_size;

            // decode bitstream to yuv420sp
            int32 frame_effective = 0;
            int32 type = 0;
            uint32 width_new = 0;
            uint32 height_new = 0;
            uint8* pyuv420sp = pyuv[framenum_bs % DEC_YUV_BUFFER_NUM];
            uint_32or64 pyuv420sp_phy = pyuv_phy[framenum_bs % DEC_YUV_BUFFER_NUM];
            (*mVPXDecSetCurRecPic)(mHandle, pyuv420sp, (uint8*)pyuv420sp_phy, NULL);
            framenum_bs ++;
            int64_t start = systemTime();
            if (dec_decode_frame(mHandle, pbuf_stream,pbuf_stream_phy, frame_size, &frame_effective, &width_new, &height_new, &type) < 0) {
                ERR("failed to decode one frame");
                goto err;
            }
            int64_t end = systemTime();
            unsigned int duration = (uint32)((end-start) / 1000000L);
            time_total_ms += duration;

            if (duration < 40) {
                usleep((40 - duration)*1000);
            }

            if ((width_new != width) || (height_new != height)) {
                width = width_new;
                height = height_new;
            }
            INFO("frame %d[%dx%d]: time = %dms, size = %d, type = %s, effective(%d)\n", framenum_bs, width, height, duration, frame_size, type2str(type), frame_effective);

            if ((frame_effective) && (fp_yuv != NULL)) {
                // yuv420sp to yuv420p
                yuv420sp_to_yuv420p(pyuv420sp, pyuv420sp+width*height, py, pu, pv, width, height);

                // write yuv420p
                if (write_yuv_frame(py, pu, pv, width, height, fp_yuv)!= 0) {
                    break;
                }

                framenum_yuv ++;
            }

            if (frames != 0) {
                if (framenum_yuv >= frames) {
                    break;
                }
            }
        }

        if (buffer_size > 0) {
            memmove(buffer_data, ptmp, buffer_size);
        }
    }

    /* step 3 - release vsp */
    if (dec_release(mHandle) < 0) {
        ERR("Failed to release decoder");
        goto err;
    }

    INFO("Finish decoding %s(%s, %d frames) to %s(%d frames)", filename_bs, format2str(format), framenum_bs, filename_yuv, framenum_yuv);
    if (framenum_err > 0) {
        INFO(", %d frames failed", framenum_err);
    }
    if (framenum_bs > 0) {
        INFO(", average time = %dms", time_total_ms/framenum_bs);
    }
    INFO("\n");

err:
    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }

    vsp_free (mHandle);
    vsp_free(pbuf_inter);

    if (pbuf_extra != NULL) {
        if (mIOMMUEnabled) {
            pmem_extra->free_iova(ION_MM, (int32)pbuf_extra_phy, size_extra);
        }
        pmem_extra.clear();
        pbuf_extra = NULL;
        pbuf_extra_phy = 0;
        size_extra = 0;
    }

    if (pbuf_stream != NULL) {
        if (mIOMMUEnabled) {
            pmem_stream->free_iova(ION_MM, (int32)pbuf_stream_phy, size_stream);
        }
        pmem_stream.clear();
        pbuf_stream = NULL;
        pbuf_stream_phy = 0;
        size_stream = 0;
    }

    for (i=0; i<DEC_YUV_BUFFER_NUM; i++) {
        if (pyuv[i] != NULL) {
            if (mIOMMUEnabled) {
                pmem_yuv420sp[i]->free_iova(ION_MM, (int32)pyuv_phy[i], size_yuv[i]);
            }
            pmem_yuv420sp[i].clear();
            pyuv[i] = NULL;
            pyuv_phy[i] = 0;
            size_yuv[i] = 0;
        }
    }

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

