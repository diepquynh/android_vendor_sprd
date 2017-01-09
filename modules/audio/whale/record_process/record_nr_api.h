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

#ifndef _AUD_RECORD_NR_H
#define _AUD_RECORD_NR_H

#include<stdio.h>
#include"aud_proc.h"
#include <utils/Log.h>
#include"audio_record_nr.h"

typedef void *record_nr_handle;

typedef int (*CallBack)(void *stream, void *buffer, size_t bytes);

record_nr_handle AudioRecordNr_Init(int16 *pNvPara, CallBack get_data_func ,
                                    void *data_read_handle, int request_channel);
int AudioRecordNr_Proc(record_nr_handle nr_hande, void *buffer, size_t bytes);
void AudioRecordNr_Deinit(record_nr_handle nr_hande);

#endif
