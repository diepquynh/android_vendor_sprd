/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
#include <sys/types.h>
#include <semaphore.h>
#include <pthread.h>

#define LOG_TAG "isp_otp"
#include <cutils/log.h>
#include <hardware/camera.h>

#include "isp_log.h"
#include "isp_video.h"

#include "isp_otp.h"
#include "cmr_oem.h"


//#define SCI_Trace_Dcam(...)
#define SCI_Trace_Dcam(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define DATA_SIZE                (4*1024)

#define otp_data_start_index  40
#define OTP_DATA_LENG            sizeof(struct otp_setting)

enum otp_cmd_type {
	OTP_CALIBRATION_CMD = 0,
	OTP_CTRL_PARAM_CMD,
	OTP_ACTUATOR_I2C_CMD,
	OTP_SENSOR_I2C_CMD,
	OTP_ROM_DATA_CMD,
	OTP_START_3A_CMD,
	OTP_STOP_3A_CMD,
	OTP_GOLDEN_CMD,
	OTP_WRITE_SN_CMD,
	OTP_RELOAD_ON_CMD,
	OTP_RELOAD_OFF_CMD,
	OTP_CAMERA_POWERON_CMD,
	OTP_CAMERA_POWEROFF_CMD,
	OTP_MAX_CMD
};

struct otp_setting {
    unsigned int shutter; // 4 bytes 1us¡«10s
    unsigned int line; // 4 bytes 0¡«8000
    unsigned int gain; // 4 bytes 0¡«1023
    unsigned int frame_rate; // 4 bytes 0¡«1023
    unsigned int i2c_clock; // 4 bytes 100¡«2000 khz
    unsigned int main_clock; // 4 bytes MHz
    unsigned int image_pattern; // 4 bytes RGB_GR:0x00
    unsigned int flip; // 4 bytes 0£ºno flip;  1:flip
    unsigned int set_ev; // 0--14
    unsigned int set_ae; // 0 lock 1 unlock
    unsigned int set_af_mode; // 0 no af ,1 normal af ,2 caf
    unsigned int set_af_position; // 0 ~ 1023
	unsigned int checksum; //4 bytes
};


static uint8_t s_isp_otp_src_data[DATA_SIZE];
static uint8_t s_isp_otp_rec_data[DATA_SIZE];

#define OTP_DATA_TESTLOG
static unsigned int otp_data_offset = 0;    //3792;
#define otp_data_test_length (256/16)   //3120

static unsigned int  otp_settings_buf[(otp_data_start_index+OTP_DATA_LENG)/4];
static camera_device_t  *p_camera_device=NULL;


int memcpy_ex(unsigned char type, void *dest, unsigned char * src, unsigned int lenth)
{
	unsigned int   i = 0;
	unsigned char  i_type = type;
	unsigned char  *u8_dest  = NULL;
	unsigned short *u16_dest = NULL;
	unsigned int   *u32_dest = NULL;

	if (NULL == dest || NULL == src) {
		return -1;
	}

	switch (i_type) {
	case 1 : //byte to byte
		u8_dest = (unsigned char*)dest;
		for (i=0 ; i<lenth ; i++) {
			u8_dest[i] = src[i];
		}
		break;
	case 2://byte to word
		u16_dest =(unsigned short*)dest;
		for (i=0; i<lenth ; i++) {
			(*u16_dest) |= src[i]<<((i)*8);
		}
		break;
	case 4://byte to dword little edd [b0 b1 b2 b3] -->[b3 b2 b1 b0]
		u32_dest =(unsigned int*) dest;
		*u32_dest = 0;
		for (i=0; i<lenth; i++)  {
			(*u32_dest) |= src[i]<<((i)*8);
		}
		break;
	case 5:// dword[4 bytes] to dword  [b0 b1 b2 b3] -->[b0 b1 b2 b3]
		u32_dest =(unsigned int*)dest;
		for (i=0 ; i<lenth ; i++)  {
			(*u32_dest) |= src[i]<<((lenth -1-i)*8);
		}
		break;
	default:
		break;
	}
	return 0;
}

int compare(uint8_t * a_data_buf ,uint8_t * b_data_buf ,int length)
{
	int value = -1;
	int i = 0;
	uint8_t *adst = a_data_buf;
	uint8_t *bdst = b_data_buf;

	if ( !adst  || !bdst ) {
		value = -1;
		SCI_Trace_Dcam("%s:  parameter is null ",__func__ );
		goto RET;
	}

	for( i= 0; i < length; i++) {
		if( adst[i] == bdst[i]) {
			value = 0;
		} else if(adst[i] > bdst[i] ) {
			value = 1;
			SCI_Trace_Dcam("%s:addr count =%d  adst[%d] =%d  is bigger than dst[%d]=%d",__func__, i ,i,adst[i],i,bdst[i]);
			break;
		} else {
			value = -1;
			SCI_Trace_Dcam("%s:addr count =%d  adst[%d] =%d  is little than dst[%d]=%d",__func__, i ,i,adst[i],i,bdst[i]);
			break;
		}
	}

	SCI_Trace_Dcam("%s: is %s   count =%d  result value=%d !",__func__ ,(value == 0)?"ok!":"error ",i, value);
RET:
	return value;
}

