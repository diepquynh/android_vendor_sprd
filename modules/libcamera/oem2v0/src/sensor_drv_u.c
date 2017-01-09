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
#define LOG_TAG "sensor_drv_u"

#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "sensor_cfg.h"
#include "sensor_drv_u.h"

#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4))
#include "isp_cali_interface.h"
#include "isp_param_file_update.h"
#endif

#define SENSOR_CTRL_MSG_QUEUE_SIZE             10

#define SENSOR_CTRL_EVT_BASE                   (CMR_EVT_SENSOR_BASE + 0x800)
#define SENSOR_CTRL_EVT_INIT                   (SENSOR_CTRL_EVT_BASE + 0x0)
#define SENSOR_CTRL_EVT_EXIT                   (SENSOR_CTRL_EVT_BASE + 0x1)
#define SENSOR_CTRL_EVT_SETMODE                (SENSOR_CTRL_EVT_BASE + 0x2)
#define SENSOR_CTRL_EVT_SETMODONE              (SENSOR_CTRL_EVT_BASE + 0x3)
#define SENSOR_CTRL_EVT_CFGOTP                 (SENSOR_CTRL_EVT_BASE + 0x4)
#define SENSOR_CTRL_EVT_STREAM_CTRL            (SENSOR_CTRL_EVT_BASE + 0x5)

/**---------------------------------------------------------------------------*
 **                         Local Variables                                   *
 **---------------------------------------------------------------------------*/
struct sensor_drv_context *s_local_sensor_cxt;
/**---------------------------------------------------------------------------*
 **                         Local Functions                                   *
 **---------------------------------------------------------------------------*/
static cmr_int sns_init_defaul_exif(struct sensor_drv_context *sensor_cxt);
static cmr_int sensor_set_mode(struct sensor_drv_context *sensor_cxt, cmr_u32 mode, cmr_u32 is_inited);
static cmr_int sns_set_mode(struct sensor_drv_context *sensor_cxt, cmr_u32 mode, cmr_u32 is_inited);
static cmr_int sensor_set_modone(struct sensor_drv_context *sensor_cxt);
static cmr_int sns_stream_on(struct sensor_drv_context *sensor_cxt);
static cmr_int sns_stream_off(struct sensor_drv_context *sensor_cxt);
static cmr_int sns_set_id(struct sensor_drv_context *sensor_cxt, SENSOR_ID_E sensor_id);
static cmr_int sns_cfg_otp_update_isparam(struct sensor_drv_context *sensor_cxt, cmr_u32 sensor_id);
static cmr_int sns_open(struct sensor_drv_context *sensor_cxt, cmr_u32 sensor_id);
static cmr_int sns_dev_pwdn(struct sensor_drv_context *sensor_cxt, cmr_u32 power_level);
static SENSOR_ID_E snr_get_cur_id(struct sensor_drv_context *sensor_cxt);
static cmr_int snr_set_mark(struct sensor_drv_context *sensor_cxt, cmr_u8 *buf);
static cmr_int snr_get_mark(struct sensor_drv_context *sensor_cxt, cmr_u8 *buf,cmr_u8 *is_saved_ptr);
static cmr_int snr_chk_snr_mode(SENSOR_MODE_INFO_T *mode_info);
static cmr_u32 sns_get_mipi_phy_id(struct sensor_drv_context *sensor_cxt);
static cmr_int sensor_cfg_otp_update_isparam(struct sensor_drv_context *sensor_cxt,
						cmr_u32 sensor_id);
static cmr_int sns_create_ctrl_thread(struct sensor_drv_context *sensor_cxt);
static cmr_int sns_ctrl_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int sns_destroy_ctrl_thread(struct sensor_drv_context *sensor_cxt);
static cmr_int sns_stream_ctrl_common(struct sensor_drv_context *sensor_cxt, cmr_u32 on_off);

/***---------------------------------------------------------------------------*
 **                       Local function contents                              *
 **----------------------------------------------------------------------------*/
void sensor_set_cxt_common(struct sensor_drv_context *sensor_cxt)
{
	SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
	s_local_sensor_cxt = sensor_cxt;
	return;
}

void *sensor_get_dev_cxt(void)
{
	return (void *)s_local_sensor_cxt;
}

/* This function is to set power down */
cmr_int sns_dev_pwdn(struct sensor_drv_context *sensor_cxt,
					cmr_u32 power_level)
{
	cmr_int ret = SENSOR_SUCCESS;
	cmr_u8 local_power_level  = power_level;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_PD, &local_power_level);
	if (0 != ret) {
		CMR_LOGE("failed,  power_level = %d, ret=%ld ", power_level, ret);
		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_set_motor_val(struct sensor_drv_context *sensor_cxt,
						cmr_u32 vdd_value)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_SET_CAMMOT, &vdd_value);
	if (0 != ret) {
		CMR_LOGE("failed,  vdd_value = %d, ret=%ld ",
			vdd_value, ret);
		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_set_avdd(struct sensor_drv_context *sensor_cxt,
						cmr_u32 vdd_value)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_SET_AVDD, &vdd_value);
	if (0 != ret) {
		CMR_LOGE("failed,  vdd_value = %d, ret=%ld ", vdd_value, ret);
		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_set_dvdd(struct sensor_drv_context *sensor_cxt,
						cmr_u32 vdd_value)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_SET_DVDD, &vdd_value);
	if (0 != ret) {
		CMR_LOGE("failed,  vdd_value = %d, ret=%ld ", vdd_value, ret);
		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_set_iovdd(struct sensor_drv_context *sensor_cxt,
						cmr_u32 vdd_value)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_SET_IOVDD, &vdd_value);
	if (0 != ret) {
		CMR_LOGE("failed,  vdd_value = %d, ret=%ld ", vdd_value, ret);
		ret = -1;
	}

	return ret;
}

cmr_int sns_device_read(struct sensor_drv_context *sensor_cxt,
				cmr_u8 *buff, cmr_u32 size)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = read(sensor_cxt->fd_sensor, buff, size);

	return ret;
}

cmr_int sns_device_write(struct sensor_drv_context *sensor_cxt,
				cmr_u8 *buff, cmr_u32 size)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = write(sensor_cxt->fd_sensor, buff, size);

	if (0 != ret) {
		CMR_LOGE("failed,  buff[0] = %d, size=%d, ret=%ld ", buff[0], size, ret);
		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_set_mclk(struct sensor_drv_context *sensor_cxt,
					cmr_u32 mclk)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_SET_MCLK, &mclk);

	if (0 != ret) {
		CMR_LOGE("failed,  mclk = %d, ret = %ld  ", mclk, ret);
		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_reset(struct sensor_drv_context *sensor_cxt,
					cmr_u32 *reset_val)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	CMR_LOGI("level %d, width %d",reset_val[0],reset_val[1]);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_RST, reset_val);
	if (ret) {
		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_i2c_init(struct sensor_drv_context *sensor_cxt,
					cmr_u32 sensor_id)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	CMR_LOGV("E");
/*	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_I2C_INIT, &senor_id);*/
	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_SET_ID, &sensor_id);
	if (0 != ret) {
		CMR_LOGE("failed,  senor_id = %d, ret=%ld", sensor_id, ret);
		ret = -1;
	}
	CMR_LOGV("X");
	return ret;
}

cmr_int sns_dev_i2c_deinit(struct sensor_drv_context *sensor_cxt,
					cmr_u32 sensor_id)
{
	UNUSED(sensor_cxt);
	UNUSED(sensor_id);

	cmr_int ret = SENSOR_SUCCESS;
	/*SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	CMR_LOGI("E");

	CMR_LOGI("X, now dummy handle");*/
	return ret;
}


cmr_int sns_dev_rst_lvl(struct sensor_drv_context *sensor_cxt,
					cmr_u32 level)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	CMR_LOGV("fd 0x%lx", sensor_cxt->fd_sensor);
	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_RST_LEVEL, &level);
	if (0 != ret) {
		CMR_LOGE("failed,  level = %d, ret=%ld ", level, ret);
		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_mipi_lvl(struct sensor_drv_context *sensor_cxt,
					cmr_u32 level)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	CMR_LOGV("fd 0x%lx", sensor_cxt->fd_sensor);
	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_SET_MIPI_SWITCH, &level);
	if (0 != ret) {
		CMR_LOGE("failed,  level = %d, ret=%ld ", level, ret);
		ret = -1;
	}

	return ret;
}
cmr_int sns_dev_set_i2c_addr(struct sensor_drv_context *sensor_cxt)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_I2C_ADDR, &sensor_cxt->i2c_addr);
	if (0 != ret) {
		CMR_LOGE("failed,  addr = 0x%x, ret=%ld ",
			sensor_cxt->i2c_addr,
			ret);
		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_read_reg(struct sensor_drv_context *sensor_cxt,
					SENSOR_REG_BITS_T_PTR reg)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_I2C_READ, reg);
	if (0 != ret) {
		CMR_LOGE("failed,  addr = 0x%x, value=0x%x, bit=%d, ret=%ld ",
			reg->reg_addr, reg->reg_value, reg->reg_bits, ret);
		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_write_reg(struct sensor_drv_context *sensor_cxt,
					SENSOR_REG_BITS_T_PTR reg)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_I2C_WRITE, reg);
	if (0 != ret) {
		CMR_LOGE("failed,  addr = 0x%x, value=%x, bit=%d, ret=%ld ",
			reg->reg_addr, reg->reg_value, reg->reg_bits, ret);
		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_get_flash_level (struct sensor_drv_context *sensor_cxt,
								 struct sensor_flash_level *level)
{
	int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_GET_FLASH_LEVEL, level);
	if (0 != ret) {
		CMR_LOGE("_Sensor_Device_GetFlashLevel failed, ret=%d \n",  ret);
		ret = -1;
	}

	return ret;
}

//TBSPLIT
cmr_int Sensor_Device_WriteRegTab(SENSOR_REG_TAB_PTR reg_tab)
{
	cmr_int ret = SENSOR_SUCCESS;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_I2C_WRITE_REGS, reg_tab);
	if (0 != ret) {
		CMR_LOGE("failed,  ptr=%p, count=%d, bits=%d, burst=%d ",
			reg_tab->sensor_reg_tab_ptr, reg_tab->reg_count, reg_tab->reg_bits, reg_tab->burst_mode);

		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_set_i2c_clk(struct sensor_drv_context *sensor_cxt,
						cmr_u32 clock)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_SET_I2CCLOCK, &clock);
	if (0 != ret) {
		CMR_LOGE("failed,  clock = %d, ret=%ld ", clock, ret);
		ret = -1;
	}

	return ret;
}

cmr_int sns_set_i2c_clk(struct sensor_drv_context *sensor_cxt)
{
	cmr_u32 freq;
	cmr_u32 clock;
	cmr_int ret;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	if (PNULL == sensor_cxt->sensor_info_ptr) {
		CMR_LOGE("No sensor info ");
		return -1;
	}

	freq = sensor_cxt->sensor_info_ptr->reg_addr_value_bits & SENSOR_I2C_CLOCK_MASK;

	switch(freq)
	{
		case SENSOR_I2C_FREQ_20:
			clock = 20000;
			break;

		case SENSOR_I2C_FREQ_50:
			clock = 50000;
			break;

		case SENSOR_I2C_FREQ_100:
			clock = 100000;
			break;

		case SENSOR_I2C_FREQ_200:
			clock = 200000;
			break;

		case SENSOR_I2C_FREQ_400:
			clock = 400000;
			break;

		default:
			clock = 100000;
			CMR_LOGI("no valid freq, set clock to 100k ");
			break;
	}

	CMR_LOGI("clock = %d ", clock);

	ret = sns_dev_set_i2c_clk(sensor_cxt, clock);

	return ret;
}

cmr_int sns_dev_i2c_write(struct sensor_drv_context *sensor_cxt,
					SENSOR_I2C_T_PTR i2c_tab)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_I2C_WRITE_EXT, i2c_tab);
	if (0 != ret) {
		CMR_LOGE("failed, slave_addr=0x%x, ptr=0x%p, count=%d",
			i2c_tab->slave_addr, i2c_tab->i2c_data, i2c_tab->i2c_count);
		ret = -1;
	}

	return ret;
}

cmr_int sns_dev_i2c_read(struct sensor_drv_context *sensor_cxt,
					SENSOR_I2C_T_PTR i2c_tab)
{
	int ret = SENSOR_SUCCESS;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_I2C_READ_EXT, i2c_tab);
	if (0 != ret)
	{
		CMR_LOGE("_Sensor_Device_I2CRead failed, slave_addr=0x%x, ptr=0x%p, count=%d\n",
			i2c_tab->slave_addr, i2c_tab->i2c_data, i2c_tab->i2c_count);
		ret = -1;
	}

	return ret;
}

cmr_u32 sns_get_mipi_phy_id(struct sensor_drv_context *sensor_cxt)
{
	cmr_u32         phy_id = 0;
	SENSOR_ID_E     sensor_id = snr_get_cur_id(sensor_cxt);

	if (SENSOR_MAIN == sensor_id) {
#if defined(CONFIG_BACK_CAMERA_MIPI_PHYA)
		phy_id = 0x01;
#elif defined(CONFIG_BACK_CAMERA_MIPI_PHYB)
		phy_id = 0x02;
#elif defined(CONFIG_BACK_CAMERA_MIPI_PHYAB)
		phy_id = 0x03;
#endif
		CMR_LOGI("main phy_id:%x \n", phy_id);
	} else {
#if defined(CONFIG_FRONT_CAMERA_MIPI_PHYA)
		phy_id = 0x01;
#elif defined(CONFIG_FRONT_CAMERA_MIPI_PHYB)
		phy_id = 0x02;
#elif defined(CONFIG_FRONT_CAMERA_MIPI_PHYAB)
		phy_id = 0x03;
#elif defined(CONFIG_FRONT_CAMERA_MIPI_PHYC)
		phy_id = 0x04;
#endif
		CMR_LOGI("sub phy_id:%x \n", phy_id);
	}

	return phy_id;
}

cmr_int sns_dev_mipi_init(struct sensor_drv_context *sensor_cxt,
					cmr_u32 mode)
{
	cmr_int ret      = SENSOR_SUCCESS;
	SENSOR_IF_CFG_T  if_cfg;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	cmr_u32 lane_num = sensor_cxt->sensor_exp_info.sensor_interface.bus_width;
	cmr_u32 bps      = sensor_cxt->sensor_exp_info.sensor_mode_info[mode].bps_per_lane;

	cmr_bzero((void*)&if_cfg, sizeof(SENSOR_IF_CFG_T));
	if_cfg.if_type      = INTERFACE_MIPI;
	if_cfg.is_open      = INTERFACE_OPEN;
	if_cfg.lane_num     = lane_num;
	if_cfg.phy_id       = sns_get_mipi_phy_id(sensor_cxt);
	if_cfg.bps_per_lane = bps;
	CMR_LOGI("Lane num %d, bps %d phy id %d", lane_num, bps, if_cfg.phy_id);
	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_IF_CFG, &if_cfg);
	if (0 != ret) {
		CMR_LOGE("failed, ret=%ld, lane=%d, bps=%d", ret, lane_num, bps);
		ret = -1;
	}
	//usleep(15*1000);
	return ret;
}

cmr_int sns_dev_mipi_deinit(struct sensor_drv_context *sensor_cxt)
{
	cmr_int ret = SENSOR_SUCCESS;
	SENSOR_IF_CFG_T if_cfg;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	cmr_bzero((void*)&if_cfg, sizeof(SENSOR_IF_CFG_T));
	if_cfg.if_type      = INTERFACE_MIPI;
	if_cfg.is_open      = INTERFACE_CLOSE;
	if_cfg.phy_id       = sns_get_mipi_phy_id(sensor_cxt);
	CMR_LOGI("phy id %d", if_cfg.phy_id);
	ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_IF_CFG, &if_cfg);
	if (0 != ret) {
		CMR_LOGE("failed, 0x%lx", ret);
		ret = -1;
	}
	CMR_LOGI("close mipi");

	return ret;
}

cmr_int Sensor_WriteI2C(cmr_u16 slave_addr, cmr_u8 *cmd, cmr_u16 cmd_length)
{
	SENSOR_I2C_T i2c_tab;
	cmr_int ret = SENSOR_SUCCESS;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	i2c_tab.slave_addr 	= slave_addr;
	i2c_tab.i2c_data	= cmd;
	i2c_tab.i2c_count	= cmd_length;

	CMR_LOGI("slave_addr=0x%x, ptr=0x%p, count=%d",
		i2c_tab.slave_addr, i2c_tab.i2c_data, i2c_tab.i2c_count);

	ret = sns_dev_i2c_write(sensor_cxt, &i2c_tab);

	return ret;
}

cmr_int Sensor_ReadI2C(cmr_u16 slave_addr, cmr_u8 *cmd, cmr_u16 cmd_length)
{
	SENSOR_I2C_T i2c_tab;
	cmr_int ret = SENSOR_SUCCESS;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	i2c_tab.slave_addr 	= slave_addr;
	i2c_tab.i2c_data	= cmd;
	i2c_tab.i2c_count	= cmd_length;

	CMR_LOGV("Sensor_ReadI2C, slave_addr=0x%x, ptr=0x%p, count=%d\n",
		i2c_tab.slave_addr, i2c_tab.i2c_data, i2c_tab.i2c_count);

	ret = sns_dev_i2c_read(sensor_cxt, &i2c_tab);

	return ret;
}
void Sensor_Reset(cmr_u32 level)
{
	cmr_int err = 0xff;
	cmr_u32 rst_val[2];
	SENSOR_IOCTL_FUNC_PTR reset_func;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	CMR_LOGI("in.");

	SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);

	if (PNULL == sensor_cxt->sensor_info_ptr) {
		CMR_LOGE("Sensor_SetI2CClock: No sensor info ");
		return;
	}

	reset_func = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->reset;

	if (PNULL != reset_func) {
		reset_func(level);
	} else {
		rst_val[0] = level;
		rst_val[1] = sensor_cxt->sensor_info_ptr->reset_pulse_width;
		if (rst_val[1] < SENSOR_RESET_PULSE_WIDTH_DEFAULT) {
			rst_val[1] = SENSOR_RESET_PULSE_WIDTH_DEFAULT;
		} else if (rst_val[1] > SENSOR_RESET_PULSE_WIDTH_MAX) {
			rst_val[1] = SENSOR_RESET_PULSE_WIDTH_MAX;
		}
		sns_dev_reset(sensor_cxt, rst_val);
	}
	CMR_LOGI("OK out.");
}


