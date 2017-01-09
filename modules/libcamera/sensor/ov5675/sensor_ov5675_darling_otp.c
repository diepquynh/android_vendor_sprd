#include <utils/Log.h>
#include "sensor.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include <cutils/properties.h>

#define MODULE_NAME       		"darling"    //module vendor name
#define MODULE_ID_ov5675_darling		0x0005    //ov5675: sensor P/N;  darling: module vendor
#define LSC_PARAM_QTY 240

struct otp_info_t {
	uint16_t flag;
	uint16_t module_id;
	uint16_t lens_id;
	uint16_t vcm_id;
	uint16_t vcm_driver_id;
	uint16_t year;
	uint16_t month;
	uint16_t day;
	uint16_t rg_ratio_current;
	uint16_t bg_ratio_current;
	uint16_t rg_ratio_typical;
	uint16_t bg_ratio_typical;
	uint16_t r_current;
	uint16_t g_current;
	uint16_t b_current;
	uint16_t r_typical;
	uint16_t g_typical;
	uint16_t b_typical;
	uint16_t vcm_dac_start;
	uint16_t vcm_dac_inifity;
	uint16_t vcm_dac_macro;
	uint16_t lsc_param[LSC_PARAM_QTY];
};
static struct otp_info_t s_ov5675_darling_otp_info={0x00};
#define RG_RATIO_TYPICAL_ov5675_darling    	307
#define BG_RATIO_TYPICAL_ov5675_darling		311
#define R_TYPICAL_ov5675_darling		0x0000
#define G_TYPICAL_ov5675_darling		0x0000
#define B_TYPICAL_ov5675_darling		0x0000

