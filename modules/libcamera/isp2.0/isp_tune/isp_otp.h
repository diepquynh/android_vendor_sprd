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
#ifndef _ISP_OTP_H
#define _ISP_OTP_H

#include <stdio.h>

int  start_camera(void  *pdev);
int  stop_camera(void  *pdev);

int isp_otp_needstopprev(uint8_t *data_buf, uint32_t *data_size);
int isp_otp_write(isp_handle isp_handler, unsigned char *buf, unsigned int *len);
int isp_otp_read(isp_handle isp_handler, uint8_t *data_buf, uint32_t *data_size);

int read_sensor_shutter(uint32_t *shutter_val);
int read_sensor_gain(uint32_t *gain_val);
int read_position(isp_handle handler, uint32_t *pos);
int read_otp_awb_gain(isp_handle handler, void *awbc_cfg);
#endif