cmr_int Sensor_SetMCLK(cmr_u32 mclk)
{
	cmr_int ret;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	CMR_LOGV("mclk %d ", mclk);

	ret = sns_dev_set_mclk(sensor_cxt, mclk);

	CMR_LOGI("mclk %d, ret %ld ", mclk, ret );

	return ret;
}

cmr_int Sensor_SetVoltage(SENSOR_AVDD_VAL_E dvdd_val, SENSOR_AVDD_VAL_E avdd_val,
			SENSOR_AVDD_VAL_E iodd_val)
{

	cmr_int err = 0;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	err = sns_dev_set_avdd(sensor_cxt, (cmr_u32)avdd_val);
	if(SENSOR_SUCCESS != err)
		return err;

	err = sns_dev_set_dvdd(sensor_cxt, (cmr_u32)dvdd_val);
	if(SENSOR_SUCCESS != err)
		return err;

	err = sns_dev_set_iovdd(sensor_cxt, (cmr_u32)iodd_val);
	if(SENSOR_SUCCESS != err)
		return err;

	CMR_LOGI("avdd_val = %d,  dvdd_val=%d, iodd_val=%d ", avdd_val, dvdd_val, iodd_val);

	return err;
}

cmr_int Sensor_SetAvddVoltage(SENSOR_AVDD_VAL_E vdd_val)
{
	cmr_int rtn  = SENSOR_SUCCESS;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	rtn = sns_dev_set_avdd(sensor_cxt, (cmr_u32)vdd_val);
	CMR_LOGI("vdd_val is %d, set result is =%ld ", vdd_val, rtn);
	return rtn;
}

cmr_int Sensor_SetDvddVoltage(SENSOR_AVDD_VAL_E vdd_val)
{
	cmr_int rtn  = SENSOR_SUCCESS;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	rtn = sns_dev_set_dvdd(sensor_cxt, (cmr_u32)vdd_val);
	CMR_LOGI("vdd_val is %d, set result is =%ld ", vdd_val, rtn);
	return rtn;
}


cmr_int Sensor_SetIovddVoltage(SENSOR_AVDD_VAL_E vdd_val)
{
	cmr_int rtn  = SENSOR_SUCCESS;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	rtn = sns_dev_set_iovdd(sensor_cxt, (cmr_u32)vdd_val);
	CMR_LOGI("vdd_val is %d, set result is =%ld ", vdd_val, rtn);
	return rtn;
}

cmr_int Sensor_SetMonitorVoltage(SENSOR_AVDD_VAL_E vdd_val)
{
	cmr_int err = 0;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	err = sns_dev_set_motor_val(sensor_cxt, (cmr_u32)vdd_val);
	CMR_LOGI("vdd_val = %d ", vdd_val);

	return err;
}

void Sensor_PowerOn(struct sensor_drv_context *sensor_cxt, cmr_u32 power_on)
{
	struct sensor_power_info_tag power_cfg;
	cmr_u32 power_down;
	SENSOR_AVDD_VAL_E dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val;
	SENSOR_IOCTL_FUNC_PTR power_func;
	cmr_u32 rst_lvl;
	SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);

	if (PNULL == sensor_cxt->sensor_info_ptr) {
		CMR_LOGE("No sensor info!");
		return;
	}
	rst_lvl = sensor_cxt->sensor_info_ptr->reset_pulse_level;
	power_down = (cmr_u32)sensor_cxt->sensor_info_ptr->power_down_level;
	dvdd_val = sensor_cxt->sensor_info_ptr->dvdd_val;
	avdd_val = sensor_cxt->sensor_info_ptr->avdd_val;
	iovdd_val = sensor_cxt->sensor_info_ptr->iovdd_val;
	power_func = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->power;

	CMR_LOGI("power_on = %d, power_down_level = %d, avdd_val = %d",
		power_on, power_down, avdd_val);

	/*when use the camera vendor functions, the sensor_cxt should be set at first */
	sensor_set_cxt_common(sensor_cxt);

	if (PNULL != power_func) {
		power_func(power_on);
	} else {
		memset(&power_cfg, 0, sizeof(struct sensor_power_info_tag));
		power_cfg.op_sensor_id = snr_get_cur_id(sensor_cxt);
		if (power_on) {
			power_cfg.is_on = 1;
		} else {
			power_cfg.is_on = 0;
		}
		ioctl(sensor_cxt->fd_sensor, SENSOR_IO_POWER_CFG, &power_cfg);
	}
}


void Sensor_PowerOn_Ex(struct sensor_drv_context *sensor_cxt,
				cmr_u32 sensor_id)
{
	struct sensor_power_info_tag power_cfg;
	cmr_u32 power_down;
	SENSOR_AVDD_VAL_E dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val;
	SENSOR_IOCTL_FUNC_PTR power_func;
	cmr_u32 rst_lvl = 0;
	SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);

	sns_set_id(sensor_cxt, (SENSOR_ID_E)sensor_id);

	sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[sensor_id];

	power_down = (cmr_u32) sensor_cxt->sensor_info_ptr->power_down_level;
	dvdd_val = sensor_cxt->sensor_info_ptr->dvdd_val;
	avdd_val = sensor_cxt->sensor_info_ptr->avdd_val;
	iovdd_val = sensor_cxt->sensor_info_ptr->iovdd_val;
	power_func = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->power;

	CMR_LOGI("power_down_level = %d, avdd_val = %d", power_down, avdd_val);
	/*when use the camera vendor functions, the sensor_cxt should be set at first */
	sensor_set_cxt_common(sensor_cxt);

	if (PNULL != power_func) {
		power_func(1);
	} else {
		memset(&power_cfg, 0, sizeof(struct sensor_power_info_tag));
		power_cfg.is_on = 1;
		power_cfg.op_sensor_id = sensor_id;
		ioctl(sensor_cxt->fd_sensor, SENSOR_IO_POWER_CFG, &power_cfg);
	}
}

cmr_int Sensor_PowerDown(cmr_u32 power_level)
{
	SENSOR_IOCTL_FUNC_PTR entersleep_func = PNULL;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if (PNULL == sensor_cxt->sensor_info_ptr) {
		CMR_LOGE("No sensor info ");
		return SENSOR_FAIL;
	}

	entersleep_func = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->enter_sleep;

	CMR_LOGV("power_level %d", power_level);

	if (entersleep_func) {
		entersleep_func(power_level);
		return SENSOR_SUCCESS;
	}

	if (-1 == sns_dev_pwdn(sensor_cxt, power_level))
		return SENSOR_FAIL;

	return SENSOR_SUCCESS;
}

cmr_int Sensor_SetResetLevel(cmr_u32 plus_level)
{
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();
	if (-1 == sns_dev_rst_lvl(sensor_cxt, (cmr_u32)plus_level))
		return SENSOR_FAIL;

	return SENSOR_SUCCESS;
}

cmr_int Sensor_SetMIPILevel(cmr_u32 plus_level)
{
	#ifdef CONFIG_DCAM_SENSOR_DEV_2_SUPPORT
	CMR_LOGE("Sensor_SetMIPILevel IN");
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();
	if (-1 == sns_dev_mipi_lvl(sensor_cxt, (cmr_u32)plus_level))
		return SENSOR_FAIL;
	#endif

	CMR_LOGE("Sensor_SetMIPILevel out");
	return SENSOR_SUCCESS;
}

void Sensor_SetExportInfo(struct sensor_drv_context *sensor_cxt)
{
	SENSOR_REG_TAB_INFO_T        *resolution_info_ptr = PNULL;
	SENSOR_TRIM_T_PTR            resolution_trim_ptr = PNULL;
	SENSOR_INFO_T                *sensor_info_ptr = PNULL;;
	SENSOR_VIDEO_INFO_T          *video_info_ptr = PNULL;
	cmr_u32 i = 0;

	CMR_LOGI("E");
	SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);

	SENSOR_EXP_INFO_T            *exp_info_ptr = &sensor_cxt->sensor_exp_info;

	if (PNULL == sensor_cxt->sensor_info_ptr) {
		CMR_LOGE("X. sensor_info_ptr is null.");
		return;
	} else {
		sensor_info_ptr = sensor_cxt->sensor_info_ptr;
	}

	SENSOR_MEMSET(exp_info_ptr, 0x00, sizeof(SENSOR_EXP_INFO_T));
	exp_info_ptr->name = sensor_info_ptr->name;
	exp_info_ptr->image_format = sensor_info_ptr->image_format;
	exp_info_ptr->image_pattern = sensor_info_ptr->image_pattern;

	exp_info_ptr->pclk_polarity = (sensor_info_ptr->hw_signal_polarity & 0x01);  /*the high 3bit will be the phase(delay sel)*/
	exp_info_ptr->vsync_polarity =
		((sensor_info_ptr->hw_signal_polarity >> 2) & 0x1);
	exp_info_ptr->hsync_polarity =
		((sensor_info_ptr->hw_signal_polarity >> 4) & 0x1);
	exp_info_ptr->pclk_delay =
		((sensor_info_ptr->hw_signal_polarity >> 5) & 0x07);

	if (NULL!=sensor_info_ptr->raw_info_ptr) {
		exp_info_ptr->raw_info_ptr = (struct sensor_raw_info*)*sensor_info_ptr->raw_info_ptr;
	}

	if ((NULL != exp_info_ptr)
		&&(NULL != exp_info_ptr->raw_info_ptr)
		&&(NULL != exp_info_ptr->raw_info_ptr->resolution_info_ptr)) {
		exp_info_ptr->raw_info_ptr->resolution_info_ptr->image_pattern = sensor_info_ptr->image_pattern;
	}

	exp_info_ptr->source_width_max = sensor_info_ptr->source_width_max;
	exp_info_ptr->source_height_max = sensor_info_ptr->source_height_max;

	exp_info_ptr->environment_mode = sensor_info_ptr->environment_mode;
	exp_info_ptr->image_effect = sensor_info_ptr->image_effect;
	exp_info_ptr->wb_mode = sensor_info_ptr->wb_mode;
	exp_info_ptr->step_count = sensor_info_ptr->step_count;

	exp_info_ptr->ext_info_ptr = sensor_info_ptr->ext_info_ptr;

	exp_info_ptr->preview_skip_num = sensor_info_ptr->preview_skip_num;
	exp_info_ptr->capture_skip_num = sensor_info_ptr->capture_skip_num;
	exp_info_ptr->preview_deci_num = sensor_info_ptr->preview_deci_num;
	exp_info_ptr->change_setting_skip_num = sensor_info_ptr->change_setting_skip_num;
	exp_info_ptr->video_preview_deci_num =
		sensor_info_ptr->video_preview_deci_num;

	exp_info_ptr->threshold_eb = sensor_info_ptr->threshold_eb;
	exp_info_ptr->threshold_mode = sensor_info_ptr->threshold_mode;
	exp_info_ptr->threshold_start = sensor_info_ptr->threshold_start;
	exp_info_ptr->threshold_end = sensor_info_ptr->threshold_end;

	exp_info_ptr->ioctl_func_ptr = sensor_info_ptr->ioctl_func_tab_ptr;
	if (PNULL != sensor_info_ptr->ioctl_func_tab_ptr->get_trim) {
		/*the get trim function need not the sensor_cxt(the fd)*/
		resolution_trim_ptr =
			(SENSOR_TRIM_T_PTR)sensor_info_ptr->ioctl_func_tab_ptr->get_trim(0x00);
	}
	for (i = SENSOR_MODE_COMMON_INIT; i < SENSOR_MODE_MAX; i++) {
		resolution_info_ptr =
		    &(sensor_info_ptr->resolution_tab_info_ptr[i]);

		if (SENSOR_IMAGE_FORMAT_JPEG == resolution_info_ptr->image_format) {
			exp_info_ptr->sensor_image_type = SENSOR_IMAGE_FORMAT_JPEG;
		}

		if ((PNULL != resolution_info_ptr->sensor_reg_tab_ptr)
		    || ((0x00 != resolution_info_ptr->width)
			&& (0x00 != resolution_info_ptr->width))) {
			exp_info_ptr->sensor_mode_info[i].mode = i;
			exp_info_ptr->sensor_mode_info[i].width =
			    resolution_info_ptr->width;
			exp_info_ptr->sensor_mode_info[i].height =
			    resolution_info_ptr->height;
			if ((PNULL != resolution_trim_ptr)
			    && (0x00 != resolution_trim_ptr[i].trim_width)
			    && (0x00 != resolution_trim_ptr[i].trim_height)) {
				exp_info_ptr->sensor_mode_info[i].trim_start_x =
				    resolution_trim_ptr[i].trim_start_x;
				exp_info_ptr->sensor_mode_info[i].trim_start_y =
				    resolution_trim_ptr[i].trim_start_y;
				exp_info_ptr->sensor_mode_info[i].trim_width =
				    resolution_trim_ptr[i].trim_width;
				exp_info_ptr->sensor_mode_info[i].trim_height =
				    resolution_trim_ptr[i].trim_height;
				exp_info_ptr->sensor_mode_info[i].line_time =
				    resolution_trim_ptr[i].line_time;
				exp_info_ptr->sensor_mode_info[i].bps_per_lane =
				    resolution_trim_ptr[i].bps_per_lane;
				exp_info_ptr->sensor_mode_info[i].frame_line=
				    resolution_trim_ptr[i].frame_line;
			} else {
				exp_info_ptr->sensor_mode_info[i].trim_start_x =
				    0x00;
				exp_info_ptr->sensor_mode_info[i].trim_start_y =
				    0x00;
				exp_info_ptr->sensor_mode_info[i].trim_width =
				    resolution_info_ptr->width;
				exp_info_ptr->sensor_mode_info[i].trim_height =
				    resolution_info_ptr->height;
			}

			/*scaler trim*/
			if ((PNULL != resolution_trim_ptr)
			    && (0x00 != resolution_trim_ptr[i].scaler_trim.w)
			    && (0x00 != resolution_trim_ptr[i].scaler_trim.h)) {
				exp_info_ptr->sensor_mode_info[i].scaler_trim =
				    resolution_trim_ptr[i].scaler_trim;
			} else {
				exp_info_ptr->sensor_mode_info[i].scaler_trim.x =
				    0x00;
				exp_info_ptr->sensor_mode_info[i].scaler_trim.y =
				    0x00;
				exp_info_ptr->sensor_mode_info[i].scaler_trim.w =
				    exp_info_ptr->sensor_mode_info[i].trim_width;
				exp_info_ptr->sensor_mode_info[i].scaler_trim.h =
				    exp_info_ptr->sensor_mode_info[i].trim_height;
			}

			if (SENSOR_IMAGE_FORMAT_MAX !=
			    sensor_info_ptr->image_format) {
				exp_info_ptr->sensor_mode_info[i].image_format =
				    sensor_info_ptr->image_format;
			} else {
				exp_info_ptr->sensor_mode_info[i].image_format =
				    resolution_info_ptr->image_format;
			}
		} else {
			exp_info_ptr->sensor_mode_info[i].mode =
			    SENSOR_MODE_MAX;
		}
		if (PNULL != sensor_info_ptr->video_tab_info_ptr) {
			video_info_ptr = &sensor_info_ptr->video_tab_info_ptr[i];
			if (PNULL != video_info_ptr) {
				cmr_copy((void*)&exp_info_ptr->sensor_video_info[i], (void*)video_info_ptr,sizeof(SENSOR_VIDEO_INFO_T));
			}
		}

		if ((NULL != exp_info_ptr)
			&&(NULL != exp_info_ptr->raw_info_ptr)
			&&(NULL != exp_info_ptr->raw_info_ptr->resolution_info_ptr)) {
			exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].start_x = exp_info_ptr->sensor_mode_info[i].trim_start_x;
			exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].start_y = exp_info_ptr->sensor_mode_info[i].trim_start_y;
			exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].width = exp_info_ptr->sensor_mode_info[i].trim_width;
			exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].height = exp_info_ptr->sensor_mode_info[i].trim_height;
			exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].line_time= exp_info_ptr->sensor_mode_info[i].line_time;
			exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].frame_line= exp_info_ptr->sensor_mode_info[i].frame_line;
		}

		if ((NULL != exp_info_ptr)
			&&(NULL != exp_info_ptr->raw_info_ptr)
			&&(NULL != exp_info_ptr->raw_info_ptr->ioctrl_ptr)) {
			exp_info_ptr->raw_info_ptr->ioctrl_ptr->set_focus = exp_info_ptr->ioctl_func_ptr->af_enable;
			exp_info_ptr->raw_info_ptr->ioctrl_ptr->set_exposure = exp_info_ptr->ioctl_func_ptr->write_ae_value;
			exp_info_ptr->raw_info_ptr->ioctrl_ptr->set_gain = exp_info_ptr->ioctl_func_ptr->write_gain_value;
			exp_info_ptr->raw_info_ptr->ioctrl_ptr->ext_fuc = exp_info_ptr->ioctl_func_ptr->set_focus;
			exp_info_ptr->raw_info_ptr->ioctrl_ptr->write_i2c = Sensor_WriteI2C;
			exp_info_ptr->raw_info_ptr->ioctrl_ptr->read_i2c = Sensor_ReadI2C;
			exp_info_ptr->raw_info_ptr->ioctrl_ptr->ex_set_exposure= exp_info_ptr->ioctl_func_ptr->ex_write_exp;
		}

	}
	exp_info_ptr->sensor_interface = sensor_info_ptr->sensor_interface;
	exp_info_ptr->change_setting_skip_num = sensor_info_ptr->change_setting_skip_num;
	exp_info_ptr->horizontal_view_angle = sensor_info_ptr->horizontal_view_angle;
	exp_info_ptr->vertical_view_angle = sensor_info_ptr->vertical_view_angle;

	CMR_LOGI("X");
}