cmr_int Sensor_Ioctl(SENSOR_IOCTL_CMD_E sns_cmd, void *arg)
{
	SENSOR_IOCTL_FUNC_PTR func_ptr;
	SENSOR_IOCTL_FUNC_TAB_T *func_tab_ptr;
	cmr_u32 temp;
	cmr_u32 ret = CMR_CAMERA_SUCCESS;


	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	if (SENSOR_IOCTL_GET_STATUS != sns_cmd) {
		SCI_Trace_Dcam("cmd = %d, arg = %p.\n", sns_cmd, arg);
	}

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	if (!sensor_is_init_common(sensor_cxt)) {
		SCI_Trace_Dcam("warn:sensor has not init.\n");
		return SENSOR_OP_STATUS_ERR;
	}

	if (SENSOR_IOCTL_CUS_FUNC_1 > sns_cmd) {
		SCI_Trace_Dcam("error:can't access internal command !\n");
		return SENSOR_SUCCESS;
	}

	if (PNULL == sensor_cxt->sensor_info_ptr) {
		SCI_Trace_Dcam("error:No sensor info!");
		return -1;
	}

	func_tab_ptr = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr;
	temp = *(cmr_u32 *) ((cmr_u32) func_tab_ptr + sns_cmd * S_BIT_2);
	func_ptr = (SENSOR_IOCTL_FUNC_PTR) temp;

	if (PNULL != func_ptr) {
		ret = func_ptr((cmr_u32)arg);
	}
	return ret;
}

static int Sensor_isRAW(void)
{
	SENSOR_EXP_INFO_T_PTR sensor_info_ptr = Sensor_GetInfo();
	if(sensor_info_ptr && sensor_info_ptr->image_format == SENSOR_IMAGE_FORMAT_RAW)
		return 1;
	else
		return 0;
}

int send_otp_data_to_isp(uint32_t start_addr, uint32_t data_size, uint8_t *data_buf)
{
	int ret = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t otp_start_addr =0;
	uint32_t otp_data_len    =0;
	uint8_t *dst = &s_isp_otp_src_data[0];
	SENSOR_VAL_T  val ;
	SENSOR_OTP_PARAM_T param_ptr;
	int32_t otp_start_addr_emprty =-1;

	SCI_Trace_Dcam("%s data_size =%d ",__func__,data_size);


	//memset zero
	for ( j = 0; j < DATA_SIZE; j++) {
		s_isp_otp_rec_data[j] = 0;
		s_isp_otp_src_data[j] = 0;
	}

	//initial s_isp_otp_src_data array
	for (i = 0 ; i < data_size ; i++) {
		*dst++ = *data_buf++;
	}

	//write the calibration data to sensor
	otp_start_addr = ( start_addr == 0 ) ? (otp_data_offset):(start_addr);
	otp_data_len   = ( start_addr == 0 ) ? (otp_data_test_length):(data_size);

#ifdef OTP_DATA_TESTLOG
	if(start_addr == 0){
		SCI_Trace_Dcam("%s:addr 0x%x cnt %d",__func__, otp_start_addr, otp_data_len);
		for ( j = 0; j < otp_data_len; j++){
			SCI_Trace_Dcam("%s data_buf[%d] =0x%02x ",__func__,j,s_isp_otp_src_data[j]);
		}
		//find if it is exist
		do{
			SCI_Trace_Dcam("test area %s:addr 0x%x cnt %d",__func__, otp_start_addr, otp_data_len);
			param_ptr.start_addr = otp_start_addr;
			param_ptr.len = otp_data_len;
			param_ptr.buff= s_isp_otp_rec_data;
			val.type=SENSOR_VAL_TYPE_READ_OTP;
			val.pval=&param_ptr;
			ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, &val);

			if(memcmp(s_isp_otp_rec_data,s_isp_otp_src_data,otp_data_len)==0){//already exist now
				otp_data_offset = otp_start_addr;
				SCI_Trace_Dcam("%s: find area:addr 0x%x cnt %d",__func__, otp_start_addr, otp_data_len);
				return 0;
			}

			for ( j = 0; otp_start_addr_emprty==-1 && j < otp_data_len; j++){
				if(s_isp_otp_rec_data[j]!=0){
					break;
				}
			}
			if(j >= otp_data_len){
				otp_start_addr_emprty = otp_start_addr;
				SCI_Trace_Dcam("%s: first empty area:addr 0x%x cnt %d",__func__, otp_start_addr, otp_data_len);
			}

			otp_start_addr+=otp_data_len;
		}while(otp_start_addr<8192);

		//create a new
		if(otp_start_addr>=8192){
			if(otp_start_addr_emprty != -1){
				otp_data_offset = otp_start_addr_emprty;
			}else{
				SCI_Trace_Dcam("found none empty area!");
				return -1;
			}
		}
	}
#endif

	i =0;
	do{
		SCI_Trace_Dcam("i = %d", i);

		param_ptr.start_addr = otp_start_addr;
		param_ptr.len = otp_data_len;
		param_ptr.buff= s_isp_otp_src_data;
		val.type=SENSOR_VAL_TYPE_WRITE_OTP;
		val.pval=&param_ptr;
		ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, &val);

		param_ptr.start_addr = otp_start_addr;
		param_ptr.len = otp_data_len;
		param_ptr.buff= s_isp_otp_rec_data;
		val.type=SENSOR_VAL_TYPE_READ_OTP;
		val.pval=&param_ptr;
		ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, &val);

		ret = compare( s_isp_otp_src_data, s_isp_otp_rec_data, otp_data_len);
	}while((ret!=0) && (i++ < 1));

#ifdef OTP_DATA_TESTLOG
	for ( j = 0; j < otp_data_len; j++)
	{
		SCI_Trace_Dcam("%s data_buf[%d] =0x%02x ",__func__,j,s_isp_otp_rec_data[j]);
	}

#endif
	return ret;
}

