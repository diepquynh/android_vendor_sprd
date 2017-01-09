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

#include "m4v_h263_dec_api.h"
#include "util.h"

#define MP4DEC_INTERNAL_BUFFER_SIZE  (0x200000) //(MP4DEC_OR_RUN_SIZE+MP4DEC_OR_INTER_MALLOC_SIZE)  
#define ONEFRAME_BITSTREAM_BFR_SIZE	(1500*1024)  //for bitstream size of one encoded frame.
#define DEC_YUV_BUFFER_NUM   3

typedef enum {
    H263_MODE = 0,MPEG4_MODE,
    FLV_MODE,
    UNKNOWN_MODE
} MP4DecodingMode;

static void usage() {
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

void *mLibHandle = NULL;

FT_MP4DecSetCurRecPic mMP4DecSetCurRecPic;
FT_MP4DecInit mMP4DecInit;
FT_MP4DecVolHeader mMP4DecVolHeader;
FT_MP4DecMemInit mMP4DecMemInit;
FT_MP4DecDecode mMP4DecDecode;
FT_MP4DecRelease mMP4DecRelease;
FT_Mp4GetVideoDimensions mMp4GetVideoDimensions;
FT_Mp4GetBufferDimensions mMp4GetBufferDimensions;
FT_MP4DecReleaseRefBuffers mMP4DecReleaseRefBuffers;

uint32 framenum_bs = 0;

sp<MemoryHeapIon> s_pmem_extra;
uint8*  s_pbuf_extra_v;
uint_32or64  s_pbuf_extra_p;
uint32 s_pbuf_extra_size;
static bool mIOMMUEnabled = false;
// yuv420sp buffer, decode from bs buffer
sp<MemoryHeapIon> pmem_yuv420sp[DEC_YUV_BUFFER_NUM] = {NULL};
size_t size_yuv[DEC_YUV_BUFFER_NUM]  = {0};
uint8* pyuv[DEC_YUV_BUFFER_NUM] = {NULL};
uint_32or64 pyuv_phy[DEC_YUV_BUFFER_NUM] = {0};
// yuv420p buffer, transform from yuv420sp and write to yuv file
uint8* py = NULL;
uint8* pu = NULL;
uint8* pv = NULL;

uint32 mWidth = 1280;//176;
uint32 mHeight = 720;//144;

int32 extMemoryAlloc(void *  mHandle, uint32 extra_mem_size) {

    MMCodecBuffer extra_mem[MAX_MEM_TYPE];

    if (mIOMMUEnabled) {
        s_pmem_extra = new MemoryHeapIon("/dev/ion", extra_mem_size, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);

    } else {
        s_pmem_extra = new MemoryHeapIon("/dev/ion", extra_mem_size, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    int fd = s_pmem_extra->getHeapID();
    if(fd>=0) {
        uint_32or64 phy_addr;
        size_t  buffer_size;
        int ret;

        if (mIOMMUEnabled) {
            ret = s_pmem_extra->get_iova(ION_MM, &phy_addr, &buffer_size);
        } else {
            ret = s_pmem_extra->get_phy_addr_from_ion(&phy_addr, &buffer_size);
        }
        if((ret < 0) || (buffer_size < extra_mem_size)) {
            ERR ("s_pmem_extra: get phy addr fail, ret: %d, buffer_size: %d, extra_mem_size: %d\n", ret, buffer_size, extra_mem_size);
        }

        s_pbuf_extra_p  =  phy_addr;
        s_pbuf_extra_v = (uint8*)s_pmem_extra->getBase();
        s_pbuf_extra_size = extra_mem_size;
        extra_mem[HW_NO_CACHABLE].common_buffer_ptr =(uint8 *) s_pbuf_extra_v;
        extra_mem[HW_NO_CACHABLE].common_buffer_ptr_phy = s_pbuf_extra_p;
        extra_mem[HW_NO_CACHABLE].size = extra_mem_size;
    }

    (*mMP4DecMemInit)((MP4Handle *)mHandle, extra_mem);

    return 0;
}

static int32 dec_init(MP4Handle *mHandle, int32 format, uint8* pheader_buffer, uint32 header_size,
                      uint8* pbuf_inter, uint_32or64 pbuf_inter_phy, uint32 size_inter) {
    MMCodecBuffer InterMemBfr;
    MMCodecBuffer ExtraMemBfr;
    MMDecVideoFormat video_format;
    int32 ret;

    INFO("dec_init IN\n");

    InterMemBfr.common_buffer_ptr = pbuf_inter;
    InterMemBfr.common_buffer_ptr_phy = pbuf_inter_phy;
    InterMemBfr.size = size_inter;

    video_format.video_std = format;
    video_format.i_extra = header_size;
    video_format.p_extra = pheader_buffer;
    video_format.frame_width = 0;
    video_format.frame_height = 0;

    ret = (*mMP4DecInit)(mHandle, &InterMemBfr);

    INFO("dec_init OUT\n");

    return ret;
}

static int32 dec_free_yuv_buffer()
{
    int i;
    for (i=0; i<DEC_YUV_BUFFER_NUM; i++) {
        if (pyuv[i] != NULL) {
            if (mIOMMUEnabled) {
                pmem_yuv420sp[i]->free_iova(ION_MM, (uint_32or64)pyuv_phy[i], size_yuv[i]);
            }
            pmem_yuv420sp[i].clear();
            pyuv[i] = NULL;
            pyuv_phy[i] = 0;
            size_yuv[i] = 0;
        }
    }

    if (py != NULL) {
        vsp_free(py);
        py = NULL;
    }
    if (pu != NULL) {
        vsp_free(pu);
        pu = NULL;
    }
    if (pv != NULL) {
        vsp_free(pv);
        pv = NULL;
    }

    return 0;
}

static int32 dec_alloc_yuv_buffer(int32 width, int32 height)
{
    int i;
    uint_32or64 phy_addr = 0;

    if (((width == mWidth) && (height == mHeight))
            && (pyuv[0] != NULL) )
    {
        return 1;
    }
    else
    {
        dec_free_yuv_buffer();
        mWidth  = width;
        mHeight = height;
    }

    /* yuv420sp buffer */
    for (i=0; i<DEC_YUV_BUFFER_NUM; i++) {
        if (mIOMMUEnabled) {
            pmem_yuv420sp[i] = new MemoryHeapIon("/dev/ion", width*height*3/2, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pmem_yuv420sp[i] = new MemoryHeapIon("/dev/ion", width*height*3/2, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
        }

        if (pmem_yuv420sp[i]->getHeapID() < 0) {
            ERR("Failed to alloc yuv pmem buffer\n");
            return -1;
        }

        if (mIOMMUEnabled) {
            pmem_yuv420sp[i]->get_iova(ION_MM, &phy_addr, &(size_yuv[i]));
        } else {
            pmem_yuv420sp[i]->get_phy_addr_from_ion(&phy_addr, &(size_yuv[i]));
        }

        pyuv[i] = (uint8*)(pmem_yuv420sp[i]->getBase());
        pyuv_phy[i] = phy_addr;
        if (pyuv[i] == NULL) {
            ERR("Failed to alloc yuv pmem buffer\n");
            return -1;
        }
    }
    /* yuv420p buffer */
    py = (uint8*)vsp_malloc(width * height * sizeof(uint8), 4);
    if (py == NULL) {
        ERR("Failed to alloc yuv buffer\n");
        return -1;
    }
    pu = (uint8*)vsp_malloc(width/2 * height/2 * sizeof(uint8), 4);
    if (pu == NULL) {
        ERR("Failed to alloc yuv buffer\n");
        return -1;
    }
    pv = (uint8*)vsp_malloc(width/2 * height/2 * sizeof(uint8), 4);
    if (pv == NULL) {
        ERR("Failed to alloc yuv buffer\n");
        return -1;
    }
    return 0;
}

static int32 dec_decode_frame(MP4Handle *mHandle, uint8* pframe, uint32 pframe_y, uint32 size, MMDecOutput *dec_out) {
    MMDecInput dec_in;
    int32 ret;
    int32 frame_width, frame_height;
    uint8* pyuv420sp;
    uint_32or64 pyuv420sp_phy;
    int8 memory_alloced = 0;

    do {
        dec_in.pStream= pframe;
        dec_in.pStream_phy= pframe_y;
        dec_in.dataLen = size;
        dec_in.beLastFrm = 0;
        dec_in.expected_IVOP = 0;
        dec_in.beDisplayed = 1;
        dec_in.err_pkt_num = 0;

        dec_out->VopPredType = -1;
        dec_out->frameEffective = 0;

        pyuv420sp = pyuv[framenum_bs % DEC_YUV_BUFFER_NUM];
        pyuv420sp_phy = pyuv_phy[framenum_bs % DEC_YUV_BUFFER_NUM];
        (*mMP4DecSetCurRecPic)(mHandle, pyuv420sp, (uint8*)pyuv420sp_phy, NULL);

        ret = (*mMP4DecDecode)(mHandle, &dec_in, dec_out);
        if (ret == MMDEC_ERROR) {
            return ret;
        }

        if (ret == MMDEC_MEMORY_ALLOCED)
        {
            (*mMp4GetBufferDimensions)(mHandle, &frame_width, &frame_height);
            INFO("mMp4GetBufferDimensions , frame_width = %d,frame_height = %d\n", frame_width, frame_height);
            dec_alloc_yuv_buffer(frame_width, frame_height);
        }
    } while (ret == MMDEC_MEMORY_ALLOCED);//framenum_bs == 1 &&

    return ret;
}

static int32 dec_release(MP4Handle *mHandle) {
    (*mMP4DecReleaseRefBuffers)(mHandle);

    if ((*mMP4DecRelease)(mHandle) != MMDEC_OK) {
        ERR("mMP4DecRelease err\n");
        return -1;
    }

    return 0;
}

static int32 detect_format(uint8* pbuffer, uint32 size) {
    if((pbuffer[0] == 0x00) && (pbuffer[1] == 0x00)) {
        if((pbuffer[2]&0xFC) == 0x80) {
            return ITU_H263;
        } else if((pbuffer[2]&0xF8) == 0x80) {
            return FLV_V1;
        }
    }

    return MPEG4;
}

static const char* type2str(int type) {
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

static int32 openDecoder(const char* libName) {
#if 0
    mMP4DecSetCurRecPic = &MP4DecSetCurRecPic;
    mMP4DecInit = &MP4DecInit;
    mMP4DecVolHeader = &MP4DecVolHeader;
    mMP4DecMemInit = &MP4DecMemInit;

    mMP4DecDecode = &MP4DecDecode;
    mMP4DecRelease = &MP4DecRelease;
    mMp4GetVideoDimensions = &Mp4GetVideoDimensions;
    mMp4GetBufferDimensions =  &Mp4GetBufferDimensions;
    mMP4DecReleaseRefBuffers = &MP4DecReleaseRefBuffers;
    return 0;
#endif
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

    mMP4DecSetCurRecPic = (FT_MP4DecSetCurRecPic)dlsym(mLibHandle, "MP4DecSetCurRecPic");
    if(mMP4DecSetCurRecPic == NULL) {
        ERR("Can't find MP4DecSetCurRecPic in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP4DecInit = (FT_MP4DecInit)dlsym(mLibHandle, "MP4DecInit");
    if(mMP4DecInit == NULL) {
        ERR("Can't find MP4DecInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP4DecVolHeader = (FT_MP4DecVolHeader)dlsym(mLibHandle, "MP4DecVolHeader");
    if(mMP4DecVolHeader == NULL) {
        ERR("Can't find MP4DecVolHeader in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP4DecMemInit = (FT_MP4DecMemInit)dlsym(mLibHandle, "MP4DecMemInit");
    if(mMP4DecMemInit == NULL) {
        ERR("Can't find MP4DecMemInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP4DecDecode = (FT_MP4DecDecode)dlsym(mLibHandle, "MP4DecDecode");
    if(mMP4DecDecode == NULL) {
        ERR("Can't find MP4DecDecode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP4DecRelease = (FT_MP4DecRelease)dlsym(mLibHandle, "MP4DecRelease");
    if(mMP4DecRelease == NULL) {
        ERR("Can't find MP4DecRelease in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMp4GetVideoDimensions = (FT_Mp4GetVideoDimensions)dlsym(mLibHandle, "Mp4GetVideoDimensions");
    if(mMp4GetVideoDimensions == NULL) {
        ERR("Can't find Mp4GetVideoDimensions in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMp4GetBufferDimensions = (FT_Mp4GetBufferDimensions)dlsym(mLibHandle, "Mp4GetBufferDimensions");
    if(mMp4GetBufferDimensions == NULL) {
        ERR("Can't find Mp4GetBufferDimensions in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP4DecReleaseRefBuffers = (FT_MP4DecReleaseRefBuffers)dlsym(mLibHandle, "MP4DecReleaseRefBuffers");
    if(mMP4DecReleaseRefBuffers == NULL) {
        ERR("Can't find MP4DecReleaseRefBuffers in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    char* filename_bs = NULL;
    FILE* fp_bs = NULL;
    char* filename_yuv = NULL;
    FILE* fp_yuv = NULL;
    int32 format = MPEG4;
    uint32 frames = 0;

    uint32 startcode = 0;
    uint32 maskcode = 0;
    int i;

    MP4Handle *mHandle = NULL;
    MP4DecodingMode mode;
    MMDecVideoFormat video_format;

    // bitstream buffer, read from bs file
    uint8 buffer_data[ONEFRAME_BITSTREAM_BFR_SIZE];
    int32 buffer_size = 0;

    uint32 framenum_err = 0;
    uint32 framenum_yuv = 0;
    uint32 time_total_ms = 0;

    // VSP buffer
    uint8* pbuf_inter = NULL;
    uint32 size_inter = 0;

    sp<MemoryHeapIon> pmem_stream = NULL;
    uint8* pbuf_stream = NULL;
    uint_32or64 pbuf_stream_phy = 0;
    uint32 size_stream = 0;

    uint32 size_extra = 0;

    uint_32or64 phy_addr = 0;
    size_t size = 0;
    framenum_bs = 0;

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

    if ((format < 0) || (format >= FORMAT_MAX)) {
        // detect bistream format
        uint8 header_data[64];
        int32 header_size = fread(header_data, 1, 64, fp_bs);
        if (header_size <= 0) {
            ERR("Failed to read file %s\n", filename_bs);
            goto err;
        }
        rewind(fp_bs);
        format = detect_format(header_data, header_size);
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
        pmem_stream->get_iova(ION_MM, &phy_addr, &size);
    } else {
        pmem_stream->get_phy_addr_from_ion(&phy_addr, &size);
    }
    pbuf_stream = (uint8*)pmem_stream->getBase();
    pbuf_stream_phy =  phy_addr;
    if (pbuf_stream == NULL) {
        ERR("Failed to alloc bitstream pmem buffer\n");
        goto err;
    }
    // dec_alloc_yuv_buffer(width,height);
    dec_alloc_yuv_buffer(mWidth, mHeight);
    INFO("Try to decode %s to %s, format = %s\n", filename_bs, filename_yuv, format2str(format));

    mHandle = (MP4Handle *)vsp_malloc(sizeof(MP4Handle), 4);
    memset(mHandle, 0, sizeof(tagMP4Handle));

    mHandle->userdata = (void *)mHandle;
    mHandle->VSP_extMemCb = extMemoryAlloc;
    mHandle->VSP_bindCb = NULL;
    mHandle->VSP_unbindCb = NULL;

    if (openDecoder("libomx_m4vh263dec_hw_sprd.so") < 0) {
        ERR("Failed to open library.\n");
        goto err;
    }

    /* step 1 - init vsp */
    size_inter = MP4DEC_INTERNAL_BUFFER_SIZE;
    pbuf_inter = (uint8*)vsp_malloc(size_inter, 4);
    if (pbuf_inter == NULL) {
        ERR("Failed to alloc inter memory\n");
        goto err;
    }

    if (dec_init(mHandle, format, NULL, 0, pbuf_inter, 0, size_inter) != 0) {
        ERR("Failed to init VSP\n");
        goto err;
    }
    startcode = table_startcode2[format];
    maskcode = table_maskcode2[format];

    mode = (format== MPEG4_MODE) ? MPEG4_MODE : H263_MODE;
    if (mode == MPEG4_MODE) {
        int read_size = fread(buffer_data, 1, ONEFRAME_BITSTREAM_BFR_SIZE, fp_bs);
        if (read_size <= 0) {
            ERR("Failed to read stream.\n");
            goto err;
        }

        // search a frame
        uint8 *ptmp = buffer_data;
        uint32 frame_size = 0;
        buffer_size = read_size;
        frame_size = find_frame(ptmp, buffer_size, startcode, maskcode);
        video_format.i_extra = frame_size;
        if( video_format.i_extra > 0) {
            memcpy(pbuf_stream, buffer_data,read_size);
            video_format.p_extra = pbuf_stream;
            video_format.p_extra_phy = (uint_32or64)pbuf_stream_phy;
        } else {
            video_format.p_extra = NULL;
            video_format.p_extra_phy = 0;
        }

        video_format.video_std = MPEG4;
        video_format.frame_width = 0;
        video_format.frame_height = 0;
        video_format.yuv_format = YUV420SP_NV12;
        if (MMDEC_OK != (*mMP4DecVolHeader)(mHandle, &video_format)) {
            ERR("Failed to decode Vol Header\n");
            goto err;
        }
        INFO("MP4DecVolHeader: width = %d, height = %d\n",video_format.frame_width, video_format.frame_height);

        buffer_size = 0;
        fseek(fp_bs, video_format.i_extra, SEEK_SET);
    }

    /* step 2 - decode with vsp */
    startcode = table_startcode2[format];
    maskcode = table_maskcode2[format];
    while (!feof(fp_bs)) {
        int32 read_size = fread(buffer_data+buffer_size, 1, ONEFRAME_BITSTREAM_BFR_SIZE-buffer_size, fp_bs);
        if (read_size <= 0) {
            break;
        }
        buffer_size += read_size;

        uint8* ptmp = buffer_data;
        uint32 frame_size = 0;
        while (buffer_size > 0) {
            // search a frame
            frame_size = find_frame(ptmp, buffer_size, startcode, maskcode);
            if (frame_size == 0) {
                if ((ptmp == buffer_data) || feof(fp_bs)) {
                    frame_size = buffer_size;
                } else {
                    break;
                }
            }

            // read a bitstream frame
            memcpy(pbuf_stream, ptmp, frame_size);

            ptmp += frame_size;
            buffer_size -= frame_size;

            // decode bitstream to yuv420sp
            framenum_bs++;
            int64_t start = systemTime();
            MMDecOutput dec_out;
            dec_out.frameEffective = 0;
            if (dec_decode_frame(mHandle, pbuf_stream, pbuf_stream_phy, frame_size, &dec_out) < 0) {
                ERR("Failed to decode frame.\n");
                goto err;
            }
            int64_t end = systemTime();
            uint32 duration = (uint32)((end-start) / 1000000L);
            time_total_ms += duration;
            if (duration < 40) {
                usleep((40 - duration)*1000);
            }

            INFO("frame %d[%dx%d]: time = %dms, size = %d, type = %s, effective(%d)\n",
                 framenum_bs, dec_out.frame_width, dec_out.frame_height, duration, frame_size, type2str(dec_out.VopPredType), dec_out.frameEffective);

            if ((dec_out.frameEffective) && (fp_yuv != NULL)) {
                // yuv420sp to yuv420p
                yuv420sp_to_yuv420p(dec_out.pOutFrameY, dec_out.pOutFrameU, py, pu, pv, dec_out.frame_width, dec_out.frame_height);

                // write yuv420p
                if (write_yuv_frame(py, pu, pv, dec_out.frame_width, dec_out.frame_height, fp_yuv)!= 0) {
                    break;
                }

                framenum_yuv ++;
            }

            if (frames != 0) {
                if (framenum_yuv >= frames) {
                    goto  early_terminate;
                }
            }
        }

        if (buffer_size > 0) {
            memmove(buffer_data, ptmp, buffer_size);
        }
    }

early_terminate:
    /* step 3 - release vsp */
    if (dec_release(mHandle) < 0) {
        ERR ("Failed to release decoder\n");
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

    if(s_pbuf_extra_v) {
        if (mIOMMUEnabled) {
            s_pmem_extra->free_iova(ION_MM, (int32)s_pbuf_extra_p, s_pbuf_extra_size);
        }

        s_pmem_extra.clear();
        s_pbuf_extra_v = NULL;
        s_pbuf_extra_p = 0;
        s_pbuf_extra_size = 0;
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

    dec_free_yuv_buffer();

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

