/*
 * Copyright (C) 2012 The Android Open Source Project *
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

#define LOG_TAG    "vb_effect"

#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#include <system/audio.h>

#include <sys/mman.h>
#include <eng_audio.h>
#include <stdint.h>
#include <cutils/log.h>
#include "aud_enha.h"
#include "vb_effect_if.h"
#include <tinyalsa/asoundlib.h>
#include "string_exchange_bin.h"

#define VBC_VERSION     "vbc.r0p0"

#define VBC_EQ_FIRMWARE_MAGIC_LEN       (4)
#define VBC_EQ_FIRMWARE_MAGIC_ID        ("VBEQ")
#define VBC_EQ_PROFILE_VERSION          (0x00000001)
#define VBC_EQ_PROFILE_CNT_MAX          (50)
#define VBC_EQ_PROFILE_NAME_MAX         (32)
/* about 61 registers*/
#define VBC_EFFECT_PARAS_LEN            (61)
#define VBC_EFFECT_PROFILE_CNT          (4)

#define STORED_VBC_EFFECT_PARAS_PATH    "/data/local/media/vbc_eq"

/* ALSA cards for sprd */
#define CARD_SPRDPHONE "sprdphone"
/* mixer control */
#define MIXER_CTL_VBC_EQ_UPDATE            "VBC EQ Update"
#define MIXER_CTL_VBC_EQ_PROFILE_SELECT    "VBC EQ Profile Select"

struct vbc_fw_header {
    char magic[VBC_EQ_FIRMWARE_MAGIC_LEN];
    uint32_t profile_version;
    uint32_t num_profile;
};

struct vbc_eq_profile {
    char magic[VBC_EQ_FIRMWARE_MAGIC_LEN];
    char name[VBC_EQ_PROFILE_NAME_MAX];
    /* FIXME */
    uint32_t effect_paras[VBC_EFFECT_PARAS_LEN];
};

static const uint32_t vbc_reg_default[VBC_EFFECT_PARAS_LEN] = {
    0x0,       /*DAPATCHCTL*/
    0x1818,    /*DADGCTL   */
    0x7F,      /*DAHPCTL   */
    0x0,       /*DAALCCTL0 */
    0x0,       /*DAALCCTL1 */
    0x0,       /*DAALCCTL2 */
    0x0,       /*DAALCCTL3 */
    0x0,       /*DAALCCTL4 */
    0x0,       /*DAALCCTL5 */
    0x0,       /*DAALCCTL6 */
    0x0,       /*DAALCCTL7 */
    0x0,       /*DAALCCTL8 */
    0x0,       /*DAALCCTL9 */
    0x0,       /*DAALCCTL10*/
    0x183,     /*STCTL0    */
    0x183,     /*STCTL1    */
    0x0,       /*ADPATCHCTL*/
    0x1818,    /*ADDGCTL   */
    0x0,       /*HPCOEF0   */
    0x0,       /*HPCOEF1   */
    0x0,       /*HPCOEF2   */
    0x0,       /*HPCOEF3   */
    0x0,       /*HPCOEF4   */
    0x0,       /*HPCOEF5   */
    0x0,       /*HPCOEF6   */
    0x0,       /*HPCOEF7   */
    0x0,       /*HPCOEF8   */
    0x0,       /*HPCOEF9   */
    0x0,       /*HPCOEF10  */
    0x0,       /*HPCOEF11  */
    0x0,       /*HPCOEF12  */
    0x0,       /*HPCOEF13  */
    0x0,       /*HPCOEF14  */
    0x0,       /*HPCOEF15  */
    0x0,       /*HPCOEF16  */
    0x0,       /*HPCOEF17  */
    0x0,       /*HPCOEF18  */
    0x0,       /*HPCOEF19  */
    0x0,       /*HPCOEF20  */
    0x0,       /*HPCOEF21  */
    0x0,       /*HPCOEF22  */
    0x0,       /*HPCOEF23  */
    0x0,       /*HPCOEF24  */
    0x0,       /*HPCOEF25  */
    0x0,       /*HPCOEF26  */
    0x0,       /*HPCOEF27  */
    0x0,       /*HPCOEF28  */
    0x0,       /*HPCOEF29  */
    0x0,       /*HPCOEF30  */
    0x0,       /*HPCOEF31  */
    0x0,       /*HPCOEF32  */
    0x0,       /*HPCOEF33  */
    0x0,       /*HPCOEF34  */
    0x0,       /*HPCOEF35  */
    0x0,       /*HPCOEF36  */
    0x0,       /*HPCOEF37  */
    0x0,       /*HPCOEF38  */
    0x0,       /*HPCOEF39  */
    0x0,       /*HPCOEF40  */
    0x0,       /*HPCOEF41  */
    0x0,       /*HPCOEF42  */
};