int write_otp_calibration_data( uint32_t data_size, uint8_t *data_buf)
{
	int ret = 0;
	int otp_data_len = 0;
	int write_cnt = 0;
	int addr = 0;
	uint8_t *log_ptr = data_buf;

/*	while(index < data_size)  {
	      SCI_Trace_Dcam ("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}*/
	memcpy_ex(4,&otp_data_len,&data_buf[0],4);
	write_cnt = otp_data_len - 4;

	memcpy_ex(4,&addr,&data_buf[otp_data_start_index],4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d",__func__, addr, write_cnt);
	ret = send_otp_data_to_isp(addr, write_cnt, &data_buf[otp_data_start_index+4]);
	//write the calibration data to sensor
	return ret;
}

int write_sensor_shutter(uint32_t shutter_val)
{
	int ret = 0;
	uint32_t line_time=0x00;
	uint32_t expsure_line=0x00;

//	struct camera_context    *cxt = camera_get_cxt();
	SENSOR_EXP_INFO_T *psensorinfo = Sensor_GetInfo();

//	line_time = psensorinfo->sensor_mode_info[cxt->sn_cxt.preview_mode].line_time ;
	expsure_line = shutter_val/line_time;
	ret = Sensor_Ioctl(SENSOR_IOCTL_WRITE_EV, (void *)expsure_line);

	SCI_Trace_Dcam("%s:0x%x\n",__func__, shutter_val);
	/*ret = _hi544_set_shutter(shutter_val);*/
	return ret;
}

int write_sensor_line(uint32_t line_val)
{
	int ret = 0;
	SCI_Trace_Dcam("%s:0x%x\n",__func__, line_val);
	//ret = _hi544_set_shutter(line_val);

	return ret;
}

int write_sensor_gain(uint32_t gain_val)
{
	int ret = 0;
	SCI_Trace_Dcam("%s:0x%x\n",__func__, gain_val);
	//ret = _hi544_write_gain(gain_val);
	ret = Sensor_Ioctl(SENSOR_IOCTL_WRITE_GAIN, (void *)gain_val);
	return ret;
}

int write_i2c_clock(uint32_t clk)
{
	int ret = 0;
	SCI_Trace_Dcam("%s:0x%x\n",__func__, clk);

//	_Sensor_Device_SetI2CClock(clk);
	return ret;
}

int write_mclk(uint32_t mclk)
{
	int ret = 0;

	SCI_Trace_Dcam("%s:0x%x\n",__func__, mclk);
//	Sensor_SetMCLK(mclk);
	return ret;
}

int write_image_pattern(uint32_t pattern)
{
	int ret = 0;
	SCI_Trace_Dcam("%s:0x%x\n",__func__, pattern);

	return ret;
}

int write_sensor_flip(uint32_t flip)
{
	int ret = 0;
	SCI_Trace_Dcam("%s:0x%x\n",__func__, flip);
//	ret = _hi544_set_flip(flip);
	return ret;
}

int write_position(uint32_t pos)
{
	int ret = 0;

	SCI_Trace_Dcam("%s:0x%x\n",__func__, pos);

	//ret = _hi544_write_af(pos);
	ret = Sensor_Ioctl(SENSOR_IOCTL_AF_ENABLE, (void *)pos);

	return ret;
}

int write_ev(uint32_t ev)
{
	int ret = 0;

	if(!Sensor_isRAW())
	SCI_Trace_Dcam("%s:0x%x\n",__func__, ev);
//	ret = isp_ioctl(ISP_CTRL_EV, (void*)&ev);

	return ret;
}

int write_ae(uint32_t ae)
{
	int ret = 0;
	struct isp_alg cmd_param;

	if(!Sensor_isRAW())
		return 0;
	SCI_Trace_Dcam("%s:0x%x\n",__func__, ae);
	if (0 == ae) {//lock
        cmd_param.mode = ISP_AE_BYPASS ;
//        ret = isp_ioctl(ISP_CTRL_ALG, (void*)&cmd_param);
	} else {//unlock
		cmd_param.mode = ISP_ALG_NORMAL;
//		ret = isp_ioctl(ISP_CTRL_ALG, (void*)&cmd_param);
	}
	return ret;
}

int write_frame_rate(uint32_t rate)
{
	int          ret = 0;
    uint32_t cmd_param = rate;

	if(!Sensor_isRAW())
		return 0;
	SCI_Trace_Dcam("%s:0x%x\n",__func__, rate);
//    ret = isp_ioctl(ISP_CTRL_VIDEO_MODE, (void*)&cmd_param);

	return ret;
}

int write_af_mode(uint32_t af_mode)
{
	int ret = 0;
	struct isp_af_win isp_af_param;
	int i = 0;
	int zone_cnt = 1;
	int win_width = 100;
	int win_height = 100;
	int win_x = 0;
	int win_y = 0;
	SENSOR_EXP_INFO_T *exp_info;
	uint32_t mode = 0;
	int image_width = 0;
	int image_height = 0;


	if(!Sensor_isRAW())
		return 0;
	SCI_Trace_Dcam("%s:0x%x\n",__func__, af_mode);

	Sensor_GetMode(&mode);
	exp_info = Sensor_GetInfo();
	image_width = exp_info->sensor_mode_info[mode].trim_width;
	image_height = exp_info->sensor_mode_info[mode].trim_height;

	win_width = (win_width>>2)<<2;
	win_height = (win_height>>2)<<2;
	win_x = (image_width - win_width) / 2 ;
	win_y = (image_height - win_height) / 2;
	win_x = (win_x >>1)<<1;
	win_y = (win_y>>1)<<1;


	memset(&isp_af_param, 0, sizeof(struct isp_af_win));
	isp_af_param.mode = af_mode;
	isp_af_param.valid_win = zone_cnt;

	for (i = 0; i < zone_cnt; i++) {
		isp_af_param.win[i].start_x = win_x;
		isp_af_param.win[i].start_y = win_y;
		isp_af_param.win[i].end_x = win_x + win_width - 1;
		isp_af_param.win[i].end_y = win_y + win_height - 1;

		SCI_Trace_Dcam("af_win num:%d, x:%d y:%d e_x:%d e_y:%d",
			zone_cnt,
			isp_af_param.win[i].start_x,
			isp_af_param.win[i].start_y,
			isp_af_param.win[i].end_x,
			isp_af_param.win[i].end_y);
	}
//	ret = isp_ioctl(ISP_CTRL_AF, &isp_af_param);

	return ret;
}
int write_otp_ctrl_param( uint32_t data_size, uint8_t *data_buf)
{
	int ret = 0;
	int otp_settings_index = 40;
	struct otp_setting write_otp_settings;
	uint32_t index = 0;
	uint8_t *log_ptr = data_buf;

	while(index < data_size)  {
	      SCI_Trace_Dcam ("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}

	if(data_buf != NULL) {
		memcpy_ex(4,&write_otp_settings.shutter,            &data_buf[otp_settings_index],4);
		memcpy_ex(4,&write_otp_settings.line,                 &data_buf[otp_settings_index+=4],4);
		memcpy_ex(4,&write_otp_settings.gain,                &data_buf[otp_settings_index+=4],4);
		memcpy_ex(4,&write_otp_settings.frame_rate,       &data_buf[otp_settings_index+=4],4);
		memcpy_ex(4,&write_otp_settings.i2c_clock,          &data_buf[otp_settings_index+=4],4);
		memcpy_ex(4,&write_otp_settings.main_clock,       &data_buf[otp_settings_index+=4],4);
		memcpy_ex(4,&write_otp_settings.image_pattern,  &data_buf[otp_settings_index+=4],4);
		memcpy_ex(4,&write_otp_settings.flip,                  &data_buf[otp_settings_index+=4],4);
		memcpy_ex(4,&write_otp_settings.set_ev,             &data_buf[otp_settings_index+=4],4);
		memcpy_ex(4,&write_otp_settings.set_ae     ,        &data_buf[otp_settings_index+=4],4);
		memcpy_ex(4,&write_otp_settings.set_af_mode,     &data_buf[otp_settings_index+=4],4);
		memcpy_ex(4,&write_otp_settings.set_af_position, &data_buf[otp_settings_index+=4],4);
		memcpy_ex(4,&write_otp_settings.checksum,        &data_buf[otp_settings_index+=4],4);


		SCI_Trace_Dcam("shutter = %d , line = %d ,gain = %d,  frame_rate = %d",
						write_otp_settings.shutter,write_otp_settings.line,write_otp_settings.gain,
						write_otp_settings.frame_rate);
		SCI_Trace_Dcam("i2c_clock = %d , main_clock = %d, image_pattern = %d , flip = %d",
						write_otp_settings.i2c_clock,write_otp_settings.main_clock,
						write_otp_settings.image_pattern,write_otp_settings.flip);
		SCI_Trace_Dcam(" set_ev = %d, set_ae = %d , set_af_mode = %d set_af_position = %d",
						write_otp_settings.set_ev,write_otp_settings.set_ae,write_otp_settings.set_af_mode,
						write_otp_settings.set_af_position);
		//SCI_Trace_Dcam(" checksum = 0x%x", write_otp_settings.checksum);

		ret = write_sensor_shutter(write_otp_settings.shutter);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		ret = write_sensor_line(write_otp_settings.line);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		ret = write_sensor_gain(write_otp_settings.gain);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		ret = write_i2c_clock(write_otp_settings.i2c_clock);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		ret = write_mclk(write_otp_settings.main_clock);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		ret = write_image_pattern(write_otp_settings.image_pattern);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		ret = write_sensor_flip(write_otp_settings.flip);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}

		ret = write_position(write_otp_settings.set_af_position);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		ret = write_ev(write_otp_settings.set_ev);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		ret = write_ae(write_otp_settings.set_ae);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		ret = write_af_mode(write_otp_settings.set_af_mode);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		ret = write_frame_rate(write_otp_settings.frame_rate);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
	}
	return ret;
}

int write_otp_actuator_i2c( uint32_t data_size, uint8_t *data_buf)
{
	int ret = 0;
	int otp_data_len = 0;
	int write_cnt = 0;
	int addr = 0;
	int i = 0;
	uint32_t index = 0;
	uint8_t *log_ptr = data_buf;
	uint8_t *read_ptr ;
	uint32_t reg_val = 0;
	SENSOR_VAL_T  io_val ={SENSOR_VAL_TYPE_WRITE_VCM, NULL};
	uint32_t param;

	while(index < data_size)  {
	      SCI_Trace_Dcam ("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}
	memcpy_ex(4,&otp_data_len,&data_buf[0],4);
	write_cnt = (otp_data_len -4)/4;

	memcpy_ex(4,&addr,&data_buf[otp_data_start_index],4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d",__func__, addr, write_cnt);
	read_ptr = &data_buf[otp_data_start_index+4];
	//write to sensor's rom data
	io_val.pval = &param;
	for (i=0;i<write_cnt;i++) {
		memcpy_ex(4, &reg_val,read_ptr,4);

		param = (addr<<16)  | (reg_val & 0xffff);
		ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL,(void *)&io_val);
		if (0 != ret) {
			SCI_Trace_Dcam ("%s failed  addr=0x%x val=0x%x", __func__, addr, reg_val);
			break;
		}

		addr++;
		read_ptr += 4;
	}


	return ret;
}

int write_otp_sensor_i2c(uint32_t data_size, uint8_t *data_buf)
{
	int ret = 0;
	int otp_data_len = 0;
	int write_cnt = 0;
	int addr = 0;
	int i = 0;
	uint32_t index = 0;
	uint8_t *log_ptr = data_buf;
	uint8_t *read_ptr ;
	uint32_t reg_val = 0;

	while(index < data_size)  {
	      SCI_Trace_Dcam ("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}
	memcpy_ex(4,&otp_data_len,&data_buf[0],4);
	write_cnt = (otp_data_len -4)/4;

	memcpy_ex(4,&addr,&data_buf[otp_data_start_index],4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d",__func__, addr, write_cnt);
	read_ptr = &data_buf[otp_data_start_index+4];
	//write to sensor's rom data
	for (i=0;i<write_cnt;i++) {
		memcpy_ex(4, &reg_val,read_ptr,4);

		/*ret = dcam_i2c_write_reg_16((uint16_t)addr,(uint16_t)reg_val);*/
		Sensor_WriteReg(addr,reg_val);

		addr++;
		read_ptr += 4;
	}
	return ret;
}

int write_otp_rom_data(uint32_t data_size, uint8_t *data_buf)
{
	int ret = 0;
	int otp_data_len = 0;
	int write_cnt = 0;
	int addr = 0;
	int i = 0;
	int write_val = 0;
	uint32_t index = 0;
	uint8_t *log_ptr = data_buf;

	while(index < data_size)  {
	      SCI_Trace_Dcam ("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}

	memcpy_ex(4,&otp_data_len,&data_buf[0],4);
	write_cnt = (otp_data_len -4)/4;

	memcpy_ex(4,&addr,&data_buf[otp_data_start_index],4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d",__func__, addr, write_cnt);
	//write to sensor's rom data
	return ret;
}

int write_otp_start_3A( uint32_t data_size, uint8_t *data_buf)
{
	int ret = 0;
	uint32_t cmd_param = 0;
	uint32_t on_off = 0;

	if(!Sensor_isRAW())
		return 0;

	SCI_Trace_Dcam("%s\n",__func__);
	//add start 3A
    cmd_param = ISP_ALG_NORMAL;
//    isp_ioctl(ISP_CTRL_ALG, (void*)&cmd_param);

	return ret;
}

int write_otp_stop_3A( uint32_t data_size, uint8_t *data_buf)
{
	int ret = 0;
	uint32_t cmd_param = 0;
	uint32_t on_off = 0;

	if(!Sensor_isRAW())
		return 0;

	SCI_Trace_Dcam("%s\n",__func__);

	cmd_param = ISP_AWB_BYPASS;
//	isp_ioctl(ISP_CTRL_ALG, (void*)&cmd_param);

	cmd_param = ISP_AE_BYPASS;
//	isp_ioctl(ISP_CTRL_ALG, (void*)&cmd_param);

	return ret;
}

int write_otp_sn(uint32_t data_size, uint8_t *data_buf)
{
	int ret = 0;
	int fd = -1;
	int size = 0;
	int data_len = 0;
	//char array[] = {'8','9','8','9','8','9','8','9'};
	unsigned char *sn_ptr = &data_buf[otp_data_start_index];

	fd = open("/dev/mmcblk0p16", O_CREAT | O_RDWR, 0);
	if (-1 == fd) {
		return -1;
	}
	memcpy_ex(4,&data_len,&data_buf[0],4);
	size = write(fd, &data_buf[otp_data_start_index],data_len);
	if (size != data_len) {
		return -1;
	}
	/*size = write(fd, array,8);
	if (size != 8) {
		return -1;
	}*/
	close(fd);
	return ret;
}

int  _start_camera(camera_device_t  **pdev)
{
	return 0;
}
int  _stop_camera(camera_device_t  *pdev)
{
	return 0;
}
#pragma weak start_camera =_start_camera
#pragma weak stop_camera  =_stop_camera

int isp_otp_needstopprev(uint8_t *data_buf, uint32_t *data_size)//DATA
{
	int ret = 0;
	int otp_type = 0xffff;

	if(data_buf == NULL) {
		SCI_Trace_Dcam("%s return error \n",__func__);
		return ret;
	}

	 //the real data need to obtain from otp tool settings
	 //according to protocol bit[40]~bit[87] is otp 48 bytes data
	 //parse the otp data maybe writre a function to do it
        SCI_Trace_Dcam("%s \n",__func__);
	 //get otp type
	 memcpy_ex(4,&otp_type,&data_buf[4],4);
	 SCI_Trace_Dcam("%s otp_type %d\n",__func__, otp_type);
	switch(otp_type) {
	case OTP_CALIBRATION_CMD:
		ret = 1;
		break;

	default:
		SCI_Trace_Dcam("%s:other,otp_type=%d \n",__func__, otp_type);
		break;
	}

	SCI_Trace_Dcam("%s: ret=%d ",__func__, ret);
	return ret;
}

int isp_otp_write(uint8_t *data_buf, uint32_t *data_size)//DATA
{
	int ret = 0;
	int otp_type = 0xffff;

	if(data_buf == NULL) {
		SCI_Trace_Dcam("%s return error \n",__func__);
		return ret;
	}

	 //the real data need to obtain from otp tool settings
	 //according to protocol bit[40]~bit[87] is otp 48 bytes data
	 //parse the otp data maybe writre a function to do it
        SCI_Trace_Dcam("%s \n",__func__);
	 //get otp type
	 memcpy_ex(4,&otp_type,&data_buf[4],4);
	 SCI_Trace_Dcam("%s otp_type %d\n",__func__, otp_type);

	if(((OTP_CAMERA_POWERON_CMD!=otp_type)&&(OTP_CAMERA_POWEROFF_CMD!=otp_type)
		&&(OTP_RELOAD_ON_CMD!=otp_type)&&(OTP_RELOAD_OFF_CMD!=otp_type)
		&&(OTP_WRITE_SN_CMD!=otp_type))
		&& (!p_camera_device)){
		SCI_Trace_Dcam("%s camera off now",__func__);
		*data_size = 4;
		return -1;
	}

	switch(otp_type) {
	case OTP_CALIBRATION_CMD:
		ret = write_otp_calibration_data(*data_size, data_buf);
		break;
	case OTP_CTRL_PARAM_CMD:
		ret = write_otp_ctrl_param(*data_size, data_buf);
		break;
	case OTP_ACTUATOR_I2C_CMD:
		ret = write_otp_actuator_i2c(*data_size, data_buf);
		break;
	case OTP_SENSOR_I2C_CMD:
		ret = write_otp_sensor_i2c(*data_size, data_buf);
		break;
	case OTP_ROM_DATA_CMD:
		ret = write_otp_rom_data(*data_size, data_buf);
		break;
	case OTP_START_3A_CMD:
		ret= write_otp_start_3A(*data_size, data_buf);
		break;
	case OTP_STOP_3A_CMD:
		ret = write_otp_stop_3A(*data_size, data_buf);
		break;
	case OTP_WRITE_SN_CMD:
		ret = write_otp_sn(*data_size, data_buf);
		break;

	case OTP_RELOAD_ON_CMD:
		camera_set_reload_support(1);
		break;
	case OTP_RELOAD_OFF_CMD:
		camera_set_reload_support(0);
		break;
	case OTP_CAMERA_POWERON_CMD:
		if(!p_camera_device){
			ret = start_camera(&p_camera_device);
		}
		break;
	case OTP_CAMERA_POWEROFF_CMD:
		if(p_camera_device){
			ret = stop_camera(p_camera_device);
			p_camera_device=NULL;
		}
		break;
	default:
		SCI_Trace_Dcam("%s:error,otp_type=%d \n",__func__, otp_type);
		break;
	}

	*data_size = 4;
	SCI_Trace_Dcam("data_size= %d \r\n", *data_size);


/*	while(index < data_size)  {
	      SCI_Trace_Dcam ("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}
*/
	SCI_Trace_Dcam("%s: ret=%d ",__func__, ret);
	return ret;
}

int read_otp_calibration_data(uint32_t *data_size, uint8_t *data_buf)
{
	int ret = 0;
	uint32_t read_cnt = 0;
	uint32_t addr = 0;
	uint32_t otp_start_addr =0;
	uint32_t otp_data_len   =0;
	SENSOR_VAL_T  val ;
	SENSOR_OTP_PARAM_T param_ptr;

	memcpy_ex(4,&addr,&data_buf[otp_data_start_index],4);
	memcpy_ex(4,&read_cnt,&data_buf[otp_data_start_index+4],4);
	SCI_Trace_Dcam("origi %s:addr 0x%x cnt %d",__func__, addr, read_cnt);
	//read to calibration data and write to data_buf[write_value_index]

	otp_start_addr = (addr == 0) ? (otp_data_offset):(addr);
	otp_data_len   = (read_cnt == 0) ? (otp_data_test_length):(read_cnt);

	SCI_Trace_Dcam("final %s:addr 0x%x cnt %d",__func__, otp_start_addr, otp_data_len);

	if(p_camera_device){
		param_ptr.start_addr = otp_start_addr;
		param_ptr.len = otp_data_len;
		param_ptr.buff= &data_buf[otp_data_start_index];
		val.type=SENSOR_VAL_TYPE_READ_OTP;
		val.pval =&param_ptr;
		ret = Sensor_Ioctl( SENSOR_IOCTL_ACCESS_VAL, (void *)&val);

		*data_size = otp_data_start_index + otp_data_len;
	}else{
		ret=-1;
	}

	return ret;
}

int read_sensor_shutter(uint32_t *shutter_val)
{
	int ret = 0;
	uint32_t line_time=0x00;
	uint32_t expsure_line=0x00;
	//	*shutter_val = _hi544_get_shutter();
	SENSOR_VAL_T  val ={SENSOR_VAL_TYPE_SHUTTER,shutter_val};
	SENSOR_EXP_INFO_T *exp_info;
	uint32_t mode = 0;

	ret = Sensor_Ioctl(SENSOR_IOCTL_GET_VAL,(void *)&val);
	//get mode & line time
	Sensor_GetMode(&mode);
	exp_info = Sensor_GetInfo();
	line_time = exp_info->sensor_mode_info[mode].line_time;
	*shutter_val = *shutter_val * line_time;

	SCI_Trace_Dcam("%s:0x%x ret %d\n",__func__, *shutter_val,ret);
	return ret;
}

int read_sensor_line(uint32_t *line_val)
{
	int ret = 0;
    //*line_val = _hi544_get_shutter();
	//SCI_Trace_Dcam("%s:0x%x\n",__func__, *line_val);
	return ret;
}

int read_sensor_gain(uint32_t *gain_val)
{
	int           ret = 0;
	SENSOR_VAL_T  val;
	uint32_t      gain = 0;

	val.type = SENSOR_VAL_TYPE_READ_OTP_GAIN;
	val.pval = &gain;

	ret = Sensor_Ioctl(SENSOR_IOCTL_GET_VAL, (void *)&val);
	*gain_val = gain & 0xffff;

	SCI_Trace_Dcam("%s:gain %d ret %d\n",__func__, *gain_val,ret);

	return ret;
}

int read_i2c_clock(uint32_t *clk)
{
	int ret = 0;
//	SCI_Trace_Dcam("%s:0x%x\n",__func__, *clk);

	return ret;
}

int read_mclk(uint32_t *mclk)
{
	int ret = 0;
//	SCI_Trace_Dcam("%s:0x%x\n",__func__, *mclk);

	return ret;
}

int read_image_pattern(uint32_t *pattern)
{
	int ret = 0;
//	SCI_Trace_Dcam("%s:0x%x\n",__func__, *pattern);

	return ret;
}

int read_sensor_flip(uint32_t *flip)
{
	int ret = 0;
//	*flip = _hi544_get_flip();
//	SCI_Trace_Dcam("%s:0x%x\n",__func__, *flip);
	return ret;
}

int read_position(uint32_t *pos)
{
	int ret = 0;
	SENSOR_VAL_T  val ;
	SCI_Trace_Dcam("%s:0x%x\n",__func__, *pos);

	val.type=SENSOR_VAL_TYPE_GET_AFPOSITION;
	val.pval =pos;
	ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, (void *)&val);

	return ret;
}

int read_ev(uint32_t *ev)
{
	int ret = 0;

	if(!Sensor_isRAW())
		return 0;
//	isp_capability(ISP_CUR_EV, (void*)ev);
	SCI_Trace_Dcam("%s:0x%x\n",__func__, *ev);

	return ret;
}

int read_ae(uint32_t *ae)
{
	int ret = 0;

	if(!Sensor_isRAW())
		return 0;
//    isp_capability(ISP_CUR_AE_LOCK, (void*)ae);
	SCI_Trace_Dcam("%s:0x%x\n",__func__, *ae);
	return ret;
}

int read_frame_rate(uint32_t *rate)
{
	int ret = 0;


	if(!Sensor_isRAW())
		return 0;
//	isp_capability(ISP_CUR_FPS, (void*)rate);
	SCI_Trace_Dcam("%s:0x%x\n",__func__, *rate);
	return ret;
}

int read_af_mode(uint32_t *af_mode)
{
	int ret = 0;

	if(!Sensor_isRAW())
		return 0;
//	isp_capability(ISP_CUR_AF_MODE, (void*)af_mode);
	SCI_Trace_Dcam("%s:0x%x\n",__func__, *af_mode);
	return ret;
}

int read_otpdata_checksum(uint32_t *checksum)
{
	int ret = 0;

    *checksum = 0;
	SCI_Trace_Dcam("%s:0x%x\n",__func__, *checksum);
	return ret;
}

int read_otp_ctrl_param(uint32_t *data_size, uint8_t *data_buf)
{
	int          ret = 0;
	int          index = 0;
	unsigned int length = 0;
	struct otp_setting read_otp_settings;

	memset(&read_otp_settings,0,sizeof(struct otp_setting));

	if(p_camera_device){

		read_otp_settings.shutter = -1;
		ret = read_sensor_shutter(&read_otp_settings.shutter);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		read_otp_settings.line = read_otp_settings.shutter;
		ret = read_sensor_line(&read_otp_settings.line);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		read_otp_settings.gain = 0;
		ret = read_sensor_gain(&read_otp_settings.gain);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		read_otp_settings.i2c_clock = -1;
		ret = read_i2c_clock(&read_otp_settings.i2c_clock);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		read_otp_settings.main_clock = -1;
		ret = read_mclk(&read_otp_settings.main_clock);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		read_otp_settings.image_pattern = -1;
		ret = read_image_pattern(&read_otp_settings.image_pattern);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		read_otp_settings.flip = -1;
		ret = read_sensor_flip(&read_otp_settings.flip);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		read_otp_settings.set_af_position = -1;
		ret = read_position(&read_otp_settings.set_af_position);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}

		read_otp_settings.frame_rate = -1;
		ret = read_frame_rate(&read_otp_settings.frame_rate);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		read_otp_settings.set_ev = -1;
		ret = read_ev(&read_otp_settings.set_ev);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		read_otp_settings.set_ae = -1;
		ret = read_ae(&read_otp_settings.set_ae);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
		read_otp_settings.set_af_mode = -1;
		ret = read_af_mode(&read_otp_settings.set_af_mode);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}

		read_otp_settings.checksum = -1;
		ret = read_otpdata_checksum(&read_otp_settings.checksum);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n",__func__, ret);
		}
	}else{
		ret=-1;
	}

     //the real data need to obtain from otp or sensor
     // and need to check data range
//	read_otp_settings.shutter             = 6;
//	read_otp_settings.line                  = -1;
//	read_otp_settings.gain                 = 902;
//	read_otp_settings.frame_rate        = 1000;
	read_otp_settings.i2c_clock           = 400;
	read_otp_settings.main_clock        = 24;
	read_otp_settings.image_pattern   = 1;
	read_otp_settings.flip                   = 1;
 //      read_otp_settings.set_ev              = 12;
 //      read_otp_settings.set_ae              = 1;
 //      read_otp_settings.set_af_mode     = 2;
 //      read_otp_settings.set_af_position  = 20;
      /*otp data lenth 48 bytes .
       *it needs to double check when change in the future
       */
	otp_settings_buf[index] = OTP_DATA_LENG;

	//otp data type
	 index++;
	 otp_settings_buf[index] = OTP_CTRL_PARAM_CMD;

      //otp reserve 32 bytes
	 index++;
	 otp_settings_buf[index] = 0x0;

	 index++;
	 otp_settings_buf[index] = 0x0;

	 index++;
	 otp_settings_buf[index] = 0x0;

	 index++;
	 otp_settings_buf[index] = 0x0;

	 index++;
	 otp_settings_buf[index] = 0x0;

	 index++;
	 otp_settings_buf[index] = 0x0;

	 index++;
	 otp_settings_buf[index] = 0x0;

	 index++;
	 otp_settings_buf[index] = 0x0;

	 //otp data
       //shutter
	 index++;
	otp_settings_buf[index] = read_otp_settings.shutter;

	//line
	index++;
	otp_settings_buf[index] = read_otp_settings.line;

	//gain
	index++;
	otp_settings_buf[index] = read_otp_settings.gain;

	//frame_rate
	index++;
	otp_settings_buf[index] = read_otp_settings.frame_rate;

	//i2c clock
	index++;
	otp_settings_buf[index] = read_otp_settings.i2c_clock;

	//main_clock
	index++;
	otp_settings_buf[index] = read_otp_settings.main_clock;

	//image_pattern
	index++;
	otp_settings_buf[index] = read_otp_settings.image_pattern;

	//flip
	index++;
	otp_settings_buf[index] = read_otp_settings.flip;

	//set ev
	index++;
	otp_settings_buf[index] = read_otp_settings.set_ev;

	//set ae
	index++;
	otp_settings_buf[index] = read_otp_settings.set_ae;

	//set af mode
	index++;
	otp_settings_buf[index] = read_otp_settings.set_af_mode;

	//set af position
	index++;
	otp_settings_buf[index] = read_otp_settings.set_af_position;

	//checksum
	index++;
	otp_settings_buf[index] = read_otp_settings.checksum;

	//data lenth bytes
	length = (++index) * sizeof(uint32_t);
	memcpy_ex(1, data_buf, (uint8_t*)otp_settings_buf, length);

	*data_size = length;

	return ret;
}

int read_otp_actuator_i2c(uint32_t *data_size, uint8_t *data_buf)
{
	int ret = 0;
	int read_cnt = 0;
	int addr = 0;
	int i = 0;
	uint8_t *read_ptr;
	uint16_t reg_val = 0;
	SENSOR_VAL_T  io_val ={SENSOR_VAL_TYPE_READ_VCM, NULL};
	uint32_t param;


	memcpy_ex(4,&addr,&data_buf[otp_data_start_index],4);
	memcpy_ex(4,&read_cnt,&data_buf[otp_data_start_index+4],4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d",__func__, addr, read_cnt);

	if(p_camera_device){
		//read to calibration data and write to data_buf[write_value_index]
		read_ptr = &data_buf[otp_data_start_index+4];
		io_val.pval = &param;

		for (i=0 ; i<read_cnt ; i++) {
		//	ret = dcam_i2c_read_reg_16((uint16_t)addr,&reg_val);

			param = addr<<16;
			ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL,(void *)&io_val);
			if (0 != ret) {
				SCI_Trace_Dcam ("%s failed  addr=0x%x ", __func__, addr);
				break;
			}
			reg_val = param & 0xffff;

			*read_ptr++ = reg_val&0xff;
			*read_ptr++ = (reg_val >> 8);
			*read_ptr++ = 0;
			*read_ptr++ = 0;
			addr++;
		}
		SCI_Trace_Dcam("%s %d %d %d\n",__func__, read_cnt,(4 + 4*read_cnt),reg_val);
	}else{
		ret=-1;
	}
	*data_size = 4 + 4*read_cnt+40;

	return ret;
}

int read_otp_sensor_i2c(uint32_t *data_size, uint8_t *data_buf)
{
	int ret = 0;
	int read_cnt = 0;
	int addr = 0;
	int i = 0;
	uint8_t *read_ptr;
	uint16_t reg_val = 0;

	memcpy_ex(4,&addr,&data_buf[otp_data_start_index],4);
	memcpy_ex(4,&read_cnt,&data_buf[otp_data_start_index+4],4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d",__func__, addr, read_cnt);

	if(p_camera_device){
		//read to calibration data and write to data_buf[write_value_index]
		read_ptr = &data_buf[otp_data_start_index+4];
		for (i=0 ; i<read_cnt ; i++) {
		//	ret = dcam_i2c_read_reg_16((uint16_t)addr,&reg_val);
			reg_val = Sensor_ReadReg(addr);
			*read_ptr++ = reg_val&0xff;
			*read_ptr++ = (reg_val >> 8);
			*read_ptr++ = 0;
			*read_ptr++ = 0;
			addr++;
		}
		SCI_Trace_Dcam("%s %d %d %d\n",__func__, read_cnt,(4 + 4*read_cnt),reg_val);
	}else{
		ret=-1;
	}
	*data_size = 4 + 4*read_cnt+40;
	return ret;
}

int read_otp_rom_data(uint32_t *data_size, uint8_t *data_buf)
{
	int ret = 0;
	int otp_data_len = 0;
	int read_cnt = 0;
	int addr = 0;

	memcpy_ex(4,&addr,&data_buf[otp_data_start_index],4);
	memcpy_ex(4,&read_cnt,&data_buf[otp_data_start_index+4],4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d",__func__, addr, read_cnt);
	//read to calibration data and write to data_buf[write_value_index]

	return ret;
}

int isp_otp_read(uint8_t *data_buf, uint32_t *data_size)//DATA
{
	int           ret = 0;
	uint32_t           index = 0;
	uint8_t *log_ptr = data_buf;
	int           otp_type = 0xffff;

	if(data_buf == NULL) {
		SCI_Trace_Dcam("%s return error \n",__func__);
		return ret;
	}
	SCI_Trace_Dcam("%s \n",__func__);

	while(index < *data_size)  {
	      SCI_Trace_Dcam ("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}

	 //the real data need to obtain from otp tool settings
	 //according to protocol bit[40]~bit[87] is otp 48 bytes data
	 //parse the otp data maybe writre a function to do it
	 //get otp type
	 memcpy_ex(4,&otp_type,&data_buf[4],4);
	 SCI_Trace_Dcam("%s otp_type %d\n",__func__, otp_type);
	 switch(otp_type) {
	 case OTP_CALIBRATION_CMD:
		ret = read_otp_calibration_data(data_size, data_buf);
		break;
	 case OTP_CTRL_PARAM_CMD:
		ret = read_otp_ctrl_param(data_size, data_buf);
		break;
	 case OTP_ACTUATOR_I2C_CMD:
		ret = read_otp_actuator_i2c(data_size, data_buf);
		break;
	 case OTP_SENSOR_I2C_CMD:
		ret = read_otp_sensor_i2c(data_size, data_buf);
		break;
	 case OTP_ROM_DATA_CMD:
		ret = read_otp_rom_data(data_size, data_buf);
		break;
	 default:
		SCI_Trace_Dcam("%s:error,otp_type=%d \n",__func__, otp_type);
		break;
	 }

	SCI_Trace_Dcam("data_size= %d \r\n", *data_size);

	index = 0;
	log_ptr = data_buf;
	while(index < *data_size)  {
	      SCI_Trace_Dcam ("%s data_buf[%d]= %02x\n", __func__, index++, *log_ptr++);
	}

	SCI_Trace_Dcam("%s: ret=%d ",__func__, ret);
	return ret;
}