cmr_int Sensor_WriteReg(cmr_u16 subaddr, cmr_u16 data)
{
	cmr_u8 ret = -1;
	SENSOR_IOCTL_FUNC_PTR write_reg_func = PNULL;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	if (PNULL == sensor_cxt->sensor_info_ptr) {
		CMR_LOGE("SENSOR: sensor_info_ptr is null.");
		return 0xFF;
	}

	write_reg_func = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->write_reg;

	if (PNULL != write_reg_func) {
		if (SENSOR_OP_SUCCESS != write_reg_func((subaddr << S_BIT_4) + data)) {
			CMR_LOGI("SENSOR: IIC write : reg:0x%04x, val:0x%04x error",
					subaddr, data);
		}
	} else {

		SENSOR_REG_BITS_T reg;

		reg.reg_addr = subaddr;
		reg.reg_value = data;
		reg.reg_bits = sensor_cxt->sensor_info_ptr->reg_addr_value_bits;

		ret = sns_dev_write_reg(sensor_cxt, &reg);
	}

	return ret;
}

cmr_u32 Sensor_ReadReg(cmr_u16 reg_addr)
{

	cmr_u32 i = 0;
	cmr_u16 ret_val = 0xffff;
	cmr_int ret = -1;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	SENSOR_IOCTL_FUNC_PTR read_reg_func;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if (PNULL == sensor_cxt->sensor_info_ptr) {
		CMR_LOGE("No sensor info!");
		return 0xFF;
	}

	read_reg_func = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->read_reg;

	if (PNULL != read_reg_func) {
		ret_val = (cmr_u16)read_reg_func((cmr_u32)(reg_addr & SENSOR_LOW_SIXTEEN_BIT));
	} else {
		SENSOR_REG_BITS_T reg;

		reg.reg_addr = reg_addr;
		reg.reg_bits = sensor_cxt->sensor_info_ptr->reg_addr_value_bits;

		ret = sns_dev_read_reg(sensor_cxt, &reg);
		if(SENSOR_SUCCESS == ret){
			ret_val = reg.reg_value;
		}

	}

	return ret_val;
}

cmr_int Sensor_WriteReg_8bits(cmr_u16 reg_addr, cmr_u8 value)
{
	cmr_u8 ret = -1;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	if (0xFFFF == reg_addr) {
		SENSOR_Sleep(value);
		CMR_LOGI("wait %d ms.", value);
		return 0;
	}

	SENSOR_REG_BITS_T reg;

	reg.reg_addr = reg_addr;
	reg.reg_value = value;
	reg.reg_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT;

	ret = sns_dev_write_reg(sensor_cxt, &reg);

	return 0;
}

cmr_int Sensor_ReadReg_8bits(cmr_u8 reg_addr, cmr_u8 * reg_val)
{

	cmr_u8 ret = -1;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	SENSOR_REG_BITS_T reg;

	reg.reg_addr = reg_addr;
	reg.reg_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT;

	ret = sns_dev_read_reg(sensor_cxt, &reg);
	if(SENSOR_SUCCESS == ret){
		*reg_val  = reg.reg_value;
	}

	return ret;
}

cmr_int Sensor_SendRegTabToSensor(SENSOR_REG_TAB_INFO_T *sensor_reg_tab_info_ptr)
{
	cmr_u32 i;
	SENSOR_IOCTL_FUNC_PTR write_reg_func;
	cmr_u16 subaddr;
	cmr_u16 data;
	cmr_u8 ret = -1;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	CMR_LOGI("E");

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if (PNULL == sensor_cxt->sensor_info_ptr) {
		CMR_LOGE("No sensor info!");
		return 0xFF;
	}

	write_reg_func = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->write_reg;

	if (PNULL != write_reg_func) {
		for (i = 0; i < sensor_reg_tab_info_ptr->reg_count; i++) {
			subaddr = sensor_reg_tab_info_ptr->sensor_reg_tab_ptr[i].reg_addr;
			data	= sensor_reg_tab_info_ptr->sensor_reg_tab_ptr[i].reg_value;
			if (SENSOR_OP_SUCCESS != write_reg_func((subaddr << S_BIT_4) + data))
				CMR_LOGI("SENSOR: IIC write : reg:0x%04x, val:0x%04x error", subaddr, data);
		}
	}else{
		SENSOR_REG_TAB_T regTab;
		regTab.reg_count = sensor_reg_tab_info_ptr->reg_count;
		regTab.reg_bits = sensor_cxt->sensor_info_ptr->reg_addr_value_bits;
		regTab.burst_mode = 0;
		regTab.sensor_reg_tab_ptr = sensor_reg_tab_info_ptr->sensor_reg_tab_ptr;

		ret = Sensor_Device_WriteRegTab(&regTab);
	}

	CMR_LOGI("reg_count %d, is_main_sensor: %ld",
		sensor_reg_tab_info_ptr->reg_count, sensor_cxt->is_main_sensor);

	CMR_LOGI("X");

	return SENSOR_SUCCESS;
}

void sns_clean_info(struct sensor_drv_context *sensor_cxt)
{
	SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

	SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
	sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
	sensor_cxt->sensor_mode[SENSOR_MAIN] = SENSOR_MODE_MAX;
	sensor_cxt->sensor_mode[SENSOR_SUB] = SENSOR_MODE_MAX;
	sensor_cxt->sensor_mode[SENSOR_DEV_2] = SENSOR_MODE_MAX;
	sensor_cxt->sensor_info_ptr = PNULL;
	sensor_cxt->sensor_isInit = SENSOR_FALSE;
	sensor_cxt->sensor_index[SENSOR_MAIN] = 0xFF;
	sensor_cxt->sensor_index[SENSOR_SUB] = 0xFF;
	sensor_cxt->sensor_index[SENSOR_DEV_2] = 0xFF;
	sensor_cxt->sensor_index[SENSOR_ATV] = 0xFF;
	SENSOR_MEMSET(&sensor_cxt->sensor_exp_info, 0x00, sizeof(SENSOR_EXP_INFO_T));
	sensor_register_info_ptr->cur_id = SENSOR_ID_MAX;
	return;
}

cmr_int sns_set_id(struct sensor_drv_context *sensor_cxt, SENSOR_ID_E sensor_id)
{
	SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

	sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
	sensor_register_info_ptr->cur_id = sensor_id;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	CMR_LOGI("id %d,is_register_sensor %ld",
		sensor_id, sensor_cxt->is_register_sensor);

	if (1 == sensor_cxt->is_register_sensor) {
		if ((SENSOR_MAIN == sensor_id) && (1 == sensor_cxt->is_main_sensor))
			return SENSOR_SUCCESS;
		if ((SENSOR_SUB == sensor_id) && (0 == sensor_cxt->is_main_sensor))
			return SENSOR_SUCCESS;
		if ((SENSOR_DEV_2 == sensor_id) && (2 == sensor_cxt->is_main_sensor))
			return SENSOR_SUCCESS;
	}
	if ((SENSOR_MAIN == sensor_id) || (SENSOR_SUB == sensor_id) ||(SENSOR_DEV_2 == sensor_id)) {
		if (SENSOR_DEV_2 == sensor_id) {
			if ((1 == sensor_cxt->is_register_sensor) && (1 == sensor_cxt->is_main_sensor)) {
				sns_dev_i2c_deinit(sensor_cxt, SENSOR_MAIN);
			}
			if ((1 == sensor_cxt->is_register_sensor) && (0== sensor_cxt->is_main_sensor)) {
				sns_dev_i2c_deinit(sensor_cxt, SENSOR_SUB);
			}
			sensor_cxt->is_main_sensor = 2;
		}else if (SENSOR_SUB == sensor_id) {
			if ((1 == sensor_cxt->is_register_sensor) && (1 == sensor_cxt->is_main_sensor)) {
				sns_dev_i2c_deinit(sensor_cxt, SENSOR_MAIN);
			}
			if ((1 == sensor_cxt->is_register_sensor) && (2 == sensor_cxt->is_main_sensor)) {
				sns_dev_i2c_deinit(sensor_cxt, SENSOR_DEV_2);
			}
			sensor_cxt->is_main_sensor = 0;
		} else if (SENSOR_MAIN == sensor_id) {
			if ((1 == sensor_cxt->is_register_sensor) && (0 == sensor_cxt->is_main_sensor)) {
				sns_dev_i2c_deinit(sensor_cxt, SENSOR_SUB);
			}
			if ((1 == sensor_cxt->is_register_sensor) && (2 == sensor_cxt->is_main_sensor)) {
				sns_dev_i2c_deinit(sensor_cxt, SENSOR_DEV_2);
			}
			sensor_cxt->is_main_sensor = 1;
		}
		sensor_cxt->is_register_sensor = 0;
		if (sns_dev_i2c_init(sensor_cxt, sensor_id)) {
			if (SENSOR_MAIN == sensor_id) {
				sensor_cxt->is_main_sensor = 0;
			}
			CMR_LOGI("add I2C error");
			return SENSOR_FAIL;
		} else {
			CMR_LOGV("add I2C OK.");
			sensor_cxt->is_register_sensor = 1;
		}
	}

	return SENSOR_SUCCESS;
}

SENSOR_ID_E snr_get_cur_id(struct sensor_drv_context *sensor_cxt)
{
	SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
	CMR_LOGV("id %d", sensor_register_info_ptr->cur_id);
	return (SENSOR_ID_E) sensor_register_info_ptr->cur_id;
}

void sns_i2c_init(struct sensor_drv_context *sensor_cxt, SENSOR_ID_E sensor_id)
{
	SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

	sensor_register_info_ptr = PNULL;
	SENSOR_INFO_T **sensor_info_tab_ptr = PNULL;
	SENSOR_INFO_T *sensor_info_ptr= PNULL;
	cmr_u32 i2c_clock = 100000;
	cmr_u32 set_i2c_clock = 0;

	SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
	sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
	sensor_register_info_ptr->cur_id = sensor_id;

	if (0 == sensor_cxt->is_register_sensor) {
		if ((SENSOR_MAIN == sensor_id) || (SENSOR_SUB == sensor_id) ||(SENSOR_DEV_2 == sensor_id) ) {

			if(sns_dev_i2c_init(sensor_cxt, sensor_id)){
				CMR_LOGE("SENSOR: add I2C driver error");
				return;
			} else {
				CMR_LOGI("SENSOR: add I2C driver OK");
				sensor_cxt->is_register_sensor = 1;
			}
		}
	} else {
		CMR_LOGI("Sensor: Init I2c %d ternimal! exits", sensor_id);
	}
	CMR_LOGI("sensor_id=%d, is_register_sensor=%ld",
				sensor_id, sensor_cxt->is_register_sensor);
}

cmr_int sns_i2c_deinit(struct sensor_drv_context *sensor_cxt, SENSOR_ID_E sensor_id)
{
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if (1 == sensor_cxt->is_register_sensor) {
		if ((SENSOR_MAIN == sensor_id) || (SENSOR_SUB == sensor_id)|(SENSOR_DEV_2 == sensor_id)) {
			sns_dev_i2c_deinit(sensor_cxt, sensor_id);
			sensor_cxt->is_register_sensor = 0;
			CMR_LOGI("delete I2C %d driver OK", sensor_id);
		}
	} else {
		CMR_LOGI("delete I2C %d driver OK", SENSOR_ID_MAX);
	}

	return SENSOR_SUCCESS;
}
cmr_u32 sns_identify_search(struct sensor_drv_context *sensor_cxt, SENSOR_ID_E sensor_id)
{
	cmr_u32 sensor_index = 0;
	SENSOR_MATCH_T *sensor_strinfo_tab_ptr = PNULL;
	cmr_u32 valid_tab_index_max = 0x00;
	SENSOR_INFO_T *sensor_info_ptr = PNULL;
	cmr_u32 retValue = SCI_FALSE;
	SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr =  &sensor_cxt->sensor_register_info;

	CMR_LOGI("search all sensor...");
	sensor_strinfo_tab_ptr = (SENSOR_MATCH_T*)Sensor_GetInforTab(sensor_cxt, sensor_id);
	valid_tab_index_max = Sensor_GetInforTabLenght(sensor_cxt, sensor_id) - SENSOR_ONE_I2C;
	sns_i2c_init(sensor_cxt, sensor_id);

	//search the sensor in the table
	for (sensor_index = 0x00; sensor_index < valid_tab_index_max; sensor_index++) {
		sensor_info_ptr = sensor_strinfo_tab_ptr[sensor_index].sensor_info;
		if (NULL==sensor_info_ptr) {
			CMR_LOGW("%d info of Sensor table %d is null", sensor_index, sensor_id);
			continue ;
		}
		sensor_cxt->sensor_info_ptr = sensor_info_ptr;
		Sensor_PowerOn(sensor_cxt, SCI_TRUE);

		if (PNULL != sensor_info_ptr->ioctl_func_tab_ptr->identify) {
			if (SENSOR_ATV != snr_get_cur_id(sensor_cxt)) {
				sensor_cxt->i2c_addr = (sensor_cxt->sensor_info_ptr->salve_i2c_addr_w & 0xFF);
				sns_dev_set_i2c_addr(sensor_cxt);
			}
			CMR_LOGI("identify  Sensor 01");
			if (SENSOR_SUCCESS == sensor_info_ptr->ioctl_func_tab_ptr->identify(SENSOR_ZERO_I2C)) {
				sensor_cxt->sensor_list_ptr[sensor_id] = sensor_info_ptr;
				sensor_register_info_ptr->is_register[sensor_id] = SCI_TRUE;
				if (SENSOR_ATV != snr_get_cur_id(sensor_cxt))
					sensor_cxt->sensor_index[sensor_id] = sensor_index;
				sensor_register_info_ptr->img_sensor_num++;
				Sensor_PowerOn(sensor_cxt, SCI_FALSE);
				retValue = SCI_TRUE;
				CMR_LOGI("sensor_id :%d,img_sensor_num=%d",
					sensor_id, sensor_register_info_ptr->img_sensor_num);
				break ;
			}
		}
		Sensor_PowerOn(sensor_cxt, SCI_FALSE);
	}
	sns_i2c_deinit(sensor_cxt, sensor_id);
	if (SCI_TRUE == sensor_register_info_ptr->is_register[sensor_id]) {
		CMR_LOGI("SENSOR TYPE of %d indentify OK",(cmr_u32)sensor_id);
		sensor_cxt->sensor_param_saved = SCI_TRUE;
	} else {
		CMR_LOGI("SENSOR TYPE of %d indentify failed!",(cmr_u32)sensor_id);
	}

	return retValue;
}

