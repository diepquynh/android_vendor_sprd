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


#ifndef AUDIO_DEVICE_OUT_FM_HEADSET
#define AUDIO_DEVICE_OUT_FM_HEADSET 0x1000000
#endif
#ifndef AUDIO_DEVICE_OUT_FM_SPEAKER
#define AUDIO_DEVICE_OUT_FM_SPEAKER 0x2000000
#endif

#define VBC_VERSION     "vbc.r0p0"

#define VBC_EQ_FIRMWARE_MAGIC_LEN       (4)
#define VBC_EQ_FIRMWARE_MAGIC_ID        ("VBEQ")
#define VBC_EQ_PROFILE_VERSION          (0x00000002)
#define VBC_EQ_PROFILE_CNT_MAX          (50)
#define VBC_EQ_PROFILE_NAME_MAX         (32)
/* about 61 registers*/
#define VBC_EFFECT_PARAS_LEN            (336)
#define VBC_EFFECT_PROFILE_CNT          (4)

#define STORED_VBC_EFFECT_PARAS_PATH    "/data/local/media/vbc_eq"

/* ALSA cards for sprd */
#define CARD_SPRDPHONE "sprdphone"
/* mixer control */
#define MIXER_CTL_VBC_EQ_UPDATE            "VBC EQ Update"
#define MIXER_CTL_VBC_EQ_PROFILE_SELECT    "VBC EQ Profile Select"

struct vbc_fw_header {
    char magic[VBC_EQ_FIRMWARE_MAGIC_LEN];
    unsigned profile_version;
    unsigned num_profile; /*total num =  num_da + num_ad01 + num_ad23  */
    unsigned num_da;   /*DA profile num*/
    unsigned num_ad01; /*ad01 profile num*/
    unsigned num_ad23; /*ad23 profile num*/

};
#define VBC_AD_CTL_PARAS_LEN            (2)

struct vbc_eq_profile {
    char magic[VBC_EQ_FIRMWARE_MAGIC_LEN];
    char name[VBC_EQ_PROFILE_NAME_MAX];
    /* FIXME */
    unsigned effect_paras[VBC_EFFECT_PARAS_LEN];
    unsigned ad_ctl_paras[VBC_AD_CTL_PARAS_LEN];
};
#define VBC_DA_EFFECT_PARAS_LEN            (20+72*2)

struct vbc_da_eq_profile {
    char magic[VBC_EQ_FIRMWARE_MAGIC_LEN];
    char name[VBC_EQ_PROFILE_NAME_MAX];
    unsigned effect_paras[VBC_DA_EFFECT_PARAS_LEN];
};

#define VBC_AD_EFFECT_PARAS_LEN             (2+ 43*2 )

struct vbc_ad_eq_profile{
    char magic[VBC_EQ_FIRMWARE_MAGIC_LEN];
    char name[VBC_EQ_PROFILE_NAME_MAX];
    unsigned effect_paras[VBC_AD_EFFECT_PARAS_LEN];
};
static const unsigned vbc_ad_ctrl_reg_default[VBC_AD_CTL_PARAS_LEN] = {
    0x0, 	 /*  ADPATCHCTL      */
    0x0,    /*  ADHPCTL         */
};

