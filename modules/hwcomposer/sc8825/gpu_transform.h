#ifndef GPU_TRANSFORM_H
#define GPU_TRANSFORM_H

#include <stdio.h>
#include <stdlib.h>
#include "dcam_hal.h"
#include "../SprdUtil.h"

using namespace android;

enum LAYERS_COMBINATION_TYPE
{
     VIDEO_LAYER_EXIST = 1,
     OSD_LAYER_EXIST = 2,
};

#ifdef __cplusplus
extern "C" {
#endif


typedef struct transform_data {
     uint32_t srcPhy;
     uint32_t srcVirt;
     uint32_t srcFormat;
     uint32_t transform;
     uint32_t srcWidth;
     uint32_t srcHeight;
     uint32_t dstPhy;
     uint32_t dstVirt;
     uint32_t dstFormat;
     uint32_t dstWidth;
     uint32_t dstHeight;
     uint32_t tmp_phy_addr;
     uint32_t tmp_vir_addr;
     struct sprd_rect  trim_rect;
}transform_data_t;

typedef struct gpu_transform_info {
    int flag;
    transform_data_t video;
    transform_data_t osd;
}gpu_transform_info_t;

int gpu_transform_layers(gpu_transform_info_t *data);

void destroy_transform_thread();

#ifdef __cplusplus
}
#endif

#endif /* GPU_TRANSFORM */
