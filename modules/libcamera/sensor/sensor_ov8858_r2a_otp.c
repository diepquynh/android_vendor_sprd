#include <utils/Log.h>
#include "sensor.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#define OV8858_RAW_PARAM_Sunny    0x0001
#define OV8858_RAW_PARAM_Truly    0x0002

struct otp_typical_value_t {
	uint16_t rg_ratio_typical;
	uint16_t bg_ratio_typical;
};

struct otp_param_info_tab {
	uint32_t module_id;
	struct otp_typical_value_t typical_value;
	struct sensor_raw_info *info_ptr;
	uint32_t(*update_otp) (void *param_ptr);
};

struct otp_info_t {
	int flag;
	int module_id;
	int lens_id;
	int year;
	int month;
	int day;
	int rg_ratio;
	int bg_ratio;
};

static struct otp_info_t otp_info={0x00};
static struct otp_typical_value_t otp_typical_value={0x0};
static int lenc[240]={0x00};

static uint32_t ov8858_otp_read_otp_info(struct otp_info_t *otp_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	int otp_flag, addr, temp, i;
	//set 0x5002[3] to ¡°0¡±
	int temp1;
       Sensor_WriteReg(0x0100, 0x01);
	temp1 = Sensor_ReadReg(0x5002);
       SENSOR_PRINT_ERR("temp1 = %d", temp1);
       SENSOR_PRINT_ERR("(0x00 & 0x08) | (temp1 & (~0x08) = %d", (0x00 & 0x08) | (temp1 & (~0x08)));
	Sensor_WriteReg(0x5002, (0x00 & 0x08) | (temp1 & (~0x08)));
	// read OTP into buffer
	Sensor_WriteReg(0x3d84, 0xC0);
	Sensor_WriteReg(0x3d88, 0x70); // OTP start address
	Sensor_WriteReg(0x3d89, 0x10);
	Sensor_WriteReg(0x3d8A, 0x72); // OTP end address
	Sensor_WriteReg(0x3d8B, 0x0a);
	Sensor_WriteReg(0x3d81, 0x01); // load otp into buffer
	usleep(10 * 1000);

	// OTP base information and WB calibration data
	otp_flag = Sensor_ReadReg(0x7010);
       SENSOR_PRINT_ERR("otp_flag = %d", otp_flag);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7011; // base address of info group 1
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x7019; // base address of info group 2
	}

	if(addr != 0) {
		otp_ptr->flag = 0xC0; // valid info and AWB in OTP
		otp_ptr->module_id = Sensor_ReadReg(addr);
		otp_ptr->lens_id = Sensor_ReadReg( addr + 1);
		otp_ptr->year = Sensor_ReadReg( addr + 2);
		otp_ptr->month = Sensor_ReadReg( addr + 3);
		otp_ptr->day = Sensor_ReadReg(addr + 4);
		temp = Sensor_ReadReg(addr + 7);
		otp_ptr->rg_ratio = (Sensor_ReadReg(addr + 5)<<2) + ((temp>>6) & 0x03);
		otp_ptr->bg_ratio = (Sensor_ReadReg(addr + 6)<<2) + ((temp>>4) & 0x03);
	}
	else {
		otp_ptr->flag = 0x00; // not info and AWB in OTP
		otp_ptr->module_id = 0;
		otp_ptr->lens_id = 0;
		otp_ptr->year = 0;
		otp_ptr->month = 0;
		otp_ptr->day = 0;
		otp_ptr->rg_ratio = 0;
		otp_ptr->bg_ratio = 0;
	}
	SENSOR_PRINT("year=0x%x\n", otp_ptr->year);
	SENSOR_PRINT("month=0x%x\n",  otp_ptr->month);
	SENSOR_PRINT("day=0x%x\n", otp_ptr->day);
	SENSOR_PRINT("rg_ratio=0x%x\n", otp_ptr->rg_ratio);
	SENSOR_PRINT("bg_ratio=0x%x\n",  otp_ptr->bg_ratio);
	SENSOR_PRINT("rg_ratio_typical=0x%x\n",  otp_typical_value.rg_ratio_typical);
	SENSOR_PRINT("bg_ratio_typical=0x%x\n",  otp_typical_value.bg_ratio_typical);


	// OTP Lenc Calibration
	otp_flag = Sensor_ReadReg(0x7028);
	addr = 0;
	int checksum2=0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7029; // base address of Lenc Calibration group 1
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x711a; // base address of Lenc Calibration group 2
	}
	if(addr != 0) {
		for(i=0;i<240;i++) {
			lenc[i]=Sensor_ReadReg(addr + i);
			checksum2 += lenc[i];
		}
		checksum2 = (checksum2)%255 +1;
		if(Sensor_ReadReg((addr + 240)) == (uint)checksum2){
			otp_ptr->flag |= 0x10;
		}
	}
	else {
		for(i=0;i<240;i++) {
			lenc[i]=0;
		}
	}

	for(i=0x7010;i<=0x720a;i++) {
		Sensor_WriteReg(i,0); // clear OTP buffer, recommended use continuous write to accelarate
	}
	//set 0x5002[3] to ¡°1¡±
	temp1 = Sensor_ReadReg(0x5002);
	Sensor_WriteReg(0x5002, (0x08 & 0x08) | (temp1 & (~0x08)));

       Sensor_WriteReg(0x0100, 0x00);


	return rtn;
}