cmr_u32 sns_identify_strsearch(struct sensor_drv_context *sensor_cxt, SENSOR_ID_E sensor_id)
{
	cmr_u32 sensor_index = 0;
	SENSOR_INFO_T *sensor_info_ptr = PNULL;
	cmr_u32 retValue = SCI_FALSE;
	SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr =  &sensor_cxt->sensor_register_info;
	cmr_u32 index_get=0;
	SENSOR_MATCH_T *sensor_strinfo_tab_ptr = PNULL;

	index_get = Sensor_IndexGet(sensor_cxt, sensor_id);
	if (index_get == 0xFF) {
		return retValue;
	}

	sns_i2c_init(sensor_cxt, sensor_id);
	sensor_strinfo_tab_ptr = (SENSOR_MATCH_T*)Sensor_GetInforTab(sensor_cxt, sensor_id);
	sensor_info_ptr = sensor_strinfo_tab_ptr[index_get].sensor_info;

	if (NULL==sensor_info_ptr) {
		CMR_LOGW("%d info of Sensor table %d is null", index_get, sensor_id);
		return retValue ;
	}
	sensor_cxt->sensor_info_ptr = sensor_info_ptr;
	Sensor_PowerOn(sensor_cxt, SCI_TRUE);
	if (PNULL != sensor_info_ptr->ioctl_func_tab_ptr->identify) {
		if (SENSOR_ATV != snr_get_cur_id(sensor_cxt)) {
			sensor_cxt->i2c_addr = (sensor_cxt->sensor_info_ptr->salve_i2c_addr_w & 0xFF);
			sns_dev_set_i2c_addr(sensor_cxt);
		}
		CMR_LOGI("identify  Sensor 01");
		if (SENSOR_SUCCESS == sensor_info_ptr->ioctl_func_tab_ptr->identify(SENSOR_ZERO_I2C)) {
			sensor_cxt->sensor_list_ptr[sensor_id] = sensor_info_ptr;
			sensor_register_info_ptr->is_register[sensor_id] = SCI_TRUE;
			if (SENSOR_ATV != snr_get_cur_id(sensor_cxt))
				sensor_cxt->sensor_index[sensor_id] = index_get;
			sensor_register_info_ptr->img_sensor_num++;
			//Sensor_PowerOn(sensor_cxt, SCI_FALSE);
			retValue = SCI_TRUE;
			CMR_LOGI("sensor_id :%d,img_sensor_num=%d",
				sensor_id, sensor_register_info_ptr->img_sensor_num);
		}
	}
	Sensor_PowerOn(sensor_cxt, SCI_FALSE);
	sns_i2c_deinit(sensor_cxt, sensor_id);
	if (SCI_TRUE == sensor_register_info_ptr->is_register[sensor_id]) {
		CMR_LOGI("SENSOR TYPE of %d indentify OK",(cmr_u32)sensor_id);
		sensor_cxt->sensor_param_saved = SCI_TRUE;
	} else {
		CMR_LOGI("SENSOR TYPE of %d indentify failed!",(cmr_u32)sensor_id);
	}

	return retValue;
}
cmr_int sns_identify(struct sensor_drv_context *sensor_cxt, SENSOR_ID_E sensor_id)
{
	cmr_u32 sensor_index = 0;
	SENSOR_MATCH_T *sensor_strinfo_tab_ptr = PNULL;
	cmr_u32 valid_tab_index_max = 0x00;
	SENSOR_INFO_T *sensor_info_ptr = PNULL;
	cmr_u32 retValue = SCI_FALSE;
	SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

	CMR_LOGI("sensor identifing %d", sensor_id);
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	sensor_register_info_ptr = &sensor_cxt->sensor_register_info;

	/*when use the camera vendor functions, the sensor_cxt should be set at first */
	sensor_set_cxt_common(sensor_cxt);

	//if already identified
	if (SCI_TRUE == sensor_register_info_ptr->is_register[sensor_id]) {
		CMR_LOGI("sensor identified");
		return SCI_TRUE;
	}
	if (sensor_cxt->sensor_identified && (SENSOR_ATV != sensor_id)) {
		sensor_index = sensor_cxt->sensor_index[sensor_id];
		CMR_LOGI("sensor_index = %d",sensor_index);
		if (0xFF != sensor_index) {
			sensor_strinfo_tab_ptr = (SENSOR_MATCH_T*)Sensor_GetInforTab(sensor_cxt, sensor_id);
			sns_i2c_init(sensor_cxt, sensor_id);
			sensor_info_ptr = sensor_strinfo_tab_ptr[sensor_index].sensor_info;
			if (NULL == sensor_info_ptr) {
				CMR_LOGW("%d info of Sensor table %d is null", sensor_index, sensor_id);
				sns_i2c_deinit(sensor_cxt, sensor_id);
				goto IDENTIFY_SEARCH;
			}
			sensor_cxt->sensor_info_ptr = sensor_info_ptr;
			Sensor_PowerOn(sensor_cxt, SCI_TRUE);
			if (PNULL != sensor_info_ptr->ioctl_func_tab_ptr->identify) {
				sensor_cxt->i2c_addr = (sensor_cxt->sensor_info_ptr->salve_i2c_addr_w & 0xFF);
				sns_dev_set_i2c_addr(sensor_cxt);

				CMR_LOGI("identify  Sensor 01");
				if(SENSOR_SUCCESS == sensor_info_ptr->ioctl_func_tab_ptr->identify(SENSOR_ZERO_I2C)) {
					sensor_cxt->sensor_list_ptr[sensor_id] = sensor_info_ptr;
					sensor_register_info_ptr->is_register[sensor_id] = SCI_TRUE;
					sensor_register_info_ptr->img_sensor_num++;
					retValue = SCI_TRUE;
					CMR_LOGI("sensor_id :%d,img_sensor_num=%d",
								sensor_id, sensor_register_info_ptr->img_sensor_num);
				} else {
					Sensor_PowerOn(sensor_cxt, SCI_FALSE);
					sns_i2c_deinit(sensor_cxt, sensor_id);
					CMR_LOGI("identify failed!");
					goto IDENTIFY_SEARCH;
				}
			}
			Sensor_PowerOn(sensor_cxt, SCI_FALSE);
			sns_i2c_deinit(sensor_cxt, sensor_id);
			return retValue;
		}
	}

IDENTIFY_SEARCH:
	if ( sensor_cxt->sensor_identified==SCI_FALSE && ((strlen(CAMERA_SENSOR_TYPE_BACK)) || (strlen(CAMERA_SENSOR_TYPE_FRONT)) ||
		(strlen(AT_CAMERA_SENSOR_TYPE_BACK)) || (strlen(AT_CAMERA_SENSOR_TYPE_FRONT))
		|| (strlen(CAMERA_SENSOR_TYPE_DEV_2)) || (strlen(AT_CAMERA_SENSOR_TYPE_DEV_2))))
	{
		retValue = sns_identify_strsearch(sensor_cxt, sensor_id);
		if ( retValue == SCI_TRUE) {
			return retValue;
		}
	}
	retValue = sns_identify_search(sensor_cxt, sensor_id);
	return retValue;
}

void sns_set_status(struct sensor_drv_context *sensor_cxt, SENSOR_ID_E sensor_id)
{
	cmr_u32 i = 0;
	cmr_u32 rst_lvl = 0;
	SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

	SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
	sensor_register_info_ptr = &sensor_cxt->sensor_register_info;

	/*pwdn all the sensor to avoid confilct as the sensor output*/
	CMR_LOGV("1");
	for (i=0; i<=SENSOR_DEV_2; i++) {
		if (i == sensor_id) {
			continue;
		}
		if(SENSOR_TRUE == sensor_register_info_ptr->is_register[i]) {
			sns_set_id(sensor_cxt, i);
			sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[i];
			if (SENSOR_ATV != snr_get_cur_id(sensor_cxt)) {
				sensor_cxt->i2c_addr = (sensor_cxt->sensor_info_ptr->salve_i2c_addr_w & 0xFF);
				sns_dev_set_i2c_addr(sensor_cxt);
			}

			/*when use the camera vendor functions, the sensor_cxt should be set at first */
			sensor_set_cxt_common(sensor_cxt);

			Sensor_PowerDown((cmr_u32)sensor_cxt->sensor_info_ptr->power_down_level);
			CMR_LOGI("Sensor_sleep of id %d",i);
		}
	}

	/*Give votage according the target sensor*/
	/*For dual sensor solution, the dual sensor should share all the power*/
	CMR_LOGV("1_1");

	Sensor_PowerOn_Ex(sensor_cxt, sensor_id);

	CMR_LOGI("2");

	sns_set_id(sensor_cxt, sensor_id);
	sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[sensor_id];
	CMR_LOGI("3");
	//reset target sensor. and make normal.
	Sensor_SetExportInfo(sensor_cxt);
	CMR_LOGI("4");
}

cmr_int sns_device_init(struct sensor_drv_context *sensor_cxt, cmr_u32 sensor_id)
{
	cmr_int                    ret = SENSOR_SUCCESS;
	cmr_s8                     sensor_dev_name[50] = CMR_SENSOR_DEV_NAME;

	CMR_LOGV("To open sensor device.");

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if (SENSOR_FD_INIT == sensor_cxt->fd_sensor) {
		sensor_cxt->fd_sensor = open(CMR_SENSOR_DEV_NAME, O_RDWR, 0);
		CMR_LOGV("fd 0x%lx", sensor_cxt->fd_sensor);
		if (SENSOR_FD_INIT == sensor_cxt->fd_sensor) {
			CMR_LOGE("Failed to open sensor device.errno : %d", errno);
			fprintf(stderr, "Cannot open '%s': %d, %s", sensor_dev_name, errno,  strerror(errno));
		} else {
			CMR_LOGV("OK to open device.");
			/*the sensor id should be set after open OK*/
			ret = ioctl(sensor_cxt->fd_sensor, SENSOR_IO_SET_ID, &sensor_id);
			if (0 != ret) {
				CMR_LOGE("SENSOR_IO_SET_ID %u failed %ld", sensor_id, ret);
				ret = SENSOR_FAIL;
			}
		}
	}

	return ret;
}

cmr_int sns_device_deinit(struct sensor_drv_context *sensor_cxt)
{
	cmr_int ret;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if (-1 != sensor_cxt->fd_sensor) {
		ret = close(sensor_cxt->fd_sensor);
		sensor_cxt->fd_sensor = SENSOR_FD_INIT;
		CMR_LOGI("is done, ret = %ld ", ret);
	}
	return 0;
}

cmr_int sns_register(struct sensor_drv_context *sensor_cxt, SENSOR_ID_E sensor_id)
{
	cmr_u32 sensor_index = 0;
	SENSOR_MATCH_T *sensor_strinfo_tab_ptr = PNULL;
	cmr_u32 valid_tab_index_max = 0x00;
	SENSOR_INFO_T* sensor_info_ptr = PNULL;
	SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	sensor_register_info_ptr = &sensor_cxt->sensor_register_info;

	CMR_LOGI("id %d ", sensor_id);
	//if already identified
	if (SCI_TRUE == sensor_register_info_ptr->is_register[sensor_id]) {
		CMR_LOGI("identified ");
		return SENSOR_SUCCESS;
	}
	if (sensor_cxt->sensor_identified && (SENSOR_ATV != sensor_id)) {
		sensor_index = sensor_cxt->sensor_index[sensor_id];
		CMR_LOGI("sensor_index = %d.", sensor_index);
		if (0xFF != sensor_index) {
			valid_tab_index_max = Sensor_GetInforTabLenght(sensor_cxt, sensor_id) - SENSOR_ONE_I2C;
			if (sensor_index>=valid_tab_index_max) {
				CMR_LOGE("saved index is larger than sensor sum.");
				return SENSOR_FAIL;
			}

			sensor_strinfo_tab_ptr = (SENSOR_MATCH_T*)Sensor_GetInforTab(sensor_cxt, sensor_id);
			sensor_info_ptr = sensor_strinfo_tab_ptr[sensor_index].sensor_info;
			if (NULL == sensor_info_ptr) {
				CMR_LOGE("index %d info of Sensor table %d is null", sensor_index, sensor_id);
				return SENSOR_FAIL;
			}
			sensor_cxt->sensor_info_ptr = sensor_info_ptr;
			sensor_cxt->sensor_list_ptr[sensor_id] = sensor_info_ptr;
			sensor_register_info_ptr->is_register[sensor_id] = SCI_TRUE;
			sensor_register_info_ptr->img_sensor_num++;
		}
	}

	return SENSOR_SUCCESS;

}

void sns_load_sensor_type(struct sensor_drv_context *sensor_cxt)
{
	FILE 		*fp;
	cmr_u8 	    sensor_param[SENSOR_PARAM_NUM];
	cmr_u32 	len = 0;

	cmr_bzero(&sensor_param[0], SENSOR_PARAM_NUM);

	fp = fopen(SENSOR_PARA, "rb+");
	if(NULL == fp){
		fp = fopen(SENSOR_PARA, "wb+");
		if(NULL == fp){
			CMR_LOGE("sns_load_sensor_type: file %s open error:%s", SENSOR_PARA, strerror(errno));
		}
		cmr_bzero(&sensor_param[0], SENSOR_PARAM_NUM);
	}else{
		len = fread(sensor_param, 1, SENSOR_PARAM_NUM, fp);
		CMR_LOGI("rd sns param len %d %x,%x,%x,%x,%x,%x,%x,%x ",
			len, sensor_param[0], sensor_param[1], sensor_param[2], sensor_param[3],
			sensor_param[4], sensor_param[5], sensor_param[6], sensor_param[7]);
	}

	if(NULL != fp)
		fclose(fp);

	snr_set_mark(sensor_cxt, sensor_param);
}

void sns_save_sensor_type(struct sensor_drv_context *sensor_cxt)
{
	FILE                 *fp;
	cmr_u8               is_saved = 0;
	cmr_u8               sensor_param[SENSOR_PARAM_NUM];

	cmr_bzero(&sensor_param[0], SENSOR_PARAM_NUM);
	snr_get_mark(sensor_cxt, sensor_param, &is_saved);

	if (is_saved) {
		fp = fopen(SENSOR_PARA,"wb+");
		if(NULL == fp){
			CMR_LOGI("file %s open error:%s ", SENSOR_PARA, strerror(errno));
		} else {
			fwrite(sensor_param, 1, SENSOR_PARAM_NUM, fp);
			fclose(fp);
		}
	}
}

cmr_int Sensor_set_calibration(cmr_u32 value)
{
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	sensor_cxt->is_calibration = value;
	return SENSOR_SUCCESS;
}

void _sensor_calil_lnc_param_recover(struct sensor_drv_context *sensor_cxt,
						SENSOR_INFO_T *sensor_info_ptr)
{
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4))
	cmr_u32 i = 0;
	cmr_u32 index = 0;
	cmr_u32 length = 0;
	cmr_uint addr = 0;
	struct sensor_raw_fix_info *raw_fix_info_ptr = PNULL;
	struct sensor_raw_info* raw_info_ptr = PNULL;

	raw_info_ptr = (struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr));

	SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);

	if (PNULL != raw_info_ptr) {
		raw_fix_info_ptr = raw_info_ptr->fix_ptr;
		if (PNULL != raw_fix_info_ptr) {
			for (i = 0; i < 8; i++) {
				if (sensor_cxt->lnc_addr_bakup[i][1]) {

					free((void*)sensor_cxt->lnc_addr_bakup[i][1]);
					sensor_cxt->lnc_addr_bakup[i][1] = 0;
					index = sensor_cxt->lnc_addr_bakup[i][0];     /*index*/
					length = sensor_cxt->lnc_addr_bakup[i][3];    /*length*/
					addr = sensor_cxt->lnc_addr_bakup[i][2];      /*original address*/

					raw_fix_info_ptr->lnc.map[index][0].param_addr = (cmr_u16*)addr;
					raw_fix_info_ptr->lnc.map[index][0].len = length;
					sensor_cxt->lnc_addr_bakup[i][0] = 0;
					sensor_cxt->lnc_addr_bakup[i][1] = 0;
					sensor_cxt->lnc_addr_bakup[i][2] = 0;
					sensor_cxt->lnc_addr_bakup[i][3] = 0;
				}
			}
		} else {
			for (i = 0; i < 8; i++) {
				if (sensor_cxt->lnc_addr_bakup[i][1]) {
					free((void*)sensor_cxt->lnc_addr_bakup[i][1]);
				}
			}
			cmr_bzero((void*)&sensor_cxt->lnc_addr_bakup[0][0], sizeof(sensor_cxt->lnc_addr_bakup));
		}
	}else {
		for (i = 0; i < 8; i++) {
			if (sensor_cxt->lnc_addr_bakup[i][1]) {
				free((void*)sensor_cxt->lnc_addr_bakup[i][1]);
			}
		}
		cmr_bzero((void*)&sensor_cxt->lnc_addr_bakup[0][0], sizeof(sensor_cxt->lnc_addr_bakup));
	}

	sensor_cxt->is_calibration = 0;
	CMR_LOGI("test: is_calibration: %d", sensor_cxt->is_calibration);
#endif
}