static uint32_t ov5675_darling_read_otp_info(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	otp_info->rg_ratio_typical=RG_RATIO_TYPICAL_ov5675_darling;
	otp_info->bg_ratio_typical=BG_RATIO_TYPICAL_ov5675_darling;
	otp_info->r_typical=R_TYPICAL_ov5675_darling;
	otp_info->g_typical=G_TYPICAL_ov5675_darling;
	otp_info->b_typical=B_TYPICAL_ov5675_darling;

	/*TODO*/
	int otp_flag, addr, temp, i;
	
    Sensor_WriteReg(0x0100, 0x01);
	//set 0x5001 to "0x02"
	Sensor_WriteReg(0x5001,0x02);

	// read OTP into buffer
	Sensor_WriteReg(0x3d84, 0xC0);
	Sensor_WriteReg(0x3d88, 0x70); // OTP start address
	Sensor_WriteReg(0x3d89, 0x10);
	Sensor_WriteReg(0x3d8A, 0x70); // OTP end address
	Sensor_WriteReg(0x3d8B, 0x29);
	Sensor_WriteReg(0x3d81, 0x01); // load otp into buffer
	usleep(10 * 1000);

	//otp info
	otp_flag = Sensor_ReadReg(0x7010);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7011; // base address of info group 1
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x7016; // base address of info group 2
	}
	else if((otp_flag & 0x0C) == 0x04) {
		addr = 0x701b; // base address of info group 3
	}
	if(addr != 0) {
		otp_info->flag = 0x80; // valid base info in OTP
		otp_info->module_id = Sensor_ReadReg(addr);
		otp_info->lens_id = Sensor_ReadReg( addr + 1);
		otp_info->year = Sensor_ReadReg( addr + 2);
		otp_info->month = Sensor_ReadReg( addr + 3);
		otp_info->day = Sensor_ReadReg(addr + 4);
	}
	else {
		otp_info->flag = 0x00; // not info in OTP
		otp_info->module_id = 0;
		otp_info->lens_id = 0;
		otp_info->year = 0;
		otp_info->month = 0;
		otp_info->day = 0;
	}

	//otp wb calibration
	otp_flag = Sensor_ReadReg(0x7020);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7021; // base address of WB Calibration group 1
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x7024; // base address of WB Calibration group 2
	}
	else if((otp_flag & 0x0C) == 0x04) {
		addr = 0x7027; // base address of WB Calibration group 3
	}
	if(addr != 0) {
		otp_info->flag |= 0x40; 
		temp = Sensor_ReadReg(addr + 2);
		otp_info->rg_ratio_current= (Sensor_ReadReg(addr)<<2) + ((temp>>6) & 0x03);
		otp_info->bg_ratio_current= (Sensor_ReadReg(addr + 1)<<2) + ((temp>>4) & 0x03);
	}
	else {
		otp_info->rg_ratio_current = 0;
		otp_info->bg_ratio_current = 0;
	}

	for(i=0x7010;i<=0x7029;i++) {
		Sensor_WriteReg(i,0); // clear OTP buffer, recommended use continuous write to accelarate
	}
	
	//set 0x5001 to "0x0a"
	Sensor_WriteReg(0x5001,0x0a);
    Sensor_WriteReg(0x0100, 0x00);
	
	/*print otp information*/
	SENSOR_PRINT("flag=0x%x",otp_info->flag);
	SENSOR_PRINT("module_id=0x%x",otp_info->module_id);
	SENSOR_PRINT("lens_id=0x%x",otp_info->lens_id);
	SENSOR_PRINT("vcm_id=0x%x",otp_info->vcm_id);
	SENSOR_PRINT("vcm_id=0x%x",otp_info->vcm_id);
	SENSOR_PRINT("vcm_driver_id=0x%x",otp_info->vcm_driver_id);
	SENSOR_PRINT("data=%d-%d-%d",otp_info->year,otp_info->month,otp_info->day);
	SENSOR_PRINT("rg_ratio_current=0x%x",otp_info->rg_ratio_current);
 	SENSOR_PRINT("bg_ratio_current=0x%x",otp_info->bg_ratio_current);
	SENSOR_PRINT("rg_ratio_typical=0x%x",otp_info->rg_ratio_typical);
	SENSOR_PRINT("bg_ratio_typical=0x%x",otp_info->bg_ratio_typical);
	SENSOR_PRINT("r_current=0x%x",otp_info->r_current);
	SENSOR_PRINT("g_current=0x%x",otp_info->g_current);
	SENSOR_PRINT("b_current=0x%x",otp_info->b_current);
	SENSOR_PRINT("r_typical=0x%x",otp_info->r_typical);
	SENSOR_PRINT("g_typical=0x%x",otp_info->g_typical);
	SENSOR_PRINT("b_typical=0x%x",otp_info->b_typical);
	SENSOR_PRINT("vcm_dac_start=0x%x",otp_info->vcm_dac_start);
	SENSOR_PRINT("vcm_dac_inifity=0x%x",otp_info->vcm_dac_inifity);
	SENSOR_PRINT("vcm_dac_macro=0x%x",otp_info->vcm_dac_macro);
	
	return rtn;
}

static void ov5675_darling_enable_awb_otp(void)
{
	/*TODO enable awb otp update*/
	//Sensor_WriteReg(0xYYYY, 0xff);
}

static uint32_t ov5675_darling_update_awb(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	ov5675_darling_enable_awb_otp();

	/*TODO*/
	int rg, bg, R_gain, G_gain, B_gain, Base_gain;

	// apply OTP WB Calibration
	if (otp_info->flag & 0x40) {
		rg = otp_info->rg_ratio_current;
		bg = otp_info->bg_ratio_current;
		//calculate G gain
		R_gain = (otp_info->rg_ratio_typical*1000) / rg;
		B_gain = (otp_info->bg_ratio_typical*1000) / bg;
		G_gain = 1000;
		if (R_gain < 1000 || B_gain < 1000){
			if (R_gain < B_gain)
				Base_gain = R_gain;
			else
				Base_gain = B_gain;
		}
		else{
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
			Sensor_WriteReg(0x5019, R_gain>>8);
			Sensor_WriteReg(0x501a, R_gain & 0x00ff);
		}
		if (G_gain>0x400) {
			Sensor_WriteReg(0x501b, G_gain>>8);
			Sensor_WriteReg(0x501c, G_gain & 0x00ff);
		}
		if (B_gain>0x400) {
			Sensor_WriteReg(0x501d, B_gain>>8);
			Sensor_WriteReg(0x501e, B_gain & 0x00ff);
		}
	}
	return rtn;
	
}

