#ifndef THREAD_H
#define THREAD_H

#ifdef __cplusplus
extern "C" {
#endif
typedef signed char     int8;
typedef signed short	int16;
typedef unsigned char   uint8;
typedef unsigned short	uint16;
typedef unsigned int	uint32;
typedef signed   int	int32;
#define YDENOISE_MASK_SIZE	32

struct mask_class {
    int8 *mask_1gain_to_lessthan_1d5gain[YDENOISE_MASK_SIZE];
    int8 *mask_1d5gain_to_lessthan_4gain[YDENOISE_MASK_SIZE];
    int8 *mask_4gain_to_lessthan_8gain[YDENOISE_MASK_SIZE];
    int8 *mask_8gain_to_lessthan_16gain[YDENOISE_MASK_SIZE];
    int8 *mask_morethan_16gain[YDENOISE_MASK_SIZE];
};

extern void ynoise_function(uint8 *in_ptr,uint8 *out_ptr,int8 *mask,int w,int h);

#ifdef __cplusplus
}
#endif

#endif