cmr_int _sensor_cali_lnc_param_update(struct sensor_drv_context *sensor_cxt,
						cmr_s8 *cfg_file_dir,
						SENSOR_INFO_T *sensor_info_ptr,
						SENSOR_ID_E sensor_id)
{
	UNUSED(sensor_id);
	cmr_u32 rtn = SENSOR_SUCCESS;
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4))
	const cmr_s8 *sensor_name = sensor_info_ptr->name;
	FILE *fp = PNULL;
	cmr_s8 file_name[80] = {0};
	cmr_s8* file_name_ptr = 0;
	cmr_u32 str_len = 0;
	cmr_int file_pos = 0;
	cmr_u32 file_size = 0;
	cmr_s8 *data_ptr;
	cmr_s32 i,j;
	cmr_u16 *temp_buf_16 = PNULL;
	cmr_u32 width;
	cmr_u32 height;
	cmr_u32 index = 0;
	SENSOR_TRIM_T *trim_ptr = 0;
	struct sensor_raw_fix_info *raw_fix_info_ptr = PNULL;

	if(SENSOR_IMAGE_FORMAT_RAW != sensor_info_ptr->image_format){
		rtn = SENSOR_FAIL;
		goto cali_lnc_param_update_exit;
	}

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if (PNULL == sensor_cxt->sensor_info_ptr) {
		CMR_LOGE("No sensor info!");
		return -1;
	}

	str_len = sprintf(file_name, "%ssensor_%s",cfg_file_dir, sensor_name);
	file_name_ptr = (cmr_s8*)&file_name[0] + str_len;

	/*LNC DATA Table*/
	temp_buf_16 = (cmr_u16*)malloc(128*1024*2);
	if(!temp_buf_16){
		rtn = SENSOR_FAIL;
		goto cali_lnc_param_update_exit;
	}
	/*the get trim function need not set the sensor_cxt to work(mainly fd)*/
	trim_ptr = (SENSOR_TRIM_T *)(sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->get_trim(0));
	raw_fix_info_ptr = ((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->fix_ptr;
	i = 1;
	while(1) {
		height = trim_ptr[i].trim_height;
		width = trim_ptr[i].trim_width;
		if ((0 == height) || (0 == width)) {
			break;
		}

		sprintf(file_name_ptr, "_lnc_%d_%d_%d_rdm.dat", width, height, (i - 1));

		fp = fopen(file_name, "rb");
		if (0 == fp) {
			CMR_LOGI("does not find calibration file");
			i++;
			continue;
		}

		fseek(fp, 0L, SEEK_END);
		file_pos = ftell(fp);
		if (file_pos >= 0) {
			file_size = (cmr_u32)file_pos;
		} else {
			fclose(fp);
			free(temp_buf_16);
			temp_buf_16 = NULL;
			CMR_LOGI("file pointers error!");
			rtn = SENSOR_FAIL;
			goto cali_lnc_param_update_exit;
		}
		fseek(fp, 0L, SEEK_SET);

		fread(temp_buf_16,1,file_size,fp);
		fclose(fp);

		if (file_size != raw_fix_info_ptr->lnc.map[i-1][0].len) {
			CMR_LOGI("file size dis-match, do not replace, w:%d, h:%d, ori: %d, now:%d/n",
				width, height, raw_fix_info_ptr->lnc.map[i-1][0].len, file_size);
		} else {
			if (sensor_cxt->lnc_addr_bakup[index][1]) {
				free((void*)sensor_cxt->lnc_addr_bakup[index][1]);
				sensor_cxt->lnc_addr_bakup[index][1] = 0;
			}
			sensor_cxt->lnc_addr_bakup[index][1] = (cmr_uint)malloc(file_size);
			if (0 == sensor_cxt->lnc_addr_bakup[index][1]) {
				rtn = SENSOR_FAIL;
				CMR_LOGI("malloc failed i = %d", i);
				goto cali_lnc_param_update_exit;
			}
			cmr_bzero((void*)sensor_cxt->lnc_addr_bakup[index][1], file_size);

			sensor_cxt->lnc_addr_bakup[index][0] = i -1;
			sensor_cxt->lnc_addr_bakup[index][2] = (cmr_uint)raw_fix_info_ptr->lnc.map[i-1][0].param_addr;	/*save the original address*/
			sensor_cxt->lnc_addr_bakup[index][3] = file_size;
			data_ptr = (cmr_s8*)sensor_cxt->lnc_addr_bakup[index][1];
			raw_fix_info_ptr->lnc.map[i-1][0].param_addr = (cmr_u16*)data_ptr;
			cmr_copy(data_ptr, temp_buf_16, file_size);
			index++;
			CMR_LOGI("replace finished");
		}
		i++;
	}

	if (temp_buf_16) {
		free((void*)temp_buf_16);
		temp_buf_16 = 0;
	}
	return rtn;

cali_lnc_param_update_exit:

	if (temp_buf_16) {
		free((void*)temp_buf_16);
		temp_buf_16 = 0;
	}

	_sensor_calil_lnc_param_recover(sensor_cxt, sensor_info_ptr);
#endif
	return rtn;
}

cmr_int _sensor_cali_awb_param_update(cmr_s8 *cfg_file_dir,
						SENSOR_INFO_T *sensor_info_ptr,
						SENSOR_ID_E sensor_id)
{
	UNUSED(sensor_id);

	cmr_int rtn = 0;
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4))
	const cmr_s8 *sensor_name = sensor_info_ptr->name;
	FILE    *fp = PNULL;
	cmr_s8  file_name[80] = {0};
	cmr_s8  buf[256] = {0x00};
	cmr_s8  *file_name_ptr = 0;
	cmr_u32 str_len = 0;
	cmr_int file_pos = 0;
	cmr_u32 file_size = 0;
	struct isp_bayer_ptn_stat_t *stat_ptr = PNULL;
	struct sensor_cali_info *cali_info_ptr = PNULL;
	struct sensor_raw_tune_info *raw_tune_info_ptr = PNULL;

	if (SENSOR_IMAGE_FORMAT_RAW != sensor_info_ptr->image_format) {
		return SENSOR_FAIL;
	}
	raw_tune_info_ptr = (struct sensor_raw_tune_info*)(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->tune_ptr);
	cali_info_ptr = (struct sensor_cali_info*)&(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->cali_ptr->awb.cali_info);

	str_len = sprintf(file_name, "%ssensor_%s",cfg_file_dir, sensor_name);
	file_name_ptr = (cmr_s8*)&file_name[0] + str_len;

	sprintf(file_name_ptr, "_awb_rdm.dat");

	CMR_LOGI("file_name: %s", file_name);
	fp = fopen(file_name, "rb");
	if (0 == fp) {
		CMR_LOGI("does not find calibration file");

		cali_info_ptr->r_sum = 1024;
		cali_info_ptr->b_sum = 1024;
		cali_info_ptr->gr_sum = 1024;
		cali_info_ptr->gb_sum = 1024;

		cali_info_ptr = (struct sensor_cali_info*)&(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->cali_ptr->awb.golden_cali_info);
		cali_info_ptr->r_sum = 1024;
		cali_info_ptr->b_sum = 1024;
		cali_info_ptr->gr_sum = 1024;
		cali_info_ptr->gb_sum = 1024;
		rtn = SENSOR_SUCCESS;
		return rtn;
	} else {
		fseek(fp, 0L, SEEK_END);
		file_pos = ftell(fp);
		if (file_pos >= 0) {
			file_size = (cmr_u32)file_pos;
		} else {
			fclose(fp);
			CMR_LOGI("file pointers error!");
			return SENSOR_FAIL;
		}

		fseek(fp, 0L, SEEK_SET);
		fread(buf,1,file_size,fp);
		fclose(fp);

		stat_ptr = (struct isp_bayer_ptn_stat_t*)&buf[0];
		cali_info_ptr->r_sum = stat_ptr->r_stat;
		cali_info_ptr->b_sum = stat_ptr->b_stat;
		cali_info_ptr->gr_sum = stat_ptr->gr_stat;
		cali_info_ptr->gb_sum = stat_ptr->gb_stat;

		rtn = SENSOR_SUCCESS;
	}

	cmr_bzero(&file_name[0], sizeof(file_name));
	cmr_bzero(&buf[0], sizeof(buf));
	str_len = sprintf(file_name, "%ssensor_%s",cfg_file_dir, sensor_name);
	file_name_ptr = (cmr_s8*)&file_name[0] + str_len;

	sprintf(file_name_ptr, "_awb_gldn.dat");

	CMR_LOGI("file_name: %s", file_name);
	cali_info_ptr = (struct sensor_cali_info*)&(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->cali_ptr->awb.golden_cali_info);
	fp = fopen(file_name, "rb");
	if (0 == fp) {
		CMR_LOGI("does not find calibration file");

		cali_info_ptr->r_sum = 1024;
		cali_info_ptr->b_sum = 1024;
		cali_info_ptr->gr_sum = 1024;
		cali_info_ptr->gb_sum = 1024;

		cali_info_ptr = (struct sensor_cali_info*)&(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->cali_ptr->awb.cali_info);
		cali_info_ptr->r_sum = 1024;
		cali_info_ptr->b_sum = 1024;
		cali_info_ptr->gr_sum = 1024;
		cali_info_ptr->gb_sum = 1024;

		rtn = SENSOR_SUCCESS;
		return rtn;
	} else {
		fseek(fp, 0L, SEEK_END);
		file_pos = ftell(fp);
		if (file_pos >= 0) {
			file_size = (cmr_u32)file_pos;
		} else {
			fclose(fp);
			CMR_LOGI("file pointers error!");
			return SENSOR_FAIL;
		}
		fseek(fp, 0L, SEEK_SET);

		fread(buf,1,file_size,fp);
		fclose(fp);

		stat_ptr = (struct isp_bayer_ptn_stat_t*)&buf[0];
		cali_info_ptr->r_sum = stat_ptr->r_stat;
		cali_info_ptr->b_sum = stat_ptr->b_stat;
		cali_info_ptr->gr_sum = stat_ptr->gr_stat;
		cali_info_ptr->gb_sum = stat_ptr->gb_stat;

		rtn = SENSOR_SUCCESS;
	}
#endif
	return rtn;
}

cmr_int _sensor_cali_flashlight_param_update(cmr_s8 *cfg_file_dir,
							SENSOR_INFO_T *sensor_info_ptr,
							SENSOR_ID_E sensor_id)
{
	UNUSED(sensor_id);

	cmr_int rtn = 0;
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4))
	const cmr_s8 *sensor_name = sensor_info_ptr->name;
	FILE   *fp = PNULL;
	cmr_s8 file_name[80] = {0};
	cmr_s8 buf[256] = {0x00};
	cmr_s8* file_name_ptr = 0;
	cmr_u32 str_len = 0;
	cmr_int file_pos = 0;
	cmr_u32 file_size = 0;
	struct isp_bayer_ptn_stat_t *stat_ptr = PNULL;
	struct sensor_cali_info *cali_info_ptr = PNULL;
	struct sensor_raw_tune_info *raw_tune_info_ptr = PNULL;

	if(SENSOR_IMAGE_FORMAT_RAW != sensor_info_ptr->image_format){
		return SENSOR_FAIL;
	}
	raw_tune_info_ptr = (struct sensor_raw_tune_info*)(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->tune_ptr);
	cali_info_ptr = (struct sensor_cali_info*)&(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->cali_ptr->flashlight.cali_info);

	str_len = sprintf(file_name, "%ssensor_%s",cfg_file_dir, sensor_name);
	file_name_ptr = (cmr_s8*)&file_name[0] + str_len;

	sprintf(file_name_ptr, "_flashlight_rdm.dat");

	CMR_LOGI("file_name: %s", file_name);
	fp = fopen(file_name, "rb");
	if (0 == fp) {
		CMR_LOGI("does not find calibration file");

		cali_info_ptr->r_sum = 1024;
		cali_info_ptr->b_sum = 1024;
		cali_info_ptr->gr_sum = 1024;
		cali_info_ptr->gb_sum = 1024;

		cali_info_ptr = (struct sensor_cali_info*)&(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->cali_ptr->flashlight.golden_cali_info);
		cali_info_ptr->r_sum = 1024;
		cali_info_ptr->b_sum = 1024;
		cali_info_ptr->gr_sum = 1024;
		cali_info_ptr->gb_sum = 1024;

		rtn = SENSOR_SUCCESS;
		return rtn;
	} else {
		fseek(fp, 0L, SEEK_END);
		file_pos = ftell(fp);
		if (file_pos >= 0) {
			file_size = (cmr_u32)file_pos;
		} else {
			fclose(fp);
			CMR_LOGI("file pointers error!");
			return SENSOR_FAIL;
		}
		fseek(fp, 0L, SEEK_SET);

		fread(buf,1,file_size,fp);
		fclose(fp);

		stat_ptr = (struct isp_bayer_ptn_stat_t*)&buf[0];
		cali_info_ptr->r_sum = stat_ptr->r_stat;
		cali_info_ptr->b_sum = stat_ptr->b_stat;
		cali_info_ptr->gr_sum = stat_ptr->gr_stat;
		cali_info_ptr->gb_sum = stat_ptr->gb_stat;

		rtn = SENSOR_SUCCESS;
	}

	cmr_bzero(&file_name[0], sizeof(file_name));
	cmr_bzero(&buf[0], sizeof(buf));
	str_len = sprintf(file_name, "%ssensor_%s",cfg_file_dir, sensor_name);
	file_name_ptr = (cmr_s8*)&file_name[0] + str_len;

	sprintf(file_name_ptr, "_flashlight_gldn.dat");

	CMR_LOGI("file_name: %s", file_name);
	cali_info_ptr = (struct sensor_cali_info*)&(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->cali_ptr->flashlight.golden_cali_info);
	fp = fopen(file_name, "rb");
	if (0 == fp) {
		CMR_LOGI("does not find calibration file");

		cali_info_ptr->r_sum = 1024;
		cali_info_ptr->b_sum = 1024;
		cali_info_ptr->gr_sum = 1024;
		cali_info_ptr->gb_sum = 1024;

		cali_info_ptr = (struct sensor_cali_info*)&(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->cali_ptr->flashlight.cali_info);
		cali_info_ptr->r_sum = 1024;
		cali_info_ptr->b_sum = 1024;
		cali_info_ptr->gr_sum = 1024;
		cali_info_ptr->gb_sum = 1024;

		rtn = SENSOR_SUCCESS;
		return rtn;
	} else {
		fseek(fp, 0L, SEEK_END);

		file_pos = ftell(fp);
		if (file_pos >= 0) {
			file_size = (cmr_u32)file_pos;
		} else {
			fclose(fp);
			CMR_LOGI("file pointers error!");
			return SENSOR_FAIL;
		}
		fseek(fp, 0L, SEEK_SET);

		fread(buf,1,file_size,fp);
		fclose(fp);

		stat_ptr = (struct isp_bayer_ptn_stat_t*)&buf[0];
		cali_info_ptr->r_sum = stat_ptr->r_stat;
		cali_info_ptr->b_sum = stat_ptr->b_stat;
		cali_info_ptr->gr_sum = stat_ptr->gr_stat;
		cali_info_ptr->gb_sum = stat_ptr->gb_stat;

		rtn = SENSOR_SUCCESS;
	}
#endif
	return rtn;
}

cmr_int  _sensor_cali_load_param(struct sensor_drv_context *sensor_cxt,
					cmr_s8 *cfg_file_dir,
					SENSOR_INFO_T *sensor_info_ptr,
					SENSOR_ID_E sensor_id)
{
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4))
	cmr_int rtn = 0;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if (1 != sensor_cxt->is_calibration) {/*for normal*/

		rtn = _sensor_cali_lnc_param_update(sensor_cxt, cfg_file_dir,sensor_info_ptr, sensor_id);
		if (rtn) {

			return SENSOR_FAIL;
		}
		rtn = _sensor_cali_flashlight_param_update(cfg_file_dir,sensor_info_ptr, sensor_id);
		if (rtn) {
			return SENSOR_FAIL;
		}
		rtn = _sensor_cali_awb_param_update(cfg_file_dir,sensor_info_ptr, sensor_id);
		if (rtn) {
			return SENSOR_FAIL;
		}
	} else {/*for calibration*/
		struct sensor_cali_info *cali_info_ptr = PNULL;

		/*for awb calibration*/
		cali_info_ptr = (struct sensor_cali_info*)&(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->cali_ptr->awb.cali_info);
		cali_info_ptr->r_sum = 1024;
		cali_info_ptr->b_sum = 1024;
		cali_info_ptr->gr_sum = 1024;
		cali_info_ptr->gb_sum = 1024;

		cali_info_ptr = (struct sensor_cali_info*)&(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->cali_ptr->awb.golden_cali_info);
		cali_info_ptr->r_sum = 1024;
		cali_info_ptr->b_sum = 1024;
		cali_info_ptr->gr_sum = 1024;
		cali_info_ptr->gb_sum = 1024;

		/*for flash  calibration*/
		cali_info_ptr = (struct sensor_cali_info*)&(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->cali_ptr->flashlight.cali_info);
		cali_info_ptr->r_sum = 1024;
		cali_info_ptr->b_sum = 1024;
		cali_info_ptr->gr_sum = 1024;
		cali_info_ptr->gb_sum = 1024;

		cali_info_ptr = (struct sensor_cali_info*)&(((struct sensor_raw_info*)(*(sensor_info_ptr->raw_info_ptr)))->cali_ptr->flashlight.golden_cali_info);
		cali_info_ptr->r_sum = 1024;
		cali_info_ptr->b_sum = 1024;
		cali_info_ptr->gr_sum = 1024;
		cali_info_ptr->gb_sum = 1024;
	}
#endif
	return SENSOR_SUCCESS;
}


cmr_int sns_create_ctrl_thread(struct sensor_drv_context *sensor_cxt)
{
	cmr_int                  ret = CMR_CAMERA_SUCCESS;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	CMR_LOGI("is_inited %ld", sensor_cxt->ctrl_thread_cxt.is_inited);

	if (!sensor_cxt->ctrl_thread_cxt.is_inited) {
		pthread_mutex_init(&sensor_cxt->ctrl_thread_cxt.sensor_mutex, NULL);
		sem_init(&sensor_cxt->ctrl_thread_cxt.sensor_sync_sem, 0, 0);

		ret = cmr_thread_create(&sensor_cxt->ctrl_thread_cxt.thread_handle,
					SENSOR_CTRL_MSG_QUEUE_SIZE,
					sns_ctrl_thread_proc,
					(void *)sensor_cxt);
		if (ret) {
			CMR_LOGE("send msg failed!");
			ret = CMR_CAMERA_FAIL;
			goto end;
		}

		sensor_cxt->ctrl_thread_cxt.is_inited = 1;

	}

end:
	if (ret) {
		sem_destroy(&sensor_cxt->ctrl_thread_cxt.sensor_sync_sem);
		pthread_mutex_destroy(&sensor_cxt->ctrl_thread_cxt.sensor_mutex);
		sensor_cxt->ctrl_thread_cxt.is_inited = 0;
	}

	CMR_LOGI("ret %ld", ret);
	return ret;
}

cmr_int sns_ctrl_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int                   ret = CMR_CAMERA_SUCCESS;
	cmr_u32                   evt = 0, on_off = 0;
	cmr_u32                   camera_id = CAMERA_ID_MAX;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)p_data;

	if (!message || !p_data) {
		return CMR_CAMERA_INVALID_PARAM;
	}

	evt = (cmr_u32)message->msg_type;
	CMR_LOGI("evt %d", evt);

	switch(evt) {
	case SENSOR_CTRL_EVT_INIT:
		/*common control info config*/
		CMR_LOGI("INIT DONE!");
		break;

	case SENSOR_CTRL_EVT_SETMODE:
		{
		cmr_u32 mode = (cmr_u32)message->sub_msg_type;
		cmr_u32 is_inited = (cmr_u32)((cmr_uint)message->data);
		usleep(100);
		sns_set_mode(sensor_cxt, mode, is_inited);
		}
		break;

	case SENSOR_CTRL_EVT_SETMODONE:
		CMR_LOGI("SENSOR_CTRL_EVT_SETMODONE OK");
		break;

	case SENSOR_CTRL_EVT_CFGOTP:
		camera_id = (cmr_u32)message->sub_msg_type;
		sns_cfg_otp_update_isparam(sensor_cxt, camera_id);
		break;
	case SENSOR_CTRL_EVT_STREAM_CTRL:
		on_off = (cmr_u32)message->sub_msg_type;
		sns_stream_ctrl_common(sensor_cxt, on_off);
		break;

	case SENSOR_CTRL_EVT_EXIT:
		/*common control info clear*/
		CMR_LOGI("EXIT DONE!");
		break;

	default:
		CMR_LOGE("jpeg:not correct message");
		break;
	}

	return ret;
}

cmr_int sns_destroy_ctrl_thread(struct sensor_drv_context *sensor_cxt)
{
	CMR_MSG_INIT(message);
	cmr_int                  ret = CMR_CAMERA_SUCCESS;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	CMR_LOGI("is_inited %ld", sensor_cxt->ctrl_thread_cxt.is_inited);

	if (sensor_cxt->ctrl_thread_cxt.is_inited) {
		message.msg_type  = SENSOR_CTRL_EVT_EXIT;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle, &message);
		if (ret) {
			CMR_LOGE("send msg failed!");
		}

		ret = cmr_thread_destroy(sensor_cxt->ctrl_thread_cxt.thread_handle);
		sensor_cxt->ctrl_thread_cxt.thread_handle = 0;

		sem_destroy(&sensor_cxt->ctrl_thread_cxt.sensor_sync_sem);
		pthread_mutex_destroy(&sensor_cxt->ctrl_thread_cxt.sensor_mutex);
		sensor_cxt->ctrl_thread_cxt.is_inited = 0;
	}

	return ret ;
}


