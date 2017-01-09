#include <stdlib.h>
#include "util.h"

void* vsp_malloc(uint32 size, uint8 alignment) {
    uint8 *mem_ptr;
    if (!alignment) {
        if ((mem_ptr = (uint8*) malloc(size + 1)) != NULL) {
            *mem_ptr = (uint8)1;
            return ((void *)(mem_ptr+1));
        }
    } else {
        uint8 *tmp;

        if ((tmp = (uint8 *) malloc(size + alignment)) != NULL) {
            mem_ptr = (uint8 *) ((uint_32or64) (tmp + alignment - 1) & (~(int)(alignment - 1)));

            if (mem_ptr == tmp)
                mem_ptr += alignment;

            *(mem_ptr - 1) = (uint8) (mem_ptr - tmp);
            return ((void *)mem_ptr);
        }
    }

    return(NULL);
}

void vsp_free(void *mem_ptr) {
    uint8 *ptr;

    if (mem_ptr == NULL)
        return;

    ptr = mem_ptr;
    ptr -= *(ptr - 1);

    free(ptr);
}

void yuv420p_to_yuv420sp(uint8* py_src, uint8* pu_src, uint8* pv_src,
                         uint8* py_dst, uint8* puv_dst, uint32 width, uint32 height) {
    uint32 i;

    memcpy(py_dst, py_src, width*height);
    for (i=0; i<width*height/4; i++) {
        *puv_dst ++ = *pu_src ++;
        *puv_dst ++ = *pv_src ++;
    }
}

void yuv420p_to_yvu420sp(uint8* py_src, uint8* pu_src, uint8* pv_src,
                         uint8* py_dst, uint8* pvu_dst, uint32 width, uint32 height) {
    uint32 i;

    memcpy(py_dst, py_src, width*height);
    for (i=0; i<width*height/4; i++) {
        *pvu_dst ++ = *pv_src ++;
        *pvu_dst ++ = *pu_src ++;
    }
}

void yuv420p_to_yuv420sp_opt(uint32* py_src, uint32* pu_src, uint32* pv_src,
                             uint32* py_dst, uint32* puv_dst, uint32 width, uint32 height) {
    uint32 a, b;
    uint32 i;

    memcpy(py_dst, py_src, width*height);
    for (i=0; i<width*height/4/4; i++) {
        uint32 u = *pu_src ++;
        uint32 v = *pv_src ++;

        a = (u & 0x000000ff) | ((v & 0x000000ff) << 8);
        b = ((u & 0x0000ff00) >> 8) | (v & 0x0000ff00);
        *puv_dst ++ = (b << 16) | a;

        a = ((u & 0xff000000) >> 8) | (v & 0xff000000);
        b = (u & 0x00ff0000) | ((v & 0x00ff0000) << 8);
        *puv_dst ++ =  b | (a >> 16);
    }
}

void yuv420sp_to_yuv420p(uint8* py_src, uint8* puv_src, uint8* py_dst,
                         uint8* pu_dst, uint8* pv_dst, uint32 width, uint32 height) {
    uint32 i;

    memcpy(py_dst, py_src, width*height);
    for (i=0; i<width*height/4; i++) {
        *pu_dst ++ = *puv_src ++;
        *pv_dst ++ = *puv_src ++;
    }
}

void yvu420sp_to_yuv420p(uint8* py_src, uint8* pvu_src, uint8* py_dst, uint8* pu_dst,
                         uint8* pv_dst, uint32 width, uint32 height) {
    uint32 i;

    memcpy(py_dst, py_src, width*height);
    for (i=0; i<width*height/4; i++) {
        *pv_dst ++ = *pvu_src ++;
        *pu_dst ++ = *pvu_src ++;
    }
}

void yuv420sp_to_yuv420p_opt(uint32* py_src, uint32* puv_src, uint32* py_dst,
                             uint32* pu_dst, uint32* pv_dst, uint32 width, uint32 height) {
    uint32 a, b;
    uint32 i;

    memcpy(py_dst, py_src, width*height);
    for (i=0; i<width*height/4/4; i++) {
        uint32 uv0 = *puv_src ++;
        uint32 uv1 = *puv_src ++;

        a = (uv0 & 0x00ff0000) | ((uv1 & 0x00ff0000) >> 16);
        b = ((uv0 & 0x000000ff) << 16) | (uv1 & 0x000000ff);
        *pu_dst ++ =  a | (b << 8);

        a = (uv0 & 0xff000000) | ((uv1 & 0xff000000) >> 16);
        b = ((uv0 & 0x0000ff00) << 16) | (uv1 & 0x0000ff00);
        *pv_dst ++ = (a >> 8) | b;
    }
}