static const unsigned vbc_reg_default[VBC_EFFECT_PARAS_LEN] = {
    0x0,       /*DAPATCHCTL*/
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
    0x00000000,	 /*  DACSRCCTL       */
    0x00000000,	 /*  MIXERCTL        */
    0x00000000,	 /*  VBNGCVTHD       */
    0x00000000,	 /*  VBNGCTTHD       */
    0x00000000,	 /*  VBNGCTL         */
    0x0,       /*HPCOEF0_H   */
    0x0,       /*HPCOEF0_L   */
    0x0,       /*HPCOEF1_H   */
    0x0,       /*HPCOEF1_L   */
    0x0,       /*HPCOEF2_H   */
    0x0,       /*HPCOEF2_L   */
    0x0,       /*HPCOEF3_H   */
    0x0,       /*HPCOEF3_L   */
    0x0,       /*HPCOEF4_H   */
    0x0,       /*HPCOEF4_L   */
    0x0,       /*HPCOEF5_H   */
    0x0,       /*HPCOEF5_L   */
    0x0,       /*HPCOEF6_H   */
    0x0,       /*HPCOEF6_L   */
    0x0,       /*HPCOEF7_H   */
    0x0,       /*HPCOEF7_L   */
    0x0,       /*HPCOEF8_H   */
    0x0,       /*HPCOEF8_L   */
    0x0,       /*HPCOEF9_H   */
    0x0,       /*HPCOEF9_L   */
    0x0,       /*HPCOEF10_H  */
    0x0,       /*HPCOEF10_L  */
    0x0,       /*HPCOEF11_H  */
    0x0,       /*HPCOEF11_L  */
    0x0,       /*HPCOEF12_H  */
    0x0,       /*HPCOEF12_L  */
    0x0,       /*HPCOEF13_H  */
    0x0,       /*HPCOEF13_L  */
    0x0,       /*HPCOEF14_H  */
    0x0,       /*HPCOEF14_L  */
    0x0,       /*HPCOEF15_H  */
    0x0,       /*HPCOEF15_L  */
    0x0,       /*HPCOEF16_H  */
    0x0,       /*HPCOEF16_L  */
    0x0,       /*HPCOEF17_H  */
    0x0,       /*HPCOEF17_L  */
    0x0,       /*HPCOEF18_H  */
    0x0,       /*HPCOEF18_L  */
    0x0,       /*HPCOEF19_H  */
    0x0,       /*HPCOEF19_L  */
    0x0,       /*HPCOEF20_H  */
    0x0,       /*HPCOEF20_L  */
    0x0,       /*HPCOEF21_H  */
    0x0,       /* HPCOEF21_L */
    0x0,       /* HPCOEF22_H */
    0x0,       /* HPCOEF22_L */
    0x0,       /* HPCOEF23_H */
    0x0,       /* HPCOEF23_L */
    0x0,       /* HPCOEF24_H */
    0x0,       /* HPCOEF24_L */
    0x0,       /* HPCOEF25_H */
    0x0,       /* HPCOEF25_L */
    0x0,       /* HPCOEF26_H */
    0x0,       /* HPCOEF26_L */
    0x0,       /* HPCOEF27_H */
    0x0,       /* HPCOEF27_L */
    0x0,       /* HPCOEF28_H */
    0x0,       /* HPCOEF28_L */
    0x0,       /* HPCOEF29_H */
    0x0,       /* HPCOEF29_L */
    0x0,       /* HPCOEF30_H */
    0x0,       /* HPCOEF30_L */
    0x0,       /* HPCOEF31_H */
    0x0,       /* HPCOEF31_L */
    0x0,       /* HPCOEF32_H */
    0x0,       /* HPCOEF32_L */
    0x0,       /* HPCOEF33_H */
    0x0,       /* HPCOEF33_L */
    0x0,       /* HPCOEF34_H */
    0x0,       /* HPCOEF34_L */
    0x0,       /* HPCOEF35_H */
    0x0,       /* HPCOEF35_L */
    0x0,       /* HPCOEF36_H */
    0x0,       /* HPCOEF36_L */
    0x0,       /* HPCOEF37_H */
    0x0,       /* HPCOEF37_L */
    0x0,       /* HPCOEF38_H */
    0x0,       /* HPCOEF38_L */
    0x0,       /* HPCOEF39_H */
    0x0,       /* HPCOEF39_L */
    0x0,       /* HPCOEF40_H */
    0x0,       /* HPCOEF40_L */
    0x0,       /* HPCOEF41_H */
    0x0,       /* HPCOEF41_L */
    0x0,       /* HPCOEF42_H */
    0x0,       /* HPCOEF42_L */
    0x1000,       /*HPCOEF43_H ;         */
    0x0,       /*HPCOEF43_L ;         */
    0x4000,       /*HPCOEF44_H ;         */
    0x0,       /*HPCOEF44_L ;         */
    0x4000,       /*HPCOEF45_H ;         */
    0x0,       /*HPCOEF45_L ;         */
    0x0,       /*HPCOEF46_H ;         */
    0x0,       /*HPCOEF46_L ;         */
    0x0,       /*HPCOEF47_H ;         */
    0x0,       /*HPCOEF47_L ;         */
    0x0,       /*HPCOEF48_H ;         */
    0x0,       /*HPCOEF48_L ;         */
    0x0,       /*HPCOEF49_H ;         */
    0x0,       /*HPCOEF49_L ;         */
    0x1000,       /*HPCOEF50_H ;         */
    0x0,       /*HPCOEF50_L ;         */
    0x4000,       /*HPCOEF51_H ;         */
    0x0,       /*HPCOEF51_L ;         */
    0x4000,       /*HPCOEF52_H ;         */
    0x0,       /*HPCOEF52_L ;         */
    0x0,       /*HPCOEF53_H ;         */
    0x0,       /*HPCOEF53_L ;         */
    0x0,       /*HPCOEF54_H ;         */
    0x0,       /*HPCOEF54_L ;         */
    0x0,       /*HPCOEF55_H ;         */
    0x0,       /*HPCOEF55_L ;         */
    0x0,       /*HPCOEF56_H ;         */
    0x0,       /*HPCOEF56_L ;         */
    0x1000,       /*HPCOEF57_H ;         */
    0x0,       /*HPCOEF57_L ;         */
    0x4000,       /*HPCOEF58_H ;         */
    0x0,       /*HPCOEF58_L ;         */
    0x4000,       /*HPCOEF59_H ;         */
    0x0,       /*HPCOEF59_L ;         */
    0x0,       /*HPCOEF60_H ;         */
    0x0,       /*HPCOEF60_L ;         */
    0x0,       /*HPCOEF61_H ;         */
    0x0,       /*HPCOEF61_L ;         */
    0x0,       /*HPCOEF62_H ;         */
    0x0,       /*HPCOEF62_L ;         */
    0x0,       /*HPCOEF63_H ;         */
    0x0,       /*HPCOEF63_L ;         */
    0x1000,       /*HPCOEF64_H ;         */
    0x0,       /*HPCOEF64_L ;         */
    0x4000,       /*HPCOEF65_H ;         */
    0x0,       /*HPCOEF65_L ;         */
    0x4000,       /*HPCOEF66_H ;         */
    0x0,       /*HPCOEF66_L ;         */
    0x0,       /*HPCOEF67_H ;         */
    0x0,       /*HPCOEF67_L ;         */
    0x0,       /*HPCOEF68_H ;         */
    0x0,       /*HPCOEF68_L ;         */
    0x0,       /*HPCOEF69_H ;         */
    0x0,       /*HPCOEF69_L ;         */
    0x0,       /*HPCOEF70_H ;         */
    0x0,       /*HPCOEF70_L ;         */
    0x1000,       /*HPCOEF71_H ;         */
    0x0,       /*HPCOEF71_L ;         */
    0x0,       /*ADC01_HPCOEF0_H      */
    0x0,       /*ADC01_HPCOEF0_L      */
    0x0,       /*ADC01_HPCOEF1_H      */
    0x0,       /*ADC01_HPCOEF1_L      */
    0x0,       /*ADC01_HPCOEF2_H      */
    0x0,       /*ADC01_HPCOEF2_L      */
    0x0,       /*ADC01_HPCOEF3_H      */
    0x0,       /*ADC01_HPCOEF3_L      */
    0x0,       /*ADC01_HPCOEF4_H      */
    0x0,       /*ADC01_HPCOEF4_L      */
    0x0,       /*ADC01_HPCOEF5_H      */
    0x0,       /*ADC01_HPCOEF5_L      */
    0x0,       /*ADC01_HPCOEF6_H      */
    0x0,       /*ADC01_HPCOEF6_L      */
    0x0,       /*ADC01_HPCOEF7_H      */
    0x0,       /*ADC01_HPCOEF7_L      */
    0x0,       /*ADC01_HPCOEF8_H      */
    0x0,       /*ADC01_HPCOEF8_L      */
    0x0,       /*ADC01_HPCOEF9_H      */
    0x0,       /*ADC01_HPCOEF9_L      */
    0x0,       /*ADC01_HPCOEF10_H     */
    0x0,       /*ADC01_HPCOEF10_L     */
    0x0,       /*ADC01_HPCOEF11_H     */
    0x0,       /*ADC01_HPCOEF11_L     */
    0x0,       /*ADC01_HPCOEF12_H     */
    0x0,       /*ADC01_HPCOEF12_L     */
    0x0,       /*ADC01_HPCOEF13_H     */
    0x0,       /*ADC01_HPCOEF13_L     */
    0x0,       /*ADC01_HPCOEF14_H     */
    0x0,       /*ADC01_HPCOEF14_L     */
    0x0,       /*ADC01_HPCOEF15_H     */
    0x0,       /*ADC01_HPCOEF15_L     */
    0x0,       /*ADC01_HPCOEF16_H     */
    0x0,       /*ADC01_HPCOEF16_L     */
    0x0,       /*ADC01_HPCOEF17_H     */
    0x0,       /*ADC01_HPCOEF17_L     */
    0x0,       /*ADC01_HPCOEF18_H     */
    0x0,       /*ADC01_HPCOEF18_L     */
    0x0,       /*ADC01_HPCOEF19_H     */
    0x0,       /*ADC01_HPCOEF19_L     */
    0x0,       /*ADC01_HPCOEF20_H     */
    0x0,       /*ADC01_HPCOEF20_L     */
    0x0,       /*ADC01_HPCOEF21_H     */
    0x0,       /*ADC01_HPCOEF21_L     */
    0x0,       /*ADC01_HPCOEF22_H     */
    0x0,       /*ADC01_HPCOEF22_L     */
    0x0,       /*ADC01_HPCOEF23_H     */
    0x0,       /*ADC01_HPCOEF23_L     */
    0x0,       /*ADC01_HPCOEF24_H     */
    0x0,       /*ADC01_HPCOEF24_L     */
    0x0,       /*ADC01_HPCOEF25_H     */
    0x0,       /*ADC01_HPCOEF25_L     */
    0x0,       /*ADC01_HPCOEF26_H     */
    0x0,       /*ADC01_HPCOEF26_L     */
    0x0,       /*ADC01_HPCOEF27_H     */
    0x0,       /*ADC01_HPCOEF27_L     */
    0x0,       /*ADC01_HPCOEF28_H     */
    0x0,       /*ADC01_HPCOEF28_L     */
    0x0,       /*ADC01_HPCOEF29_H     */
    0x0,       /*ADC01_HPCOEF29_L     */
    0x0,       /*ADC01_HPCOEF30_H     */
    0x0,       /*ADC01_HPCOEF30_L     */
    0x0,       /*ADC01_HPCOEF31_H     */
    0x0,       /*ADC01_HPCOEF31_L     */
    0x0,       /*ADC01_HPCOEF32_H     */
    0x0,       /*ADC01_HPCOEF32_L     */
    0x0,       /*ADC01_HPCOEF33_H     */
    0x0,       /*ADC01_HPCOEF33_L     */
    0x0,       /*ADC01_HPCOEF34_H     */
    0x0,       /*ADC01_HPCOEF34_L     */
    0x0,       /*ADC01_HPCOEF35_H     */
    0x0,       /*ADC01_HPCOEF35_L     */
    0x0,       /*ADC01_HPCOEF36_H     */
    0x0,       /*ADC01_HPCOEF36_L     */
    0x0,       /*ADC01_HPCOEF37_H     */
    0x0,       /*ADC01_HPCOEF37_L     */
    0x0,       /*ADC01_HPCOEF38_H     */
    0x0,       /*ADC01_HPCOEF38_L     */
    0x0,       /*ADC01_HPCOEF39_H     */
    0x0,       /*ADC01_HPCOEF39_L     */
    0x0,       /*ADC01_HPCOEF40_H     */
    0x0,       /*ADC01_HPCOEF40_L     */
    0x0,       /*ADC01_HPCOEF41_H     */
    0x0,       /*ADC01_HPCOEF41_L     */
    0x0,       /*ADC01_HPCOEF42_H     */
    0x0,       /*ADC01_HPCOEF42_L     */
    0x0,       /*ADC23_HPCOEF0_H      */
    0x0,       /*ADC23_HPCOEF0_L      */
    0x0,       /*ADC23_HPCOEF1_H      */
    0x0,       /*ADC23_HPCOEF1_L      */
    0x0,       /*ADC23_HPCOEF2_H      */
    0x0,       /*ADC23_HPCOEF2_L      */
    0x0,       /*ADC23_HPCOEF3_H      */
    0x0,       /*ADC23_HPCOEF3_L      */
    0x0,       /*ADC23_HPCOEF4_H      */
    0x0,       /*ADC23_HPCOEF4_L      */
    0x0,       /*ADC23_HPCOEF5_H      */
    0x0,       /*ADC23_HPCOEF5_L      */
    0x0,       /*ADC23_HPCOEF6_H      */
    0x0,       /*ADC23_HPCOEF6_L      */
    0x0,       /*ADC23_HPCOEF7_H      */
    0x0,       /*ADC23_HPCOEF7_L      */
    0x0,       /*ADC23_HPCOEF8_H      */
    0x0,       /*ADC23_HPCOEF8_L      */
    0x0,       /*ADC23_HPCOEF9_H      */
    0x0,       /*ADC23_HPCOEF9_L      */
    0x0,       /*ADC23_HPCOEF10_H     */
    0x0,       /*ADC23_HPCOEF10_L     */
    0x0,       /*ADC23_HPCOEF11_H     */
    0x0,       /*ADC23_HPCOEF11_L     */
    0x0,       /*ADC23_HPCOEF12_H     */
    0x0,       /*ADC23_HPCOEF12_L     */
    0x0,       /*ADC23_HPCOEF13_H     */
    0x0,       /*ADC23_HPCOEF13_L     */
    0x0,       /*ADC23_HPCOEF14_H     */
    0x0,       /*ADC23_HPCOEF14_L     */
    0x0,       /*ADC23_HPCOEF15_H     */
    0x0,       /*ADC23_HPCOEF15_L     */
    0x0,       /*ADC23_HPCOEF16_H     */
    0x0,       /*ADC23_HPCOEF16_L     */
    0x0,       /*ADC23_HPCOEF17_H     */
    0x0,       /*ADC23_HPCOEF17_L     */
    0x0,       /*ADC23_HPCOEF18_H     */
    0x0,       /*ADC23_HPCOEF18_L     */
    0x0,       /*ADC23_HPCOEF19_H     */
    0x0,       /*ADC23_HPCOEF19_L     */
    0x0,       /*ADC23_HPCOEF20_H     */
    0x0,       /*ADC23_HPCOEF20_L     */
    0x0,       /*ADC23_HPCOEF21_H     */
    0x0,       /*ADC23_HPCOEF21_L     */
    0x0,       /*ADC23_HPCOEF22_H     */
    0x0,       /*ADC23_HPCOEF22_L     */
    0x0,       /*ADC23_HPCOEF23_H     */
    0x0,       /*ADC23_HPCOEF23_L     */
    0x0,       /*ADC23_HPCOEF24_H     */
    0x0,       /*ADC23_HPCOEF24_L     */
    0x0,       /*ADC23_HPCOEF25_H     */
    0x0,       /*ADC23_HPCOEF25_L     */
    0x0,       /*ADC23_HPCOEF26_H     */
    0x0,       /*ADC23_HPCOEF26_L     */
    0x0,       /*ADC23_HPCOEF27_H     */
    0x0,       /*ADC23_HPCOEF27_L     */
    0x0,       /*ADC23_HPCOEF28_H     */
    0x0,       /*ADC23_HPCOEF28_L     */
    0x0,       /*ADC23_HPCOEF29_H     */
    0x0,       /*ADC23_HPCOEF29_L     */
    0x0,       /*ADC23_HPCOEF30_H     */
    0x0,       /*ADC23_HPCOEF30_L     */
    0x0,       /*ADC23_HPCOEF31_H     */
    0x0,       /*ADC23_HPCOEF31_L     */
    0x0,       /*ADC23_HPCOEF32_H     */
    0x0,       /*ADC23_HPCOEF32_L     */
    0x0,       /*ADC23_HPCOEF33_H     */
    0x0,       /*ADC23_HPCOEF33_L     */
    0x0,       /*ADC23_HPCOEF34_H     */
    0x0,       /*ADC23_HPCOEF34_L     */
    0x0,       /*ADC23_HPCOEF35_H     */
    0x0,       /*ADC23_HPCOEF35_L     */
    0x0,       /*ADC23_HPCOEF36_H     */
    0x0,       /*ADC23_HPCOEF36_L     */
    0x0,       /*ADC23_HPCOEF37_H     */
    0x0,       /*ADC23_HPCOEF37_L     */
    0x0,       /*ADC23_HPCOEF38_H     */
    0x0,       /*ADC23_HPCOEF38_L     */
    0x0,       /*ADC23_HPCOEF39_H     */
    0x0,       /*ADC23_HPCOEF39_L     */
    0x0,       /*ADC23_HPCOEF40_H     */
    0x0,       /*ADC23_HPCOEF40_L     */
    0x0,       /*ADC23_HPCOEF41_H     */
    0x0,       /*ADC23_HPCOEF41_L     */
    0x0,       /*ADC23_HPCOEF42_H     */
    0x0,       /*ADC23_HPCOEF42_L     */


};