static int fd_src_paras;
static FILE *  fd_dest_paras;

/*
 * for VBC EQ tuning by audiotester
 */
static struct mixer_ctl *s_ctl_eq_update = NULL;
static struct mixer_ctl *s_ctl_eq_select = NULL;
static int s_cur_devices = 0;
static AUDIO_TOTAL_T * s_vb_effect_ptr = NULL;

extern int get_snd_card_number(const char *card_name);
//get audio nv struct
static AUDIO_TOTAL_T *get_aud_paras();
static int do_parse(AUDIO_TOTAL_T *aud_ptr, unsigned int size);

static int do_parse(AUDIO_TOTAL_T *audio_params_ptr, unsigned int params_size)
{
    AUDIO_TOTAL_T *temp_params_ptr = NULL;
    AUDIO_TOTAL_T *cur_params_ptr = NULL;
    struct vbc_fw_header  *fw_header;
    struct vbc_eq_profile  *effect_profile;
    uint32_t i = 0;

    if (NULL == audio_params_ptr) {
       ALOGE(" Error: audio_params_ptr is NULL.");
       return -1;
    }
    if (4*sizeof(AUDIO_TOTAL_T) != params_size) {
        ALOGE("Error: params_size = %d, total size = %d", params_size, 4*sizeof(AUDIO_TOTAL_T));
        return -1;
    }

    fw_header = (struct vbc_fw_header *) malloc(sizeof(struct vbc_fw_header));
    effect_profile = (struct vbc_eq_profile *)malloc(sizeof(struct vbc_eq_profile));
    if ((fw_header != NULL) && (effect_profile != NULL)) {
        memset(fw_header, 0, sizeof(struct vbc_fw_header));
        memset(effect_profile, 0, sizeof(struct vbc_eq_profile));
    } else {
        ALOGE("Error: malloc failed for internal struct.");
        if (fw_header)
            free(fw_header);
        if (effect_profile)
            free(effect_profile);
        return -1;
    }
    ALOGI("do_parse...start");
    //audio para nv file--> fd_src
    //vb effect paras file-->fd_dest
    fd_dest_paras = fopen(STORED_VBC_EFFECT_PARAS_PATH, "wb");
    if (NULL  == fd_dest_paras) {
        free(fw_header);
        free(effect_profile);
        ALOGE("file %s open failed:%s", STORED_VBC_EFFECT_PARAS_PATH, strerror(errno));
        return -1;
    }
    //init temp buffer for paras calculated.

    memcpy(fw_header->magic, VBC_EQ_FIRMWARE_MAGIC_ID, VBC_EQ_FIRMWARE_MAGIC_LEN);
    fw_header->profile_version = VBC_EQ_PROFILE_VERSION;
    fw_header->num_profile = VBC_EFFECT_PROFILE_CNT; //TODO

    ALOGI("fd_dest_paras(0x%x), header_len(%d), profile_len(%d)", (unsigned int)fd_dest_paras,
         sizeof(struct vbc_fw_header), sizeof(struct vbc_eq_profile));
    //write dest file header
    fwrite(fw_header, sizeof(struct vbc_fw_header), 1, fd_dest_paras);
    temp_params_ptr = audio_params_ptr;
    for (i=0; i<fw_header->num_profile; i++) {
        cur_params_ptr = temp_params_ptr + i;
        //reset the paras buffer and copy default register value.
        memset(effect_profile, 0, sizeof(struct vbc_eq_profile));
        memcpy(effect_profile->effect_paras, &vbc_reg_default[0], sizeof(vbc_reg_default));
        //set paras to buffer.
        AUDENHA_SetPara(cur_params_ptr, effect_profile->effect_paras);
        //write buffer to stored file.
        memcpy(effect_profile->magic, VBC_EQ_FIRMWARE_MAGIC_ID, VBC_EQ_FIRMWARE_MAGIC_LEN);
        memcpy(effect_profile->name, cur_params_ptr->audio_nv_arm_mode_info.ucModeName, 16);
        //strcpy(effect_profile->name, cur_params_ptr->audio_nv_arm_mode_info.ucModeName);
        ALOGI("effect_profile->name is %s", effect_profile->name);
        fwrite(effect_profile, sizeof(struct vbc_eq_profile), 1, fd_dest_paras);
    }
    fclose(fd_dest_paras);
    free(fw_header);
    free(effect_profile);
    ALOGI("do_parse...end");
    return 0;
}