/*********************************************************************************
 todo:
 now the sensor_open_common only support open one sensor on 1 times, and the function
 should be updated when the hardware can supported multiple sensors;
 *********************************************************************************/
 cmr_int sensor_open_common(struct sensor_drv_context *sensor_cxt,
			cmr_u32 sensor_id, cmr_uint is_autotest)
{
	cmr_int ret_val = SENSOR_FAIL;
	cmr_u32 sensor_num = 0;

	CMR_LOGI("0, start,id %d autotest %ld",sensor_id, is_autotest);
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	if (NULL != sensor_cxt) {
		if (SENSOR_TRUE == sensor_is_init_common(sensor_cxt)) {
			CMR_LOGI("sensor close.");
			sensor_close_common(sensor_cxt, SENSOR_MAIN);
		}
	}

	cmr_bzero((void*)sensor_cxt, sizeof(struct sensor_drv_context));
	sensor_cxt->fd_sensor = SENSOR_FD_INIT;
	sensor_cxt->i2c_addr = 0xff;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	{
		sns_clean_info(sensor_cxt);
		sns_init_defaul_exif(sensor_cxt);
		sns_load_sensor_type(sensor_cxt);
		sensor_cxt->is_autotest = is_autotest;
		if (sns_create_ctrl_thread(sensor_cxt)) {
			CMR_LOGE("Failed to create sensor ctrl thread");
			goto init_exit;;
		}
		/*now device opened set with ID*/
		if (sns_device_init(sensor_cxt, sensor_id)) {
			CMR_LOGE("sns_device_init %d error, return", sensor_id);
			ret_val = SENSOR_FAIL;
			goto init_exit;
		}
		if (SCI_TRUE == sensor_cxt->sensor_identified) {
			if (SENSOR_SUCCESS == sns_register(sensor_cxt, SENSOR_MAIN)) {
				sensor_num++;
			}
#ifndef CONFIG_DCAM_SENSOR_NO_FRONT_SUPPORT
			if (SENSOR_SUCCESS == sns_register(sensor_cxt, SENSOR_SUB)) {
				sensor_num++;
			}
#endif

#ifdef CONFIG_DCAM_SENSOR_DEV_2_SUPPORT
			if (SENSOR_SUCCESS == sns_register(sensor_cxt, SENSOR_DEV_2)) {
				sensor_num++;
			}
#endif
			CMR_LOGI("1 is identify, register OK");
			ret_val = sns_open(sensor_cxt, sensor_id);
			if (ret_val != SENSOR_SUCCESS ) {
				Sensor_PowerOn(sensor_cxt, SENSOR_FALSE);
			}
		}

		if (ret_val != SENSOR_SUCCESS) {
			sensor_num = 0;
			CMR_LOGI("register sensor fail, start identify");
			if (sns_identify(sensor_cxt, SENSOR_MAIN))
				sensor_num++;
#ifndef CONFIG_DCAM_SENSOR_NO_FRONT_SUPPORT
			if (sns_identify(sensor_cxt, SENSOR_SUB))
				sensor_num++;
#endif

#ifdef CONFIG_DCAM_SENSOR_DEV_2_SUPPORT
			if (sns_identify(sensor_cxt, SENSOR_DEV_2))
				sensor_num++;
#endif

			ret_val = sns_open(sensor_cxt, sensor_id);
		}
		sensor_cxt->sensor_identified = SCI_TRUE;
	}

	sns_save_sensor_type(sensor_cxt);
	CMR_LOGI("1 debug %p", sensor_cxt->sensor_info_ptr);    //for debug
//	CMR_LOGI("2 debug %d", sensor_cxt->sensor_info_ptr->image_format);    //for debug
	if (sensor_cxt->sensor_info_ptr &&
		SENSOR_IMAGE_FORMAT_RAW == sensor_cxt->sensor_info_ptr->image_format) {
		if (SENSOR_SUCCESS == ret_val) {
			ret_val = _sensor_cali_load_param(sensor_cxt,
				CALI_FILE_DIR,
				sensor_cxt->sensor_info_ptr,
				sensor_id);
			if (ret_val) {
				CMR_LOGI("load cali data failed!! rtn:%ld",ret_val);
				goto init_exit;
			}
		}
	}

	CMR_LOGI("total camera number %d", sensor_num);

init_exit:
	if (SENSOR_SUCCESS != ret_val) {
		sns_destroy_ctrl_thread(sensor_cxt);
		sns_device_deinit(sensor_cxt);
	}

	CMR_LOGI("2 init OK!");
	return ret_val;
}

cmr_int sensor_is_init_common(struct sensor_drv_context *sensor_cxt)
{
	return sensor_cxt->sensor_isInit;
}

cmr_int sns_open(struct sensor_drv_context *sensor_cxt, cmr_u32 sensor_id)
{
	cmr_u32 ret_val = SENSOR_FAIL;
	cmr_u32 is_inited = 0;
	SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	sensor_register_info_ptr = &sensor_cxt->sensor_register_info;

	if (1 == sensor_cxt->is_autotest) {
		is_inited = 1;
	}
	if (SENSOR_TRUE == sensor_register_info_ptr->is_register[sensor_id]) {
		CMR_LOGI("1, sensor register ok");
		sns_set_status(sensor_cxt,sensor_id);
		CMR_LOGI("2, sensor set status");
		sensor_cxt->sensor_isInit = SENSOR_TRUE;

		if (SENSOR_ATV != snr_get_cur_id(sensor_cxt)) {
			sensor_cxt->i2c_addr = (sensor_cxt->sensor_info_ptr->salve_i2c_addr_w & 0xFF);
			sns_dev_set_i2c_addr(sensor_cxt);
		}

		CMR_LOGI("3:sensor_id: %d, addr = 0x%x", sensor_id, sensor_cxt->i2c_addr);
		sns_set_i2c_clk(sensor_cxt);

		/*when use the camera vendor functions, the sensor_cxt should be set at first */
		sensor_set_cxt_common(sensor_cxt);

		//confirm camera identify OK
		if (SENSOR_SUCCESS != sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->identify(SENSOR_ZERO_I2C)) {
			sensor_register_info_ptr->is_register[sensor_id] = SENSOR_FALSE;
			sns_i2c_deinit(sensor_cxt, sensor_id);
			CMR_LOGI("sensor identify not correct!!");
			return SENSOR_FAIL;
		}


		Sensor_SetExportInfo(sensor_cxt);

		ret_val = SENSOR_SUCCESS;
		if (SENSOR_SUCCESS != sensor_set_mode(sensor_cxt, SENSOR_MODE_COMMON_INIT, is_inited)) {
			CMR_LOGE("Sensor set init mode error!");
			sns_i2c_deinit(sensor_cxt, sensor_id);
			ret_val = SENSOR_FAIL;
		}
		if(SENSOR_IMAGE_FORMAT_RAW == sensor_cxt->sensor_info_ptr->image_format){
			sensor_cfg_otp_update_isparam(sensor_cxt, sensor_id);
		}
		if (SENSOR_SUCCESS == ret_val) {
			if (SENSOR_SUCCESS != sensor_set_mode(sensor_cxt, SENSOR_MODE_PREVIEW_ONE, is_inited)) {
				CMR_LOGE("Sensor set init mode error!");
				sns_i2c_deinit(sensor_cxt, sensor_id);
				ret_val = SENSOR_FAIL;
			}
		}
		sensor_cxt->stream_on = 1;
		sns_stream_ctrl_common(sensor_cxt, 0);
		CMR_LOGI("4 open success");
	} else {
		CMR_LOGE("Sensor not register, open failed, sensor_id = %d", sensor_id);
	}

	if ((SENSOR_SUCCESS == ret_val) && (1 == sensor_cxt->is_autotest)) {
		Sensor_SetMode_WaitDone();
	}
	return ret_val;
}

cmr_int sensor_set_mode(struct sensor_drv_context *sensor_cxt, cmr_u32 mode, cmr_u32 is_inited)
{
	CMR_MSG_INIT(message);
	cmr_int                  ret = CMR_CAMERA_SUCCESS;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	message.msg_type     = SENSOR_CTRL_EVT_SETMODE;
	message.sub_msg_type = mode;
	message.sync_flag    = CMR_MSG_SYNC_NONE;
	message.data         = (void *)((cmr_uint)is_inited);
	ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle, &message);
	if (ret) {
		CMR_LOGE("send msg failed!");
		return CMR_CAMERA_FAIL;
	}

	return ret;
}

static cmr_int sensor_set_modone(struct sensor_drv_context *sensor_cxt)
{
	CMR_MSG_INIT(message);
	cmr_int                  ret = CMR_CAMERA_SUCCESS;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	/*the set mode function can be async control*/
	message.msg_type     = SENSOR_CTRL_EVT_SETMODONE;
	message.sync_flag    = CMR_MSG_SYNC_RECEIVED;
	ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle, &message);
	if (ret) {
		CMR_LOGE("send msg failed!");
		return CMR_CAMERA_FAIL;
	}
	return ret;
}


cmr_int sns_set_mode(struct sensor_drv_context *sensor_cxt, cmr_u32 mode, cmr_u32 is_inited)
{
	cmr_int rtn;
	cmr_u32 mclk;
	SENSOR_IOCTL_FUNC_PTR set_reg_tab_func = PNULL;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if (PNULL == sensor_cxt->sensor_info_ptr) {
		CMR_LOGE("No sensor info!");
		return -1;
	}

	/*when use the camera vendor functions, the sensor_cxt should be set at first */
	sensor_set_cxt_common(sensor_cxt);

	set_reg_tab_func = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->cus_func_1;

	CMR_LOGI("mode = %d.", mode);
	if (SENSOR_FALSE == sensor_is_init_common(sensor_cxt)) {
		CMR_LOGI("sensor has not init");
		return SENSOR_OP_STATUS_ERR;
	}

	if (sensor_cxt->sensor_mode[snr_get_cur_id(sensor_cxt)] == mode) {
		CMR_LOGI("The sensor mode as before");
	} else {
		if (PNULL != sensor_cxt->sensor_info_ptr->resolution_tab_info_ptr[mode].sensor_reg_tab_ptr) {
			mclk = sensor_cxt->sensor_info_ptr->resolution_tab_info_ptr[mode].xclk_to_sensor;
			Sensor_SetMCLK(mclk);
			sensor_cxt->sensor_exp_info.image_format = sensor_cxt->sensor_exp_info.sensor_mode_info[mode].image_format;

			if((SENSOR_MODE_COMMON_INIT == mode) && set_reg_tab_func){
				set_reg_tab_func(SENSOR_MODE_COMMON_INIT);
			}else{
				Sensor_SendRegTabToSensor(&sensor_cxt->sensor_info_ptr->resolution_tab_info_ptr[mode]);
			}
			sensor_cxt->sensor_mode[snr_get_cur_id(sensor_cxt)] = mode;
		} else {
			if(set_reg_tab_func)
				set_reg_tab_func(0);
			CMR_LOGI("No this resolution information !!!");
		}

		if (is_inited) {
			if (SENSOR_INTERFACE_TYPE_CSI2 == sensor_cxt->sensor_info_ptr->sensor_interface.type) {
				rtn = sns_stream_off(sensor_cxt);/*stream off first for MIPI sensor switch*/
				if (SENSOR_SUCCESS == rtn) {
					sns_dev_mipi_deinit(sensor_cxt);
				}
			}
		}
	}

	return SENSOR_SUCCESS;
}

cmr_int sensor_set_mode_common(struct sensor_drv_context *sensor_cxt, cmr_uint mode)
{
	cmr_int                      ret = 0;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = sensor_set_mode(sensor_cxt, mode, 1);
	return ret;
}

cmr_int Sensor_SetMode(cmr_u32 mode)
{
	cmr_int                      ret = 0;

	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	ret = sensor_set_mode(sensor_cxt, mode, 1);
	return ret;
}

cmr_int sensor_set_modone_common(struct sensor_drv_context *sensor_cxt)
{
	cmr_int                      ret = 0;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	ret = sensor_set_modone(sensor_cxt);
	return ret;
}

cmr_int Sensor_SetMode_WaitDone(void)
{
	cmr_int                      ret = 0;
	struct sensor_drv_context    *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	sensor_set_modone(sensor_cxt);
	return ret;
}

cmr_int sensor_get_mode_common(struct sensor_drv_context *sensor_cxt, cmr_uint *mode)
{
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if (SENSOR_FALSE == sensor_is_init_common(sensor_cxt)) {
		CMR_LOGI("sensor has not init");
		return SENSOR_OP_STATUS_ERR;
	}
	*mode = sensor_cxt->sensor_mode[snr_get_cur_id(sensor_cxt)];
	return SENSOR_SUCCESS;
}

cmr_int Sensor_GetMode(cmr_u32 *mode)
{
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if (SENSOR_FALSE == sensor_is_init_common(sensor_cxt)) {
		CMR_LOGI("sensor has not init");
		return SENSOR_OP_STATUS_ERR;
	}
	*mode = sensor_cxt->sensor_mode[snr_get_cur_id(sensor_cxt)];
	return SENSOR_SUCCESS;
}

cmr_int sns_stream_on(struct sensor_drv_context *sensor_cxt)
{
	cmr_int                    err = 0xff;
	cmr_u32               param = 0;
	SENSOR_IOCTL_FUNC_PTR  stream_on_func;

	CMR_LOGI("E");

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if (!sensor_is_init_common(sensor_cxt)) {
		CMR_LOGE("X: sensor has not been initialized");
		return SENSOR_FAIL;
	}

	if (PNULL == sensor_cxt->sensor_info_ptr) {
		CMR_LOGE("X: No sensor info!");
		return -1;
	}

	/*when use the camera vendor functions, the sensor_cxt should be set at first */
	//sensor_set_cxt_common(sensor_cxt);

	if (!sensor_cxt->stream_on) {
		stream_on_func = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->stream_on;

		if (PNULL != stream_on_func) {
			err = stream_on_func(param);
		}

		if (0 == err) {
			sensor_cxt->stream_on = 1;
		}
	}

	CMR_LOGI("X");
	return err;
}

cmr_int sensor_stream_ctrl_common(struct sensor_drv_context *sensor_cxt, cmr_u32 on_off)
{
	CMR_MSG_INIT(message);
	cmr_int                  ret = CMR_CAMERA_SUCCESS;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	/*the set mode function can be async control*/
	message.msg_type     = SENSOR_CTRL_EVT_STREAM_CTRL;
	message.sync_flag    = CMR_MSG_SYNC_PROCESSED;
	message.sub_msg_type = on_off;
	ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle, &message);
	if (ret) {
		CMR_LOGE("send msg failed!");
		return CMR_CAMERA_FAIL;
	}
	return ret;
}

cmr_int sns_stream_ctrl_common(struct sensor_drv_context *sensor_cxt, cmr_u32 on_off)
{
	cmr_int                      ret = 0;
	cmr_u32           		 mode;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	if (on_off) {
		if (SENSOR_INTERFACE_TYPE_CSI2 == sensor_cxt->sensor_info_ptr->sensor_interface.type) {
			mode = sensor_cxt->sensor_mode[snr_get_cur_id(sensor_cxt)];
			sns_dev_mipi_init(sensor_cxt, mode);
		}
		ret = sns_stream_on(sensor_cxt);
	} else {
		ret = sns_stream_off(sensor_cxt);
		if (SENSOR_SUCCESS == ret) {
			if (SENSOR_INTERFACE_TYPE_CSI2 == sensor_cxt->sensor_info_ptr->sensor_interface.type) {
				sns_dev_mipi_deinit(sensor_cxt);
				mode = sensor_cxt->sensor_mode[snr_get_cur_id(sensor_cxt)];
			}
		}
	}

	return ret;
}

cmr_int sns_stream_off(struct sensor_drv_context *sensor_cxt)
{
	cmr_int                   err = 0xff;
	cmr_u32               param = 0;
	SENSOR_IOCTL_FUNC_PTR stream_off_func;

	CMR_LOGI("E");
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	if (!sensor_is_init_common(sensor_cxt)) {
		CMR_LOGE("X: sensor has not been initialized");
		return SENSOR_FAIL;
	}

	if (PNULL == sensor_cxt->sensor_info_ptr) {
		CMR_LOGE("X: No sensor info!");
		return -1;
	}

	/*when use the camera vendor functions, the sensor_cxt should be set at first */
	sensor_set_cxt_common(sensor_cxt);

	stream_off_func = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->stream_off;

	if (PNULL != stream_off_func && sensor_cxt->stream_on == 1) {
		err = stream_off_func(param);
	}
	sensor_cxt->stream_on = 0;


	CMR_LOGI("X");
	return err;
}

cmr_int sensor_get_info_common(struct sensor_drv_context *sensor_cxt,
			SENSOR_EXP_INFO_T **sensor_exp_info_pptr)
{
	if (PNULL == sensor_cxt) {
		CMR_LOGE("zero pointer ");
		return SENSOR_FAIL;
	}
	if (!sensor_is_init_common(sensor_cxt)) {
		CMR_LOGE("sensor has not init");
		return SENSOR_FAIL;
	}

	CMR_LOGI("info = 0x%p ", (void*)&sensor_cxt->sensor_exp_info);
	*sensor_exp_info_pptr = &sensor_cxt->sensor_exp_info;
	return SENSOR_SUCCESS;
}

SENSOR_EXP_INFO_T *Sensor_GetInfo(void)
{
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	if (PNULL == sensor_cxt) {
		CMR_LOGE("zero pointer ");
		return PNULL;
	}
	if (!sensor_is_init_common(sensor_cxt)) {
		CMR_LOGI("sensor has not init");
		return PNULL;
	}

	CMR_LOGI("info=%p ", (void*)&sensor_cxt->sensor_exp_info);
	return &sensor_cxt->sensor_exp_info;
}

/*********************************************************************************
 todo:
 now the sensor_close_common only support close all sensor, and the function should be
 updated when the hardware can supported multiple sensors;
 *********************************************************************************/
