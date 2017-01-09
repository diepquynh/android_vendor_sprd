#ifndef _IMAGE_DITHER_H_
#define _IMAGE_DITHER_H_

#include <sys/types.h>

enum {
    img_dither_rtn_sucess = 0,
    img_dither_rtn_pointer_null,
    img_dither_rtn_param_unsupport,
    img_dither_rtn_param_invalidate,
    img_dither_rtn_unknow_error,
    img_dither_rtn_no_memory,
    img_dither_rtn_max,
};

struct img_dither_in_param {
    void* data_addr;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t alg_id;
};

struct img_dither_out_param {
    uint32_t param;
};

struct img_dither_init_in_param {
    uint32_t width;
    uint32_t height;
    uint32_t alg_id;
};

struct img_dither_init_out_param {
    uint32_t param;
};

int32_t  img_dither_init(struct img_dither_init_in_param *in_param, struct img_dither_init_out_param *out_param);

int32_t img_dither_process(uint32_t handle, struct img_dither_in_param *in_param, struct img_dither_out_param *out_param);

int32_t img_dither_deinit(uint32_t handle);

#endif //_IMAGE_DITHER_H_