uint32 find_frame(uint8* pbuffer, uint32 size, uint32 startcode, uint32 maskcode) {
    uint32 len = 0;
    uint32 i;

    int32 flag = 0;

    if (size <= 4) {
        return 0;
    }

    for (i=0; i<=size-4; i++) {
        uint32 code = (pbuffer[i] << 24) | (pbuffer[i+1] << 16) | (pbuffer[i+2] << 8) | pbuffer[i+3];
        if ((code & (~maskcode)) == (startcode & (~maskcode))) {
            flag = 1;
            break;
        }
    }

    if (flag) {
        for (i+=4; i<=size-4; i++) {
            uint32 code = (pbuffer[i] << 24) | (pbuffer[i+1] << 16) | (pbuffer[i+2] << 8) | pbuffer[i+3];
            if ((code & (~maskcode)) == (startcode & (~maskcode))) {
                len = i;
                break;
            }
        }
    }

    return len;
}

int32 read_yuv_frame(uint8* py, uint8* pu, uint8* pv, uint32 width, uint32 height, FILE* fp_yuv) {
    if (fread(py, sizeof(uint8), width*height, fp_yuv) != width*height) {
        return -1;
    }
    if (fread(pu, sizeof(uint8), width/2*height/2, fp_yuv) != width/2*height/2) {
        return -1;
    }
    if (fread(pv, sizeof(uint8), width/2*height/2, fp_yuv) != width/2*height/2) {
        return -1;
    }
    return 0;
}

int32 write_yuv_frame(uint8* py, uint8* pu, uint8* pv, uint32 width, uint32 height, FILE* fp_yuv) {
    if (fwrite(py, sizeof(uint8), width*height, fp_yuv) != width*height) {
        return -1;
    }
    if (fwrite(pu, sizeof(uint8), width*height/4, fp_yuv) != width*height/4) {
        return -1;
    }
    if (fwrite(pv, sizeof(uint8), width*height/4, fp_yuv) != width*height/4) {
        return -1;
    }
    return 0;
}

#define	STARTCODE_H263_PSC	0x00008000
#define STARTCODE_MP4V_VO	0x000001b0
#define STARTCODE_MP4V_VOP	0x000001b6
#define	STARTCODE_MJPG_SOI	0xffd80000
#define	STARTCODE_FLV1_PSC	0x00008400
#define STARTCODE_H264_NAL1	0x00000100
#define STARTCODE_H264_NAL2	0x00000101

const uint32 table_startcode1[] = {
    STARTCODE_H263_PSC,
    STARTCODE_MP4V_VO,
    STARTCODE_MJPG_SOI,
    STARTCODE_FLV1_PSC,
    STARTCODE_H264_NAL1
};

const uint32 table_maskcode1[] = {
    0x000073ff,
    0x00000005,
    0x0000ffff,
    0x000073ff,
    0x0000007f
};

const uint32 table_startcode2[] = {
    STARTCODE_H263_PSC,
    STARTCODE_MP4V_VOP,
    STARTCODE_MJPG_SOI,
    STARTCODE_FLV1_PSC,
    STARTCODE_H264_NAL2
};

const uint32 table_maskcode2[] = {
    0x000073ff,
    0x00000005,
    0x0000ffff,
    0x000073ff,
    0x00000074
};

const char* format2str(int format) {
    if (format == ITU_H263) {
        return "H263";
    } else if (format == MPEG4) {
        return "MPEG4";
    } else if (format == JPEG) {
        return "MJPG";
    } else if (format == FLV_V1) {
        return "FLVH263";
    } else if (format == H264) {
        return "H264";
    } else if (format == VP8) {
        return "VP8";
    } else if (format == VP9) {
        return "VP9";
    } else {
        return "UnKnown";
    }
}