cmr_int sensor_close_common(struct sensor_drv_context *sensor_cxt, cmr_u32 sensor_id)
{
	UNUSED(sensor_id);

	SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

	CMR_LOGI("E");
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	sensor_register_info_ptr = &sensor_cxt->sensor_register_info;

	sns_destroy_ctrl_thread(sensor_cxt);

	sns_stream_off(sensor_cxt);
	sns_dev_mipi_deinit(sensor_cxt);
	if (1 == sensor_cxt->is_register_sensor) {
		if (2 == sensor_cxt->is_main_sensor) {
			sns_dev_i2c_deinit(sensor_cxt, SENSOR_DEV_2);
		}else if (1 == sensor_cxt->is_main_sensor) {
			sns_dev_i2c_deinit(sensor_cxt, SENSOR_MAIN);
		} else {
			sns_dev_i2c_deinit(sensor_cxt, SENSOR_SUB);
		}
		sensor_cxt->is_register_sensor = 0;
		sensor_cxt->is_main_sensor = 0;
	}

	if (SENSOR_TRUE == sensor_is_init_common(sensor_cxt)) {
		if (SENSOR_IMAGE_FORMAT_RAW == sensor_cxt->sensor_info_ptr->image_format) {
			if (0 == sensor_cxt->is_calibration) {
				_sensor_calil_lnc_param_recover(sensor_cxt, sensor_cxt->sensor_info_ptr);
			}
		}
		Sensor_PowerOn(sensor_cxt, SENSOR_FALSE);
		if (SENSOR_MAIN == snr_get_cur_id(sensor_cxt)) {
			CMR_LOGI("0.");
			if (SCI_TRUE == sensor_register_info_ptr->is_register[SENSOR_SUB]) {
				CMR_LOGI("1.");
				sns_set_id(sensor_cxt, SENSOR_SUB);
				sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[SENSOR_SUB];
				Sensor_SetExportInfo(sensor_cxt);
				Sensor_PowerOn(sensor_cxt, SENSOR_FALSE);
				if (1 == sensor_cxt->is_register_sensor) {
					CMR_LOGI ("2.");
					sns_dev_i2c_deinit(sensor_cxt, SENSOR_SUB);
					sensor_cxt->is_register_sensor = 0;
					sensor_cxt->is_main_sensor = 0;
				}
			}
			if (SCI_TRUE == sensor_register_info_ptr->is_register[SENSOR_DEV_2]) {
				CMR_LOGI("1.1");
				sns_set_id(sensor_cxt, SENSOR_DEV_2);
				sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[SENSOR_DEV_2];
				Sensor_SetExportInfo(sensor_cxt);
				Sensor_PowerOn(sensor_cxt, SENSOR_FALSE);
				if (1 == sensor_cxt->is_register_sensor) {
					CMR_LOGI ("2.1");
					sns_dev_i2c_deinit(sensor_cxt, SENSOR_DEV_2);
					sensor_cxt->is_register_sensor = 0;
					sensor_cxt->is_main_sensor = 0;
				}
			}
		} else if (SENSOR_SUB == snr_get_cur_id(sensor_cxt)) {
			CMR_LOGI("3.");
			if (SCI_TRUE == sensor_register_info_ptr->is_register[SENSOR_MAIN]) {
				CMR_LOGI("4.");
				sns_set_id(sensor_cxt, SENSOR_MAIN);
				sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[SENSOR_MAIN];
				Sensor_SetExportInfo(sensor_cxt);
				Sensor_PowerOn(sensor_cxt, SENSOR_FALSE);
				if (1 == sensor_cxt->is_register_sensor) {
					CMR_LOGI ("5.");
					sns_dev_i2c_deinit(sensor_cxt, SENSOR_MAIN);
					sensor_cxt->is_register_sensor = 0;
					sensor_cxt->is_main_sensor = 0;
				}
			}
			if (SCI_TRUE == sensor_register_info_ptr->is_register[SENSOR_DEV_2]) {
				CMR_LOGI("4.1");
				sns_set_id(sensor_cxt, SENSOR_DEV_2);
				sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[SENSOR_DEV_2];
				Sensor_SetExportInfo(sensor_cxt);
				Sensor_PowerOn(sensor_cxt, SENSOR_FALSE);
				if (1 == sensor_cxt->is_register_sensor) {
					CMR_LOGI ("4.1");
					sns_dev_i2c_deinit(sensor_cxt, SENSOR_DEV_2);
					sensor_cxt->is_register_sensor = 0;
					sensor_cxt->is_main_sensor = 0;
				}
			}
		}
		else if (SENSOR_DEV_2 == snr_get_cur_id(sensor_cxt)) {
			if (SCI_TRUE == sensor_register_info_ptr->is_register[SENSOR_MAIN]) {
				CMR_LOGI("4.1");
				sns_set_id(sensor_cxt, SENSOR_MAIN);
				sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[SENSOR_MAIN];
				Sensor_SetExportInfo(sensor_cxt);
				Sensor_PowerOn(sensor_cxt, SENSOR_FALSE);
				if (1 == sensor_cxt->is_register_sensor) {
					CMR_LOGI("6.1");
					sns_dev_i2c_deinit(sensor_cxt, SENSOR_MAIN);
					sensor_cxt->is_register_sensor = 0;
					sensor_cxt->is_main_sensor = 0;
				}
			}
			if (SCI_TRUE == sensor_register_info_ptr->is_register[SENSOR_SUB]) {
				CMR_LOGI("7.1");
				sns_set_id(sensor_cxt, SENSOR_SUB);
				sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[SENSOR_SUB];
				Sensor_SetExportInfo(sensor_cxt);
				Sensor_PowerOn(sensor_cxt, SENSOR_FALSE);
				if (1 == sensor_cxt->is_register_sensor) {
					CMR_LOGI("8.1");
					sns_dev_i2c_deinit(sensor_cxt, SENSOR_SUB);
					sensor_cxt->is_register_sensor = 0;
					sensor_cxt->is_main_sensor = 0;
				}
			}
		} else if (SENSOR_ATV == snr_get_cur_id(sensor_cxt)) {
			if (SCI_TRUE == sensor_register_info_ptr->is_register[SENSOR_MAIN]) {
				CMR_LOGI("4.");
				sns_set_id(sensor_cxt, SENSOR_MAIN);
				sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[SENSOR_MAIN];
				Sensor_SetExportInfo(sensor_cxt);
				Sensor_PowerOn(sensor_cxt, SENSOR_FALSE);
				if (1 == sensor_cxt->is_register_sensor) {
					CMR_LOGI("6.");
					sns_dev_i2c_deinit(sensor_cxt, SENSOR_MAIN);
					sensor_cxt->is_register_sensor = 0;
					sensor_cxt->is_main_sensor = 0;
				}
			}
			if (SCI_TRUE == sensor_register_info_ptr->is_register[SENSOR_SUB]) {
				CMR_LOGI("7.");
				sns_set_id(sensor_cxt, SENSOR_SUB);
				sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[SENSOR_SUB];
				Sensor_SetExportInfo(sensor_cxt);
				Sensor_PowerOn(sensor_cxt, SENSOR_FALSE);
				if (1 == sensor_cxt->is_register_sensor) {
					CMR_LOGI("8.");
					sns_dev_i2c_deinit(sensor_cxt, SENSOR_SUB);
					sensor_cxt->is_register_sensor = 0;
					sensor_cxt->is_main_sensor = 0;
				}
			}
		}
	}
	CMR_LOGI("9.");

	sns_device_deinit(sensor_cxt);
	sensor_cxt->sensor_isInit = SENSOR_FALSE;
	sensor_cxt->sensor_mode[SENSOR_MAIN] = SENSOR_MODE_MAX;
	sensor_cxt->sensor_mode[SENSOR_SUB] = SENSOR_MODE_MAX;
	sensor_cxt->sensor_mode[SENSOR_DEV_2] = SENSOR_MODE_MAX;

	CMR_LOGI("X");

	return SENSOR_SUCCESS;
}

cmr_int sns_init_defaul_exif(struct sensor_drv_context *sensor_cxt)
{
	EXIF_SPEC_PIC_TAKING_COND_T* exif_ptr = PNULL;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	exif_ptr = &sensor_cxt->default_exif;
	cmr_bzero(&sensor_cxt->default_exif, sizeof(EXIF_SPEC_PIC_TAKING_COND_T));

	CMR_LOGV("E");

	exif_ptr->valid.FNumber=1;
	exif_ptr->FNumber.numerator=14;
	exif_ptr->FNumber.denominator=5;
	exif_ptr->valid.ExposureProgram=1;
	exif_ptr->ExposureProgram=0x04;
	exif_ptr->valid.ApertureValue=1;
	exif_ptr->ApertureValue.numerator=14;
	exif_ptr->ApertureValue.denominator=5;
	exif_ptr->valid.MaxApertureValue=1;
	exif_ptr->MaxApertureValue.numerator=14;
	exif_ptr->MaxApertureValue.denominator=5;
	exif_ptr->valid.FocalLength=1;
	exif_ptr->FocalLength.numerator=289;
	exif_ptr->FocalLength.denominator=100;
	exif_ptr->valid.FileSource=1;
	exif_ptr->FileSource=0x03;
	exif_ptr->valid.ExposureMode=1;
	exif_ptr->ExposureMode=0x00;
	exif_ptr->valid.WhiteBalance=1;
	exif_ptr->WhiteBalance=0x00;
	exif_ptr->valid.Flash = 1;

	CMR_LOGV("X");
	return SENSOR_SUCCESS;
}

cmr_int sensor_set_exif_common(struct sensor_drv_context *sensor_cxt,
					SENSOR_EXIF_CTRL_E cmd,
					cmr_u32 param)
{
	SENSOR_EXP_INFO_T_PTR sensor_info_ptr = NULL;
	EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_info_ptr = PNULL;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	sensor_get_info_common(sensor_cxt, &sensor_info_ptr);
	if (PNULL == sensor_info_ptr) {
		CMR_LOGW("sensor not ready yet, direct return");
		return SENSOR_FAIL;
	} else if (PNULL != sensor_info_ptr->ioctl_func_ptr->get_exif) {
		sensor_exif_info_ptr =
			(EXIF_SPEC_PIC_TAKING_COND_T *)sensor_info_ptr->ioctl_func_ptr->get_exif(0x00);
	} else {
		CMR_LOGI("the fun is null, set it to default");
		sensor_exif_info_ptr = &sensor_cxt->default_exif;
	}
	switch (cmd) {
	case SENSOR_EXIF_CTRL_EXPOSURETIME:
		{
			enum sensor_mode img_sensor_mode = sensor_cxt->sensor_mode[snr_get_cur_id(sensor_cxt)];
			cmr_u32 exposureline_time =
				sensor_info_ptr->sensor_mode_info[img_sensor_mode].line_time;
			cmr_u32 exposureline_num = param;
			cmr_u32 exposure_time = 0x00;

			exposure_time = exposureline_time * exposureline_num/10;
			sensor_exif_info_ptr->valid.ExposureTime = 1;

			if (0x00 == exposure_time) {
				sensor_exif_info_ptr->valid.ExposureTime = 0;
			} else if (1000000 >= exposure_time) {
				sensor_exif_info_ptr->ExposureTime.numerator =
				    0x01;
				sensor_exif_info_ptr->ExposureTime.denominator =
				    1000000 / exposure_time;
			} else {
				cmr_u32 second = 0x00;
				do {
					second++;
					exposure_time -= 1000000;
					if (1000000 >= exposure_time) {
						break;
					}
				} while (1);
				sensor_exif_info_ptr->ExposureTime.denominator =
					1000000 / exposure_time;
				sensor_exif_info_ptr->ExposureTime.numerator =
					sensor_exif_info_ptr->ExposureTime.denominator * second;
			}
			break;
		}
	case SENSOR_EXIF_CTRL_FNUMBER:
		break;
	case SENSOR_EXIF_CTRL_EXPOSUREPROGRAM:
		break;
	case SENSOR_EXIF_CTRL_SPECTRALSENSITIVITY:
		break;
	case SENSOR_EXIF_CTRL_ISOSPEEDRATINGS:
		if (param == 6) {
			/*6 = CAMERA_ISO_MAX*/
			sensor_exif_info_ptr->valid.ISOSpeedRatings = 0;
		} else {
			sensor_exif_info_ptr->valid.ISOSpeedRatings = 1;
		}
		sensor_exif_info_ptr->ISOSpeedRatings.count = 1;
		sensor_exif_info_ptr->ISOSpeedRatings.type  = EXIF_SHORT;
		sensor_exif_info_ptr->ISOSpeedRatings.size  = 2;
		cmr_copy((void*)&sensor_exif_info_ptr->ISOSpeedRatings.ptr[0],
			   (void*)&param, 2);
		break;
	case SENSOR_EXIF_CTRL_OECF:
		break;
	case SENSOR_EXIF_CTRL_SHUTTERSPEEDVALUE:
		break;
	case SENSOR_EXIF_CTRL_APERTUREVALUE:
		break;
	case SENSOR_EXIF_CTRL_BRIGHTNESSVALUE:
		{
			sensor_exif_info_ptr->valid.BrightnessValue = 1;
			switch (param) {
			case 0:
			case 1:
			case 2:
				sensor_exif_info_ptr->BrightnessValue.numerator = 1;
				sensor_exif_info_ptr->BrightnessValue.denominator = 1;
				break;
			case 3:
				sensor_exif_info_ptr->BrightnessValue.numerator = 0;
				sensor_exif_info_ptr->BrightnessValue.denominator = 0;
				break;
			case 4:
			case 5:
			case 6:
				sensor_exif_info_ptr->BrightnessValue.numerator = 2;
				sensor_exif_info_ptr->BrightnessValue.denominator = 2;
				break;
			default:
				sensor_exif_info_ptr->BrightnessValue.numerator = 0xff;
				sensor_exif_info_ptr->BrightnessValue.denominator = 0xff;
				break;
			}
			break;
		}
		break;
	case SENSOR_EXIF_CTRL_EXPOSUREBIASVALUE:
		break;
	case SENSOR_EXIF_CTRL_MAXAPERTUREVALUE:
		break;
	case SENSOR_EXIF_CTRL_SUBJECTDISTANCE:
		break;
	case SENSOR_EXIF_CTRL_METERINGMODE:
		break;
	case SENSOR_EXIF_CTRL_LIGHTSOURCE:
		{
			sensor_exif_info_ptr->valid.LightSource = 1;
			switch (param) {
			case 0:
				sensor_exif_info_ptr->LightSource = 0x00;
				break;
			case 1:
				sensor_exif_info_ptr->LightSource = 0x03;
				break;
			case 2:
				sensor_exif_info_ptr->LightSource = 0x0f;
				break;
			case 3:
				sensor_exif_info_ptr->LightSource = 0x0e;
				break;
			case 4:
				sensor_exif_info_ptr->LightSource = 0x02;
				break;
			case 5:
				sensor_exif_info_ptr->LightSource = 0x01;
				break;
			case 6:
				sensor_exif_info_ptr->LightSource = 0x0a;
				break;
			default:
				sensor_exif_info_ptr->LightSource = 0xff;
				break;
			}
			break;
		}
	case SENSOR_EXIF_CTRL_FLASH:
		sensor_exif_info_ptr->valid.Flash = 1;
		sensor_exif_info_ptr->Flash = param;
		break;
	case SENSOR_EXIF_CTRL_FOCALLENGTH:
		break;
	case SENSOR_EXIF_CTRL_SUBJECTAREA:
		break;
	case SENSOR_EXIF_CTRL_FLASHENERGY:
		break;
	case SENSOR_EXIF_CTRL_SPATIALFREQUENCYRESPONSE:
		break;
	case SENSOR_EXIF_CTRL_FOCALPLANEXRESOLUTION:
		break;
	case SENSOR_EXIF_CTRL_FOCALPLANEYRESOLUTION:
		break;
	case SENSOR_EXIF_CTRL_FOCALPLANERESOLUTIONUNIT:
		break;
	case SENSOR_EXIF_CTRL_SUBJECTLOCATION:
		break;
	case SENSOR_EXIF_CTRL_EXPOSUREINDEX:
		break;
	case SENSOR_EXIF_CTRL_SENSINGMETHOD:
		break;
	case SENSOR_EXIF_CTRL_FILESOURCE:
		break;
	case SENSOR_EXIF_CTRL_SCENETYPE:
		break;
	case SENSOR_EXIF_CTRL_CFAPATTERN:
		break;
	case SENSOR_EXIF_CTRL_CUSTOMRENDERED:
		break;
	case SENSOR_EXIF_CTRL_EXPOSUREMODE:
		break;

	case SENSOR_EXIF_CTRL_WHITEBALANCE:
		sensor_exif_info_ptr->valid.WhiteBalance = 1;
		if(param)
			sensor_exif_info_ptr->WhiteBalance = 1;
		else
			sensor_exif_info_ptr->WhiteBalance = 0;
		break;

	case SENSOR_EXIF_CTRL_DIGITALZOOMRATIO:
		break;
	case SENSOR_EXIF_CTRL_FOCALLENGTHIN35MMFILM:
		break;
	case SENSOR_EXIF_CTRL_SCENECAPTURETYPE:
		{
			sensor_exif_info_ptr->valid.SceneCaptureType = 1;
			switch (param) {
			case 0:
				sensor_exif_info_ptr->SceneCaptureType = 0x00;
				break;
			case 1:
				sensor_exif_info_ptr->SceneCaptureType = 0x03;
				break;
			default:
				sensor_exif_info_ptr->LightSource = 0xff;
				break;
			}
			break;
		}
	case SENSOR_EXIF_CTRL_GAINCONTROL:
		break;
	case SENSOR_EXIF_CTRL_CONTRAST:
		{
			sensor_exif_info_ptr->valid.Contrast = 1;
			switch (param) {
			case 0:
			case 1:
			case 2:
				sensor_exif_info_ptr->Contrast = 0x01;
				break;
			case 3:
				sensor_exif_info_ptr->Contrast = 0x00;
				break;
			case 4:
			case 5:
			case 6:
				sensor_exif_info_ptr->Contrast = 0x02;
				break;
			default:
				sensor_exif_info_ptr->Contrast = 0xff;
				break;
			}
			break;
		}
	case SENSOR_EXIF_CTRL_SATURATION:
		{
			sensor_exif_info_ptr->valid.Saturation = 1;
			switch (param) {
			case 0:
			case 1:
			case 2:
				sensor_exif_info_ptr->Saturation = 0x01;
				break;
			case 3:
				sensor_exif_info_ptr->Saturation = 0x00;
				break;
			case 4:
			case 5:
			case 6:
				sensor_exif_info_ptr->Saturation = 0x02;
				break;
			default:
				sensor_exif_info_ptr->Saturation = 0xff;
				break;
			}
			break;
		}
	case SENSOR_EXIF_CTRL_SHARPNESS:
		{
			sensor_exif_info_ptr->valid.Sharpness = 1;
			switch (param) {
			case 0:
			case 1:
			case 2:
				sensor_exif_info_ptr->Sharpness = 0x01;
				break;
			case 3:
				sensor_exif_info_ptr->Sharpness = 0x00;
				break;
			case 4:
			case 5:
			case 6:
				sensor_exif_info_ptr->Sharpness = 0x02;
				break;
			default:
				sensor_exif_info_ptr->Sharpness = 0xff;
				break;
			}
			break;
		}
	case SENSOR_EXIF_CTRL_DEVICESETTINGDESCRIPTION:
		break;
	case SENSOR_EXIF_CTRL_SUBJECTDISTANCERANGE:
		break;
	default:
		break;
	}
	return SENSOR_SUCCESS;
}

