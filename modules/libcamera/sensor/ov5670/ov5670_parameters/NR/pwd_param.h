/*PWD*/		/*pwd_param.h*/

/*
struct isp_pos{
	uint32_t x;
	uint32_t y;
};

struct sensor_pwd_radial {
	uint32_t radial_bypass;
	struct isp_pos center_pos;
	struct isp_pos center_square;
	uint32_t r2_thr;
	uint32_t p1_param;
	uint32_t p2_param;
	uint16_t gain_max_thr;
	uint16_t reserved0;
};

struct sensor_pwd_level {
	uint16_t gain_thrs0;
	uint16_t gain_thrs1;
	uint16_t nsr_thr_slope;
	uint16_t offset;
	uint16_t bitshif1;
	uint16_t bitshif0;
	uint32_t lum_ratio;
	struct sensor_pwd_radial radial_var;
	uint16_t addback;
	uint16_t reserved1;
};
*/
/*param 0.*/
{0x64, 0xc8, 0x40, 0x00, 0x4, 0x2, 0x8,{0x00,{0x04c8,0x0660,},{0x16dc40,0x28a400,},0x16dc40,0x0fff,0x00,0x0c00,0x00},0x50,0x00,0x00},
