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

#ifndef AUDIO_PGA_H
#define AUDIO_PGA_H

struct audio_pga;
struct mixer;

/* Initialises and frees the audio PGA */
struct audio_pga *audio_pga_init(struct mixer *mixer);
void audio_pga_free(struct audio_pga *pga);

/* Applies an audio pga by name */
int audio_pga_apply(struct audio_pga *pga, int val, const char *name);
int get_snd_card_number(const char *card_name);

/*
 * for PGA tuning by audiotester
 */
#include "eng_audio.h"
int SetAudio_pga_parameter_eng(AUDIO_TOTAL_T *aud_params_ptr, unsigned int params_size, uint32_t vol_level);

#endif
