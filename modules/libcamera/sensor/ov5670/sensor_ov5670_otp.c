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

#include <utils/Log.h>
#include "sensor.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#define DBG_OTP
#ifdef DBG_OTP
#define DEBUG_OTP_STR     "OV5670_OTP_debug: L %d, %s: "
#define DEBUG_OTP_ARGS    __LINE__,__FUNCTION__

#define OTP_PRINT(format,...) ALOGI(DEBUG_OTP_STR format, DEBUG_OTP_ARGS, ##__VA_ARGS__)
#else
#define OTP_PRINT
#endif

#define OV5670_RAW_PARAM_Sunny    0x0001
#define OV5670_RAW_PARAM_Truly    0x0002
#define OV5670_RAW_PARAM_Sunrise  0x000C

struct otp_typical_value_t
{
    uint16_t RG_Ratio_Typical ;
    uint16_t BG_Ratio_Typical ;
};

struct otp_struct
{
    cmr_uint 	flag; /*bit[7]: info, bit[6]:wb*/
    cmr_uint	module_integrator_id;
    cmr_uint	lens_id;
    cmr_uint	production_year;
    cmr_uint	production_month;
    cmr_uint	production_day;
    cmr_uint	rg_ratio;
    cmr_uint	bg_ratio;
};

static struct otp_struct current_otp = {0x00};
static struct otp_typical_value_t otp_typical_value = {0x00};

LOCAL cmr_uint ov5670_read_otp_info(struct otp_struct *otp_ptr)
{
    cmr_uint otp_flag = 0,addr = 0;

    otp_flag = Sensor_ReadReg(0x7010);
    OTP_PRINT("otp_flag info, = 0x%x \n",(uint32_t)otp_flag);
    if((otp_flag & 0xc0) == 0x40) {
        addr = 0x7011; /*base address of info group 1*/
    }
    else if((otp_flag & 0x30) == 0x10) {
        addr = 0x7016; /*base address of info group 2*/
    }
    else if((otp_flag & 0x0c) == 0x04) {
        addr = 0x701b; /*base address of info group 3*/
    }
    if(addr != 0) {
        (*otp_ptr).flag = 0x80; /*valid base info in OTP*/
        (*otp_ptr).module_integrator_id = Sensor_ReadReg(addr);
        (*otp_ptr).lens_id = Sensor_ReadReg(addr + 1);
        (*otp_ptr).production_year = Sensor_ReadReg(addr + 2);
        (*otp_ptr).production_month = Sensor_ReadReg(addr + 3);
        (*otp_ptr).production_day = Sensor_ReadReg(addr + 4);
    }
    else {
        (*otp_ptr).flag = 0x00; /*not info in OTP*/
        (*otp_ptr).module_integrator_id = 0;
        (*otp_ptr).lens_id = 0;
        (*otp_ptr).production_year = 0;
        (*otp_ptr).production_month = 0;
        (*otp_ptr).production_day = 0;
    }

    OTP_PRINT("lence id(%x),year,month,day(%d,%d,%d)\n",(uint32_t)(*otp_ptr).lens_id,
            (uint32_t)(*otp_ptr).production_year,(uint32_t)(*otp_ptr).production_month,(uint32_t)(*otp_ptr).production_day);

    return 0;
}
LOCAL cmr_uint ov5670_read_otp_wb(struct otp_struct *otp_ptr)
{
    cmr_uint otp_flag = 0, addr = 0, temp = 0;

    otp_flag = Sensor_ReadReg(0x7020);
    OTP_PRINT("otp_flag wb, = 0x%x \n",(uint32_t)otp_flag);

    if((otp_flag & 0xc0) == 0x40) {
        addr = 0x7021; /*base address of WB Calibration group 1*/
    }
    else if((otp_flag & 0x30) == 0x10) {
        addr = 0x7024; /*base address of WB Calibration group 2*/
    }
    else if((otp_flag & 0x0c) == 0x04) {
        addr = 0x7027; /*base address of WB Calibration group 3*/
    }
    if(addr != 0) {
        (*otp_ptr).flag |= 0x40;
        temp = Sensor_ReadReg(addr + 2);
        (*otp_ptr).rg_ratio = (Sensor_ReadReg(addr)<<2) + ((temp>>6) & 0x03);
        (*otp_ptr).bg_ratio = (Sensor_ReadReg(addr + 1)<<2) + ((temp>>4) & 0x03);
    }
    else {
        (*otp_ptr).rg_ratio = 0;
        (*otp_ptr).bg_ratio = 0;
    }
    OTP_PRINT("flag(0x%x),module_integrator_id(0x%x),rg_ratio(0x%x),bg_ratio(0x%x),RG_Ratio_Typical(0x%x),BG_Ratio_Typical(0x%x)\n",
            (uint32_t)(*otp_ptr).flag,(uint32_t)(*otp_ptr).module_integrator_id,(uint32_t)(*otp_ptr).rg_ratio,(uint32_t)(*otp_ptr).bg_ratio,
            otp_typical_value.RG_Ratio_Typical,otp_typical_value.BG_Ratio_Typical);
    return 0;
}
LOCAL cmr_uint ov5670_read_otp(struct otp_struct *otp_ptr)
{
    cmr_uint temp = 0, i = 0;
    cmr_u16 stream_value = 0;

    stream_value = Sensor_ReadReg(0x0100);
    if(1 != (stream_value & 0x01)) {
        Sensor_WriteReg(0x0100, 0x01);
        usleep(50*1000);
    }
    /*set 0x5002[3] to "0"*/
    temp = Sensor_ReadReg(0x5002);
    Sensor_WriteReg(0x5002, (0x00 & 0x08) | (temp & (~0x08)));
    /*read OTP into buffer*/
    Sensor_WriteReg(0x3d84, 0xC0);
    Sensor_WriteReg(0x3d88, 0x70); /*OTP start address*/
    Sensor_WriteReg(0x3d89, 0x10);
    Sensor_WriteReg(0x3d8A, 0x70); /*OTP end address*/
    Sensor_WriteReg(0x3d8B, 0x29);
    Sensor_WriteReg(0x3d81, 0x01); /*load otp into buffer*/
    usleep(5*1000);
    /*read OTP into*/
    ov5670_read_otp_info(otp_ptr);
    /*read OTP WB Calibration*/
    ov5670_read_otp_wb(otp_ptr);
    for(i=0x7010;i<=0x7029;i++) {
        Sensor_WriteReg(i,0); /*clear OTP buffer, recommended use continuous write to accelarate*/
    }
    /*set 0x5002[3] to "1"*/
    temp = Sensor_ReadReg(0x5002);
    Sensor_WriteReg(0x5002, (0x08 & 0x08) | (temp & (~0x08)));
    if(1 != (stream_value & 0x01))
        Sensor_WriteReg(0x0100, stream_value);
    return (*otp_ptr).flag;
}

