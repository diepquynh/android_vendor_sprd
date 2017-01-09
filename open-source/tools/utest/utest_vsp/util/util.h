#ifndef UTEST_VSP_UTIL_H_
#define UTEST_VSP_UTIL_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define ERR(x...)	fprintf(stderr, ##x)
#define INFO(x...)	fprintf(stdout, ##x)

#define CLIP_16(x)	(((x) + 15) & (~15))

/*standard*/
typedef enum {
    ITU_H263 = 0,
    MPEG4,
    JPEG,
    FLV_V1,
    H264,
    VP8,
    VP9,
    FORMAT_MAX
}
VIDEO_STANDARD_E;

typedef unsigned char		BOOLEAN;
typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
typedef signed char			int8;
typedef signed short		int16;
typedef signed int			int32;
#define uint_32or64  unsigned long
void* vsp_malloc(unsigned int size, unsigned char alignment);
void vsp_free(void *mem_ptr);

void yuv420p_to_yuv420sp(unsigned char* py_src, unsigned char* pu_src, unsigned char* pv_src, unsigned char* py_dst, unsigned char* puv_dst, unsigned int width, unsigned int height);
void yuv420p_to_yvu420sp(unsigned char* py_src, unsigned char* pu_src, unsigned char* pv_src, unsigned char* py_dst, unsigned char* puv_dst, unsigned int width, unsigned int height);
void yuv420p_to_yuv420sp_opt(unsigned int* py_src, unsigned int* pu_src, unsigned int* pv_src, unsigned int* py_dst, unsigned int* puv_dst, unsigned int width, unsigned int height);

void yuv420sp_to_yuv420p(unsigned char* py_src, unsigned char* puv_src, unsigned char* py_dst, unsigned char* pu_dst, unsigned char* pv_dst, unsigned int width, unsigned int height);
void yvu420sp_to_yuv420p(unsigned char* py_src, unsigned char* pvu_src, unsigned char* py_dst, unsigned char* pu_dst, unsigned char* pv_dst, unsigned int width, unsigned int height);
void yuv420sp_to_yuv420p_opt(unsigned int* py_src, unsigned int* puv_src, unsigned int* py_dst, unsigned int* pu_dst, unsigned int* pv_dst, unsigned int width, unsigned int height);


/* to find a frame in buffer, if found return the length or return 0 */
unsigned int find_frame(unsigned char* pbuffer, unsigned int size, unsigned int startcode, unsigned int maskcode);
int read_yuv_frame(unsigned char* py, unsigned char* pu, unsigned char* pv, unsigned int width, unsigned int height, FILE* fp_yuv);
int write_yuv_frame(unsigned char* py, unsigned char* pu, unsigned char* pv, unsigned int width, unsigned int height, FILE* fp_yuv);

extern const unsigned int table_startcode1[];
extern const unsigned int table_maskcode1[];
extern const unsigned int table_startcode2[];
extern const unsigned int table_maskcode2[];

const char* format2str(int format);


#ifdef __cplusplus
}
#endif


#endif // UTEST_VSP_UTIL_H_