static int fd_src_paras;
static FILE *  fd_dest_paras;

/*
 * for VBC EQ tuning by audiotester
 */
static struct mixer_ctl *s_ctl_eq_update = NULL;
static struct mixer_ctl *s_ctl_eq_select = NULL;
static int s_cur_out_devices = 0;
static int s_cur_in_devices = 0;
static AUDIO_TOTAL_T * s_vb_effect_ptr = NULL;
static struct mixer_ctl *s_ctl_da_eq_profile_select = NULL;
static struct mixer_ctl *s_ctl_ad01_eq_profile_select = NULL;
static struct mixer_ctl *s_ctl_ad23_eq_profile_select = NULL;

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
    struct vbc_da_eq_profile *vbc_da_effect_profile;
    struct vbc_ad_eq_profile *vbc_ad01_effect_profile;
    struct vbc_ad_eq_profile *vbc_ad23_effect_profile;
    uint32_t i = 0;

    if (NULL == audio_params_ptr) {
        ALOGE(" Error: audio_params_ptr is NULL.");
        return -1;
    }
    if (adev_get_audiomodenum4eng()*sizeof(AUDIO_TOTAL_T) != params_size) {
        ALOGE("Error: params_size = %d, total size = %d", params_size, adev_get_audiomodenum4eng()*sizeof(AUDIO_TOTAL_T));
        return -1;
    }
    fw_header = (struct vbc_fw_header *) malloc(sizeof(struct vbc_fw_header));
    effect_profile = (struct vbc_eq_profile *)malloc(sizeof(struct vbc_eq_profile));
    vbc_da_effect_profile = (struct vbc_da_eq_profile *)malloc(sizeof(struct vbc_da_eq_profile));
    vbc_ad01_effect_profile = (struct vbc_ad_eq_profile *)malloc(sizeof(struct vbc_ad_eq_profile));
    vbc_ad23_effect_profile = (struct vbc_ad_eq_profile *)malloc(sizeof(struct vbc_ad_eq_profile));

    if ((fw_header != NULL) && (effect_profile != NULL) && (vbc_da_effect_profile != NULL) && (vbc_ad01_effect_profile != NULL)&& (vbc_ad23_effect_profile != NULL)) {
        memset(fw_header, 0, sizeof(struct vbc_fw_header));
        memset(effect_profile, 0, sizeof(struct vbc_eq_profile));
        memset(vbc_da_effect_profile, 0, sizeof(struct vbc_da_eq_profile));
        memset(vbc_ad01_effect_profile, 0, sizeof(struct vbc_ad_eq_profile));
        memset(vbc_ad23_effect_profile, 0, sizeof(struct vbc_ad_eq_profile));

    } else {
        ALOGE("Error: malloc failed for internal struct.");
        if (fw_header)
            free(fw_header);
        if (effect_profile)
            free(effect_profile);
        if(vbc_da_effect_profile){
            free(vbc_da_effect_profile);
        }
        if(vbc_ad01_effect_profile){
            free(vbc_ad01_effect_profile);
        }
        if(vbc_ad23_effect_profile){
            free(vbc_ad23_effect_profile);
        }
        return -1;
    }
    ALOGI("do_parse...start");
    //audio para nv file--> fd_src
    //vb effect paras file-->fd_dest
    fd_dest_paras = fopen(STORED_VBC_EFFECT_PARAS_PATH, "wb");
    if (NULL  == fd_dest_paras) {
        free(fw_header);
        free(effect_profile);
        free(vbc_da_effect_profile);
        free(vbc_ad01_effect_profile);
        free(vbc_ad23_effect_profile);
        ALOGE("file %s open failed:%s", STORED_VBC_EFFECT_PARAS_PATH, strerror(errno));
        return -1;
    }
    //init temp buffer for paras calculated.

    memcpy(fw_header->magic, VBC_EQ_FIRMWARE_MAGIC_ID, VBC_EQ_FIRMWARE_MAGIC_LEN);
    fw_header->profile_version = VBC_EQ_PROFILE_VERSION;
    fw_header->num_profile = VBC_EFFECT_PROFILE_CNT*3; //TODO
    fw_header->num_da = VBC_EFFECT_PROFILE_CNT; //TODO
    fw_header->num_profile = VBC_EFFECT_PROFILE_CNT; //TODO
    fw_header->num_profile = VBC_EFFECT_PROFILE_CNT; //TODO

    ALOGI("fd_dest_paras(%p), header_len(%d), da_profile_len(%d),ad_profile_len(%d)", fd_dest_paras,
            sizeof(struct vbc_fw_header), sizeof(struct vbc_da_eq_profile),sizeof(struct vbc_ad_eq_profile));
    //write dest file header
    fwrite(fw_header, sizeof(struct vbc_fw_header), 1, fd_dest_paras);
    temp_params_ptr = audio_params_ptr;
    for (i=0; i<VBC_EFFECT_PROFILE_CNT; i++) {
        cur_params_ptr = temp_params_ptr + i;
        //reset the paras buffer and copy default register value.
        memset(effect_profile, 0, sizeof(struct vbc_eq_profile));
        memcpy(effect_profile->effect_paras, &vbc_reg_default[0], sizeof(vbc_reg_default));
        memcpy(effect_profile->ad_ctl_paras, &vbc_ad_ctrl_reg_default[0], sizeof(vbc_ad_ctrl_reg_default));

        //set paras to buffer.
        AUDENHA_SetPara(cur_params_ptr, effect_profile->effect_paras,effect_profile->ad_ctl_paras);

        //write buffer to stored file.first,write DA eq.
        memcpy(vbc_da_effect_profile->magic, VBC_EQ_FIRMWARE_MAGIC_ID, VBC_EQ_FIRMWARE_MAGIC_LEN);
        memcpy(vbc_da_effect_profile->name, cur_params_ptr->audio_nv_arm_mode_info.ucModeName, 16);
        memcpy(vbc_da_effect_profile->effect_paras, effect_profile->effect_paras, sizeof(vbc_da_effect_profile->effect_paras));
        ALOGI("vbc_da_effect_profile->name is %s", vbc_da_effect_profile->name);
        fseek(fd_dest_paras,i*sizeof(struct vbc_da_eq_profile)+sizeof(struct vbc_fw_header), SEEK_SET);
        fwrite(vbc_da_effect_profile, sizeof(struct vbc_da_eq_profile), 1, fd_dest_paras);

        //second,write AD01 eq.
        memcpy(vbc_ad01_effect_profile->magic, VBC_EQ_FIRMWARE_MAGIC_ID, VBC_EQ_FIRMWARE_MAGIC_LEN);
        memcpy(vbc_ad01_effect_profile->name, cur_params_ptr->audio_nv_arm_mode_info.ucModeName, 16);
        vbc_ad01_effect_profile->effect_paras[0] = effect_profile->ad_ctl_paras[0];
        vbc_ad01_effect_profile->effect_paras[1] = effect_profile->ad_ctl_paras[1];
        memcpy((void*)(vbc_ad01_effect_profile->effect_paras)+sizeof(uint32_t)*VBC_AD_CTL_PARAS_LEN, (void*)(effect_profile->effect_paras)+sizeof(vbc_da_effect_profile->effect_paras), sizeof(vbc_ad01_effect_profile->effect_paras)-sizeof(uint32_t)*VBC_AD_CTL_PARAS_LEN);
        ALOGI("vbc_ad01_effect_profile->name is %s", vbc_ad01_effect_profile->name);
        fseek(fd_dest_paras,sizeof(struct vbc_fw_header)+i*sizeof(struct vbc_ad_eq_profile)+VBC_EFFECT_PROFILE_CNT*sizeof(struct vbc_da_eq_profile), SEEK_SET);
        fwrite(vbc_ad01_effect_profile, sizeof(struct vbc_ad_eq_profile), 1, fd_dest_paras);

        //third,wirte AD23 eq.
        memcpy(vbc_ad23_effect_profile->magic, VBC_EQ_FIRMWARE_MAGIC_ID, VBC_EQ_FIRMWARE_MAGIC_LEN);
        memcpy(vbc_ad23_effect_profile->name, cur_params_ptr->audio_nv_arm_mode_info.ucModeName, 16);
        vbc_ad23_effect_profile->effect_paras[0] = effect_profile->ad_ctl_paras[0];
        vbc_ad23_effect_profile->effect_paras[1] = effect_profile->ad_ctl_paras[1];
        memcpy((void*)(vbc_ad23_effect_profile->effect_paras)+sizeof(uint32_t)*VBC_AD_CTL_PARAS_LEN, (void*)(effect_profile->effect_paras)+sizeof(vbc_da_effect_profile->effect_paras)+sizeof(vbc_ad01_effect_profile->effect_paras)-sizeof(uint32_t)*VBC_AD_CTL_PARAS_LEN, sizeof(vbc_ad23_effect_profile->effect_paras)-sizeof(uint32_t)*VBC_AD_CTL_PARAS_LEN);
        ALOGI("vbc_ad23_effect_profile->name is %s", vbc_ad23_effect_profile->name);
        fseek(fd_dest_paras,sizeof(struct vbc_fw_header)+i*sizeof(struct vbc_ad_eq_profile)+VBC_EFFECT_PROFILE_CNT*sizeof(struct vbc_da_eq_profile)+VBC_EFFECT_PROFILE_CNT*sizeof(struct vbc_ad_eq_profile), SEEK_SET);
        fwrite(vbc_ad23_effect_profile, sizeof(struct vbc_ad_eq_profile), 1, fd_dest_paras);


    }
    fclose(fd_dest_paras);
    free(fw_header);
    free(effect_profile);
    free(vbc_da_effect_profile);
    free(vbc_ad01_effect_profile);
    free(vbc_ad23_effect_profile);

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

    ALOGI("create_vb_effect_params...start,aud_params_ptr:0x%x",aud_params_ptr);
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
    //ignore profile_sele 
}

