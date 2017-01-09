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
#ifndef __VB_EFFECT_IF_H__
#define __VB_EFFECT_IF_H__

#include <tinyalsa/asoundlib.h>
#include <eng_audio.h>

/*
 * Success if return 0
 */

void vb_effect_setpara(AUDIO_TOTAL_T *para);

int parse_vb_effect_params(void *audio_params_ptr, unsigned int params_size);

void vb_effect_config_mixer_ctl(struct mixer_ctl *eq_update, struct mixer_ctl *da_profile_select);

void vb_ad_effect_config_mixer_ctl(struct mixer_ctl *ad01_profile_select, struct mixer_ctl *ad23_profile_select);

void vb_da_effect_config_mixer_ctl(struct mixer_ctl *da_profile_select);

void vb_effect_sync_devices(int cur_out_devices, int cur_in_devices);

int vb_effect_loading(void);

int vb_effect_profile_apply(void);

int create_vb_effect_params(void);
int  vb_da_effect_profile_apply(int index);
int  vb_ad01_effect_profile_apply(int index);
int  vb_ad23_effect_profile_apply(int index);
#endif
