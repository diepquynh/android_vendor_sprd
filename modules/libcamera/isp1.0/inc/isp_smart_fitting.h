#ifndef isp_smart_fitting_h
#define isp_smart_fitting_h

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_SCENE_COUNT 8
#define MAX_BV_THR_COUNT 6
#define MAX_SAMPLE_COUNT 16

typedef unsigned int uint32_t;
typedef int int32_t;

struct isp_smart_sample{
	union{
		uint32_t level;
		uint32_t index;
	};
	union{
		uint32_t gain;
		uint32_t luma;
		uint32_t scene;
	};
};

struct isp_smart_scene{
	uint32_t sample_count;
	struct isp_smart_sample sample_group[MAX_SAMPLE_COUNT];
};

struct isp_smart_bv_thr{
	int32_t bv_high;
	int32_t bv_low;
};

struct isp_smart_scene_info{
	uint32_t scene_count;
	struct isp_smart_scene scene_group[MAX_SCENE_COUNT];
};

struct isp_smart_bv_thr_info{
	uint32_t bv_count;
	struct isp_smart_bv_thr bv[MAX_BV_THR_COUNT];
};

struct isp_smart_in_param{
	uint32_t cur_gain;
	uint32_t cur_luma;
	int32_t cur_bv;
	uint32_t low_lux;
	struct isp_smart_scene_info scene_info;
	struct isp_smart_bv_thr_info bv_info;
};

int32_t isp_smart_get_level(struct isp_smart_in_param * in_param_ptr, uint32_t *level);
int32_t isp_smart_get_index(struct isp_smart_in_param *in_param_ptr, uint32_t *index);
int32_t isp_smart_get_gamma_index(struct isp_smart_in_param *in_param_ptr, uint32_t *index1, uint32_t *index2, uint32_t *bv_index);

#ifdef __cplusplus
}
#endif

#endif
