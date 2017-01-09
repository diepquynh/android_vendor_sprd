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

#ifndef _AUDIO_LOCAL_SOCKET_H_
#define _AUDIO_LOCAL_SOCKET_H_
#define AUDIO_SOCKET_NAME "audio_local_socket"
#define LOCAL_SOCKET_BUFFER_SIZE  4096
#define LOCAL_SOCKET_CLIENT_MAX_NUMBER 8
#ifdef LOCAL_SOCKET_SERVER
#include "audio_hw.h"
void start_audio_local_server(struct tiny_audio_device *adev);
#endif

#ifdef LOCAL_SOCKET_CLIENT
#endif

#endif
