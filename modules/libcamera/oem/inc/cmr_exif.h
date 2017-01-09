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
#ifndef  _CMR_EXIF_H_
#define _CMR_EXIF_H_


#include "jpeg_exif_header.h"
#include "cmr_setting.h"


cmr_int cmr_exif_init(JINF_EXIF_INFO_T *jinf_exif_info_ptr, setting_get_pic_taking_cb setting_cb, void *priv_data);

#endif
