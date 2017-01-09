/*
* Copyright (C) 2010 The Android Open Source Project
* Copyright (C) 2012-2015, The Linux Foundation. All rights reserved.
*
* Not a Contribution, Apache license notifications and license are retained
* for attribution purposes only.
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


/******************************************************************************
 ** File Name:      aud_record_nr.h                                                *
 ** Author:         Cherry.Liu                                             *
 ** DATE:           05/29/2015                                               *
 ** Copyright:      2015 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 05/29/2015       Cherry.Liu       Create.                                  *
 ******************************************************************************/
#ifndef audio_record_nr_h
#define audio_record_nr_h
typedef short int16;
void audio_record_nr_init(int16
                          *audio_recorder_nr_nv_ptr);/*init param length request 8 words*/
void audio_record_nr_set(int16
                         *audio_recorder_nr_nv_ptr);/*set dynamically param length request 8 words*/
void audio_record_nr(int16 *data_buf); /*data length shouble be 480 words!!!*/
void audio_record_nr_stereo(int16 *in_buf,
                            int16 *in_buf_r);/*data length shouble be 480 words!!!!*/
#endif
