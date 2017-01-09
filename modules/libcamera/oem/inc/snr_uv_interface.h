#ifndef INTERFACE_H
#define INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WINDOW_D 2
#define MAX_CNR_GAIN_STEP 32


typedef unsigned long int  uint32;
typedef unsigned short int uint16;
typedef unsigned char      uint8;
typedef signed long int    int32;
typedef signed short int   int16;
typedef signed char        int8;

enum {
	CAMERA_MODE_PREVIEW = 0,
	CAMERA_MODE_CAPTURE,
	CAMERA_MODE_MAX
};


typedef struct threshold
{
	uint8 uthr[4]; //shrinkage threshold for different scale
	uint8 vthr[4];
	uint8 udw[3][25];// distance weight for different scale
	uint8 vdw[3][25];
	uint8 urw[3][64]; // range weight for different scale
	uint8 vrw[3][64];
}snr_cur_thr;


 typedef struct threshold_cur
 {
	 uint8 uthr[4]; //shrinkage threshold for different scale
	 uint8 vthr[4];
	 uint8 *udw[3];// distance weight for different scale
	 uint8 *vdw[3];
	 uint8 *urw[3]; // range weight for different scale
	 uint8 *vrw[3];
 }cur_thr;


 struct sigma
{
	float sigma_d_u[3];
	float sigma_d_v[3];
	float sigma_r_u[3];
	float sigma_r_v[3];
};

 struct resolution
{
	uint32 height;
	uint32 width;
};

 struct gain_range_t {
	float gain_lower;
	float gain_upper;
};

 typedef struct cnr_param
{
	struct threshold thr[MAX_CNR_GAIN_STEP];
	struct sigma sig[MAX_CNR_GAIN_STEP];
	struct gain_range_t gain_range[MAX_CNR_GAIN_STEP];
	struct resolution res;
} cur_param;

//struct cnr_param param[2];

int cnr(uint8 * img,int32 srcw,int32 srch, cur_thr *curthr);
int cnr_init();
int cnr_destroy();

#ifdef __cplusplus
}
#endif

#endif