static void ov5675_darling_enable_lsc_otp(void)
{
	/*TODO enable lsc otp update*/
	//Sensor_WriteReg(0xYYYY, 0xff);
}

static uint32_t ov5675_darling_update_lsc(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
    ov5675_darling_enable_lsc_otp();

	/*TODO*/
	
	return rtn;
}

static uint32_t ov5675_darling_test_awb(void *param_ptr)
{
	uint32_t flag = 1;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	char value[PROPERTY_VALUE_MAX];
    property_get("persist.sys.camera.otp.awb", value, "on");
		
	if(!strcmp(value,"on")){
		SENSOR_PRINT("apply awb otp normally!");
		otp_info->rg_ratio_typical=RG_RATIO_TYPICAL_ov5675_darling;
		otp_info->bg_ratio_typical=BG_RATIO_TYPICAL_ov5675_darling;
		otp_info->r_typical=R_TYPICAL_ov5675_darling;
		otp_info->g_typical=G_TYPICAL_ov5675_darling;
		otp_info->b_typical=B_TYPICAL_ov5675_darling;
		
	} else if(!strcmp(value,"test")){
		SENSOR_PRINT("apply awb otp on test mode!");
		otp_info->rg_ratio_typical=RG_RATIO_TYPICAL_ov5675_darling*1.5;
		otp_info->bg_ratio_typical=BG_RATIO_TYPICAL_ov5675_darling*1.5;
		otp_info->r_typical=G_TYPICAL_ov5675_darling;
		otp_info->g_typical=G_TYPICAL_ov5675_darling;
		otp_info->b_typical=G_TYPICAL_ov5675_darling;
		
	} else {
		SENSOR_PRINT("without apply awb otp!");
		flag = 0;
    }
	return flag;
}

static uint32_t ov5675_darling_test_lsc(void)
{
	uint32_t flag = 1;
	char value[PROPERTY_VALUE_MAX];
	property_get("persist.sys.camera.otp.lsc", value, "on");
	
	if(!strcmp(value,"on")){
		SENSOR_PRINT("apply lsc otp normally!");
		flag = 1;
	} else{
		SENSOR_PRINT("without apply lsc otp!");
		flag = 0;
	}
    return flag;
}

static uint32_t ov5675_darling_update_otp(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	/*update awb*/
	if(ov5675_darling_test_awb(otp_info)){
		rtn=ov5675_darling_update_awb(param_ptr);
		if(rtn!=SENSOR_SUCCESS)
		{
			SENSOR_PRINT_ERR("OTP awb apply error!");
			return rtn;
		}
	} else {
		rtn = SENSOR_SUCCESS;
	}
	
	/*update lsc*/
	if(ov5675_darling_test_lsc()){
		rtn=ov5675_darling_update_lsc(param_ptr);
		if(rtn!=SENSOR_SUCCESS)
		{
			SENSOR_PRINT_ERR("OTP lsc apply error!");
			return rtn;
		}
	} else {
		rtn = SENSOR_SUCCESS;
	}
	
	return rtn;
}

static uint32_t ov5675_darling_identify_otp(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
    struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	rtn=ov5675_darling_read_otp_info(param_ptr);
	if(rtn!=SENSOR_SUCCESS){
		SENSOR_PRINT_ERR("read otp information failed\n!");		
		return rtn;
	} else if(MODULE_ID_ov5675_darling !=otp_info->module_id){
		SENSOR_PRINT_ERR("identify otp fail! module id mismatch!");		
		rtn=SENSOR_FAIL;
	} else {
		SENSOR_PRINT("identify otp success!");
	}

	return rtn;
}

static struct raw_param_info_tab s_ov5675_darling_raw_param_tab[] = 
	{MODULE_ID_ov5675_darling, &s_ov5675_mipi_raw_info, ov5675_darling_identify_otp, ov5675_darling_update_otp};