LOCAL cmr_uint ov5670_check_otp_module_id(void)
{
    ov5670_read_otp(&current_otp);
    OTP_PRINT("module_integrator_id = 0x%x\n",(uint32_t)(current_otp.module_integrator_id));
    return (current_otp.module_integrator_id);
}

LOCAL cmr_int ov5670_update_awb_gain(cmr_uint R_gain,cmr_uint G_gain,cmr_uint B_gain) 
{
    OTP_PRINT("R_gain,G_gain,B_gain:(0x%x,0x%x,0x%x) \n",(uint32_t)R_gain,(uint32_t)G_gain,(uint32_t)B_gain);
    if (R_gain>0x400) {
        Sensor_WriteReg(0x5032, R_gain>>8);
        Sensor_WriteReg(0x5033, R_gain & 0x00ff);
    }
    if(G_gain>0x400) {
        Sensor_WriteReg(0x5034, G_gain>>8);
        Sensor_WriteReg(0x5035, G_gain & 0x00ff);
    }
    if(B_gain>0x400) {
        Sensor_WriteReg(0x5036, B_gain>>8);
        Sensor_WriteReg(0x5037, B_gain & 0x00ff);
    }
    return 0;
}
/* return value:
 bit[7]: 0 no otp info, 1 valid otp info
 bit[6]: 0 no otp wb, 1 valib otp wb
*/
LOCAL uint32_t ov5670_update_otp_wb(void)
{

    cmr_uint rg, bg, R_gain, G_gain, B_gain, Base_gain;

    /*apply OTP WB Calibration*/
    if (current_otp.flag & 0x40) {
        rg = current_otp.rg_ratio;
        bg = current_otp.bg_ratio;
        /*calculate G gain*/
        R_gain = (otp_typical_value.RG_Ratio_Typical*1000) / rg;
        B_gain = (otp_typical_value.BG_Ratio_Typical*1000) / bg;
        G_gain = 1000;

        if (R_gain < 1000 || B_gain < 1000) {
            if (R_gain < B_gain)
                Base_gain = R_gain;
            else
                Base_gain = B_gain;
        }
        else {
            Base_gain = G_gain;
        }
        R_gain = 0x400 * R_gain / (Base_gain);
        B_gain = 0x400 * B_gain / (Base_gain);
        G_gain = 0x400 * G_gain / (Base_gain);

        /*update sensor WB gain*/
        ov5670_update_awb_gain(R_gain, G_gain, B_gain);
    }
    return current_otp.flag;
}

LOCAL uint32_t _ov5670_Identify_otp(void* param_ptr)
{
    uint32_t rtn = SENSOR_SUCCESS;
    cmr_uint module_id;
    UNUSED(param_ptr);
    OTP_PRINT("_ov5670_Identify_otp E\n");
    /*read param id from sensor omap*/
    module_id = ov5670_check_otp_module_id();

    if(OV5670_RAW_PARAM_Sunny == module_id) {
        OTP_PRINT("This is Sunny module!!\n");
        otp_typical_value.RG_Ratio_Typical = 0x152;
        otp_typical_value.BG_Ratio_Typical = 0x152;
    }
    else if(OV5670_RAW_PARAM_Truly == module_id) {
        OTP_PRINT("This is Truly module!!\n");
        otp_typical_value.RG_Ratio_Typical = 0x152;
        otp_typical_value.BG_Ratio_Typical = 0x137;
    }
    else if(OV5670_RAW_PARAM_Sunrise == module_id) {
        OTP_PRINT("This is Sunrise module!!\n");
        otp_typical_value.RG_Ratio_Typical =0x133;
        otp_typical_value.BG_Ratio_Typical =0x115;
    }
    else {
        CMR_LOGE("This is unknow module!!\n");
        return SENSOR_FAIL;
    }
    ov5670_update_otp_wb();
    return rtn;
}
