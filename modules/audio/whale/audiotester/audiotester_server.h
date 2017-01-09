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

#ifndef _AUDIOTESTER_SERVER_H_
#define _AUDIOTESTER_SERVER_H_
typedef enum {
    GET_INFO,
    GET_PARAM_FROM_RAM,
    SET_PARAM_FROM_RAM,
    GET_PARAM_FROM_FLASH,
    SET_PARAM_FROM_FLASH,
    GET_REGISTER,
    SET_REGISTER,
    SET_PARAM_VOLUME=14,
    SET_VOLUME_APP_TYPE=15,
    IMPORT_XML_AUDIO_PARAM=16,
    GET_CURRENT_AUDIO_PARAM=17,
    AUDIO_PCM_DUMP=18,
    UPATE_AUDIOPARAM_TIME=19,
    CLONE_XML_AUDIO_PARAM,
    CONNECT_AUDIOTESTER,
    DIS_CONNECT_AUDIOTESTER,
    HARDWARE_TEST_CMD=0xf0,
    AUDIO_EXT_TEST_CMD=0xf1,
    SOCKET_TEST_CMD=0xff,
} AUDIO_CMD;

typedef enum {
    DATA_SINGLE = 1,
    DATA_START,
    DATA_END,
    DATA_MIDDLE,
    DATA_STATUS_OK,
    DATA_STATUS_ERROR,
} AUDIO_DATA_STATE;

// This is the communication frame head
typedef struct msg_head_tag {
    unsigned int  seq_num;      // Message sequence number, used for flow control
    unsigned short
    len;          // The totoal size of the packet "sizeof(MSG_HEAD_T)+ packet size"
    unsigned char   type;         // Main command type
    unsigned char   subtype;      // Sub command type
} __attribute__((packed)) MSG_HEAD_T;

//byte 1 is data state, and other 19 bytes is reserve
typedef struct data_command {
    unsigned char data_state;
    unsigned char reserve[19];
} __attribute__((packed)) DATA_HEAD_T;


#endif
