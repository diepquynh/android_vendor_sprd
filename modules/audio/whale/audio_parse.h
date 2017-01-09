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

#ifndef __AUDIO_PARSE_H
#define __AUDIO_PARSE_H
#include "audio_control.h"
#ifdef __cplusplus
extern "C" {
#endif

 int parse_audio_route(struct audio_control *control);
 int parse_audio_pcm_config(struct pcm_handle_t *pcm);
 int init_sprd_codec_mixer(struct sprd_codec_mixer_t *codec,struct mixer *mixer);
 void free_private_control(struct private_control *pri);
void free_device_route(struct device_route_handler *reoute_handler);

#ifdef __cplusplus
}
#endif

#endif