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

#ifndef VBC_CONTROL_PARAMETERS_H
#define VBC_CONTROL_PARAMETERS_H

#include "pthread.h"


#define BUF_SIZE 1024

#define VBC_PIPE_NAME_MAX_LEN 16
#define VOIP_PIPE_NAME_MAX     VBC_PIPE_NAME_MAX_LEN
#define CP_NAME_MAX_LEN 10

#define MAX_CTRL_FILE 10

#define I2S_CTL_PATH_MAX      100
#define I2S_CTL_VALUE_MAX      4
#define I2S_CTL_INDEX_MAX     3
#define AUDIO_MODE_NAME_MAX_LEN 16

#define AUDIO_XML_PATH "/system/etc/audio_hw.xml"

#define MODEM_T_ENABLE_PROPERTY     "persist.modem.t.enable"
#define MODEM_W_ENABLE_PROPERTY     "persist.modem.w.enable"
#define MODEM_TDDCSFB_ENABLE_PROPERTY     "persist.modem.tl.enable"
#define MODEM_FDDCSFB_ENABLE_PROPERTY     "persist.modem.lf.enable"
#define MODEM_CSFB_ENABLE_PROPERTY           "persist.modem.l.enable"

#define AUDIO_NV_FM_GAINL_INDEX         18
#define AUDIO_NV_FM_DGAIN_INDEX         19
#define AUDIO_NV_CAPTURE_GAIN_INDEX     43
#define AUDIO_NV_INTPA_SWITCH_INDEX     44
#define AUDIO_NV_INTPA_GAIN_INDEX       45
#define AUDIO_NV_LINEIN_GAIN_INDEX      46
#define AUDIO_NV_LINEIN_APP_CONFIG_INFO      1
#define AUDIO_NV_PLAYBACK_APP_CONFIG_INFO      0

/* list vbc cmds */
enum VBC_CMD_E
{
    VBC_CMD_NONE = 0,
    /* current mode and volume gain parameters.*/
    VBC_CMD_SET_MODE = 1,
    VBC_CMD_RSP_MODE = 2,
    VBC_CMD_SET_GAIN = 3,
    VBC_CMD_RSP_GAIN = 4,
    /* whether switch vb control to dsp parameters.*/
    VBC_CMD_SWITCH_CTRL = 5,
    VBC_CMD_RSP_SWITCH = 6,
    /* whether mute or not.*/
    VBC_CMD_SET_MUTE = 7,
    VBC_CMD_RSP_MUTE = 8,
    /* open/close device parameters.*/
    VBC_CMD_DEVICE_CTRL = 9,
    VBC_CMD_RSP_DEVICE = 10,

    VBC_CMD_HAL_OPEN = 11,
    VBC_CMD_RSP_OPEN  =12,

    VBC_CMD_HAL_CLOSE = 13,
    VBC_CMD_RSP_CLOSE = 14,

    VBC_CMD_SET_SAMPLERATE = 15,
    VBC_CMD_RSP_SAMPLERATE = 16,

    VBC_CMD_LINEIN_CTRL = 17,
    VBC_CMD_LINEIN_RSP_CTRL = 18,

    VBC_CMD_GET_HP_POLE_TYPE = 19,
    VBC_CMD_RSP_HP_POLE_TYPE = 20,

    VBC_CMD_MAX
};
typedef enum {
    CP_W,
    CP_TG,
    CP_CSFB,
    AP_TYPE = 3 , //the num of audio_hw.xml must be adjusted  at once
    CP_MAX,
    FM_IIS //add for i2s_pin_mux for fm
}cp_type_t;

typedef enum FuntionType{
    BTCALL=0,
    EXTSPK,
    EXT_DG_FM
}FUN_TYPE;
/*support multiple call for multiple modem(cp0/cp1/...):
different modem is corresponding to different pipe and all pipes use the only vbc.
support multiple pipe:
1. change VBC_PIPE_COUNT
2. change the definition of s_vbc_ctrl_pipe_info.
3. change channel_id for different cp .On sharp, 0 for cp0,  1 for cp1,2 for ap
*/

typedef struct
{
    char s_vbc_ctrl_pipe_name[VBC_PIPE_NAME_MAX_LEN];
    int channel_id;
    cp_type_t cp_type;
   char cp_name[CP_NAME_MAX_LEN];
    int cpu_index;
}vbc_ctrl_pipe_para_t;


struct voip_res
{
    cp_type_t cp_type;
    int8_t  pipe_name[VOIP_PIPE_NAME_MAX];
    int  channel_id;
    int enable;
    int is_done;
   void *adev;
    pthread_t thread_id;
};


  typedef struct ctrl_node
  {
       int8_t ctrl_path[I2S_CTL_PATH_MAX];
	int ctrl_file_fd;
	int8_t ctrl_value[4];
	struct ctrl_node *next;
  }ctrl_node;


typedef struct
{
   ctrl_node *p_ctlr_node_head;
    int8_t cpu_index;
    int8_t i2s_index;
    int is_switch;
    int8_t is_ext;
    int max_num;//smartpa add
}i2s_ctl_t;


typedef struct
{
    int num;
    i2s_ctl_t *i2s_ctl_info;
    int cpu_index;
}i2s_bt_t;


typedef struct debuginfo
{
    int enable;
    int sleeptime_gate;
    int pcmwritetime_gate;
    int lastthis_outwritetime_gate;
}debuginfo;

typedef struct{
    int num;
    vbc_ctrl_pipe_para_t *vbc_ctrl_pipe_info;
    i2s_ctl_t *i2s_extspk;
    i2s_ctl_t *cur_i2s_extspk;
    ctrl_node *i2s_fm;
    struct voip_res  voip_res;
    debuginfo debug_info;
    ctrl_node *i2s_btsco_fm;
}audio_modem_t;

/*audio mode structure,we can expand  for more fields if necessary*/
typedef struct
{
	int index;
    char mode_name[NAME_LEN_MAX];

}audio_mode_item_t;

/*we mostly have four mode,(headset,headfree,handset,handsfree),
    differet product may configure different mode number,htc have 25 modes.*/
typedef struct{
	int num;
	audio_mode_item_t *audio_mode_item_info;
}aud_mode_t;
struct modem_config_parse_state{
	audio_modem_t *modem_info;
	vbc_ctrl_pipe_para_t *vbc_ctrl_pipe_info;
	aud_mode_t  *audio_mode_info;
	audio_mode_item_t *audio_mode_item_info;
	i2s_bt_t  *i2s_btcall_info;
    i2s_ctl_t *i2s_ctl_info;
    char* cp_nbio_pipe;
    int fm_type;
};

#endif