static uint32_t ov8858_otp_update_otp(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_param_info_tab *otp_param_info = (struct otp_param_info_tab *)param_ptr;
	int rg, bg, R_gain, G_gain, B_gain, Base_gain, temp, i;

	if(otp_typical_value.bg_ratio_typical==0 && otp_typical_value.rg_ratio_typical ==0)
	{
		otp_typical_value.bg_ratio_typical=otp_param_info->typical_value.bg_ratio_typical;
		otp_typical_value.rg_ratio_typical=otp_param_info->typical_value.rg_ratio_typical;
	}

	// apply OTP WB Calibration
	if (otp_info.flag & 0x40) {
		rg = otp_info.rg_ratio;
		bg = otp_info.bg_ratio;

	//calculate G gain
		R_gain = (otp_typical_value.rg_ratio_typical*1000) / rg;
		B_gain = (otp_typical_value.bg_ratio_typical*1000) / bg;
		G_gain = 1000;

		if (R_gain < 1000 || B_gain < 1000)
		{
		if (R_gain < B_gain)
			Base_gain = R_gain;
		else
			Base_gain = B_gain;
		}
		else
		{
			Base_gain = G_gain;
		}
		R_gain = 0x400 * R_gain / (Base_gain);
		B_gain = 0x400 * B_gain / (Base_gain);
		G_gain = 0x400 * G_gain / (Base_gain);

		SENSOR_PRINT("r_Gain=0x%x\n", R_gain);
		SENSOR_PRINT("g_Gain=0x%x\n", G_gain);
		SENSOR_PRINT("b_Gain=0x%x\n", B_gain);

		// update sensor WB gain
		if (R_gain>0x400) {
			Sensor_WriteReg(0x5032, R_gain>>8);
			Sensor_WriteReg(0x5033, R_gain & 0x00ff);
		}
		if (G_gain>0x400) {
			Sensor_WriteReg(0x5034, G_gain>>8);
			Sensor_WriteReg(0x5035, G_gain & 0x00ff);
		}
		if (B_gain>0x400) {
			Sensor_WriteReg(0x5036, B_gain>>8);
			Sensor_WriteReg(0x5037, B_gain & 0x00ff);
		}
	}

	// apply OTP Lenc Calibration
	if (otp_info.flag & 0x10) {
		SENSOR_PRINT("apply otp lsc \n");
		temp = Sensor_ReadReg(0x5000);
		temp = 0x80 | temp;
		Sensor_WriteReg(0x5000, temp);
		for(i=0;i<240;i++) {
			Sensor_WriteReg(0x5800 + i, lenc[i]);
		}
	}

	return rtn;
}
static uint32_t ov8858_otp_get_module_id(void)
{
	int i = 0;
	int otp_index = 0;
	int temp = 0;

	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	ov8858_otp_read_otp_info(&otp_info);

	SENSOR_PRINT("read ov8858 otp  module_id = %x \n", otp_info.module_id);

	return otp_info.module_id;
}
static uint32_t ov8858_otp_identify_otp(void *param_ptr)
{
	uint32_t rtn = SENSOR_FAIL;
	uint32_t  module_id;
	struct otp_param_info_tab *tab_ptr = (struct otp_param_info_tab *)param_ptr;

	SENSOR_PRINT("SENSOR_ov8858: _ov8858_com_Identify_otp");

	/*read param id from sensor omap */
	module_id = ov8858_otp_get_module_id();
	if(OV8858_RAW_PARAM_Truly==module_id){
		SENSOR_PRINT("SENSOR_OV8858: This is Truly module!!\n");
		otp_typical_value.rg_ratio_typical=0x120;
		otp_typical_value.bg_ratio_typical=0x12e;
		rtn=SENSOR_SUCCESS;
	}
	else if (OV8858_RAW_PARAM_Sunny==module_id){
		SENSOR_PRINT("SENSOR_OV8858: This is Sunny module!!\n");
		otp_typical_value.rg_ratio_typical=0x120;
		otp_typical_value.bg_ratio_typical=0x138;
		rtn=SENSOR_SUCCESS;
	}
	else
		rtn = SENSOR_FAIL;

	return rtn;
}