void vb_da_effect_config_mixer_ctl(struct mixer_ctl *da_profile_select)
{
    s_ctl_da_eq_profile_select = da_profile_select;
}


void vb_ad_effect_config_mixer_ctl(struct mixer_ctl *ad01_profile_select, struct mixer_ctl *ad23_profile_select)
{
    s_ctl_ad01_eq_profile_select = ad01_profile_select;
    s_ctl_ad23_eq_profile_select = ad23_profile_select;
}


void vb_effect_sync_devices(int cur_out_devices, int cur_in_devices)
{
    s_cur_out_devices = cur_out_devices;
    s_cur_in_devices = cur_in_devices;
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

int  vb_da_effect_profile_apply(int index)
{
    int ret = 0;
    ALOGI("s_cur_out_devices(0x%08x),index(%d)", s_cur_out_devices,index);
    if(index < VBC_EFFECT_PROFILE_CNT)
    {
        ret = mixer_ctl_set_value(s_ctl_da_eq_profile_select, 0, index);
    }
    else
    {
        ret = -1;
    }
    return ret;
}
int  vb_ad01_effect_profile_apply(int index)
{
    int ret = 0;
    ALOGI("s_cur_in_devices(0x%08x),index(%d)", s_cur_in_devices,index);
    if(index < VBC_EFFECT_PROFILE_CNT)
    {
        ret = mixer_ctl_set_value(s_ctl_ad01_eq_profile_select, 0, index); 
    }
    else
    {
        ret = -1;
    }
    return ret;
}
int  vb_ad23_effect_profile_apply(int index)
{
    int ret = 0;
    ALOGI("s_cur_in_devices(0x%08x),index(%d)", s_cur_in_devices,index);
    if(index < VBC_EFFECT_PROFILE_CNT)
    {
        ret = mixer_ctl_set_value(s_ctl_ad23_eq_profile_select, 0, index);
    }
    else
    {
        ret = -1;
    }
    return ret;
}

int vb_effect_profile_apply(void)
{
    int ret = 0;
    ALOGI("s_cur_out_devices(0x%08x), s_cur_in_devices(0x%08x)", s_cur_out_devices, s_cur_in_devices);

    if (s_ctl_da_eq_profile_select) {

        if(((s_cur_out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET)&&(s_cur_out_devices & AUDIO_DEVICE_OUT_SPEAKER))
                ||((s_cur_out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE)&&(s_cur_out_devices & AUDIO_DEVICE_OUT_SPEAKER))){
            //ret = mixer_ctl_set_enum_by_string(s_ctl_eq_select, "Headfree");
            ret = mixer_ctl_set_value(s_ctl_da_eq_profile_select, 0, 1);
            ALOGI("profile is Headfree, ret=%d", ret);
        }else if(s_cur_out_devices & AUDIO_DEVICE_OUT_EARPIECE){
            //ret = mixer_ctl_set_enum_by_string(s_ctl_eq_select, "Handset");
            ret = mixer_ctl_set_value(s_ctl_da_eq_profile_select, 0, 2);
            ALOGI("profile is Handset, ret=%d", ret);
        }else if((s_cur_out_devices & AUDIO_DEVICE_OUT_SPEAKER)
                ||(s_cur_out_devices & AUDIO_DEVICE_OUT_FM_SPEAKER) ){
            //ret = mixer_ctl_set_enum_by_string(s_ctl_eq_select, "Handsfree");
            ret = mixer_ctl_set_value(s_ctl_da_eq_profile_select, 0, 3);
            ALOGI("profile is Handsfree, ret=%d", ret);
        }else if((s_cur_out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET)
                ||(s_cur_out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE)
                ||(s_cur_out_devices & AUDIO_DEVICE_OUT_FM_HEADSET)
                ||(s_cur_in_devices & AUDIO_DEVICE_IN_WIRED_HEADSET)){
            //ret = mixer_ctl_set_enum_by_string(s_ctl_eq_select, "Headset");
            ret = mixer_ctl_set_value(s_ctl_da_eq_profile_select, 0, 0);
            ALOGI("profile is Headset, ret=%d", ret);
        }else{
            ALOGE("s_cur_out_devices(0x%08x) s_cur_in_devices(0x%08x)IS NOT SUPPORT!\n",
                    s_cur_out_devices, s_cur_in_devices);
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