/* to initialize vbc eq parameters at productinfo
   soft link to vendor/firmware/vbc_eq
   when system boot
*/
int create_vb_effect_params(void)
{
    AUDIO_TOTAL_T * aud_params_ptr = NULL;
    int ret = -1;
    ALOGI("create_vb_effect_params...start");

    //read audio params from source file.
    aud_params_ptr = get_aud_paras();

    //close fd
    if (aud_params_ptr) {
        ret = do_parse(aud_params_ptr, adev_get_audiomodenum4eng()*sizeof(AUDIO_TOTAL_T));
    }

    ALOGI("create_vb_effect_params...done");
    return ret;
}

static AUDIO_TOTAL_T *get_aud_paras()
{
    return s_vb_effect_ptr;
}
void vb_effect_setpara(AUDIO_TOTAL_T *para)
{
    s_vb_effect_ptr = para;
}

void vb_effect_config_mixer_ctl(struct mixer_ctl *eq_update, struct mixer_ctl *profile_select)
{
    s_ctl_eq_update = eq_update;
    s_ctl_eq_select = profile_select;
}
void vb_da_effect_config_mixer_ctl(struct mixer_ctl *da_profile_select)
{
	return;
}

void vb_ad_effect_config_mixer_ctl(struct mixer_ctl *ad01_profile_select, struct mixer_ctl *ad23_profile_select)
{
	return;
}
void vb_effect_sync_devices(int cur_devices)
{
    s_cur_devices = cur_devices;
}

int vb_effect_loading(void)
{
    int ret = -1;

    if (s_ctl_eq_update) {
        ret = mixer_ctl_set_enum_by_string(s_ctl_eq_update, "loading");
        ALOGI("vb_effect_loading, ret(%d)", ret);
    }
    else
        ALOGW("warning: s_ctl_eq_update is NULL");

    return ret;
}

int vb_effect_profile_apply(void)
{
    int ret = 0;
    ALOGI("s_cur_devices(0x%08x)", s_cur_devices);

    if (s_ctl_eq_select) {

        if(((s_cur_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET)&&(s_cur_devices & AUDIO_DEVICE_OUT_SPEAKER))
               ||((s_cur_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE)&&(s_cur_devices & AUDIO_DEVICE_OUT_SPEAKER))){
            //ret = mixer_ctl_set_enum_by_string(s_ctl_eq_select, "Headfree");
            ret = mixer_ctl_set_value(s_ctl_eq_select, 0, 1);
            ALOGI("profile is Headfree, ret=%d", ret);
        }else if(s_cur_devices & AUDIO_DEVICE_OUT_EARPIECE){
            //ret = mixer_ctl_set_enum_by_string(s_ctl_eq_select, "Handset");
            ret = mixer_ctl_set_value(s_ctl_eq_select, 0, 2);
            ALOGI("profile is Handset, ret=%d", ret);
        }else if((s_cur_devices & AUDIO_DEVICE_OUT_SPEAKER)
                ||(s_cur_devices & AUDIO_DEVICE_OUT_FM_SPEAKER)){
            //ret = mixer_ctl_set_enum_by_string(s_ctl_eq_select, "Handsfree");
            ret = mixer_ctl_set_value(s_ctl_eq_select, 0, 3);
            ALOGI("profile is Handsfree, ret=%d", ret);
        }else if((s_cur_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET)
                ||(s_cur_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE)
                ||(s_cur_devices & AUDIO_DEVICE_OUT_FM_HEADSET)
                ||(s_cur_devices & AUDIO_DEVICE_IN_WIRED_HEADSET)){
            //ret = mixer_ctl_set_enum_by_string(s_ctl_eq_select, "Headset");
            ret = mixer_ctl_set_value(s_ctl_eq_select, 0, 0);
            ALOGI("profile is Headset, ret=%d", ret);
        }else{
            ALOGE("s_cur_devices(0x%08x) IS NOT SUPPORT!\n", s_cur_devices);
            return -1;
        }
        return 0;
    }
    ALOGW("Warning: EQ Mixer select control is NULL.");
    return -1;
}

int parse_vb_effect_params(void *audio_params_ptr, unsigned int params_size)
{
    struct mixer *mixer;
    struct mixer_ctl *eq_update;
    int ret;
    int card_id;

    card_id = get_snd_card_number(CARD_SPRDPHONE);
    ALOGI("card_id = %d", card_id);
    if (card_id < 0) return -1;
    mixer = mixer_open(card_id);
    if (!mixer) {
        ALOGE("Failed to open mixer");
        return -1;
    }
    eq_update = mixer_get_ctl_by_name(mixer, MIXER_CTL_VBC_EQ_UPDATE);
     
    do_parse((AUDIO_TOTAL_T *) audio_params_ptr, params_size);
    
    //Loading and enable vb effect.
    ret = mixer_ctl_set_enum_by_string(eq_update, "loading");
    ALOGI("parse_vb_effect_params, ret(%d)", ret);
    
    mixer_close(mixer);
    
    return 0;
}