cmr_int sensor_update_isparm_from_file(struct sensor_drv_context *sensor_cxt, cmr_u32 sensor_id)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;

	if (!sensor_cxt) {
		return CMR_CAMERA_FAIL;
	}

	ret = isp_raw_para_update_from_file(sensor_cxt->sensor_info_ptr, sensor_id);
	return ret;
}

cmr_int Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_E cmd, cmr_u32 param)
{
	struct sensor_drv_context    *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();
	SENSOR_EXP_INFO_T_PTR        sensor_info_ptr = NULL;
	EXIF_SPEC_PIC_TAKING_COND_T  *sensor_exif_info_ptr = PNULL;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	sensor_get_info_common(sensor_cxt, &sensor_info_ptr);
	if (PNULL == sensor_info_ptr) {
		CMR_LOGW("sensor not ready yet, direct return");
		return SENSOR_FAIL;
	} else if (PNULL != sensor_info_ptr->ioctl_func_ptr->get_exif) {
		sensor_exif_info_ptr =
			(EXIF_SPEC_PIC_TAKING_COND_T *)sensor_info_ptr->ioctl_func_ptr->get_exif(0x00);
	} else {
		CMR_LOGI("the fun is null, set it to default");
		sensor_exif_info_ptr = &sensor_cxt->default_exif;
	}

	switch (cmd) {
	case SENSOR_EXIF_CTRL_EXPOSURETIME:
		{
			enum sensor_mode img_sensor_mode = sensor_cxt->sensor_mode[snr_get_cur_id(sensor_cxt)];
			cmr_u32 exposureline_time =
				sensor_info_ptr->sensor_mode_info[img_sensor_mode].line_time;
			cmr_u32 exposureline_num = param;
			cmr_u32 exposure_time = 0x00;

			exposure_time = exposureline_time * exposureline_num/10;
			sensor_exif_info_ptr->valid.ExposureTime = 1;

			if (0x00 == exposure_time) {
				sensor_exif_info_ptr->valid.ExposureTime = 0;
			} else if (1000000 >= exposure_time) {
				sensor_exif_info_ptr->ExposureTime.numerator =
				    0x01;
				sensor_exif_info_ptr->ExposureTime.denominator =
				    ((10000000 / exposure_time)+5)/10;
			} else {
				cmr_u32 second = 0x00;
				do {
					second++;
					exposure_time -= 1000000;
					if (1000000 >= exposure_time) {
						break;
					}
				} while (1);
				sensor_exif_info_ptr->ExposureTime.denominator =
					1000000 / exposure_time;
				sensor_exif_info_ptr->ExposureTime.numerator =
					sensor_exif_info_ptr->ExposureTime.denominator * second;
			}
			break;
		}
	case SENSOR_EXIF_CTRL_FNUMBER:
		break;
	case SENSOR_EXIF_CTRL_EXPOSUREPROGRAM:
		break;
	case SENSOR_EXIF_CTRL_SPECTRALSENSITIVITY:
		break;
	case SENSOR_EXIF_CTRL_ISOSPEEDRATINGS:
		sensor_exif_info_ptr->valid.ISOSpeedRatings = 1;
		sensor_exif_info_ptr->ISOSpeedRatings.count = 1;
		sensor_exif_info_ptr->ISOSpeedRatings.type  = EXIF_SHORT;
		sensor_exif_info_ptr->ISOSpeedRatings.size  = 2;
		cmr_copy((void*)&sensor_exif_info_ptr->ISOSpeedRatings.ptr[0],
			   (void*)&param, 2);
		break;
	case SENSOR_EXIF_CTRL_OECF:
		break;
	case SENSOR_EXIF_CTRL_SHUTTERSPEEDVALUE:
		break;
	case SENSOR_EXIF_CTRL_APERTUREVALUE:
		break;
	case SENSOR_EXIF_CTRL_BRIGHTNESSVALUE:
		{
			sensor_exif_info_ptr->valid.BrightnessValue = 1;
			switch (param) {
			case 0:
			case 1:
			case 2:
				sensor_exif_info_ptr->BrightnessValue.numerator = 1;
				sensor_exif_info_ptr->BrightnessValue.denominator = 1;
				break;
			case 3:
				sensor_exif_info_ptr->BrightnessValue.numerator = 0;
				sensor_exif_info_ptr->BrightnessValue.denominator = 0;
				break;
			case 4:
			case 5:
			case 6:
				sensor_exif_info_ptr->BrightnessValue.numerator = 2;
				sensor_exif_info_ptr->BrightnessValue.denominator = 2;
				break;
			default:
				sensor_exif_info_ptr->BrightnessValue.numerator = 0xff;
				sensor_exif_info_ptr->BrightnessValue.denominator = 0xff;
				break;
			}
			break;
		}
		break;
	case SENSOR_EXIF_CTRL_EXPOSUREBIASVALUE:
		break;
	case SENSOR_EXIF_CTRL_MAXAPERTUREVALUE:
		break;
	case SENSOR_EXIF_CTRL_SUBJECTDISTANCE:
		break;
	case SENSOR_EXIF_CTRL_METERINGMODE:
		break;
	case SENSOR_EXIF_CTRL_LIGHTSOURCE:
		{
			sensor_exif_info_ptr->valid.LightSource = 1;
			switch (param) {
			case 0:
				sensor_exif_info_ptr->LightSource = 0x00;
				break;
			case 1:
				sensor_exif_info_ptr->LightSource = 0x03;
				break;
			case 2:
				sensor_exif_info_ptr->LightSource = 0x0f;
				break;
			case 3:
				sensor_exif_info_ptr->LightSource = 0x0e;
				break;
			case 4:
				sensor_exif_info_ptr->LightSource = 0x03;
				break;
			case 5:
				sensor_exif_info_ptr->LightSource = 0x01;
				break;
			case 6:
				sensor_exif_info_ptr->LightSource = 0x0a;
				break;
			default:
				sensor_exif_info_ptr->LightSource = 0xff;
				break;
			}
			break;
		}
	case SENSOR_EXIF_CTRL_FLASH:
		sensor_exif_info_ptr->valid.Flash = 1;
		sensor_exif_info_ptr->Flash = param;
		break;
	case SENSOR_EXIF_CTRL_FOCALLENGTH:
		break;
	case SENSOR_EXIF_CTRL_SUBJECTAREA:
		break;
	case SENSOR_EXIF_CTRL_FLASHENERGY:
		break;
	case SENSOR_EXIF_CTRL_SPATIALFREQUENCYRESPONSE:
		break;
	case SENSOR_EXIF_CTRL_FOCALPLANEXRESOLUTION:
		break;
	case SENSOR_EXIF_CTRL_FOCALPLANEYRESOLUTION:
		break;
	case SENSOR_EXIF_CTRL_FOCALPLANERESOLUTIONUNIT:
		break;
	case SENSOR_EXIF_CTRL_SUBJECTLOCATION:
		break;
	case SENSOR_EXIF_CTRL_EXPOSUREINDEX:
		break;
	case SENSOR_EXIF_CTRL_SENSINGMETHOD:
		break;
	case SENSOR_EXIF_CTRL_FILESOURCE:
		break;
	case SENSOR_EXIF_CTRL_SCENETYPE:
		break;
	case SENSOR_EXIF_CTRL_CFAPATTERN:
		break;
	case SENSOR_EXIF_CTRL_CUSTOMRENDERED:
		break;
	case SENSOR_EXIF_CTRL_EXPOSUREMODE:
		break;

	case SENSOR_EXIF_CTRL_WHITEBALANCE:
		sensor_exif_info_ptr->valid.WhiteBalance = 1;
		if(param)
			sensor_exif_info_ptr->WhiteBalance = 1;
		else
			sensor_exif_info_ptr->WhiteBalance = 0;
		break;

	case SENSOR_EXIF_CTRL_DIGITALZOOMRATIO:
		break;
	case SENSOR_EXIF_CTRL_FOCALLENGTHIN35MMFILM:
		break;
	case SENSOR_EXIF_CTRL_SCENECAPTURETYPE:
		{
			sensor_exif_info_ptr->valid.SceneCaptureType = 1;
			switch (param) {
			case 0:
				sensor_exif_info_ptr->SceneCaptureType = 0x00;
				break;
			case 1:
				sensor_exif_info_ptr->SceneCaptureType = 0x03;
				break;
			default:
				sensor_exif_info_ptr->LightSource = 0xff;
				break;
			}
			break;
		}
	case SENSOR_EXIF_CTRL_GAINCONTROL:
		break;
	case SENSOR_EXIF_CTRL_CONTRAST:
		{
			sensor_exif_info_ptr->valid.Contrast = 1;
			switch (param) {
			case 0:
			case 1:
			case 2:
				sensor_exif_info_ptr->Contrast = 0x01;
				break;
			case 3:
				sensor_exif_info_ptr->Contrast = 0x00;
				break;
			case 4:
			case 5:
			case 6:
				sensor_exif_info_ptr->Contrast = 0x02;
				break;
			default:
				sensor_exif_info_ptr->Contrast = 0xff;
				break;
			}
			break;
		}
	case SENSOR_EXIF_CTRL_SATURATION:
		{
			sensor_exif_info_ptr->valid.Saturation = 1;
			switch (param) {
			case 0:
			case 1:
			case 2:
				sensor_exif_info_ptr->Saturation = 0x01;
				break;
			case 3:
				sensor_exif_info_ptr->Saturation = 0x00;
				break;
			case 4:
			case 5:
			case 6:
				sensor_exif_info_ptr->Saturation = 0x02;
				break;
			default:
				sensor_exif_info_ptr->Saturation = 0xff;
				break;
			}
			break;
		}
	case SENSOR_EXIF_CTRL_SHARPNESS:
		{
			sensor_exif_info_ptr->valid.Sharpness = 1;
			switch (param) {
			case 0:
			case 1:
			case 2:
				sensor_exif_info_ptr->Sharpness = 0x01;
				break;
			case 3:
				sensor_exif_info_ptr->Sharpness = 0x00;
				break;
			case 4:
			case 5:
			case 6:
				sensor_exif_info_ptr->Sharpness = 0x02;
				break;
			default:
				sensor_exif_info_ptr->Sharpness = 0xff;
				break;
			}
			break;
		}
	case SENSOR_EXIF_CTRL_DEVICESETTINGDESCRIPTION:
		break;
	case SENSOR_EXIF_CTRL_SUBJECTDISTANCERANGE:
		break;
	default:
		break;
	}
	return SENSOR_SUCCESS;
}

cmr_int sensor_get_exif_common(struct sensor_drv_context *sensor_cxt, EXIF_SPEC_PIC_TAKING_COND_T **sensor_exif_info_pptr)
{
	SENSOR_EXP_INFO_T_PTR sensor_info_ptr = NULL;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	sensor_get_info_common(sensor_cxt, &sensor_info_ptr);
	if (!sensor_info_ptr) {
		CMR_LOGE("ZERO poiner sensor_cxt %p sensor_info_ptr %p",
			sensor_cxt,
			sensor_info_ptr);
		return SENSOR_SUCCESS;
	}

	if (PNULL != sensor_info_ptr->ioctl_func_ptr->get_exif) {
		*sensor_exif_info_pptr =
			(EXIF_SPEC_PIC_TAKING_COND_T *)
			sensor_info_ptr->ioctl_func_ptr->get_exif(0x00);
		CMR_LOGI("get_exif.");
	} else {
		//struct sensor_drv_context *cur_cxt = (struct sensor_drv_context*)sensor_get_dev_cxt();
		*sensor_exif_info_pptr = &sensor_cxt->default_exif;
		//memcpy((void*)&sensor_cxt->default_exif, (void*)&cur_cxt->default_exif,
		//	sizeof(EXIF_SPEC_PIC_TAKING_COND_T));
	}
	return SENSOR_SUCCESS;
}

EXIF_SPEC_PIC_TAKING_COND_T *Sensor_GetSensorExifInfo(void)
{
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();
	SENSOR_EXP_INFO_T_PTR sensor_info_ptr = NULL;
	EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_info_ptr = PNULL;

	if (!sensor_cxt) {
		CMR_LOGE("ZERO poiner sensor_cxt %p", sensor_cxt);
		return NULL;
	}

	sensor_get_info_common(sensor_cxt, &sensor_info_ptr);

	if (!sensor_info_ptr) {
		CMR_LOGE("ZERO poiner sensor_info_ptr %p", sensor_info_ptr);
		return NULL;
	}

	if (PNULL != sensor_info_ptr->ioctl_func_ptr->get_exif) {
		sensor_exif_info_ptr =
			(EXIF_SPEC_PIC_TAKING_COND_T *)sensor_info_ptr->ioctl_func_ptr->get_exif(0x00);
		CMR_LOGI("get_exif.");
	} else {
		sensor_exif_info_ptr = &sensor_cxt->default_exif;
		CMR_LOGI("fun null, so use the default");
	}
	return sensor_exif_info_ptr;
}

cmr_int snr_set_mark(struct sensor_drv_context *sensor_cxt, cmr_u8 *buf)
{
	cmr_u32 i;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	if ((SIGN_0 != buf[0]) && (SIGN_1 != buf[1])
		&& (SIGN_2 != buf[2]) && (SIGN_3 != buf[3])) {
		sensor_cxt->sensor_identified = SCI_FALSE;
	} else {
		sensor_cxt->sensor_identified = SCI_TRUE;
		for( i=0 ; i<=SENSOR_DEV_2 ; i++) {
			sensor_cxt->sensor_index[i] = buf[4+i];
		}
	}
	CMR_LOGI("%d,idex %d,%d.",
		sensor_cxt->sensor_identified,
		sensor_cxt->sensor_index[SENSOR_MAIN],
		sensor_cxt->sensor_index[SENSOR_SUB],
		sensor_cxt->sensor_index[SENSOR_DEV_2]);
	return 0;
}

cmr_int snr_get_mark(struct sensor_drv_context *sensor_cxt,
			cmr_u8 *buf,
			cmr_u8 *is_saved_ptr)
{
	cmr_u32 i, j = 0;
	cmr_u8 *ptr = buf;
	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	if(SCI_TRUE == sensor_cxt->sensor_param_saved) {
		*is_saved_ptr = 1;
		*ptr++ = SIGN_0;
		*ptr++ = SIGN_1;
		*ptr++ = SIGN_2;
		*ptr++ = SIGN_3;
		for( i=0 ; i<=SENSOR_DEV_2 ; i++) {
			*ptr++ = sensor_cxt->sensor_index[i];
		}
		CMR_LOGI("index is %d,%d,%d.",
			sensor_cxt->sensor_index[SENSOR_MAIN],
			sensor_cxt->sensor_index[SENSOR_SUB],
			sensor_cxt->sensor_index[SENSOR_DEV_2]);
	} else {
		*is_saved_ptr = 0;
	}
	return 0;
}

cmr_int Sensor_WriteData(cmr_u8 *regPtr, cmr_u32 length)
{
	cmr_int ret;
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();
	ret = sns_device_write(sensor_cxt, regPtr, length);
	return ret;
}

cmr_int sensor_cfg_otp_update_isparam(struct sensor_drv_context *sensor_cxt,
						cmr_u32 sensor_id)
{
	CMR_MSG_INIT(message);
	cmr_int                  ret = CMR_CAMERA_SUCCESS;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	/*the set mode function can be async control*/
	message.msg_type     = SENSOR_CTRL_EVT_CFGOTP;
	message.sub_msg_type = sensor_id;
	message.sync_flag    = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle, &message);
	if (ret) {
		CMR_LOGE("send msg failed!");
		return CMR_CAMERA_FAIL;
	}
	return ret;
}

cmr_int sns_cfg_otp_update_isparam(struct sensor_drv_context *sensor_cxt,
						cmr_u32 sensor_id)
{
	cmr_int                      ret = 0;

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);
	if ((NULL != sensor_cxt->sensor_info_ptr)&& (NULL != sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr)){

#if 0
		isp_raw_para_update_from_file(sensor_cxt->sensor_info_ptr, sensor_id);
#endif

		if(PNULL != sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->cfg_otp){
			sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->cfg_otp(0);
		}
	}else{
		CMR_LOGE("invalid param failed!");
		return CMR_CAMERA_INVALID_PARAM;
	}
	return ret;
}

cmr_int snr_chk_snr_mode(SENSOR_MODE_INFO_T *mode_info)
{
	cmr_int ret = 0;
	if (mode_info) {
		/*jpeg format do not crop*/
		if (SENSOR_IMAGE_FORMAT_JPEG == mode_info->image_format) {
			if ((0 != mode_info->trim_start_x)
				|| (0 != mode_info->trim_start_y)) {
				ret = -1;
				goto out;
			}
		}
		if (((mode_info->trim_start_x + mode_info->trim_width) > mode_info->width)
			|| ((mode_info->trim_start_y + mode_info->trim_height) > mode_info->height)
			|| ((mode_info->scaler_trim.x + mode_info->scaler_trim.w) > mode_info->trim_width)
			|| ((mode_info->scaler_trim.y + mode_info->scaler_trim.h) > mode_info->trim_height)) {
			ret = -1;
		}
	}

out:
	return ret;
}

cmr_int Sensor_SetFlash(uint32_t is_open)
{
	UNUSED(is_open);

	//now some NULL process
	return 0;
}

